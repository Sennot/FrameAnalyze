#pragma once

#include <array>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace fwa {

enum class InputAction {
    Hold,
    Release,
};

struct MacroInput {
    std::uint32_t frame = 0;
    bool player2 = false;
    int button = 1;
    InputAction action = InputAction::Hold;
    std::size_t sequence = 0;
};

struct AnchorInputState {
    // Geometry Dash buttons are 1..3 in the bindings used by the bot.
    std::array<bool, 4> p1{};
    std::array<bool, 4> p2{};

    [[nodiscard]] bool held(bool player2, int button) const {
        if (button < 0 || button >= static_cast<int>(p1.size())) return false;
        return player2 ? p2[static_cast<std::size_t>(button)] : p1[static_cast<std::size_t>(button)];
    }
};

struct Macro {
    int gameFps = 240;
    int windowFps = 240;
    std::string levelName;
    int levelId = 0;
    bool practiceAnchored = false;
    AnchorInputState anchorInputs;
    std::vector<MacroInput> inputs;

    [[nodiscard]] std::uint32_t lastFrame() const {
        std::uint32_t out = 0;
        for (auto const& input : inputs) {
            if (input.frame > out) out = input.frame;
        }
        return out;
    }
};

struct WindowSegment {
    int minOffset = 0;
    int maxOffset = 0;

    [[nodiscard]] int size() const {
        return maxOffset - minOffset + 1;
    }
};

struct FrameWindowResult {
    std::size_t inputIndex = 0;
    MacroInput input;
    int scanRadius = 0;
    int early = 0;
    int late = 0;
    int frameWindow = 0;
    bool baselinePassed = false;
    bool clippedByScanRadius = false;
    std::vector<WindowSegment> segments;
    std::map<int, bool> verdicts;
};

inline char const* actionName(InputAction action) {
    return action == InputAction::Hold ? "HOLD" : "RELEASE";
}

} // namespace fwa
