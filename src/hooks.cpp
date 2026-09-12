#include "hooks.h"
#include "gui.h"
#include "packet.h"
#include "memory.h"
#include "imgui.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <dxgi.h>
#include <MinHook.h>
#include <cstdio>
#include <string>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

using PresentFn = HRESULT(__stdcall*)(IDXGISwapChain*, UINT, UINT);
using ResizeBuffersFn = HRESULT(__stdcall*)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);

static PresentFn       oPresent = nullptr;
static ResizeBuffersFn oResizeBuffers = nullptr;
static WNDPROC         oWndProc = nullptr;

static ID3D11Device*           g_device = nullptr;
static ID3D11DeviceContext*    g_context = nullptr;
static ID3D11RenderTargetView* g_rtv = nullptr;
static HWND                    g_hwnd = nullptr;
static bool                    g_initialized = false;
static bool                    g_minhookInitialized = false;
static std::string             g_lastError;

static void SetError(const char* message) {
    g_lastError = message ? message : "Unknown error";
}

static void SetHresultError(const char* stage, HRESULT hr) {
    char buffer[160]{};
    std::snprintf(buffer, sizeof(buffer), "%s failed (HRESULT 0x%08lX)", stage, static_cast<unsigned long>(hr));
    g_lastError = buffer;
}

static void SetMinHookError(const char* stage, MH_STATUS status) {
    char buffer[192]{};
    const char* text = MH_StatusToString(status);
    std::snprintf(buffer, sizeof(buffer), "%s failed (%s / %d)", stage, text ? text : "unknown MinHook error", static_cast<int>(status));
    g_lastError = buffer;
}

static void CreateRenderTarget(IDXGISwapChain* pSwapChain) {
    if (!pSwapChain || !g_device) return;

    ID3D11Texture2D* pBackBuffer = nullptr;
    if (SUCCEEDED(pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pBackBuffer))) && pBackBuffer) {
        g_device->CreateRenderTargetView(pBackBuffer, nullptr, &g_rtv);
        pBackBuffer->Release();
    }
}

static void CleanupRenderTarget() {
    if (g_rtv) {
        g_rtv->Release();
        g_rtv = nullptr;
    }
}

static LRESULT CALLBACK hkWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (GUI::WndProc(hWnd, msg, wParam, lParam))
        return true;
    return CallWindowProcW(oWndProc, hWnd, msg, wParam, lParam);
}

static HRESULT __stdcall hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {
    if (!g_initialized) {
        if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&g_device)))) {
            g_device->GetImmediateContext(&g_context);

            DXGI_SWAP_CHAIN_DESC desc{};
            if (SUCCEEDED(pSwapChain->GetDesc(&desc))) {
                g_hwnd = desc.OutputWindow;
                CreateRenderTarget(pSwapChain);

                if (GUI::Initialize(g_hwnd, g_device, g_context)) {
                    oWndProc = reinterpret_cast<WNDPROC>(
                        SetWindowLongPtrW(g_hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(hkWndProc)));
                    g_initialized = true;
                    Packet::Initialize();
                }
            }
        }
    }

    if (g_initialized && g_context && g_rtv) {
        GUI::BeginFrame();
        GUI::Render();
        GUI::EndFrame();

        g_context->OMSetRenderTargets(1, &g_rtv, nullptr);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }

    return oPresent(pSwapChain, SyncInterval, Flags);
}

static HRESULT __stdcall hkResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags) {
    CleanupRenderTarget();
    const HRESULT hr = oResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
    if (SUCCEEDED(hr)) {
        CreateRenderTarget(pSwapChain);
    }
    return hr;
}

