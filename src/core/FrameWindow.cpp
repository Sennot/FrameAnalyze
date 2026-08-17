#include "core/FrameWindow.hpp"

#include <algorithm>

namespace fwa {

FrameWindowResult FrameWindow::summarize(
    std::size_t inputIndex,
    MacroInput const& input,
    int scanRadius,
    std::map<int, bool> const& verdicts
) {
    FrameWindowResult result;
    result.inputIndex = inputIndex;
    result.input = input;
    result.scanRadius = std::max(0, scanRadius);
    result.verdicts = verdicts;

    auto passed = [&](int offset) {
        auto it = verdicts.find(offset);
        return it != verdicts.end() && it->second;
    };

    result.baselinePassed = passed(0);

    // Keep all disjoint passing islands for debugging. NaNDL's N_i is intentionally
    // based on the contiguous island containing the recorded timing (offset 0).
    bool inSegment = false;
    WindowSegment segment;
    for (int offset = -result.scanRadius; offset <= result.scanRadius; ++offset) {
        if (passed(offset)) {
            if (!inSegment) {
                segment = {offset, offset};
                inSegment = true;
            } else {
                segment.maxOffset = offset;
            }
        } else if (inSegment) {
            result.segments.push_back(segment);
            inSegment = false;
        }
    }
    if (inSegment) result.segments.push_back(segment);

    if (!result.baselinePassed) return result;

    WindowSegment main{0, 0};
    for (auto const& s : result.segments) {
        if (s.minOffset <= 0 && s.maxOffset >= 0) {
            main = s;
            break;
        }
    }

    result.early = -main.minOffset;
    result.late = main.maxOffset;
    result.frameWindow = main.size();
    result.clippedByScanRadius =
        (main.minOffset == -result.scanRadius && passed(-result.scanRadius)) ||
        (main.maxOffset == result.scanRadius && passed(result.scanRadius));

    return result;
}

} // namespace fwa
