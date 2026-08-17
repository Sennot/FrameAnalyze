#pragma once

#include "core/Types.hpp"
#include "runtime/AnalysisSession.hpp"
#include "runtime/PracticeAnchor.hpp"
#include "runtime/Speedhack.hpp"
#include "runtime/TrajectoryOverlay.hpp"

#include <Geode/Geode.hpp>

#include <cstddef>
#include <cstdint>
#include <string>

namespace fwa {

enum class BotMode {
    Idle,
    Recording,
    Playback,
    Analyzing,
};

class BotController {
public:
    static BotController& get();

    void initialize();

    void toggleRecording(PlayLayer* layer);
    void startRecording(PlayLayer* layer);
    void stopRecording(PlayLayer* layer, bool allowAutoAnalyze = true);
    void startPlayback(PlayLayer* layer);
    void startAnalysis(PlayLayer* layer);
    void cancelAutomation();

    void toggleFrameStepper();
    void setFrameStepper(bool enabled);
    void requestFrameStep();
    bool consumeFrameStep();
    void toggleTrajectory();

    void setSpeedhackEnabled(bool enabled);
    void setSpeedhackSpeed(float speed);
    void setSpeedhackAudio(bool enabled);
    void syncAudio();

    void recordInput(bool pressed, int button, bool player1);
    void beforePhysicsStep(PlayLayer* layer);
    void afterPhysicsStep(PlayLayer* layer);
    void onVisualFrame(PlayLayer* layer);
    void beforeReset(PlayLayer* layer);
    void onReset(PlayLayer* layer);
    void onDeath(std::uint32_t frame, GameObject* cause);
    void onLevelComplete();
    void onPlayLayerExit(PlayLayer* layer);

    [[nodiscard]] float schedulerSpeed() const;
    [[nodiscard]] float fixedDt() const;
    [[nodiscard]] bool consumeResetRequest();
    [[nodiscard]] CheckpointObject* forcedCheckpoint(PlayLayer* layer) const;
    void restartCurrentBranch(PlayLayer* layer);
    void onForcedCheckpointLoaded(PlayLayer* layer);

    [[nodiscard]] bool isInjecting() const { return m_injecting; }
    [[nodiscard]] bool suppressUserInput() const { return m_mode == BotMode::Playback || m_mode == BotMode::Analyzing; }
    [[nodiscard]] bool isAnalyzing() const { return m_mode == BotMode::Analyzing; }
    [[nodiscard]] bool waitingForBranchReset() const { return m_pendingReset; }
    [[nodiscard]] BotMode mode() const { return m_mode; }
    [[nodiscard]] std::uint32_t currentFrame() const { return m_currentFrame; }
    [[nodiscard]] Macro const& lastMacro() const { return m_lastMacro; }
    [[nodiscard]] bool hasMacro() const { return !m_lastMacro.inputs.empty(); }
    [[nodiscard]] bool frameStepperEnabled() const { return m_frameStepper; }
    [[nodiscard]] int pendingSteps() const { return m_stepRequests; }
    [[nodiscard]] bool trajectoryVisible() const { return m_trajectoryVisible; }
    [[nodiscard]] bool trajectoryDrawing() const { return m_trajectory.drawing(); }
    [[nodiscard]] bool hasPracticeAnchor() const { return m_anchor.checkpoint() != nullptr; }
    [[nodiscard]] bool speedhackEnabled() const { return m_speedhack.enabled(); }
    [[nodiscard]] bool speedhackAudio() const { return m_speedhack.audioFollow(); }
    [[nodiscard]] float speedhackSpeed() const { return m_speedhack.speed(); }
    [[nodiscard]] float gameplaySpeed() const { return m_speedhack.effectiveSpeed(); }
    [[nodiscard]] std::string statusText() const;

private:
    void injectForCurrentFrame(PlayLayer* layer, Macro const& macro);
    void advanceAnalysis(std::string const& reason, bool passed);
    void finishAnalysis();
    void fillLevelMetadata(PlayLayer* layer, Macro& macro);
    bool captureAnchor(PlayLayer* layer, Macro& macro);
    void requestAnchorRestore(PlayLayer* layer);
    int settingInt(char const* key, int fallback) const;
    bool settingBool(char const* key, bool fallback) const;

    BotMode m_mode = BotMode::Idle;
    Macro m_recordingMacro;
    Macro m_lastMacro;
    AnalysisSession m_analysis;
    PracticeAnchor m_anchor;
    Speedhack m_speedhack;
    TrajectoryOverlay m_trajectory;

    std::uint32_t m_currentFrame = 0;
    std::size_t m_playbackCursor = 0;
    std::size_t m_sequence = 0;
    bool m_injecting = false;
    bool m_pendingReset = false;
    bool m_branchResolved = false;
    bool m_frameStepper = false;
    int m_stepRequests = 0;
    bool m_trajectoryVisible = false;
    bool m_forceAnchorLoad = false;
};

} // namespace fwa
