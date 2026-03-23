#include "aim.h"
#include "hooks.h"
#include <Windows.h>
#include <cmath>

AIM::Settings AIM::g_Settings;

namespace
{
    constexpr int kMaxPlayers = 64;

    EntitySystem g_EntitySystem;

    inline bool WorldToScreen(
        const Vec3& world,
        Vector2& screen,
        const float* vm,
        int screenW,
        int screenH
    )
    {
        const float clipW =
            world.x * vm[12] +
            world.y * vm[13] +
            world.z * vm[14] +
            vm[15];

        if (clipW < 0.01f)
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

        const float halfW = screenW * 0.5f;
        const float halfH = screenH * 0.5f;

        screen.x = halfW * (clipX + 1.0f);
        screen.y = halfH * (1.0f - clipY);

        return true;
    }

    inline float Distance(float x1, float y1, float x2, float y2)
    {
        return std::sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
    }

    void MoveMouse(float x, float y)
    {
        INPUT input{};
        input.type = INPUT_MOUSE;
        input.mi.dx = (LONG)x;
        input.mi.dy = (LONG)y;
        input.mi.dwFlags = MOUSEEVENTF_MOVE;

        SendInput(1, &input, sizeof(INPUT));
    }

    bool GetBoneWorldPos(PlayerPawn* pawn, const char* boneName, Vec3& out)
    {
        if (!pawn || !boneName)
            return false;

        SceneNode* sn = (SceneNode*)pawn->GetNode();
        if (!sn) return false;

        SkeletoneInstance* sk = (SkeletoneInstance*)sn->GetSkeletoneInstance();
        if (!sk) return false;

        CModel_Imp* model = (CModel_Imp*)(*(uintptr_t*)(sk->GetCModel_Imp()));
        if (!model) return false;

        uintptr_t bonesBase = *(uintptr_t*)((uintptr_t)sk + 0x80);
        if (!bonesBase) return false;

        int index = Bones::hook::FindBoneIndexByName(model, boneName);
        if (index < 0) return false;

        out = *(Vec3*)(bonesBase + index * 0x20);
        return true;
    }
}

void AIM::Run(const float* viewMatrix)
{
    ImGuiIO& io = ImGui::GetIO();

    int screenW = (int)io.DisplaySize.x;
    int screenH = (int)io.DisplaySize.y;

    float centerX = screenW * 0.5f;
    float centerY = screenH * 0.5f;

    // -----------------------------
    // Рисуем FOV круг ВСЕГДА
    // -----------------------------
    {
        ImDrawList* draw = ImGui::GetForegroundDrawList();
        draw->AddCircle(
            ImVec2(centerX, centerY),
            g_Settings.fov,
            IM_COL32(255, 255, 255, 120),
            64,
            1.5f
        );
    }

    if (!g_Settings.enabled)
        return;

    if (!(GetAsyncKeyState(g_Settings.aimKey) & 0x8000))
        return;

    auto* local = reinterpret_cast<PlayerPawn*>(offset::CPlayerPawn);
    if (!local)
        return;

    // -----------------------------
    // Проверяем node и skeleton
    // -----------------------------
    if (!local->GetNode())
        return;

    float bestDist = g_Settings.fov;
    Vector2 bestPos{};
    bool foundTarget = false;

    for (int i = 2; i < kMaxPlayers; i++)
    {
        PlayerPawn* pawn = g_EntitySystem.GetPawn(i);

        if (!pawn)
            continue;

        if (!pawn->IsAlive())
            continue;

        if (pawn->GetTeam() == local->GetTeam())
            continue;

        // -----------------------------
        // Проверяем node и skeleton
        // -----------------------------
        SceneNode* sn = (SceneNode*)pawn->GetNode();
        if (!sn)
            continue;

        SkeletoneInstance* sk = (SkeletoneInstance*)sn->GetSkeletoneInstance();
        if (!sk)
            continue;

        uintptr_t bonesBase = *(uintptr_t*)((uintptr_t)sk + 0x80);
        if (!bonesBase)
            continue;

        // -----------------------------
        // Получаем позицию головы
        // -----------------------------
        Vec3 headPos{};
        if (!GetBoneWorldPos(pawn, "head", headPos))
            continue;

        // Проверка на NaN
        if (!std::isfinite(headPos.x) || !std::isfinite(headPos.y) || !std::isfinite(headPos.z))
            continue;

        Vector2 screen;
        if (!WorldToScreen(headPos, screen, viewMatrix, screenW, screenH))
            continue;

        float dist = Distance(centerX, centerY, screen.x, screen.y);

        if (dist < bestDist)
        {
            bestDist = dist;
            bestPos = screen;
            foundTarget = true;
        }
    }

    if (!foundTarget)
        return;

    float dx = (bestPos.x - centerX) / g_Settings.smooth;
    float dy = (bestPos.y - centerY) / g_Settings.smooth;

    // Проверка на NaN
    if (!std::isfinite(dx) || !std::isfinite(dy))
        return;

    MoveMouse(dx, dy);
}


void AIM::DrawMenu()
{
    if (ImGui::Begin("AIM"))
    {
        ImGui::Checkbox("Enable Aim", &g_Settings.enabled);

        ImGui::SliderFloat("FOV", &g_Settings.fov, 10.f, 500.f);
        ImGui::SliderFloat("Smooth", &g_Settings.smooth, 1.f, 20.f);

        ImGui::Text("Aim Key: Mouse4");

        ImGui::End();
    }
}
