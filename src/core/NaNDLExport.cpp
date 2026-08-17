#include "core/NaNDLExport.hpp"

#include <iomanip>
#include <sstream>

namespace fwa {
namespace {

std::string escapeJson(std::string const& value) {
    std::ostringstream out;
    for (unsigned char ch : value) {
        switch (ch) {
            case '"': out << "\\\""; break;
            case '\\': out << "\\\\"; break;
            case '\b': out << "\\b"; break;
            case '\f': out << "\\f"; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            default:
                if (ch < 0x20) {
                    out << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(ch)
                        << std::dec << std::setfill(' ');
                } else {
                    out << static_cast<char>(ch);
                }
        }
    }
    return out.str();
}

} // namespace

std::string NaNDLExport::toJson(
    Macro const& macro,
    std::vector<FrameWindowResult> const& results,
    double respawnSeconds,
    bool useFrameNumbers
) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(6);
    out << "{\n";
    out << "  \"gameFPS\": " << macro.gameFps << ",\n";
    out << "  \"windowFPS\": " << macro.windowFps << ",\n";
    out << "  \"respawnTime\": " << respawnSeconds << ",\n";
    out << "  \"useFrameNumbers\": " << (useFrameNumbers ? "true" : "false") << ",\n";
    out << "  \"inputs\": [\n";

    for (std::size_t i = 0; i < results.size(); ++i) {
        auto const& r = results[i];
        double time = useFrameNumbers
            ? static_cast<double>(r.input.frame)
            : static_cast<double>(r.input.frame) / static_cast<double>(macro.gameFps);
        out << "    {\"input\": " << (i + 1)
            << ", \"time\": " << time
            << ", \"frameWindow\": " << r.frameWindow << "}";
        if (i + 1 != results.size()) out << ',';
        out << "\n";
    }

    out << "  ],\n";
    out << "  \"frameWindowAnalyzer\": {\n";
    out << "    \"formatVersion\": 1,\n";
    out << "    \"levelName\": \"" << escapeJson(macro.levelName) << "\",\n";
    out << "    \"levelId\": " << macro.levelId << ",\n";
    out << "    \"formula\": \"w_i = N_i / f\",\n";
    out << "    \"results\": [\n";
    for (std::size_t i = 0; i < results.size(); ++i) {
        auto const& r = results[i];
        out << "      {\"index\": " << (r.inputIndex + 1)
            << ", \"frame\": " << r.input.frame
            << ", \"action\": \"" << actionName(r.input.action) << "\""
            << ", \"player\": " << (r.input.player2 ? 2 : 1)
            << ", \"button\": " << r.input.button
            << ", \"early\": " << r.early
            << ", \"late\": " << r.late
            << ", \"frameWindow\": " << r.frameWindow
            << ", \"baselinePassed\": " << (r.baselinePassed ? "true" : "false")
            << ", \"clipped\": " << (r.clippedByScanRadius ? "true" : "false")
            << "}";
        if (i + 1 != results.size()) out << ',';
        out << "\n";
    }
    out << "    ]\n";
    out << "  }\n";
    out << "}\n";
    return out.str();
}

std::string NaNDLExport::toCsv(
    Macro const& macro,
    std::vector<FrameWindowResult> const& results,
    bool useFrameNumbers
) {
    std::ostringstream out;
    out << "Input number," << (useFrameNumbers ? "Time (frames)" : "Time (s)") << ",Frame window\n";
    out << std::fixed << std::setprecision(6);
    for (std::size_t i = 0; i < results.size(); ++i) {
        auto const& r = results[i];
        double time = useFrameNumbers
            ? static_cast<double>(r.input.frame)
            : static_cast<double>(r.input.frame) / static_cast<double>(macro.gameFps);
        out << (i + 1) << ',' << time << ',' << r.frameWindow << '\n';
    }
    return out.str();
}

} // namespace fwa
