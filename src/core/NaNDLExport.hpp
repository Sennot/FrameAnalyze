#pragma once

#include "core/Types.hpp"

#include <string>
#include <vector>

namespace fwa {

class NaNDLExport {
public:
    // JSON intentionally carries both the concise NaNDL calculator fields and an
    // analyzer metadata block. The calculator-facing rows are input/time/frameWindow.
    static std::string toJson(
        Macro const& macro,
        std::vector<FrameWindowResult> const& results,
        double respawnSeconds = 0.0,
        bool useFrameNumbers = true
    );

    static std::string toCsv(
        Macro const& macro,
        std::vector<FrameWindowResult> const& results,
        bool useFrameNumbers = true
    );
};

} // namespace fwa
