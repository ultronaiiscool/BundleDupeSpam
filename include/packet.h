#pragma once
#include <Windows.h>
#include <cstdint>
#include <vector>
#include <atomic>
#include <thread>

// ItemStackRequest action types
enum class StackRequestActionType : uint8_t {
    Take = 0,
    Place = 1,
    Swap = 2,
    Drop = 3,
    Destroy = 4,
    Consume = 5,
    Create = 6,
    PlaceInContainer = 7,
    TakeOutContainer = 8,
};

// Container IDs (approximate common values, version dependent)
enum class ContainerID : uint8_t {
    None = 0xFF,
    Inventory = 0,
    Container = 7,          // open container (dropper etc)
    Cursor = 58,
};

#pragma pack(push, 1)
struct FullContainerName {
    uint8_t containerId;
};

struct StackRequestSlotInfo {
    FullContainerName container;
    uint8_t slot;
    int32_t stackNetworkId;
};
#pragma pack(pop)

namespace Packet {
    extern std::atomic<bool> g_running;
    extern std::atomic<bool> g_enabled;
    extern std::atomic<int>  g_delayMs;
    extern std::atomic<int>  g_containerSlot;
    extern std::atomic<int>  g_count;
    extern std::atomic<int>  g_requestId;

    using SendPacketFn = void(__fastcall*)(void* networkHandler, void* packet);

    bool Initialize();
    void Shutdown();
    void StartSpamLoop();
    void StopSpamLoop();

    std::vector<uint8_t> BuildTakeFromBundlePacket(int requestId, uint8_t srcContainerId, uint8_t srcSlot, uint8_t count);
    void SendRaw(const std::vector<uint8_t>& data);
}
