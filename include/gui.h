#pragma once
#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>

namespace GUI {
    bool Initialize(HWND hwnd, ID3D11Device* device, ID3D11DeviceContext* context);
    void Shutdown();
    void BeginFrame();
    void Render();
    void EndFrame();
    bool IsVisible();
    void Toggle();
    LRESULT WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
}