static HRESULT CreateDummyDevice(IDXGISwapChain** pSwapchain, ID3D11Device** pDevice, ID3D11DeviceContext** pContext) {
    if (!pSwapchain || !pDevice || !pContext) return E_POINTER;

    *pSwapchain = nullptr;
    *pDevice = nullptr;
    *pContext = nullptr;

    HWND hwnd = GetForegroundWindow();
    if (!hwnd) return HRESULT_FROM_WIN32(ERROR_INVALID_WINDOW_HANDLE);

    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 1;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    D3D_FEATURE_LEVEL featureLevel{};
    const D3D_FEATURE_LEVEL featureLevelArray[] = {
        D3D_FEATURE_LEVEL_11_0
    };

    return D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0,
        featureLevelArray,
        static_cast<UINT>(std::size(featureLevelArray)),
        D3D11_SDK_VERSION,
        &sd,
        pSwapchain,
        pDevice,
        &featureLevel,
        pContext);
}

namespace Hooks {

bool Initialize() {
    g_lastError.clear();

    const MH_STATUS initStatus = MH_Initialize();
    if (initStatus != MH_OK) {
        SetMinHookError("MH_Initialize", initStatus);
        return false;
    }
    g_minhookInitialized = true;

    IDXGISwapChain* pSwapchain = nullptr;
    ID3D11Device* pDevice = nullptr;
    ID3D11DeviceContext* pContext = nullptr;

    const HRESULT deviceHr = CreateDummyDevice(&pSwapchain, &pDevice, &pContext);
    if (FAILED(deviceHr)) {
        SetHresultError("D3D11CreateDeviceAndSwapChain", deviceHr);
        MH_Uninitialize();
        g_minhookInitialized = false;
        return false;
    }

    void** vtable = *reinterpret_cast<void***>(pSwapchain);
    if (!vtable) {
        SetError("Swap-chain vtable was null");
        pContext->Release();
        pDevice->Release();
        pSwapchain->Release();
        MH_Uninitialize();
        g_minhookInitialized = false;
        return false;
    }

    const MH_STATUS presentStatus = MH_CreateHook(vtable[8], &hkPresent, reinterpret_cast<void**>(&oPresent));
    if (presentStatus != MH_OK) {
        SetMinHookError("MH_CreateHook(Present)", presentStatus);
        pContext->Release();
        pDevice->Release();
        pSwapchain->Release();
        MH_Uninitialize();
        g_minhookInitialized = false;
        return false;
    }

    const MH_STATUS resizeStatus = MH_CreateHook(vtable[13], &hkResizeBuffers, reinterpret_cast<void**>(&oResizeBuffers));
    if (resizeStatus != MH_OK) {
        SetMinHookError("MH_CreateHook(ResizeBuffers)", resizeStatus);
        MH_RemoveHook(vtable[8]);
        pContext->Release();
        pDevice->Release();
        pSwapchain->Release();
        MH_Uninitialize();
        g_minhookInitialized = false;
        return false;
    }

    const MH_STATUS enableStatus = MH_EnableHook(MH_ALL_HOOKS);
    if (enableStatus != MH_OK) {
        SetMinHookError("MH_EnableHook", enableStatus);
        MH_RemoveHook(vtable[13]);
        MH_RemoveHook(vtable[8]);
        pContext->Release();
        pDevice->Release();
        pSwapchain->Release();
        MH_Uninitialize();
        g_minhookInitialized = false;
        return false;
    }

    pContext->Release();
    pDevice->Release();
    pSwapchain->Release();
    return true;
}

void Shutdown() {
    Packet::Shutdown();

    if (g_initialized) {
        if (g_hwnd && oWndProc) {
            SetWindowLongPtrW(g_hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(oWndProc));
            oWndProc = nullptr;
        }

        GUI::Shutdown();
        CleanupRenderTarget();
        g_initialized = false;
    }

    if (g_minhookInitialized) {
        MH_DisableHook(MH_ALL_HOOKS);
        MH_Uninitialize();
        g_minhookInitialized = false;
    }

    if (g_context) {
        g_context->Release();
        g_context = nullptr;
    }
    if (g_device) {
        g_device->Release();
        g_device = nullptr;
    }
    g_hwnd = nullptr;
}

bool IsInitialized() {
    return g_initialized;
}

const char* GetLastErrorMessage() {
    return g_lastError.empty() ? "Unknown hook initialization error" : g_lastError.c_str();
}

} // namespace Hooks
