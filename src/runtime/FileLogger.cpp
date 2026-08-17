#include "runtime/FileLogger.hpp"

#include "core/NaNDLExport.hpp"

#include <Geode/Geode.hpp>

#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>

using namespace geode::prelude;

namespace fwa {

FileLogger& FileLogger::get() {
    static FileLogger logger;
    return logger;
}

void FileLogger::initialize() {
    std::scoped_lock lock(m_mutex);
    m_logDir = Mod::get()->getSaveDir() / "logs";
    std::filesystem::create_directories(m_logDir / "debug");
    m_debugPath = m_logDir / "debug" / "latest.log";
    std::ofstream out(m_debugPath, std::ios::trunc);
    out << "Frame Window Analyzer debug log\n";
    out << "Started: " << timestamp(false) << "\n";
}

std::string FileLogger::timestamp(bool filenameSafe) const {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::ostringstream out;
    out << std::put_time(&tm, filenameSafe ? "%Y-%m-%d_%H-%M-%S" : "%Y-%m-%d %H:%M:%S");
    return out.str();
}

void FileLogger::debug(std::string const& message) {
    std::scoped_lock lock(m_mutex);
    if (m_debugPath.empty()) return;
    std::ofstream out(m_debugPath, std::ios::app);
    out << '[' << timestamp(false) << "] " << message << '\n';
}

std::string FileLogger::cleanReport(Macro const& macro, std::vector<FrameWindowResult> const& results) const {
    std::ostringstream out;
    out << "============================================================\n";
    out << "FRAME WINDOW ANALYZER\n";
    out << "============================================================\n\n";
    out << "Level: " << (macro.levelName.empty() ? "Unknown" : macro.levelName) << '\n';
    out << "Level ID: " << macro.levelId << '\n';
    out << "Game FPS: " << macro.gameFps << '\n';
    out << "Window FPS: " << macro.windowFps << '\n';
    out << "Inputs: " << results.size() << '\n';
    out << "NaNDL formula: w_i = N_i / f\n";
    out << "N_i below is FRAME WINDOW (contiguous pass window containing recorded input).\n\n";

    for (auto const& r : results) {
        out << "------------------------------------------------------------\n";
        out << '#' << std::setfill('0') << std::setw(4) << (r.inputIndex + 1) << std::setfill(' ') << '\n';
        out << "Frame: " << r.input.frame << '\n';
        out << "Time: " << std::fixed << std::setprecision(6)
            << (static_cast<double>(r.input.frame) / static_cast<double>(macro.gameFps)) << " s\n";
        out << "Player: P" << (r.input.player2 ? 2 : 1) << '\n';
        out << "Button: " << r.input.button << '\n';
        out << "Input: " << actionName(r.input.action) << "\n\n";
        out << "Early: " << r.early << '\n';
        out << "Late: " << r.late << '\n';
        out << "FRAME WINDOW (N_i): " << r.frameWindow << '\n';
        out << "FRAME PERFECT: " << (r.frameWindow == 1 ? "YES" : "NO") << '\n';
        out << "Baseline: " << (r.baselinePassed ? "PASS" : "FAIL") << '\n';
        if (r.clippedByScanRadius) out << "WARNING: window touched scan boundary; increase Frame Scan Radius.\n";
        if (r.segments.size() > 1) {
            out << "Passing segments (offsets): ";
            for (std::size_t i = 0; i < r.segments.size(); ++i) {
                if (i) out << ", ";
                out << '[' << r.segments[i].minOffset << ", " << r.segments[i].maxOffset << ']';
            }
            out << '\n';
        }
        out << "\nOffsets: ";
        for (auto const& [offset, pass] : r.verdicts) {
            out << (offset >= 0 ? "+" : "") << offset << '=' << (pass ? "PASS" : "FAIL") << ' ';
        }
        out << "\n\n";
    }
    return out.str();
}

std::filesystem::path FileLogger::exportAnalysis(Macro const& macro, std::vector<FrameWindowResult> const& results) {
    std::scoped_lock lock(m_mutex);
    std::filesystem::create_directories(m_logDir);
    auto stem = std::string("frame-window_") + timestamp(true);
    auto logPath = m_logDir / (stem + ".log");
    auto jsonPath = m_logDir / (stem + "_nandl.json");
    auto csvPath = m_logDir / (stem + "_nandl.csv");

    { std::ofstream out(logPath); out << cleanReport(macro, results); }
    { std::ofstream out(jsonPath); out << NaNDLExport::toJson(macro, results, 0.0, true); }
    { std::ofstream out(csvPath); out << NaNDLExport::toCsv(macro, results, true); }

    return logPath;
}

} // namespace fwa
