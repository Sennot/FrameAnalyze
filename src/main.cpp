#include "runtime/BotController.hpp"
#include "ui/BotMenu.hpp"

#include <Geode/Geode.hpp>
#include <Geode/loader/GameEvent.hpp>
#include <Geode/loader/SettingV3.hpp>

#include <utility>

using namespace geode::prelude;

namespace {

template <class F>
void bindPress(char const* setting, F&& callback) {
    listenForKeybindSettingPresses(setting,
        [fn = std::forward<F>(callback)](Keybind const&, bool down, bool repeat, double) mutable -> bool {
            if (down && !repeat) fn();
            // KeybindSettingPressedEventV3 listeners return bool. Returning false
            // lets the event continue to other listeners instead of consuming it.
            return false;
        }
    );
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
