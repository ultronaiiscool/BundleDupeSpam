#pragma once
#include <Windows.h>
#include <vector>
#include <string>
#include <cstdint>

namespace Memory {
    uintptr_t GetModuleBase(const wchar_t* moduleName);
    uintptr_t FindPattern(uintptr_t start, size_t size, const char* pattern, const char* mask);
    uintptr_t FindPatternInModule(const wchar_t* moduleName, const char* pattern, const char* mask);
    bool WriteBytes(uintptr_t address, const std::vector<uint8_t>& bytes);
    void* ResolveRelative(uintptr_t address, int offset, int instructionSize);
}
