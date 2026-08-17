#include "ui/ImGuiOverlay.hpp"

#include "runtime/BotController.hpp"
#include "runtime/FileLogger.hpp"

#include <Geode/Geode.hpp>
#include <Geode/loader/Mod.hpp>
#include <Geode/modify/CCEGLView.hpp>

#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_win32.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <GL/gl.h>

#include <algorithm>
#include <climits>
#include <filesystem>
#include <string>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

using namespace geode::prelude;

namespace fwa {
namespace {

constexpr float kSafeMargin = 16.0f;
constexpr float kSafeTop = 78.0f;
constexpr float kWindowWidth = 430.0f;
constexpr float kWindowHeight = 490.0f;

LRESULT CALLBACK fwbotWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    auto& ui = ImGuiOverlay::get();
    auto result = ui.handleWndProc(hwnd, msg, static_cast<std::uintptr_t>(wParam), static_cast<std::intptr_t>(lParam));
    if (result != LLONG_MIN) return static_cast<LRESULT>(result);
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void modeBadge(BotMode mode) {
    char const* text = "IDLE";
    ImVec4 color(0.40f, 0.42f, 0.46f, 1.0f);
    switch (mode) {
        case BotMode::Idle: break;
        case BotMode::Recording: text = "REC"; color = ImVec4(0.90f, 0.28f, 0.30f, 1.0f); break;
        case BotMode::Playback: text = "PLAY"; color = ImVec4(0.27f, 0.67f, 0.95f, 1.0f); break;
        case BotMode::Analyzing: text = "ANALYZE"; color = ImVec4(0.65f, 0.48f, 0.95f, 1.0f); break;
    }
    ImGui::PushStyleColor(ImGuiCol_Text, color);
    ImGui::TextUnformatted(text);
    ImGui::PopStyleColor();
}

void helpText(char const* text) {
    ImGui::PushTextWrapPos(0.0f);
    ImGui::TextDisabled("%s", text);
    ImGui::PopTextWrapPos();
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
    style.WindowRounding = 11.0f;
    style.ChildRounding = 8.0f;
    style.FrameRounding = 6.0f;
    style.PopupRounding = 8.0f;
    style.TabRounding = 6.0f;
    style.ScrollbarRounding = 8.0f;
    style.GrabRounding = 6.0f;
    style.WindowPadding = ImVec2(14.0f, 13.0f);
    style.FramePadding = ImVec2(9.0f, 6.0f);
    style.ItemSpacing = ImVec2(8.0f, 7.0f);
    style.WindowBorderSize = 1.0f;

    auto* c = style.Colors;
    c[ImGuiCol_WindowBg]         = ImVec4(0.020f, 0.021f, 0.025f, 0.975f);
    c[ImGuiCol_ChildBg]          = ImVec4(0.040f, 0.042f, 0.050f, 0.94f);
    c[ImGuiCol_PopupBg]          = ImVec4(0.035f, 0.036f, 0.043f, 0.985f);
    c[ImGuiCol_Border]           = ImVec4(0.14f, 0.15f, 0.18f, 0.92f);
    c[ImGuiCol_FrameBg]          = ImVec4(0.080f, 0.083f, 0.098f, 1.0f);
    c[ImGuiCol_FrameBgHovered]   = ImVec4(0.125f, 0.13f, 0.15f, 1.0f);
    c[ImGuiCol_FrameBgActive]    = ImVec4(0.17f, 0.175f, 0.20f, 1.0f);
    c[ImGuiCol_Button]           = ImVec4(0.095f, 0.10f, 0.12f, 1.0f);
    c[ImGuiCol_ButtonHovered]    = ImVec4(0.15f, 0.16f, 0.19f, 1.0f);
    c[ImGuiCol_ButtonActive]     = ImVec4(0.21f, 0.22f, 0.26f, 1.0f);
    c[ImGuiCol_Header]           = ImVec4(0.095f, 0.10f, 0.12f, 1.0f);
    c[ImGuiCol_HeaderHovered]    = ImVec4(0.15f, 0.16f, 0.19f, 1.0f);
    c[ImGuiCol_HeaderActive]     = ImVec4(0.20f, 0.21f, 0.25f, 1.0f);
    c[ImGuiCol_Tab]              = ImVec4(0.065f, 0.068f, 0.080f, 1.0f);
    c[ImGuiCol_TabHovered]       = ImVec4(0.16f, 0.17f, 0.20f, 1.0f);
    c[ImGuiCol_TabSelected]      = ImVec4(0.13f, 0.135f, 0.16f, 1.0f);
    c[ImGuiCol_CheckMark]        = ImVec4(0.90f, 0.91f, 0.96f, 1.0f);
    c[ImGuiCol_SliderGrab]       = ImVec4(0.68f, 0.70f, 0.78f, 1.0f);
    c[ImGuiCol_SliderGrabActive] = ImVec4(0.90f, 0.91f, 0.97f, 1.0f);
    c[ImGuiCol_Text]             = ImVec4(0.93f, 0.94f, 0.97f, 1.0f);
    c[ImGuiCol_TextDisabled]     = ImVec4(0.49f, 0.51f, 0.57f, 1.0f);
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
    m_iniPath = (Mod::get()->getSaveDir() / "fwbot_imgui.ini").string();
    io.IniFilename = m_iniPath.c_str();
    io.LogFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    applyStyle();
    if (!ImGui_ImplWin32_InitForOpenGL(m_hwnd)) return false;
    if (!ImGui_ImplOpenGL3_Init("#version 130")) return false;

    m_oldWndProc = reinterpret_cast<void*>(SetWindowLongPtrW(
        static_cast<HWND>(m_hwnd), GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(fwbotWndProc)
    ));
    m_initialized = true;
    FileLogger::get().debug("[UI] Dear ImGui Win32/OpenGL3 initialized; position persisted in fwbot_imgui.ini");
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
        bool fwbotFunctionKey = wParam >= VK_F3 && wParam <= VK_F8;
        // Keep FWBot's own function-key binds alive while the menu has keyboard
        // focus. Otherwise F8 could open the ImGui window but be swallowed by
        // ImGui and never reach Geode again to close it.
        if ((mouseMsg && io.WantCaptureMouse) ||
            (keyMsg && io.WantCaptureKeyboard && !fwbotFunctionKey)) {
            return 1;
        }
    }

