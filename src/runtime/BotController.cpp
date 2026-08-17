#include "runtime/BotController.hpp"

#include "runtime/FileLogger.hpp"

#include <Geode/loader/Mod.hpp>

#include <algorithm>
#include <cmath>
#include <sstream>

using namespace geode::prelude;

namespace fwa {

BotController& BotController::get() {
    static BotController controller;
    return controller;
}

int BotController::settingInt(char const* key, int fallback) const {
    try {
        return static_cast<int>(Mod::get()->getSettingValue<std::int64_t>(key));
    } catch (...) {
        return fallback;
    }
}

bool BotController::settingBool(char const* key, bool fallback) const {
    try {
        return Mod::get()->getSettingValue<bool>(key);
    } catch (...) {
        return fallback;
    }
}

void BotController::initialize() {
    FileLogger::get().initialize();
    m_trajectoryVisible = settingBool("show-trajectory", true);
    m_trajectory.setVisible(m_trajectoryVisible);
    FileLogger::get().debug("[Bot] initialized");
}

void BotController::fillLevelMetadata(PlayLayer* layer, Macro& macro) {
    macro.gameFps = settingInt("analysis-tps", 240);
    macro.windowFps = macro.gameFps;
    macro.levelName = "Current Level";
    macro.levelId = 0;

    // GJGameLevel fields have changed type between bindings versions. Keeping the
    // analyzer independent of those fields avoids a hard binding dependency here;
    // gameplay logs still contain exact frame/input data. Metadata can be enriched later.
    (void)layer;
}

void BotController::toggleRecording(PlayLayer* layer) {
    if (m_mode == BotMode::Recording) stopRecording(layer);
    else startRecording(layer);
}

void BotController::startRecording(PlayLayer* layer) {
    if (!layer) return;
    cancelAutomation();
    m_mode = BotMode::Recording;
    m_recordingMacro = {};
    fillLevelMetadata(layer, m_recordingMacro);
    m_currentFrame = 0;
    m_playbackCursor = 0;
    m_sequence = 0;
    m_accumulator = 0.0;
    m_pendingReset = false;
    FileLogger::get().debug("[Record] start; resetting level for deterministic frame 0");
    layer->resetLevel();
}

void BotController::stopRecording(PlayLayer* layer, bool allowAutoAnalyze) {
    if (m_mode != BotMode::Recording) return;
    m_lastMacro = m_recordingMacro;
    m_mode = BotMode::Idle;
    m_accumulator = 0.0;

    std::ostringstream msg;
    msg << "[Record] stop; inputs=" << m_lastMacro.inputs.size() << ", lastFrame=" << m_lastMacro.lastFrame();
    FileLogger::get().debug(msg.str());

    if (allowAutoAnalyze && layer && !m_lastMacro.inputs.empty() && settingBool("auto-analyze", false)) {
        startAnalysis(layer);
    }
}

void BotController::startPlayback(PlayLayer* layer) {
    if (!layer) return;
    if (m_mode == BotMode::Recording) stopRecording(layer, false);
    if (m_lastMacro.inputs.empty()) return;
    cancelAutomation();
    m_mode = BotMode::Playback;
    m_currentFrame = 0;
    m_playbackCursor = 0;
    m_accumulator = 0.0;
    FileLogger::get().debug("[Playback] start");
    layer->resetLevel();
}

void BotController::startAnalysis(PlayLayer* layer) {
    if (!layer) return;
    if (m_mode == BotMode::Recording) stopRecording(layer, false);
    if (m_lastMacro.inputs.empty()) return;
    cancelAutomation();

    int radius = settingInt("scan-radius", 12);
    int post = settingInt("post-macro-validation", 120);
    bool exhaustive = settingBool("exhaustive-scan", false);
    m_analysis.start(m_lastMacro, radius, post, exhaustive);
    m_mode = BotMode::Analyzing;
    m_currentFrame = 0;
    m_playbackCursor = 0;
    m_accumulator = 0.0;
    m_branchResolved = false;

    if (!m_analysis.prepareNextSimulation()) {
        finishAnalysis();
        return;
    }

    auto const& job = m_analysis.currentJob();
    std::ostringstream msg;
    msg << "[Analyzer] start inputs=" << m_lastMacro.inputs.size()
        << " radius=" << radius << " exhaustive=" << (exhaustive ? "true" : "false")
        << " first=input#" << (job.inputIndex + 1) << " offset=" << job.offset;
    FileLogger::get().debug(msg.str());
    layer->resetLevel();
}

void BotController::cancelAutomation() {
    if (m_mode == BotMode::Analyzing) m_analysis.cancel();
    if (m_mode == BotMode::Playback || m_mode == BotMode::Analyzing) m_mode = BotMode::Idle;
    m_pendingReset = false;
    m_playbackCursor = 0;
    m_accumulator = 0.0;
}

void BotController::togglePause() {
    m_framePaused = !m_framePaused;
    m_stepRequests = 0;
    FileLogger::get().debug(std::string("[Stepper] pause=") + (m_framePaused ? "true" : "false"));
}

void BotController::requestFrameStep() {
    m_framePaused = true;
    m_stepRequests = std::min(m_stepRequests + 1, 32);
    FileLogger::get().debug("[Stepper] requested one physics frame");
}

void BotController::toggleTrajectory() {
    m_trajectoryVisible = !m_trajectoryVisible;
    m_trajectory.setVisible(m_trajectoryVisible);
    FileLogger::get().debug(std::string("[Trajectory] visible=") + (m_trajectoryVisible ? "true" : "false"));
}

void BotController::recordInput(bool pressed, int button, bool player1) {
    if (m_mode != BotMode::Recording || m_injecting) return;

    MacroInput input;
    input.frame = m_currentFrame;
    input.player2 = !player1;
    input.button = button;
    input.action = pressed ? InputAction::Hold : InputAction::Release;
    input.sequence = m_sequence++;
    m_recordingMacro.inputs.push_back(input);

    std::ostringstream msg;
    msg << "[Record] frame=" << input.frame << " player=P" << (input.player2 ? 2 : 1)
        << " button=" << input.button << " action=" << actionName(input.action);
    FileLogger::get().debug(msg.str());
}

void BotController::injectForCurrentFrame(PlayLayer* layer, Macro const& macro) {
    while (m_playbackCursor < macro.inputs.size() && macro.inputs[m_playbackCursor].frame < m_currentFrame) {
        ++m_playbackCursor;
    }
    while (m_playbackCursor < macro.inputs.size() && macro.inputs[m_playbackCursor].frame == m_currentFrame) {
        auto const input = macro.inputs[m_playbackCursor++];
        m_injecting = true;
        layer->handleButton(input.action == InputAction::Hold, input.button, !input.player2);
        m_injecting = false;
    }
}

void BotController::beforePhysicsStep(PlayLayer* layer) {
    if (!layer) return;
    if (m_mode == BotMode::Playback) injectForCurrentFrame(layer, m_lastMacro);
    else if (m_mode == BotMode::Analyzing) injectForCurrentFrame(layer, m_analysis.playbackMacro());
}

void BotController::afterPhysicsStep(PlayLayer* layer) {
    if (!layer) return;
    if (m_mode == BotMode::Analyzing && m_branchResolved && m_pendingReset) return;
    m_trajectory.sample(layer);
    ++m_currentFrame;

    if (m_mode == BotMode::Playback) {
        auto endFrame = m_lastMacro.lastFrame() + static_cast<std::uint32_t>(settingInt("post-macro-validation", 120));
        if (m_currentFrame >= endFrame) {
            FileLogger::get().debug("[Playback] finished validation horizon");
            m_mode = BotMode::Idle;
        }
        return;
    }

    if (m_mode == BotMode::Analyzing && !m_branchResolved && m_analysis.shouldPassAt(m_currentFrame)) {
        m_branchResolved = true;
        advanceAnalysis("survived full macro + post validation", true);
    }
}

void BotController::onReset(PlayLayer* layer) {
    m_currentFrame = 0;
    m_playbackCursor = 0;
    m_accumulator = 0.0;
    m_branchResolved = false;
    m_trajectory.reset(layer);

    if (m_mode == BotMode::Recording) {
        m_recordingMacro.inputs.clear();
        m_sequence = 0;
        FileLogger::get().debug("[Record] reset -> discarded previous partial attempt");
    }
}

void BotController::onDeath(std::uint32_t frame, GameObject* cause) {
    if (m_mode != BotMode::Analyzing || m_branchResolved) return;
    m_branchResolved = true;
    std::ostringstream reason;
    reason << "death at frame " << frame;
    if (cause) reason << " causePtr=" << cause;
    advanceAnalysis(reason.str(), false);
}

void BotController::onLevelComplete() {
    if (m_mode == BotMode::Analyzing && !m_branchResolved) {
        m_branchResolved = true;
        advanceAnalysis("level complete", true);
    }
}

void BotController::advanceAnalysis(std::string const& reason, bool passed) {
    if (m_mode != BotMode::Analyzing || !m_analysis.active()) return;
    auto job = m_analysis.currentJob();

    std::ostringstream msg;
    msg << "[Branch] input#" << (job.inputIndex + 1)
        << " offset=" << (job.offset >= 0 ? "+" : "") << job.offset
        << " result=" << (passed ? "PASS" : "FAIL") << " reason=" << reason;
    FileLogger::get().debug(msg.str());

    m_analysis.finishCurrent(passed, reason);
    m_currentFrame = 0;
    m_playbackCursor = 0;

    if (m_analysis.prepareNextSimulation()) {
        auto const& next = m_analysis.currentJob();
        std::ostringstream nextMsg;
        nextMsg << "[Analyzer] next input#" << (next.inputIndex + 1)
                << " offset=" << (next.offset >= 0 ? "+" : "") << next.offset
                << " progress=" << m_analysis.completedJobs() << '/' << m_analysis.totalJobs();
        FileLogger::get().debug(nextMsg.str());
        m_pendingReset = true;
    } else {
        finishAnalysis();
    }
}

void BotController::finishAnalysis() {
    if (!m_analysis.finished()) {
        m_mode = BotMode::Idle;
        return;
    }

    auto path = FileLogger::get().exportAnalysis(m_analysis.sourceMacro(), m_analysis.results());
    FileLogger::get().debug(std::string("[Analyzer] finished report=") + path.string());
    m_mode = BotMode::Idle;
    m_pendingReset = false;
    m_branchResolved = false;
    m_accumulator = 0.0;

}

bool BotController::shouldUseFixedStep() const {
    return m_mode != BotMode::Idle || m_framePaused;
}

int BotController::stepsForUpdate(float realDt) {
    if (!shouldUseFixedStep()) return 0;

    if (m_framePaused) {
        if (m_stepRequests > 0) {
            --m_stepRequests;
            return 1;
        }
        return 0;
    }

    double speed = (m_mode == BotMode::Analyzing) ? static_cast<double>(settingInt("analysis-speed", 8)) : 1.0;
    auto dt = static_cast<double>(fixedDt());
    m_accumulator += std::max(0.0f, std::min(realDt, 0.25f)) * speed;
    int steps = static_cast<int>(std::floor(m_accumulator / dt));
    steps = std::clamp(steps, 0, 256);
    m_accumulator -= static_cast<double>(steps) * dt;
    return steps;
}

float BotController::fixedDt() const {
    return 1.0f / static_cast<float>(std::max(1, settingInt("analysis-tps", 240)));
}

bool BotController::consumeResetRequest() {
    bool out = m_pendingReset;
    m_pendingReset = false;
    return out;
}

std::string BotController::statusText() const {
    std::ostringstream out;
    switch (m_mode) {
        case BotMode::Idle: out << "Idle"; break;
        case BotMode::Recording: out << "Recording"; break;
        case BotMode::Playback: out << "Playback"; break;
        case BotMode::Analyzing:
            out << "Analyzing " << m_analysis.completedJobs() << '/' << m_analysis.totalJobs();
            break;
    }
    out << " | frame " << m_currentFrame;
    if (m_framePaused) out << " | PAUSED";
    return out.str();
}

} // namespace fwa
