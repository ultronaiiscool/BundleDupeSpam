#include "memory.h"
#include <Psapi.h>
#include <vector>

#pragma comment(lib, "Psapi.lib")

namespace Memory {

uintptr_t GetModuleBase(const wchar_t* moduleName) {
    return reinterpret_cast<uintptr_t>(GetModuleHandleW(moduleName));
}

uintptr_t FindPattern(uintptr_t start, size_t size, const char* pattern, const char* mask) {
    size_t patternLen = strlen(mask);
    for (size_t i = 0; i < size - patternLen; ++i) {
        bool found = true;
        for (size_t j = 0; j < patternLen; ++j) {
            if (mask[j] != '?' && pattern[j] != *reinterpret_cast<char*>(start + i + j)) {
                found = false;
                break;
            }
        }
        if (found) return start + i;
    }
    return 0;
}

uintptr_t FindPatternInModule(const wchar_t* moduleName, const char* pattern, const char* mask) {
    HMODULE mod = GetModuleHandleW(moduleName);
    if (!mod) return 0;

    MODULEINFO mi{};
    if (!GetModuleInformation(GetCurrentProcess(), mod, &mi, sizeof(mi))) return 0;

    return FindPattern(reinterpret_cast<uintptr_t>(mi.lpBaseOfDll), mi.SizeOfImage, pattern, mask);
}

bool WriteBytes(uintptr_t address, const std::vector<uint8_t>& bytes) {
    DWORD old;
    if (!VirtualProtect(reinterpret_cast<void*>(address), bytes.size(), PAGE_EXECUTE_READWRITE, &old))
        return false;
    memcpy(reinterpret_cast<void*>(address), bytes.data(), bytes.size());
    VirtualProtect(reinterpret_cast<void*>(address), bytes.size(), old, &old);
    return true;
}

void* ResolveRelative(uintptr_t address, int offset, int instructionSize) {
    int32_t rel = *reinterpret_cast<int32_t*>(address + offset);
    return reinterpret_cast<void*>(address + instructionSize + rel);
}

} // namespace Memory
