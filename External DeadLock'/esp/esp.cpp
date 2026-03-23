#include "esp.h"
#include "hooks.h"

ESP::Settings ESP::g_Settings;

// matrix row-major for source2
inline bool WorldToScreen(
    const Vec3& world,
    Vector2& screen,
    const float* vm,
    int screenW,
    int screenH
) noexcept
{
    const float clipW =
        world.x * vm[12] +
        world.y * vm[13] +
        world.z * vm[14] +
        vm[15];

    if (clipW <= 0.01f)
        return false;

    const float invW = 1.0f / clipW;

    const float clipX =
        (world.x * vm[0] +
            world.y * vm[1] +
            world.z * vm[2] +
            vm[3]) * invW;

    const float clipY =
        (world.x * vm[4] +
            world.y * vm[5] +
            world.z * vm[6] +
            vm[7]) * invW;

    const float halfW = static_cast<float>(screenW) * 0.5f;
    const float halfH = static_cast<float>(screenH) * 0.5f;

    screen.x = halfW * (clipX + 1.0f);
    screen.y = halfH * (1.0f - clipY);

    return true;
}

// helpers + cache
namespace
{
    constexpr int   kMaxPlayers = 64;
    constexpr float kMinBoxHeight = 5.0f;
    constexpr float kBoxWidthRatio = 0.45f;
    constexpr float kHpBarWidth = 4.0f;
    constexpr float kHpBarOffsetX = 6.0f;
    constexpr float kHpTextOffsetX = 2.0f;
    constexpr float kHpTextOffsetY = 2.0f;

    struct CachedPlayer
    {
        Vector3 head{};
        Vector3 feet{};
        int     health{ 0 };
        int     maxHealth{ 100 };
        int     team{ 0 };
        bool    alive{ false };
    };

    EntitySystem g_EntitySystem;

    std::array<CachedPlayer, kMaxPlayers> g_PlayerCacheA;
    std::array<CachedPlayer, kMaxPlayers> g_PlayerCacheB;

    std::atomic<CachedPlayer*> g_ReadPtr{ g_PlayerCacheA.data() };
    std::atomic<CachedPlayer*> g_WritePtr{ g_PlayerCacheB.data() };

    std::atomic_bool g_MemoryThreadRunning{ false };

    inline ImU32 ToImU32(const ImVec4& c) noexcept
    {
        return ImGui::ColorConvertFloat4ToU32(c);
    }

    inline ImU32 GetColorForPlayer(int pawnTeam,
        const PlayerPawn* local,
        ImU32 enemyCol,
        ImU32 teamCol) noexcept
    {
        if (!local)
            return enemyCol;

        return (pawnTeam == local->GetTeam()) ? teamCol : enemyCol;
    }

    inline void DrawBoxFull(ImDrawList* draw,
        float x, float y,
        float w, float h,
        ImU32 col,
        float thickness) noexcept
    {
        draw->AddRect(
            ImVec2(x, y),
            ImVec2(x + w, y + h),
            col,
            0.0f,
            0,
            thickness
        );
    }

    inline void DrawBoxCorners(ImDrawList* draw,
        float x, float y,
        float w, float h,
        ImU32 col,
        float thickness,
        float frac) noexcept
    {
        const float lw = w * frac;
        const float lh = h * frac;

        const ImVec2 p1(x, y);
        const ImVec2 p2(x + w, y);
        const ImVec2 p3(x, y + h);
        const ImVec2 p4(x + w, y + h);

        draw->AddLine(p1, ImVec2(p1.x + lw, p1.y), col, thickness);
        draw->AddLine(p1, ImVec2(p1.x, p1.y + lh), col, thickness);

        draw->AddLine(p2, ImVec2(p2.x - lw, p2.y), col, thickness);
        draw->AddLine(p2, ImVec2(p2.x, p2.y + lh), col, thickness);

        draw->AddLine(p3, ImVec2(p3.x + lw, p3.y), col, thickness);
        draw->AddLine(p3, ImVec2(p3.x, p3.y - lh), col, thickness);

        draw->AddLine(p4, ImVec2(p4.x - lw, p4.y), col, thickness);
        draw->AddLine(p4, ImVec2(p4.x, p4.y - lh), col, thickness);
    }

    inline void DrawHealthBarAndText(ImDrawList* draw,
        float x, float y,
        float boxHeight,
        int health,
        int maxHealth,
        bool showBar,
        bool showText) noexcept
    {
        if (!showBar && !showText)
            return;

        if (maxHealth <= 0)
            return;

        float hpPerc = static_cast<float>(health) / static_cast<float>(maxHealth);
        hpPerc = std::clamp(hpPerc, 0.0f, 1.0f);

        const float barX = x - kHpBarOffsetX;
        const float barHeight = boxHeight * hpPerc;
        const float barY = y + (boxHeight - barHeight);

        if (showBar)
        {
            ImU32 hpColor;
            if (hpPerc > 0.66f)
                hpColor = IM_COL32(0, 255, 0, 255);
            else if (hpPerc > 0.33f)
                hpColor = IM_COL32(255, 255, 0, 255);
            else
                hpColor = IM_COL32(255, 0, 0, 255);

            draw->AddRectFilled(
                ImVec2(barX, y),
                ImVec2(barX + kHpBarWidth, y + boxHeight),
                IM_COL32(0, 0, 0, 180)
            );

            draw->AddRectFilled(
                ImVec2(barX, barY),
                ImVec2(barX + kHpBarWidth, y + boxHeight),
                hpColor
            );
        }

        if (showText)
        {
            char hpText[16];
            sprintf_s(hpText, "%d", health);

            const ImVec2 textSize = ImGui::CalcTextSize(hpText);
            const float textX = barX - textSize.x - kHpTextOffsetX;
            const float textY = y - kHpTextOffsetY;

            draw->AddText(
                ImVec2(textX, textY),
                IM_COL32(255, 255, 255, 255),
                hpText
            );
        }
    }

