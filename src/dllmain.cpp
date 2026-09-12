#include <Windows.h>
#include <string>
#include "hooks.h"
#include "packet.h"

static HMODULE g_hModule = nullptr;

static DWORD WINAPI MainThread(LPVOID) {
    Sleep(2000);

    if (!Hooks::Initialize()) {
        std::string message = "Failed to initialize hooks:\n\n";
        message += Hooks::GetLastErrorMessage();
        MessageBoxA(nullptr, message.c_str(), "BundleDupeSpam", MB_ICONERROR);
        FreeLibraryAndExitThread(g_hModule, 1);
    }

    while (true) {
        if (GetAsyncKeyState(VK_END) & 1) {
            break;
        }
        Sleep(100);
    }

    Hooks::Shutdown();
    FreeLibraryAndExitThread(g_hModule, 0);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        g_hModule = hModule;
        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, MainThread, nullptr, 0, nullptr);
    }
    return TRUE;
}
