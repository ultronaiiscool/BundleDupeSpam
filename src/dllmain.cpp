#include <Windows.h>
#include <thread>
#include "hooks.h"
#include "packet.h"

static HMODULE g_hModule = nullptr;

static DWORD WINAPI MainThread(LPVOID) {
    Sleep(2000);

    if (!Hooks::Initialize()) {
        MessageBoxA(nullptr, "Failed to initialize hooks", "BundleDupeSpam", MB_ICONERROR);
        FreeLibraryAndExitThread(g_hModule, 1);
        return 1;
    }

    while (true) {
        if (GetAsyncKeyState(VK_END) & 1) {
            break;
        }
        Sleep(100);
    }

    Hooks::Shutdown();
    FreeLibraryAndExitThread(g_hModule, 0);
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        g_hModule = hModule;
        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, MainThread, nullptr, 0, nullptr);
    }
    return TRUE;
}
