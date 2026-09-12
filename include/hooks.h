#pragma once
#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>

namespace Hooks {
    bool Initialize();
    void Shutdown();
    bool IsInitialized();
    const char* GetLastErrorMessage();
}
