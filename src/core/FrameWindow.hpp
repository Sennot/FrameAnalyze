#pragma once

#include "core/Types.hpp"

namespace fwa {

class FrameWindow {
public:
    // Summarizes already measured branch verdicts. Every offset in [-scanRadius,+scanRadius]
    // should normally be present. Missing offsets are treated as failed.
    static FrameWindowResult summarize(
        std::size_t inputIndex,
        MacroInput const& input,
        int scanRadius,
        std::map<int, bool> const& verdicts
    );
};

} // namespace fwa
