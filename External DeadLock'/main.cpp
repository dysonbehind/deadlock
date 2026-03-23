#include "../includes.h"
#include "DXHook.h"
#include "../esp/esp.h"
#include "src/hooks.h"

#define LOG(str, x) std::cout << str << x << std::endl

    // CONSOLE FOR LOGS
void console()
{
    AllocConsole();
    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);
    freopen_s(&f, "CONIN$", "r", stdin);

    LOG("hooked func addres: ", Bones::hook::hAddres);
    
    PlayerPawn* p = (PlayerPawn*)offset::CPlayerPawn;
    LOG("PlayerPawn: ", p);

    SceneNode* sn = (SceneNode*)p->GetNode();
    LOG("SceneNode: ", sn);

    SkeletoneInstance* sk = (SkeletoneInstance*)sn->GetSkeletoneInstance();
    LOG("Skeletone Instance: ", sk);

    CModel_Imp* mi = (CModel_Imp*)(*(uintptr_t*)(sk->GetCModel_Imp()));
    LOG("CModel_Imp: ", mi);

    const char* b = Bones::hook::getBone();
    LOG("Bone: ", b);

    int headIndex = Bones::hook::FindBoneIndexByName(mi, "head");
    LOG("head bone: ", headIndex);
    int neckIndex = Bones::hook::FindBoneIndexByName(mi, "neck");
    LOG("neck bone: ", neckIndex);
    int pelvisIndex = Bones::hook::FindBoneIndexByName(mi, "pelvis");
    LOG("pelivs bone: ", pelvisIndex);

    while (true)
    {
        std::string input;
        std::cin >> input;


        
    }

    FreeConsole();
}


BOOL WINAPI DllMain(HMODULE hMod, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hMod);
        CreateThread(nullptr, 0, MainThread, hMod, 0, nullptr);
        //CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)console, nullptr, 0, nullptr);
        break;

    case DLL_PROCESS_DETACH:
        ESP::StopMemoryThread();

        if (g_Window && g_OldWndProc)
            SetWindowLongPtr(g_Window, GWLP_WNDPROC, (LONG_PTR)g_OldWndProc);

        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();

        if (g_MainRTV) { g_MainRTV->Release(); g_MainRTV = nullptr; }
        if (g_Context) { g_Context->Release(); g_Context = nullptr; }
        if (g_Device) { g_Device->Release();  g_Device = nullptr; }

        kiero::shutdown();
        break;
    }
    return TRUE;
}
