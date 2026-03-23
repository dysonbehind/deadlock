#include "config.h"
#include "../esp/esp.h"
#include "../aim/aim.h"
#include <fstream>
#include <filesystem>
#include "json.hpp"

using json = nlohmann::json;

static json j;

// -----------------------------
// LOAD
// -----------------------------
bool Config::Load(const std::string& path)
{
    if (!std::filesystem::exists(path))
    {
        Save(path); // создаём дефолтный
        return true;
    }

    std::ifstream f(path);
    if (!f.is_open())
        return false;

    try
    {
        f >> j;
    }
    catch (...)
    {
        return false;
    }

    // ESP
    if (j.contains("esp"))
    {
        auto& e = j["esp"];

        if (e.contains("enemyColor"))
            memcpy(&ESP::g_Settings.enemyColor, e["enemyColor"].get<std::array<float, 4>>().data(), sizeof(float) * 4);

        if (e.contains("teamColor"))
            memcpy(&ESP::g_Settings.teamColor, e["teamColor"].get<std::array<float, 4>>().data(), sizeof(float) * 4);

        if (e.contains("skeletonColor"))
            memcpy(&ESP::g_Settings.skeletonColor, e["skeletonColor"].get<std::array<float, 4>>().data(), sizeof(float) * 4);

        if (e.contains("skeletonThickness"))
            ESP::g_Settings.skeletonThickness = e["skeletonThickness"];

        if (e.contains("showSkeleton"))
            ESP::g_Settings.showSkeleton = e["showSkeleton"];

        if (e.contains("thickness"))
            ESP::g_Settings.thickness = e["thickness"];

        if (e.contains("cornerFrac"))
            ESP::g_Settings.cornerFrac = e["cornerFrac"];

        if (e.contains("showHealthBar"))
            ESP::g_Settings.showHealthBar = e["showHealthBar"];

        if (e.contains("showHealthText"))
            ESP::g_Settings.showHealthText = e["showHealthText"];

        if (e.contains("ignoreTeammates"))
            ESP::g_Settings.ignoreTeammates = e["ignoreTeammates"];
    }

    // AIM
    if (j.contains("aim"))
    {
        auto& a = j["aim"];

        if (a.contains("enabled"))
            AIM::g_Settings.enabled = a["enabled"];

        if (a.contains("fov"))
            AIM::g_Settings.fov = a["fov"];

        if (a.contains("smooth"))
            AIM::g_Settings.smooth = a["smooth"];

        if (a.contains("aimKey"))
            AIM::g_Settings.aimKey = a["aimKey"];
    }

    return true;
}

// -----------------------------
// SAVE
// -----------------------------
bool Config::Save(const std::string& path)
{
    j = json::object();

    j["esp"] = {
        {"enemyColor", std::array<float,4>{
            ESP::g_Settings.enemyColor.x,
            ESP::g_Settings.enemyColor.y,
            ESP::g_Settings.enemyColor.z,
            ESP::g_Settings.enemyColor.w
        }},
        {"teamColor", std::array<float,4>{
            ESP::g_Settings.teamColor.x,
            ESP::g_Settings.teamColor.y,
            ESP::g_Settings.teamColor.z,
            ESP::g_Settings.teamColor.w
        }},
        {"skeletonColor", std::array<float,4>{
            ESP::g_Settings.skeletonColor.x,
            ESP::g_Settings.skeletonColor.y,
            ESP::g_Settings.skeletonColor.z,
            ESP::g_Settings.skeletonColor.w
        }},
        {"skeletonThickness", ESP::g_Settings.skeletonThickness},
        {"showSkeleton", ESP::g_Settings.showSkeleton},
        {"thickness", ESP::g_Settings.thickness},
        {"cornerFrac", ESP::g_Settings.cornerFrac},
        {"showHealthBar", ESP::g_Settings.showHealthBar},
        {"showHealthText", ESP::g_Settings.showHealthText},
        {"ignoreTeammates", ESP::g_Settings.ignoreTeammates}
    };

    j["aim"] = {
        {"enabled", AIM::g_Settings.enabled},
        {"fov", AIM::g_Settings.fov},
        {"smooth", AIM::g_Settings.smooth},
        {"aimKey", AIM::g_Settings.aimKey}
    };

    std::ofstream f(path);
    if (!f.is_open())
        return false;

    f << j.dump(4);
    return true;
}
