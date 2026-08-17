#pragma once

#include <cstdint>

namespace fwa {

class ImGuiOverlay {
public:
    static ImGuiOverlay& get();

    void toggle();
    void setVisible(bool value);
    [[nodiscard]] bool visible() const { return m_visible; }

    void drawFrame();
    void shutdown();
    long long handleWndProc(void* hwnd, unsigned int msg, std::uintptr_t wParam, std::intptr_t lParam);

private:
    bool init();
    void applyStyle();
    void drawWindow();
    void drawLogo();

    bool m_visible = false;
    bool m_initialized = false;
    void* m_hwnd = nullptr;
    void* m_oldWndProc = nullptr;
};

} // namespace fwa
