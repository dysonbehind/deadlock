#pragma once
#include "../includes.h"
#include <atomic>
#include <mutex>

	//DX11 HOOKS
typedef HRESULT(__stdcall* Present)(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
typedef LRESULT(CALLBACK* WNDPROC)(HWND, UINT, WPARAM, LPARAM);

	// Globals variable
extern Present                 oPresent;
extern HWND                    g_Window;
extern WNDPROC                 g_OldWndProc;
extern ID3D11Device* g_Device;
extern ID3D11DeviceContext* g_Context;
extern ID3D11RenderTargetView* g_MainRTV;

extern std::atomic_bool        g_ImGuiInitialized;
extern std::atomic_bool        g_ShowMenu;

extern std::once_flag          g_InitOnce;

	// Imgui cheaker callback
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// User func for init Imgui menu
void InitImGui();

// Window procedures
LRESULT __stdcall WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
HRESULT __stdcall hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);

// Main thread hook
DWORD WINAPI MainThread(LPVOID lpReserved);
