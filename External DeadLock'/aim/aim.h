#pragma once
#include "../includes.h"
#include "entity.h"
#include "../src/hooks.h"

class AIM
{
public:

    struct Settings
    {
        bool enabled = false;
        //int  aimKey = VK_LBUTTON;
        int  aimKey = VK_XBUTTON1; // боковая кнопка мыши
        float fov = 150.0f;        // радиус аима
        float smooth = 5.f;
    };

    static Settings g_Settings;

    static void Run(const float* viewMatrix);
    static void DrawMenu();
};