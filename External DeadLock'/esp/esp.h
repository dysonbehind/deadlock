#pragma once
#include "../includes.h"
#include "Player.h"
#include "entity.h"
#include "../src/hooks.h"
#include "../config/config.h"

inline bool WorldToScreen(
    const Vec3& world,
    Vector2& screen,
    const float* vm,
    int screenW,
    int screenH
) noexcept;

namespace ESP
{
    enum class BoxStyle
    {
        Full,
        Corners
    };

    struct Settings
    {
        ImVec4 enemyColor{ 1.f, 0.f, 0.f, 1.f };
        ImVec4 teamColor{ 0.f, 1.f, 0.f, 1.f };
        float thickness{ 2.f };
        float cornerFrac{ 0.25f };
        BoxStyle boxStyle{ BoxStyle::Full };
        bool showHealthBar{ true };
        bool showHealthText{ true };
        bool ignoreTeammates{ false };

        // Новые поля для скелета
        ImVec4 skeletonColor{ 0.f, 1.f, 1.f, 1.f };
        float skeletonThickness{ 2.f };
        bool showSkeleton{ false };
    };


    extern Settings g_Settings;

    void DrawESP(const float* viewMatrix);
    void DrawMenu();

    void StartMemoryThread();
    void StopMemoryThread();
}
