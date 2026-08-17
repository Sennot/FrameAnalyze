#include "runtime/AnalysisSession.hpp"

#include "core/FrameWindow.hpp"

#include <algorithm>
#include <cstdint>
#include <utility>

namespace fwa {

void AnalysisSession::start(Macro macro, int scanRadius, int postMacroValidationFrames, bool exhaustive) {
    m_source = std::move(macro);
    m_scanRadius = std::max(1, scanRadius);
    m_postMacroValidationFrames = std::max(0, postMacroValidationFrames);
    m_exhaustive = exhaustive;
    m_active = true;
    m_finished = false;
    m_jobIndex = 0;
    m_completedJobs = 0;
    m_verdicts.clear();
    m_leftStopped.clear();
    m_rightStopped.clear();
    m_baselineFailed.clear();
    m_results.clear();
    m_lastReason.clear();
    buildJobs();
}

void AnalysisSession::cancel() {
    m_active = false;
    m_finished = false;
    m_jobs.clear();
    m_results.clear();
}

void AnalysisSession::buildJobs() {
    m_jobs.clear();
    for (std::size_t inputIndex = 0; inputIndex < m_source.inputs.size(); ++inputIndex) {
        m_jobs.push_back({inputIndex, 0});
        for (int distance = 1; distance <= m_scanRadius; ++distance) {
            m_jobs.push_back({inputIndex, -distance});
            m_jobs.push_back({inputIndex, distance});
        }
    }
}

bool AnalysisSession::shouldSkip(AnalysisJob const& job) const {
    if (m_exhaustive || job.offset == 0) return false;
    if (m_baselineFailed.contains(job.inputIndex) && m_baselineFailed.at(job.inputIndex)) return true;
    if (job.offset < 0) {
        auto it = m_leftStopped.find(job.inputIndex);
        return it != m_leftStopped.end() && it->second;
    }
    auto it = m_rightStopped.find(job.inputIndex);
    return it != m_rightStopped.end() && it->second;
}

bool AnalysisSession::makeCandidate(AnalysisJob const& job) {
    if (job.inputIndex >= m_source.inputs.size()) return false;

    auto originalFrame = static_cast<std::int64_t>(m_source.inputs[job.inputIndex].frame);
    auto shifted = originalFrame + static_cast<std::int64_t>(job.offset);
    if (shifted < 0 || shifted > static_cast<std::int64_t>(UINT32_MAX)) return false;

    m_candidate = m_source;
    m_candidate.inputs[job.inputIndex].frame = static_cast<std::uint32_t>(shifted);

    std::stable_sort(m_candidate.inputs.begin(), m_candidate.inputs.end(), [](MacroInput const& a, MacroInput const& b) {
        if (a.frame != b.frame) return a.frame < b.frame;
        return a.sequence < b.sequence;
    });

    auto end = std::max(m_candidate.lastFrame(), static_cast<std::uint32_t>(shifted));
    auto extra = static_cast<std::uint64_t>(m_postMacroValidationFrames);
    auto pass64 = static_cast<std::uint64_t>(end) + extra;
    m_passFrame = static_cast<std::uint32_t>(std::min<std::uint64_t>(pass64, UINT32_MAX));
    return true;
}

bool AnalysisSession::prepareNextSimulation() {
    if (!m_active) return false;

    while (m_jobIndex < m_jobs.size()) {
        auto const job = m_jobs[m_jobIndex];
        if (shouldSkip(job)) {
            ++m_jobIndex;
            ++m_completedJobs;
            continue;
        }
        if (!makeCandidate(job)) {
            m_verdicts[job.inputIndex][job.offset] = false;
            if (job.offset == 0) m_baselineFailed[job.inputIndex] = true;
            if (job.offset < 0) m_leftStopped[job.inputIndex] = true;
            if (job.offset > 0) m_rightStopped[job.inputIndex] = true;
            ++m_jobIndex;
            ++m_completedJobs;
            continue;
        }
        return true;
    }

    finalizeResults();
    m_active = false;
    m_finished = true;
    return false;
}

void AnalysisSession::finishCurrent(bool passed, std::string reason) {
    if (!m_active || m_jobIndex >= m_jobs.size()) return;
    auto const job = m_jobs[m_jobIndex];
    m_verdicts[job.inputIndex][job.offset] = passed;
    m_lastReason = std::move(reason);

    if (!passed) {
        if (job.offset == 0) m_baselineFailed[job.inputIndex] = true;
        if (!m_exhaustive && job.offset < 0) m_leftStopped[job.inputIndex] = true;
        if (!m_exhaustive && job.offset > 0) m_rightStopped[job.inputIndex] = true;
    }

    ++m_jobIndex;
    ++m_completedJobs;
}

bool AnalysisSession::shouldPassAt(std::uint32_t frame) const {
    return m_active && frame >= m_passFrame;
}

void AnalysisSession::finalizeResults() {
    m_results.clear();
    m_results.reserve(m_source.inputs.size());
    for (std::size_t i = 0; i < m_source.inputs.size(); ++i) {
        auto it = m_verdicts.find(i);
        std::map<int, bool> empty;
        auto const& verdicts = it == m_verdicts.end() ? empty : it->second;
        m_results.push_back(FrameWindow::summarize(i, m_source.inputs[i], m_scanRadius, verdicts));
    }
}

} // namespace fwa
