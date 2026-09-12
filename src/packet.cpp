#include "packet.h"
#include "memory.h"
#include <thread>
#include <chrono>
#include <mutex>
#include <deque>

namespace Packet {

std::atomic<bool> g_running{false};
std::atomic<bool> g_enabled{false};
std::atomic<int>  g_delayMs{0};
std::atomic<int>  g_containerSlot{0};
std::atomic<int>  g_count{1};
std::atomic<int>  g_requestId{1};

static std::thread g_spamThread;
static std::mutex  g_queueMutex;
static std::deque<std::vector<uint8_t>> g_sendQueue;

static void* g_networkHandler = nullptr;
static void(__fastcall* g_originalSend)(void*, void*, void*) = nullptr;

static void WriteVarInt(std::vector<uint8_t>& buf, uint32_t value) {
    while (value >= 0x80) {
        buf.push_back(static_cast<uint8_t>(value | 0x80));
        value >>= 7;
    }
    buf.push_back(static_cast<uint8_t>(value));
}

static void WriteZigZag32(std::vector<uint8_t>& buf, int32_t value) {
    uint32_t zz = (static_cast<uint32_t>(value) << 1) ^ static_cast<uint32_t>(value >> 31);
    WriteVarInt(buf, zz);
}

std::vector<uint8_t> BuildTakeFromBundlePacket(int requestId, uint8_t srcContainerId, uint8_t srcSlot, uint8_t count) {
    std::vector<uint8_t> packet;

    WriteVarInt(packet, 1); // num requests
    WriteZigZag32(packet, requestId);
    WriteVarInt(packet, 1); // num actions

    packet.push_back(static_cast<uint8_t>(StackRequestActionType::Take));
    packet.push_back(count);

    // Source
    packet.push_back(srcContainerId);
    packet.push_back(srcSlot);
    WriteZigZag32(packet, 0);

    // Destination (cursor)
    packet.push_back(58);
    packet.push_back(0);
    WriteZigZag32(packet, 0);

    WriteVarInt(packet, 0); // filter strings
    int32_t cause = 0;
    packet.insert(packet.end(), reinterpret_cast<uint8_t*>(&cause), reinterpret_cast<uint8_t*>(&cause) + 4);

    return packet;
}

void SendRaw(const std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lock(g_queueMutex);
    g_sendQueue.push_back(data);
}

static void SpamLoop() {
    while (g_running) {
        if (g_enabled) {
            int reqId = g_requestId.fetch_add(1);
            auto pkt = BuildTakeFromBundlePacket(
                reqId,
                static_cast<uint8_t>(7),
                static_cast<uint8_t>(g_containerSlot.load()),
                static_cast<uint8_t>(g_count.load())
            );
            SendRaw(pkt);
        }

        int delay = g_delayMs.load();
        if (delay <= 0) {
            std::this_thread::yield();
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
        }
    }
}

bool Initialize() {
    g_running = true;
    g_spamThread = std::thread(SpamLoop);
    return true;
}

void Shutdown() {
    g_running = false;
    g_enabled = false;
    if (g_spamThread.joinable()) g_spamThread.join();
}

void StartSpamLoop() {
    g_enabled = true;
}

void StopSpamLoop() {
    g_enabled = false;
}

} // namespace Packet
