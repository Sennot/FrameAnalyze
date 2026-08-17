#include "ui/ImGuiOverlay.hpp"

#include "runtime/BotController.hpp"
#include "runtime/FileLogger.hpp"

#include <Geode/Geode.hpp>
#include <Geode/loader/Mod.hpp>

#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_win32.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <GL/gl.h>

#include <algorithm>
#include <climits>
#include <string>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

using namespace geode::prelude;

namespace fwa {
namespace {

LRESULT CALLBACK fwbotWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    auto& ui = ImGuiOverlay::get();
    auto result = ui.handleWndProc(hwnd, msg, static_cast<std::uintptr_t>(wParam), static_cast<std::intptr_t>(lParam));
    if (result != LLONG_MIN) return static_cast<LRESULT>(result);
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

bool sectionHeader(char const* label) {
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.13f, 0.13f, 0.15f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.16f, 0.16f, 0.18f, 1.0f));
    bool open = ImGui::CollapsingHeader(label, ImGuiTreeNodeFlags_DefaultOpen);
    ImGui::PopStyleColor(2);
    return open;
}

} // namespace

ImGuiOverlay& ImGuiOverlay::get() {
    static ImGuiOverlay overlay;
    return overlay;
}

void ImGuiOverlay::toggle() {
    setVisible(!m_visible);
}

void ImGuiOverlay::setVisible(bool value) {
    m_visible = value;
}

void ImGuiOverlay::applyStyle() {
    auto& style = ImGui::GetStyle();
    style.WindowRounding = 12.0f;
    style.ChildRounding = 8.0f;
    style.FrameRounding = 7.0f;
    style.PopupRounding = 8.0f;
    style.ScrollbarRounding = 8.0f;
    style.GrabRounding = 7.0f;
    style.WindowPadding = ImVec2(14.0f, 14.0f);
    style.FramePadding = ImVec2(9.0f, 6.0f);
    style.ItemSpacing = ImVec2(8.0f, 8.0f);

    auto* c = style.Colors;
    c[ImGuiCol_WindowBg]       = ImVec4(0.025f, 0.025f, 0.030f, 0.97f);
    c[ImGuiCol_ChildBg]        = ImVec4(0.050f, 0.050f, 0.058f, 0.92f);
    c[ImGuiCol_PopupBg]        = ImVec4(0.045f, 0.045f, 0.052f, 0.98f);
    c[ImGuiCol_Border]         = ImVec4(0.16f, 0.16f, 0.18f, 0.80f);
    c[ImGuiCol_FrameBg]        = ImVec4(0.095f, 0.095f, 0.105f, 1.0f);
    c[ImGuiCol_FrameBgHovered] = ImVec4(0.14f, 0.14f, 0.155f, 1.0f);
    c[ImGuiCol_FrameBgActive]  = ImVec4(0.18f, 0.18f, 0.20f, 1.0f);
    c[ImGuiCol_Button]         = ImVec4(0.12f, 0.12f, 0.135f, 1.0f);
    c[ImGuiCol_ButtonHovered]  = ImVec4(0.20f, 0.20f, 0.22f, 1.0f);
    c[ImGuiCol_ButtonActive]   = ImVec4(0.27f, 0.27f, 0.30f, 1.0f);
    c[ImGuiCol_CheckMark]      = ImVec4(0.90f, 0.90f, 0.94f, 1.0f);
    c[ImGuiCol_SliderGrab]     = ImVec4(0.72f, 0.72f, 0.78f, 1.0f);
    c[ImGuiCol_SliderGrabActive] = ImVec4(0.92f, 0.92f, 0.96f, 1.0f);
    c[ImGuiCol_Text]           = ImVec4(0.93f, 0.93f, 0.95f, 1.0f);
    c[ImGuiCol_TextDisabled]   = ImVec4(0.50f, 0.50f, 0.55f, 1.0f);
}

bool ImGuiOverlay::init() {
    if (m_initialized) return true;

    HDC dc = wglGetCurrentDC();
    if (!dc) return false;
    m_hwnd = static_cast<void*>(WindowFromDC(dc));
    if (!m_hwnd) return false;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    applyStyle();
    if (!ImGui_ImplWin32_InitForOpenGL(m_hwnd)) return false;
    if (!ImGui_ImplOpenGL3_Init("#version 130")) return false;

    m_oldWndProc = reinterpret_cast<void*>(SetWindowLongPtrW(static_cast<HWND>(m_hwnd), GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(fwbotWndProc)));
    m_initialized = true;
    FileLogger::get().debug("[UI] Dear ImGui Win32/OpenGL3 initialized");
    return true;
}

long long ImGuiOverlay::handleWndProc(void* hwndRaw, unsigned int msg, std::uintptr_t wParam, std::intptr_t lParam) {
    if (!m_initialized || !m_oldWndProc) return LLONG_MIN;
    HWND hwnd = static_cast<HWND>(hwndRaw);

    if (m_visible) {
        ImGui_ImplWin32_WndProcHandler(hwnd, static_cast<UINT>(msg), static_cast<WPARAM>(wParam), static_cast<LPARAM>(lParam));
        auto const& io = ImGui::GetIO();
        bool mouseMsg = msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST;
        bool keyMsg = (msg >= WM_KEYFIRST && msg <= WM_KEYLAST) || msg == WM_CHAR;
        if ((mouseMsg && io.WantCaptureMouse) || (keyMsg && io.WantCaptureKeyboard)) {
            return 1;
        }
    }

    auto oldProc = reinterpret_cast<WNDPROC>(m_oldWndProc);
    return static_cast<long long>(CallWindowProcW(oldProc, hwnd, static_cast<UINT>(msg), static_cast<WPARAM>(wParam), static_cast<LPARAM>(lParam)));
}

