#include "gui.h"
#include "packet.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <string>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace GUI {

static bool g_visible = true;
static ID3D11Device*           g_pd3dDevice = nullptr;
static ID3D11DeviceContext*    g_pd3dDeviceContext = nullptr;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;
static HWND                    g_hwnd = nullptr;

bool Initialize(HWND hwnd, ID3D11Device* device, ID3D11DeviceContext* context) {
    g_hwnd = hwnd;
    g_pd3dDevice = device;
    g_pd3dDeviceContext = context;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.08f, 0.10f, 0.94f);
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.12f, 0.12f, 0.18f, 1.0f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.18f, 0.18f, 0.28f, 1.0f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.25f, 0.35f, 0.55f, 1.0f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.35f, 0.45f, 0.70f, 1.0f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.20f, 0.30f, 0.50f, 1.0f);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.40f, 0.70f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.40f, 0.60f, 0.90f, 1.0f);

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(device, context);
    return true;
}

void Shutdown() {
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void BeginFrame() {
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void Render() {
    if (!g_visible) return;

    ImGui::SetNextWindowSize(ImVec2(380, 320), ImGuiCond_FirstUseEver);
    ImGui::Begin("Bundle Dupe Spam", &g_visible, ImGuiWindowFlags_NoCollapse);

    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Minecraft Bedrock Bundle Take Spam");
    ImGui::Separator();

    bool enabled = Packet::g_enabled.load();
    if (ImGui::Checkbox("Enable Spam", &enabled)) {
        if (enabled) Packet::StartSpamLoop();
        else Packet::StopSpamLoop();
    }

    ImGui::SameLine();
    ImGui::Text(enabled ? "[RUNNING]" : "[STOPPED]");

    int delay = Packet::g_delayMs.load();
    if (ImGui::SliderInt("Delay (ms)", &delay, 0, 50)) {
        Packet::g_delayMs = delay;
    }
    ImGui::TextDisabled("0 = maximum speed (yield)");

    int slot = Packet::g_containerSlot.load();
    if (ImGui::SliderInt("Bundle Slot in Dropper", &slot, 0, 8)) {
        Packet::g_containerSlot = slot;
    }

    int count = Packet::g_count.load();
    if (ImGui::SliderInt("Take Count", &count, 1, 64)) {
        Packet::g_count = count;
    }

    ImGui::Separator();
    ImGui::Text("Request ID: %d", Packet::g_requestId.load());
    ImGui::TextWrapped(
        "Open the dropper that contains the bundle first.\n"
        "The spam sends ItemStackRequest Take actions as fast as the delay allows.\n"
        "Normal clicks fail because the container state flips too fast; packets do not."
    );

    ImGui::Separator();
    if (ImGui::Button("Force Stop", ImVec2(120, 0))) {
        Packet::StopSpamLoop();
    }
    ImGui::SameLine();
    if (ImGui::Button("Hide GUI (Insert)", ImVec2(140, 0))) {
        g_visible = false;
    }

    ImGui::End();
}

void EndFrame() {
    ImGui::Render();
}

bool IsVisible() {
    return g_visible;
}

void Toggle() {
    g_visible = !g_visible;
}

LRESULT WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    if (msg == WM_KEYDOWN && wParam == VK_INSERT) {
        Toggle();
        return 0;
    }
    return 0;
}

} // namespace GUI
