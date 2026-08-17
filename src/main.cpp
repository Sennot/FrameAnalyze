#include "runtime/BotController.hpp"
#include "ui/BotMenu.hpp"

#include <Geode/Geode.hpp>
#include <Geode/loader/GameEvent.hpp>
#include <Geode/loader/SettingV3.hpp>

#include <type_traits>

using namespace geode::prelude;

namespace {

using KeyAction = void (*)();

struct KeybindPressListener {
    KeyAction action = nullptr;

    bool operator()(Keybind const&, bool down, bool repeat, double) const {
        if (down && !repeat && action) {
            action();
        }
        // Geode EventV3: false means propagate instead of consuming the event.
        return false;
    }
};

static_assert(std::is_invocable_r_v<
    bool,
    KeybindPressListener const&,
    Keybind const&,
    bool,
    bool,
    double
>);

void bindPress(char const* setting, KeyAction action) {
    listenForKeybindSettingPresses(setting, KeybindPressListener{action});
}

} // namespace

$on_game(Loaded) {
    fwa::BotController::get().initialize();

    bindPress("open-menu", [] { fwa::BotMenu::toggle(); });
    bindPress("toggle-record", [] {
        fwa::BotController::get().toggleRecording(PlayLayer::get());
    });
    bindPress("playback", [] {
        fwa::BotController::get().startPlayback(PlayLayer::get());
    });
    bindPress("toggle-pause", [] {
        fwa::BotController::get().togglePause();
    });
    bindPress("frame-step", [] {
        fwa::BotController::get().requestFrameStep();
    });
    bindPress("toggle-trajectory", [] {
        fwa::BotController::get().toggleTrajectory();
    });
    bindPress("analyze-last", [] {
        fwa::BotController::get().startAnalysis(PlayLayer::get());
    });
}