    void MemoryThreadFunc()
    {
        g_MemoryThreadRunning.store(true, std::memory_order_release);

        while (g_MemoryThreadRunning.load(std::memory_order_acquire))
        {
            CachedPlayer* writeBuf = g_WritePtr.load(std::memory_order_acquire);

            for (int i = 2; i < kMaxPlayers; ++i)
            {
                PlayerPawn* pawn = g_EntitySystem.GetPawn(i);
                auto& dst = writeBuf[i];

                if (!pawn || !pawn->IsAlive() || !pawn->GetNode())
                {
                    dst.alive = false;
                    continue;
                }

                dst.alive = true;
                dst.team = pawn->GetTeam();
                dst.health = pawn->health;
                dst.maxHealth = pawn->maxHealth;
                dst.head = pawn->GetHead();
                dst.feet = pawn->GetFeet();
            }

            CachedPlayer* oldRead = g_ReadPtr.load(std::memory_order_acquire);
            g_ReadPtr.store(writeBuf, std::memory_order_release);
            g_WritePtr.store(oldRead, std::memory_order_release);

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

// Threads
void ESP::StartMemoryThread()
{
    static std::once_flag once;
    std::call_once(once, []()
        {
            std::thread(MemoryThreadFunc).detach();
        });
}

void ESP::StopMemoryThread()
{
    g_MemoryThreadRunning.store(false, std::memory_order_release);
}

static void DrawBonesForPawn(PlayerPawn* pawn, ImDrawList* draw, const float* viewMatrix, int screenW, int screenH)
{
    if (!pawn)
        return;

    SceneNode* sn = (SceneNode*)pawn->GetNode();
    if (!sn)
        return;

    SkeletoneInstance* sk = (SkeletoneInstance*)sn->GetSkeletoneInstance();
    if (!sk)
        return;

    CModel_Imp* model = (CModel_Imp*)(*(uintptr_t*)(sk->GetCModel_Imp()));
    if (!model)
        return;

    uintptr_t bonesBase = *(uintptr_t*)((uintptr_t)sk + 0x80);
    if (!bonesBase)
        return;

    const int maxBones = 128;
    const size_t stride = 0x20;

    struct BoneInfo
    {
        bool valid = false;
        Vector2 screen{};
        char name[128]{};
    };

    BoneInfo bones[maxBones];

    // 1. Собираем экранные позиции и имена костей
    for (int i = 0; i < maxBones; i++)
    {
        const char* name = Bones::hook::GetBoneName(model, i);
        if (!name)
            continue;

        strcpy_s(bones[i].name, name);
        Bones::hook::ToLower(bones[i].name);

        Vector3 pos = *(Vector3*)(bonesBase + i * stride);

        Vector2 screen;
        if (!WorldToScreen(pos, screen, viewMatrix, screenW, screenH))
            continue;

        bones[i].valid = true;
        bones[i].screen = screen;
    }

    // 2. Поиск кости по подстроке
    auto Find = [&](const char* substr) -> int
        {
            for (int i = 0; i < maxBones; i++)
            {
                if (!bones[i].valid)
                    continue;

                if (strstr(bones[i].name, substr))
                    return i;
            }
            return -1;
        };

    // 3. Кости
    int head = Find("head");
    int pelvis = Find("pelvis");
    int neck = Find("neck_0");

    int arm_upper_L = Find("arm_upper_l");
    int arm_lower_L = Find("arm_lower_l");
    int hand_L = Find("hand_l");

    int arm_upper_R = Find("arm_upper_r");
    int arm_lower_R = Find("arm_lower_r");
    int hand_R = Find("hand_r");

    int leg_upper_L = Find("leg_upper_l");
    int leg_lower_L = Find("leg_lower_l");
    int ankle_L = Find("ankle_l");

    int leg_upper_R = Find("leg_upper_r");
    int leg_lower_R = Find("leg_lower_r");
    int ankle_R = Find("ankle_r");

    ImU32 col = ImGui::ColorConvertFloat4ToU32(ESP::g_Settings.skeletonColor);
    float th = ESP::g_Settings.skeletonThickness;

    // 4. Линия
    auto Line = [&](int a, int b)
        {
            if (a < 0 || b < 0)
                return;
            if (!bones[a].valid || !bones[b].valid)
                return;

            draw->AddLine(
                ImVec2(bones[a].screen.x, bones[a].screen.y),
                ImVec2(bones[b].screen.x, bones[b].screen.y),
                col,
                th
            );
        };

    // 5. Скелет
    Line(head, pelvis);

    Line(neck, arm_upper_L);
    Line(neck, arm_upper_R);

    Line(arm_upper_L, arm_lower_L);
    Line(arm_upper_R, arm_lower_R);

    Line(arm_lower_L, hand_L);
    Line(arm_lower_R, hand_R);

    Line(pelvis, leg_upper_L);
    Line(pelvis, leg_upper_R);

    Line(leg_upper_L, leg_lower_L);
    Line(leg_upper_R, leg_lower_R);

    Line(leg_lower_L, ankle_L);
    Line(leg_lower_R, ankle_R);
}


// Draw ESP
void ESP::DrawESP(const float* viewMatrix)
{
    ImGuiIO& io = ImGui::GetIO();
    const int screenW = static_cast<int>(io.DisplaySize.x);
    const int screenH = static_cast<int>(io.DisplaySize.y);

    if (screenW <= 0 || screenH <= 0 || !viewMatrix)
        return;

    ImDrawList* draw = ImGui::GetBackgroundDrawList();
    if (!draw)
        return;

    auto* local = reinterpret_cast<PlayerPawn*>(offset::CPlayerPawn);

    const ImU32 enemyColU32 = ToImU32(g_Settings.enemyColor);
    const ImU32 teamColU32 = ToImU32(g_Settings.teamColor);

    CachedPlayer* readBuf = g_ReadPtr.load(std::memory_order_acquire);

    for (int i = 2; i < kMaxPlayers; ++i)
    {
        const auto& p = readBuf[i];
        if (!p.alive)
            continue;

        if (g_Settings.ignoreTeammates && local && p.team == local->GetTeam())
            continue;

        Vector2 feet2D, head2D;

        if (!WorldToScreen(p.feet, feet2D, viewMatrix, screenW, screenH))
            continue;

        if (!WorldToScreen(p.head, head2D, viewMatrix, screenW, screenH))
            continue;

        const float boxHeight = feet2D.y - head2D.y;
        if (boxHeight < kMinBoxHeight)
            continue;

        const float boxWidth = boxHeight * kBoxWidthRatio;

        const float x = head2D.x - boxWidth * 0.5f;
        const float y = head2D.y;

        const ImU32 col = GetColorForPlayer(p.team, local, enemyColU32, teamColU32);

        switch (g_Settings.boxStyle)
        {
        case BoxStyle::Full:
            DrawBoxFull(draw, x, y, boxWidth, boxHeight, col, g_Settings.thickness);
            break;

        case BoxStyle::Corners:
            DrawBoxCorners(draw, x, y, boxWidth, boxHeight, col, g_Settings.thickness, g_Settings.cornerFrac);
            break;
        }

        DrawHealthBarAndText(
            draw,
            x,
            y,
            boxHeight,
            p.health,
            p.maxHealth,
            g_Settings.showHealthBar,
            g_Settings.showHealthText
        );

        if (g_Settings.showSkeleton)
        {
            PlayerPawn* realPawn = g_EntitySystem.GetPawn(i);
            if (realPawn)
                DrawBonesForPawn(realPawn, draw, viewMatrix, screenW, screenH);
        }
    }
}

// Draw ImGui menu
void ESP::DrawMenu()
{
    if (ImGui::Begin("NoMore"))
    {
        ImGui::Text("ESP Settings");

        ImGui::ColorEdit4("Enemy Color", (float*)&g_Settings.enemyColor);
        ImGui::ColorEdit4("Team Color", (float*)&g_Settings.teamColor);

        ImGui::SliderFloat("Thickness", &g_Settings.thickness, 0.5f, 5.0f);
        ImGui::SliderFloat("Corner Size", &g_Settings.cornerFrac, 0.1f, 0.5f);

        const char* styles[] = { "Full Box", "Corners" };
        int style = (g_Settings.boxStyle == BoxStyle::Full) ? 0 : 1;
        if (ImGui::Combo("Box Style", &style, styles, IM_ARRAYSIZE(styles)))
            g_Settings.boxStyle = (style == 0) ? BoxStyle::Full : BoxStyle::Corners;

        ImGui::Checkbox("Show HP Bar", &g_Settings.showHealthBar);
        ImGui::Checkbox("Show HP Text", &g_Settings.showHealthText);
        ImGui::Checkbox("Ignore Teammates", &g_Settings.ignoreTeammates);

        ImGui::Separator();
        ImGui::Text("Skeleton");
        ImGui::Checkbox("Show Skeleton", &g_Settings.showSkeleton);
        ImGui::ColorEdit4("Skeleton Color", (float*)&g_Settings.skeletonColor);
        ImGui::SliderFloat("Skeleton Thickness", &g_Settings.skeletonThickness, 1.0f, 5.0f);

        if (ImGui::Button("Save Config"))
            Config::Save("config.json");


        ImGui::End();
    }
}
