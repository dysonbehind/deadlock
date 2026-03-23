#include "DXHook.h"
#include "../esp/esp.h"
#include "../aim/aim.h"

#include <thread>
#include <atomic>
#include <mutex>

// ------------------------------------------------------------
// Глобальные переменные
// ------------------------------------------------------------
Present                 oPresent = nullptr;
HWND                    g_Window = nullptr;
WNDPROC                 g_OldWndProc = nullptr;
ID3D11Device* g_Device = nullptr;
ID3D11DeviceContext* g_Context = nullptr;
ID3D11RenderTargetView* g_MainRTV = nullptr;

std::atomic_bool        g_ImGuiInitialized{ false };
std::atomic_bool        g_ShowMenu{ true };

std::once_flag          g_InitOnce;

// ------------------------------------------------------------
// ImGui init
// ------------------------------------------------------------
void InitImGui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

    ImGui_ImplWin32_Init(g_Window);
    ImGui_ImplDX11_Init(g_Device, g_Context);

    g_ImGuiInitialized.store(true, std::memory_order_release);
}

inline void ToggleMenu()
{
    bool newState = !g_ShowMenu.load(std::memory_order_relaxed);
    g_ShowMenu.store(newState, std::memory_order_relaxed);

    ImGuiIO& io = ImGui::GetIO();
    io.WantCaptureKeyboard = newState;
    io.WantCaptureMouse = newState;
}

// ------------------------------------------------------------
// Рендер
// ------------------------------------------------------------
void RenderFrame()
{
    if (!g_ImGuiInitialized.load(std::memory_order_acquire))
        return;

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // ESP всегда
    ESP::DrawESP(offset::viewMatrix);
    AIM::Run(offset::viewMatrix);

    // Меню по флагу
    if (g_ShowMenu.load(std::memory_order_relaxed))
    {
        ESP::DrawMenu();
        AIM::DrawMenu();
    }

    ImGui::Render();
}

// ------------------------------------------------------------
// WndProc
// ------------------------------------------------------------
LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // хоткей на открытие/закрытие меню — в основном потоке окна
    if (uMsg == WM_KEYDOWN && wParam == VK_HOME)
    {
        ToggleMenu();
        return 0;
    }

    if (g_ShowMenu.load(std::memory_order_relaxed))
    {
        if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
            return true;

        switch (uMsg)
        {
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_MOUSEMOVE:
        case WM_MOUSEWHEEL:
        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_CHAR:
            return true;
        }
    }

    return CallWindowProc(g_OldWndProc, hWnd, uMsg, wParam, lParam);
}

// ------------------------------------------------------------
// hkPresent
// ------------------------------------------------------------
HRESULT __stdcall hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
    std::call_once(g_InitOnce, [&]()
        {
            if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&g_Device)))
            {
                g_Device->GetImmediateContext(&g_Context);

                DXGI_SWAP_CHAIN_DESC sd{};
                pSwapChain->GetDesc(&sd);
                g_Window = sd.OutputWindow;

                ID3D11Texture2D* pBackBuffer = nullptr;
                if (SUCCEEDED(pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer)))
                {
                    g_Device->CreateRenderTargetView(pBackBuffer, nullptr, &g_MainRTV);
                    pBackBuffer->Release();
                }

                g_OldWndProc = (WNDPROC)SetWindowLongPtr(g_Window, GWLP_WNDPROC, (LONG_PTR)WndProc);

                InitImGui();
            }
        });

    RenderFrame();

    if (g_MainRTV)
        g_Context->OMSetRenderTargets(1, &g_MainRTV, nullptr);

    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    return oPresent(pSwapChain, SyncInterval, Flags);
}

// ------------------------------------------------------------
// MainThread — только установка хука, без вечного цикла
// ------------------------------------------------------------
DWORD WINAPI MainThread(LPVOID lpReserved)
{
    // Пытаемся инициализировать kiero с небольшими паузами
    for (;;)
    {
        if (kiero::init(kiero::RenderType::D3D11) == kiero::Status::Success)
        {
            kiero::bind(8, (void**)&oPresent, hkPresent);
            ESP::StartMemoryThread();
            //Aim::Run(offset::viewMatrix);
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    Config::Load("config.json");

    return 0;
}
