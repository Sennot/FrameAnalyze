#include "runtime/BotController.hpp"

#include "runtime/FileLogger.hpp"

#include <Geode/loader/Mod.hpp>

#include <algorithm>
#include <sstream>

using namespace geode::prelude;

namespace fwa {

namespace {
constexpr float kPhysicsDt = 1.0f / 240.0f;
}

BotController& BotController::get() {
    static BotController controller;
    return controller;
}

int BotController::settingInt(char const* key, int fallback) const {
    try { return static_cast<int>(Mod::get()->getSettingValue<std::int64_t>(key)); }
    catch (...) { return fallback; }
}

bool BotController::settingBool(char const* key, bool fallback) const {
    try { return Mod::get()->getSettingValue<bool>(key); }
    catch (...) { return fallback; }
}

void BotController::setNotice(std::string message) {
    m_lastNotice = std::move(message);
    if (!m_lastNotice.empty()) FileLogger::get().debug(std::string("[Status] ") + m_lastNotice);
}

bool BotController::hasPlacedPracticeCheckpoint() const {
    auto* play = PlayLayer::get();
    return play && play->getLastCheckpoint() != nullptr;
}

void BotController::initialize() {
    FileLogger::get().initialize();
    // Trajectory is runtime-only and always starts OFF. Older builds persisted
    // this option, which could make stale prediction lines appear immediately
    // after updating. The user explicitly enables it with Ctrl+F5 or the menu.
    m_trajectoryVisible = false;
    m_trajectory.setVisible(false);
    m_speedhack.setAudioFollow(settingBool("speedhack-audio", true));
    syncAudio();
    FileLogger::get().debug("[FWBot] initialized v0.2.3 practice-record fix");
}

void BotController::fillLevelMetadata(PlayLayer* layer, Macro& macro) {
    // GD 2.2081 frame-window analysis in FWBot is deliberately locked to the
    // native 240 Hz physics base. Exposing arbitrary TPS without the complete
    // Silicate TPS bypass would make N_i misleading.
    macro.gameFps = 240;
    macro.windowFps = 240;
    macro.levelName = "Current Level";
    macro.levelId = 0;
    (void)layer;
}

bool BotController::captureAnchor(PlayLayer* layer, Macro& macro) {
    if (!m_anchor.capture(layer)) return false;
    macro.practiceAnchored = true;
    macro.anchorInputs = {};
    return true;
}

void BotController::requestAnchorRestore(PlayLayer* layer) {
    if (!layer || !m_anchor.validFor(layer)) {
        FileLogger::get().debug("[Practice] restore requested without a valid anchor");
        return;
    }

    m_forceAnchorLoad = true;
    m_currentFrame = 0;
    m_playbackCursor = 0;
    m_branchResolved = false;
    m_pendingReset = false;
    m_trajectory.reset(layer);
    layer->m_extraDelta = 0.0f;
    layer->resetLevel();
}

void BotController::toggleRecording(PlayLayer* layer) {
    if (m_mode == BotMode::Recording) stopRecording(layer);
    else startRecording(layer);
}

void BotController::startRecording(PlayLayer* layer) {
    if (!layer) {
        setNotice("Record failed: open a level first.");
        return;
    }
    if (layer->m_isPaused) {
        setNotice("Record failed: close the in-game Pause menu first.");
        return;
    }

    setFrameStepper(false);
    cancelAutomation();

    m_recordingMacro = {};
    fillLevelMetadata(layer, m_recordingMacro);
    if (!captureAnchor(layer, m_recordingMacro)) {
        setNotice("Record failed: place a Practice checkpoint first.");
        return;
    }

    // Arm recording, then force a real GD restore to the retained checkpoint.
    // Input capture stays disabled until loadFromCheckpoint has completed, so
    // frame 0 always means the actual Practice anchor state.
    m_mode = BotMode::Recording;
    m_recordingArming = true;
    m_currentFrame = 0;
    m_playbackCursor = 0;
    m_sequence = 0;
    m_pendingReset = false;
    m_branchResolved = false;
    layer->m_extraDelta = 0.0f;
    m_trajectory.reset(layer);
    setNotice("Record: loading Practice checkpoint...");
    requestAnchorRestore(layer);
}

void BotController::stopRecording(PlayLayer* layer, bool allowAutoAnalyze) {
    if (m_mode != BotMode::Recording) return;

    m_lastMacro = m_recordingMacro;
    m_mode = BotMode::Idle;
    m_recordingArming = false;
    m_stepRequests = 0;

    std::ostringstream msg;
    msg << "[Record] stop; inputs=" << m_lastMacro.inputs.size()
        << " lastFrame=" << m_lastMacro.lastFrame()
        << " practiceAnchor=" << (m_lastMacro.practiceAnchored ? "yes" : "no");
    FileLogger::get().debug(msg.str());
    setNotice(m_lastMacro.inputs.empty() ? "Recording stopped: no inputs captured." : "Recording stopped.");

    if (allowAutoAnalyze && layer && !m_lastMacro.inputs.empty() && settingBool("auto-analyze", true)) {
        startAnalysis(layer);
    }
}

void BotController::startPlayback(PlayLayer* layer) {
    if (!layer) return;
    if (m_mode == BotMode::Recording) stopRecording(layer, false);
    if (m_lastMacro.inputs.empty() || !m_anchor.validFor(layer)) return;

    setFrameStepper(false);
    cancelAutomation();
    m_mode = BotMode::Playback;
    m_currentFrame = 0;
    m_playbackCursor = 0;
    m_branchResolved = false;
    m_trajectory.reset(layer);
    FileLogger::get().debug("[Playback] restoring Practice anchor");
    requestAnchorRestore(layer);
}

void BotController::startAnalysis(PlayLayer* layer) {
    if (!layer) return;
    if (m_mode == BotMode::Recording) stopRecording(layer, false);
    if (m_lastMacro.inputs.empty() || !m_anchor.validFor(layer)) return;

    setFrameStepper(false);
    cancelAutomation();

    int radius = settingInt("scan-radius", 12);
    int post = settingInt("post-macro-validation", 120);
    bool exhaustive = settingBool("exhaustive-scan", false);
    m_analysis.start(m_lastMacro, radius, post, exhaustive);
    m_mode = BotMode::Analyzing;
    m_currentFrame = 0;
    m_playbackCursor = 0;
    m_branchResolved = false;
    m_trajectory.reset(layer);
    syncAudio();

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
    requestAnchorRestore(layer);
}

void BotController::cancelAutomation() {
    if (m_mode == BotMode::Analyzing) m_analysis.cancel();
    if (m_mode == BotMode::Playback || m_mode == BotMode::Analyzing) m_mode = BotMode::Idle;
    m_pendingReset = false;
    m_forceAnchorLoad = false;
    m_playbackCursor = 0;
    m_branchResolved = false;
    syncAudio();
}

void BotController::toggleFrameStepper() {
    setFrameStepper(!m_frameStepper);
}

void BotController::setFrameStepper(bool enabled) {
    // Never carry a frozen scheduler state from menus into a newly opened level.
    if (enabled) {
        auto* play = PlayLayer::get();
        if (!play || play->m_isPaused) {
            m_frameStepper = false;
            m_stepRequests = 0;
            return;
        }
    }

    // Playback/analyze own the scheduler. Starting manual stepping cancels them;
    // Recording is intentionally allowed so a macro can be captured one tick at a time.
    if (enabled && (m_mode == BotMode::Playback || m_mode == BotMode::Analyzing)) {
        cancelAutomation();
    }

    m_frameStepper = enabled;
    m_stepRequests = 0;
    if (auto* play = PlayLayer::get()) play->m_extraDelta = 0.0f;
    FileLogger::get().debug(std::string("[Stepper] enabled=") + (enabled ? "true" : "false"));
}

void BotController::requestFrameStep() {
    auto* play = PlayLayer::get();
    if (!play || play->m_isPaused) return;
    if (!m_frameStepper) setFrameStepper(true);
    m_stepRequests = std::min(m_stepRequests + 1, 8);
    FileLogger::get().debug("[Stepper] queued one scheduler/physics tick");
}

bool BotController::consumeFrameStep() {
    if (!m_frameStepper || m_stepRequests <= 0) return false;
    --m_stepRequests;
    return true;
}

void BotController::toggleTrajectory() {
    m_trajectoryVisible = !m_trajectoryVisible;
    m_trajectory.setVisible(m_trajectoryVisible);
    if (auto* play = PlayLayer::get()) m_trajectory.reset(play);
    FileLogger::get().debug(std::string("[Trajectory] visible=") + (m_trajectoryVisible ? "true" : "false"));
}

void BotController::setSpeedhackEnabled(bool enabled) {
    m_speedhack.setEnabled(enabled);
    syncAudio();
    FileLogger::get().debug(std::string("[Speedhack] enabled=") + (enabled ? "true" : "false"));
}

void BotController::setSpeedhackSpeed(float speed) {
    m_speedhack.setSpeed(speed);
    syncAudio();
    std::ostringstream out;
    out << "[Speedhack] speed=" << m_speedhack.speed();
    FileLogger::get().debug(out.str());
}

void BotController::setSpeedhackAudio(bool enabled) {
    m_speedhack.setAudioFollow(enabled);
    syncAudio();
    FileLogger::get().debug(std::string("[Speedhack] audioFollow=") + (enabled ? "true" : "false"));
}

void BotController::syncAudio() {
    // Analysis may run many times faster than realtime; don't turn that into a
    // chipmunk soundtrack. Manual speedhack audio remains immediate and persistent.
    m_speedhack.syncAudio(m_mode != BotMode::Analyzing);
}

void BotController::recordInput(bool pressed, int button, bool player1) {
    if (m_mode != BotMode::Recording || m_recordingArming || m_injecting) return;

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
    if (!layer || m_recordingArming) return;
    if (m_mode == BotMode::Playback) injectForCurrentFrame(layer, m_lastMacro);
    else if (m_mode == BotMode::Analyzing) injectForCurrentFrame(layer, m_analysis.playbackMacro());
}

void BotController::afterPhysicsStep(PlayLayer* layer) {
    if (!layer || m_recordingArming) return;
    if (m_pendingReset) return;

    ++m_currentFrame;

    // Predictive trajectory is intentionally kept out of high-speed automated
    // runs. It is a manual visual aid, never an analyzer verdict source.
    if (m_trajectoryVisible && (m_mode == BotMode::Idle || m_mode == BotMode::Recording)) {
        m_trajectory.update(layer, fixedDt(), m_currentFrame);
    }

    if (m_mode == BotMode::Playback) {
        auto post = static_cast<std::uint64_t>(std::max(0, settingInt("post-macro-validation", 120)));
        auto end64 = static_cast<std::uint64_t>(m_lastMacro.lastFrame()) + 1u + post;
        auto endFrame = static_cast<std::uint32_t>(std::min<std::uint64_t>(end64, UINT32_MAX));
        if (m_currentFrame >= endFrame) {
            FileLogger::get().debug("[Playback] finished validation horizon");
            m_mode = BotMode::Idle;
        }
        return;
    }

    if (m_mode == BotMode::Analyzing && !m_branchResolved && m_analysis.shouldPassAt(m_currentFrame)) {
        m_branchResolved = true;
        advanceAnalysis("survived macro + validation horizon", true);
    }
}

void BotController::onVisualFrame(PlayLayer* layer) {
    syncAudio();
    if (!layer) return;
    if (!m_trajectoryVisible) return;
    if (m_mode == BotMode::Playback || m_mode == BotMode::Analyzing) return;

    // If the game is visually refreshing without a physics tick (e.g. stepper
    // frozen), update() is de-duplicated by the trajectory's frame cache.
    m_trajectory.update(layer, fixedDt(), m_currentFrame);
}

void BotController::beforeReset(PlayLayer* layer) {
    if (!layer || !m_anchor.validFor(layer)) return;
    if (m_mode == BotMode::Recording || m_mode == BotMode::Playback || m_mode == BotMode::Analyzing) {
        m_forceAnchorLoad = true;
    }
}

void BotController::onReset(PlayLayer* layer) {
    m_currentFrame = 0;
    m_playbackCursor = 0;
    m_branchResolved = false;
    if (layer) layer->m_extraDelta = 0.0f;
    m_trajectory.reset(layer);
    syncAudio();

    if (m_mode == BotMode::Recording && !m_recordingArming) {
        m_recordingMacro.inputs.clear();
        m_sequence = 0;
        FileLogger::get().debug("[Record] attempt reset -> partial inputs discarded; recording continues from anchor");
    }
}

CheckpointObject* BotController::forcedCheckpoint(PlayLayer* layer) const {
    if (!m_forceAnchorLoad || !m_anchor.validFor(layer)) return nullptr;
    return m_anchor.checkpoint();
}

void BotController::restartCurrentBranch(PlayLayer* layer) {
    requestAnchorRestore(layer);
}

void BotController::onForcedCheckpointLoaded(PlayLayer* layer) {
    if (!m_forceAnchorLoad) return;
    m_forceAnchorLoad = false;

    if (m_recordingArming) {
        if (!m_anchor.captureSupplemental(layer)) {
            m_recordingArming = false;
            m_mode = BotMode::Idle;
            setNotice("Record failed: could not snapshot the loaded checkpoint.");
            return;
        }
        m_recordingMacro.anchorInputs = m_anchor.inputs();
        m_recordingArming = false;
        m_currentFrame = 0;
        m_playbackCursor = 0;
        m_sequence = 0;
        m_recordingMacro.inputs.clear();
        setNotice("Recording.");
        FileLogger::get().debug("[Record] started at loaded Practice checkpoint; frame=0");
    } else {
        m_anchor.applySupplemental(layer);
    }

    if (layer) layer->m_extraDelta = 0.0f;
    m_trajectory.reset(layer);
    syncAudio();
    FileLogger::get().debug("[Practice] FWBot anchor loaded");
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

void BotController::onPlayLayerExit(PlayLayer* layer) {
    if (!layer) return;
    cancelAutomation();
    setFrameStepper(false);
    m_speedhack.resetAudio();
    m_anchor.clear();
    m_trajectory.reset(nullptr);
    m_recordingArming = false;
    m_currentFrame = 0;
    m_lastNotice.clear();
    FileLogger::get().debug("[FWBot] PlayLayer exit: runtime state cleared");
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
        syncAudio();
        return;
    }

    auto path = FileLogger::get().exportAnalysis(m_analysis.sourceMacro(), m_analysis.results());
    FileLogger::get().debug(std::string("[Analyzer] finished report=") + path.string());
    m_mode = BotMode::Idle;
    // Return to the exact state from which the macro was recorded instead of
    // leaving the player in the final simulated branch. The scheduler hook
    // consumes this restore request immediately after the current fixed tick.
    m_pendingReset = m_anchor.validFor(PlayLayer::get());
    m_branchResolved = false;
    syncAudio();
}

float BotController::schedulerSpeed() const {
    if (m_mode == BotMode::Analyzing) {
        return static_cast<float>(std::clamp(settingInt("analysis-speed", 8), 1, 16));
    }
    return gameplaySpeed();
}

float BotController::fixedDt() const {
    return kPhysicsDt;
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
        case BotMode::Recording: out << (m_recordingArming ? "Record: loading checkpoint" : "Recording"); break;
        case BotMode::Playback: out << "Playback"; break;
        case BotMode::Analyzing: out << "Analyzing " << m_analysis.completedJobs() << '/' << m_analysis.totalJobs(); break;
    }
    out << " | F " << m_currentFrame;
    if (m_frameStepper) out << " | STEPPER";
    if (m_speedhack.enabled()) out << " | " << m_speedhack.speed() << "x";
    return out.str();
}

} // namespace fwa