void ImGuiOverlay::drawLogo() {
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImVec2 size(42.0f, 42.0f);
    auto* draw = ImGui::GetWindowDrawList();
    draw->AddRectFilled(p, ImVec2(p.x + size.x, p.y + size.y), IM_COL32(24, 24, 28, 255), 9.0f);
    draw->AddRect(p, ImVec2(p.x + size.x, p.y + size.y), IM_COL32(88, 88, 96, 255), 9.0f, 0, 1.0f);
    draw->AddText(ImVec2(p.x + 8.0f, p.y + 12.0f), IM_COL32(240, 240, 245, 255), "FW");
    ImGui::Dummy(size);
}

void ImGuiOverlay::drawWindow() {
    auto& bot = BotController::get();

    ImGui::SetNextWindowSize(ImVec2(460.0f, 610.0f), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("FWBot##main", &m_visible, ImGuiWindowFlags_NoCollapse)) {
        ImGui::End();
        return;
    }

    drawLogo();
    ImGui::SameLine();
    ImGui::BeginGroup();
    ImGui::TextUnformatted("FWBot");
    ImGui::TextDisabled("Frame Window / Practice Bot");
    ImGui::TextDisabled("%s", bot.statusText().c_str());
    ImGui::EndGroup();
    ImGui::Separator();

    if (sectionHeader("Macro")) {
        if (bot.mode() == BotMode::Recording) {
            if (ImGui::Button("Stop recording", ImVec2(200, 0))) bot.stopRecording(PlayLayer::get());
        } else {
            if (ImGui::Button("Record from current state", ImVec2(200, 0))) bot.startRecording(PlayLayer::get());
        }
        ImGui::SameLine();
        if (ImGui::Button("Playback", ImVec2(110, 0))) bot.startPlayback(PlayLayer::get());
        ImGui::SameLine();
        if (ImGui::Button("Analyze", ImVec2(100, 0))) bot.startAnalysis(PlayLayer::get());
        ImGui::TextDisabled("Practice workflow: place checkpoint -> Record -> play section -> Stop.");
        ImGui::Text("Macro inputs: %zu", bot.lastMacro().inputs.size());
        ImGui::SameLine();
        ImGui::TextDisabled("Anchor: %s", bot.hasPracticeAnchor() ? "ready" : "none");
    }

    if (sectionHeader("Frame tools")) {
        bool stepper = bot.frameStepperEnabled();
        if (ImGui::Checkbox("Frame Stepper", &stepper)) bot.setFrameStepper(stepper);
        ImGui::SameLine();
        if (ImGui::Button("Next frame")) bot.requestFrameStep();
        ImGui::SameLine();
        ImGui::TextDisabled("queued: %d", bot.pendingSteps());
        ImGui::TextDisabled("Does not open Geometry Dash PauseLayer.");
    }

    if (sectionHeader("Speedhack")) {
        bool enabled = bot.speedhackEnabled();
        if (ImGui::Checkbox("Enabled", &enabled)) bot.setSpeedhackEnabled(enabled);
        float speed = bot.speedhackSpeed();
        if (ImGui::SliderFloat("Speed", &speed, 0.10f, 5.00f, "%.2fx", ImGuiSliderFlags_Logarithmic)) {
            bot.setSpeedhackSpeed(speed);
        }
        bool audio = bot.speedhackAudio();
        if (ImGui::Checkbox("Audio follows speed", &audio)) bot.setSpeedhackAudio(audio);
    }

    if (sectionHeader("Analysis")) {
        ImGui::Text("Current physics frame: %u", bot.currentFrame());
        ImGui::TextDisabled("Frame Window N_i is searched on both sides of every recorded HOLD/RELEASE.");
        ImGui::TextDisabled("Fast scan stops after first FAIL per side; Exhaustive Scan is available in Geode settings.");
        if (bot.hasMacro()) {
            ImGui::Text("Last macro: %zu inputs / last frame %u", bot.lastMacro().inputs.size(), bot.lastMacro().lastFrame());
        }
    }

    if (sectionHeader("Visuals")) {
        bool trajectory = bot.trajectoryVisible();
        if (ImGui::Checkbox("Show predictive trajectory", &trajectory)) bot.toggleTrajectory();
        ImGui::TextDisabled("Hold/release fake-player branches. Analyzer verdicts use real gameplay physics.");
    }

    if (sectionHeader("Debug")) {
        ImGui::TextWrapped("If something breaks, send logs/debug/latest.log together with the analysis .log.");
        if (ImGui::Button("Cancel playback/analyze")) bot.cancelAutomation();
    }

    ImGui::Separator();
    ImGui::TextDisabled("F4 Stepper | F5 Next | F6 Record | F7 Playback | F8 Menu");
    ImGui::End();
}

void ImGuiOverlay::drawFrame() {
    if (!init()) return;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    if (m_visible) drawWindow();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void ImGuiOverlay::shutdown() {
    if (!m_initialized) return;
    if (m_hwnd && m_oldWndProc) {
        SetWindowLongPtrW(static_cast<HWND>(m_hwnd), GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(m_oldWndProc));
    }
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    m_initialized = false;
    m_hwnd = nullptr;
    m_oldWndProc = nullptr;
}

} // namespace fwa

namespace fwa {

struct FWBotEGLViewHook : Modify<FWBotEGLViewHook, CCEGLView> {
    void swapBuffers() {
        ImGuiOverlay::get().drawFrame();
        CCEGLView::swapBuffers();
    }
};

} // namespace fwa
