#pragma once

#include "core/Types.hpp"

#include <filesystem>
#include <mutex>
#include <string>
#include <vector>

namespace fwa {

class FileLogger {
public:
    static FileLogger& get();

    void initialize();
    void debug(std::string const& message);

    // Writes clean report + NaNDL JSON + CSV. Returns base report path.
    std::filesystem::path exportAnalysis(Macro const& macro, std::vector<FrameWindowResult> const& results);

    [[nodiscard]] std::filesystem::path const& logDir() const { return m_logDir; }
    [[nodiscard]] std::filesystem::path const& debugPath() const { return m_debugPath; }

private:
    std::string timestamp(bool filenameSafe) const;
    std::string cleanReport(Macro const& macro, std::vector<FrameWindowResult> const& results) const;

    std::filesystem::path m_logDir;
    std::filesystem::path m_debugPath;
    std::mutex m_mutex;
};

} // namespace fwa