    auto oldProc = reinterpret_cast<WNDPROC>(m_oldWndProc);
    return static_cast<long long>(CallWindowProcW(
        oldProc, hwnd, static_cast<UINT>(msg), static_cast<WPARAM>(wParam), static_cast<LPARAM>(lParam)
    ));
}

void ImGuiOverlay::drawLogo() {
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImVec2 size(40.0f, 40.0f);
    auto* draw = ImGui::GetWindowDrawList();
    draw->AddRectFilled(p, ImVec2(p.x + size.x, p.y + size.y), IM_COL32(18, 19, 23, 255), 9.0f);
    draw->AddRect(p, ImVec2(p.x + size.x, p.y + size.y), IM_COL32(78, 82, 94, 255), 9.0f, 0, 1.0f);
    draw->AddText(ImVec2(p.x + 7.0f, p.y + 11.0f), IM_COL32(242, 244, 250, 255), "FW");
    ImGui::Dummy(size);
}

void ImGuiOverlay::clampWindowToSafeArea() {
    auto const& io = ImGui::GetIO();
    ImVec2 pos = ImGui::GetWindowPos();
    ImVec2 size = ImGui::GetWindowSize();

    float maxX = std::max(kSafeMargin, io.DisplaySize.x - size.x - kSafeMargin);
    float maxY = std::max(kSafeTop, io.DisplaySize.y - size.y - kSafeMargin);
    ImVec2 clamped(
        std::clamp(pos.x, kSafeMargin, maxX),
        std::clamp(pos.y, kSafeTop, maxY)
    );

    if (clamped.x != pos.x || clamped.y != pos.y) {
        ImGui::SetWindowPos(clamped, ImGuiCond_Always);
    }
}

void ImGuiOverlay::drawWindow() {
    auto& bot = BotController::get();
    auto& io = ImGui::GetIO();

    ImGui::SetNextWindowSize(ImVec2(kWindowWidth, kWindowHeight), ImGuiCond_FirstUseEver);
    if (m_firstWindowPlacement) {
        // Default to the right-middle safe zone: away from GD's top progress bar
        // and the common top-left mod/HUD buttons. ImGui persists any user drag.
        ImVec2 initial(
            std::max(kSafeMargin, io.DisplaySize.x - kWindowWidth - 24.0f),
            std::max(kSafeTop, io.DisplaySize.y * 0.18f)
        );
        ImGui::SetNextWindowPos(initial, ImGuiCond_FirstUseEver);
        m_firstWindowPlacement = false;
    }

    if (!ImGui::Begin("FWBot##main", &m_visible, ImGuiWindowFlags_NoCollapse)) {
        ImGui::End();
        return;
    }
    clampWindowToSafeArea();

    drawLogo();
    ImGui::SameLine();
    ImGui::BeginGroup();
    ImGui::TextUnformatted("FWBot");
    ImGui::SameLine();
    modeBadge(bot.mode());
    ImGui::TextDisabled("Frame Window Analyzer");
    ImGui::TextDisabled("%s", bot.statusText().c_str());
    ImGui::EndGroup();
    ImGui::Separator();

    if (ImGui::BeginTabBar("##fwbot-tabs")) {
        if (ImGui::BeginTabItem("Macro")) {
            bool recording = bot.mode() == BotMode::Recording;
            if (recording) {
                if (ImGui::Button("Stop recording", ImVec2(180, 0))) bot.stopRecording(PlayLayer::get());
            } else {
                if (ImGui::Button("Record", ImVec2(180, 0))) bot.startRecording(PlayLayer::get());
            }
            ImGui::SameLine();
            if (ImGui::Button("Playback", ImVec2(100, 0))) bot.startPlayback(PlayLayer::get());
            ImGui::SameLine();
            if (ImGui::Button("Analyze", ImVec2(90, 0))) bot.startAnalysis(PlayLayer::get());

            ImGui::Spacing();
            ImGui::Text("Inputs: %zu", bot.lastMacro().inputs.size());
            ImGui::Text("Practice anchor: %s", bot.hasPracticeAnchor() ? "ready" : "none");
            helpText("Recommended: place a Practice checkpoint before the timing, press Record, then use realtime or Frame Stepper. Record itself never freezes gameplay.");
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Tools")) {
            bool stepper = bot.frameStepperEnabled();
            if (ImGui::Checkbox("Frame Stepper", &stepper)) bot.setFrameStepper(stepper);
            ImGui::SameLine();
            if (ImGui::Button("Next frame")) bot.requestFrameStep();
            ImGui::TextDisabled("F4 toggle | F5 next | queued: %d", bot.pendingSteps());
            helpText("Stepper freezes scheduler physics without opening Geometry Dash PauseLayer. Turning it off resumes immediately without catch-up delta.");

            ImGui::Separator();
            ImGui::TextDisabled("SPEEDHACK");
            bool enabled = bot.speedhackEnabled();
            if (ImGui::Checkbox("Enabled##speedhack", &enabled)) bot.setSpeedhackEnabled(enabled);
            float speed = bot.speedhackSpeed();
            if (ImGui::SliderFloat("Speed", &speed, 0.10f, 5.00f, "%.2fx", ImGuiSliderFlags_Logarithmic)) {
                bot.setSpeedhackSpeed(speed);
            }
            bool audio = bot.speedhackAudio();
            if (ImGui::Checkbox("Audio follows speed", &audio)) bot.setSpeedhackAudio(audio);
            helpText("Audio pitch is applied immediately and re-synchronized after respawns. Automatic analysis never speeds the audio up.");
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Analysis")) {
            ImGui::Text("Physics frame: %u", bot.currentFrame());
            if (bot.hasMacro()) {
                ImGui::Text("Last macro: %zu inputs", bot.lastMacro().inputs.size());
                ImGui::Text("Last input frame: %u", bot.lastMacro().lastFrame());
            } else {
                ImGui::TextDisabled("No recorded macro yet.");
            }
            helpText("FWBot tests every HOLD/RELEASE independently on earlier and later physics frames. The contiguous PASS window containing your recorded input is exported as N_i for NaNDL.");
            helpText("Scan radius, exhaustive scan and validation horizon are configured in Geode mod settings.");
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Visuals")) {
            bool trajectory = bot.trajectoryVisible();
            if (ImGui::Checkbox("Predictive trajectory", &trajectory)) bot.toggleTrajectory();
            helpText("Trajectory always starts OFF on game launch and only appears when explicitly enabled. Prediction is visual only; analyzer PASS/FAIL uses real gameplay branches.");
            ImGui::TextDisabled("Toggle: Ctrl+F5");
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Debug")) {
            ImGui::TextWrapped("If a runtime issue remains, send logs/debug/latest.log and the analysis .log.");
            if (ImGui::Button("Cancel playback / analysis")) bot.cancelAutomation();
            ImGui::SameLine();
            if (ImGui::Button("Disable stepper")) bot.setFrameStepper(false);
            ImGui::Separator();
            ImGui::TextDisabled("F3 Speedhack | F4 Stepper | F5 Next");
            ImGui::TextDisabled("F6 Record | F7 Playback | F8 Menu");
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

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
    ImGui::SaveIniSettingsToDisk(m_iniPath.c_str());
    if (m_hwnd && m_oldWndProc) {
        SetWindowLongPtrW(static_cast<HWND>(m_hwnd), GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(m_oldWndProc));
    }
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    m_initialized = false;
    m_firstWindowPlacement = true;
    m_hwnd = nullptr;
    m_oldWndProc = nullptr;
    m_iniPath.clear();
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
