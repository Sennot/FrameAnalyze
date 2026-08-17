#pragma once

#include "core/Types.hpp"

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace fwa {

struct AnalysisJob {
    std::size_t inputIndex = 0;
    int offset = 0;
};

class AnalysisSession {
public:
    void start(Macro macro, int scanRadius, int postMacroValidationFrames, bool exhaustive);
    void cancel();

    [[nodiscard]] bool active() const { return m_active; }
    [[nodiscard]] bool finished() const { return m_finished; }
    [[nodiscard]] Macro const& sourceMacro() const { return m_source; }
    [[nodiscard]] Macro const& playbackMacro() const { return m_candidate; }
    [[nodiscard]] AnalysisJob const& currentJob() const { return m_jobs.at(m_jobIndex); }
    [[nodiscard]] std::uint32_t passFrame() const { return m_passFrame; }
    [[nodiscard]] std::size_t completedJobs() const { return m_completedJobs; }
    [[nodiscard]] std::size_t totalJobs() const { return m_jobs.size(); }
    [[nodiscard]] std::vector<FrameWindowResult> const& results() const { return m_results; }

    // Returns true when a simulation job is ready. Invalid/skipped jobs are resolved internally.
    bool prepareNextSimulation();
    void finishCurrent(bool passed, std::string reason);
    bool shouldPassAt(std::uint32_t frame) const;

    [[nodiscard]] std::string const& lastReason() const { return m_lastReason; }

private:
    void buildJobs();
    bool shouldSkip(AnalysisJob const& job) const;
    bool makeCandidate(AnalysisJob const& job);
    bool validateInputOrder(Macro const& macro) const;
    void finalizeResults();

    Macro m_source;
    Macro m_candidate;
    int m_scanRadius = 0;
    int m_postMacroValidationFrames = 0;
    bool m_exhaustive = false;
    bool m_active = false;
    bool m_finished = false;

    std::vector<AnalysisJob> m_jobs;
    std::size_t m_jobIndex = 0;
    std::size_t m_completedJobs = 0;
    std::uint32_t m_passFrame = 0;
    std::map<std::size_t, std::map<int, bool>> m_verdicts;
    std::map<std::size_t, bool> m_leftStopped;
    std::map<std::size_t, bool> m_rightStopped;
    std::map<std::size_t, bool> m_baselineFailed;
    std::vector<FrameWindowResult> m_results;
    std::string m_lastReason;
};

} // namespace fwa
