#include "hooks.h"
#include "gui.h"
#include "packet.h"
#include "memory.h"
#include <d3d11.h>
#include <dxgi.h>
#include <MinHook.h>
#include <thread>

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

static void CreateRenderTarget(IDXGISwapChain* pSwapChain) {
    ID3D11Texture2D* pBackBuffer = nullptr;
    pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer);
    if (pBackBuffer) {
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
        if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&g_device))) {
            g_device->GetImmediateContext(&g_context);

            DXGI_SWAP_CHAIN_DESC desc;
            pSwapChain->GetDesc(&desc);
            g_hwnd = desc.OutputWindow;

            CreateRenderTarget(pSwapChain);
            GUI::Initialize(g_hwnd, g_device, g_context);

            oWndProc = (WNDPROC)SetWindowLongPtrW(g_hwnd, GWLP_WNDPROC, (LONG_PTR)hkWndProc);
            g_initialized = true;

            Packet::Initialize();
        }
    }

    if (g_initialized) {
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
    HRESULT hr = oResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
    CreateRenderTarget(pSwapChain);
    return hr;
}

static bool CreateDummyDevice(void** pSwapchain, void** pDevice, void** pContext) {
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 1;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = GetForegroundWindow();
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[1] = { D3D_FEATURE_LEVEL_11_0 };

    return SUCCEEDED(D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
        featureLevelArray, 1, D3D11_SDK_VERSION,
        &sd, (IDXGISwapChain**)pSwapchain,
        (ID3D11Device**)pDevice, &featureLevel,
        (ID3D11DeviceContext**)pContext));
}

namespace Hooks {

bool Initialize() {
    if (MH_Initialize() != MH_OK) return false;

    void* pSwapchain = nullptr;
    void* pDevice = nullptr;
    void* pContext = nullptr;

    if (!CreateDummyDevice(&pSwapchain, &pDevice, &pContext)) return false;

    void** vtable = *reinterpret_cast<void***>(pSwapchain);

    if (MH_CreateHook(vtable[8], &hkPresent, reinterpret_cast<void**>(&oPresent)) != MH_OK) return false;
    if (MH_CreateHook(vtable[13], &hkResizeBuffers, reinterpret_cast<void**>(&oResizeBuffers)) != MH_OK) return false;

    if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK) return false;

    if (pContext) ((ID3D11DeviceContext*)pContext)->Release();
    if (pDevice) ((ID3D11Device*)pDevice)->Release();
    if (pSwapchain) ((IDXGISwapChain*)pSwapchain)->Release();

    return true;
}

void Shutdown() {
    Packet::Shutdown();
    GUI::Shutdown();
    CleanupRenderTarget();

    if (g_hwnd && oWndProc)
        SetWindowLongPtrW(g_hwnd, GWLP_WNDPROC, (LONG_PTR)oWndProc);

    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();

    if (g_context) g_context->Release();
    if (g_device) g_device->Release();
}

bool IsInitialized() {
    return g_initialized;
}

} // namespace Hooks
