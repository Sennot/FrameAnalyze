#pragma once

#include "core/Types.hpp"
#include "runtime/AnalysisSession.hpp"
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

    void togglePause();
    void requestFrameStep();
    void toggleTrajectory();

    void recordInput(bool pressed, int button, bool player1);
    void beforePhysicsStep(PlayLayer* layer);
    void afterPhysicsStep(PlayLayer* layer);
    void onReset(PlayLayer* layer);
    void onDeath(std::uint32_t frame, GameObject* cause);
    void onLevelComplete();

    [[nodiscard]] bool shouldUseFixedStep() const;
    int stepsForUpdate(float realDt);
    [[nodiscard]] float fixedDt() const;
    [[nodiscard]] bool consumeResetRequest();

    [[nodiscard]] bool isInjecting() const { return m_injecting; }
    [[nodiscard]] bool suppressUserInput() const { return m_mode == BotMode::Playback || m_mode == BotMode::Analyzing; }
    [[nodiscard]] bool isAnalyzing() const { return m_mode == BotMode::Analyzing; }
    [[nodiscard]] BotMode mode() const { return m_mode; }
    [[nodiscard]] std::uint32_t currentFrame() const { return m_currentFrame; }
    [[nodiscard]] Macro const& lastMacro() const { return m_lastMacro; }
    [[nodiscard]] bool hasMacro() const { return !m_lastMacro.inputs.empty(); }
    [[nodiscard]] bool framePaused() const { return m_framePaused; }
    [[nodiscard]] bool trajectoryVisible() const { return m_trajectoryVisible; }
    [[nodiscard]] std::string statusText() const;

private:
    void injectForCurrentFrame(PlayLayer* layer, Macro const& macro);
    void advanceAnalysis(std::string const& reason, bool passed);
    void finishAnalysis();
    void fillLevelMetadata(PlayLayer* layer, Macro& macro);
    int settingInt(char const* key, int fallback) const;
    bool settingBool(char const* key, bool fallback) const;

    BotMode m_mode = BotMode::Idle;
    Macro m_recordingMacro;
    Macro m_lastMacro;
    AnalysisSession m_analysis;
    TrajectoryOverlay m_trajectory;

    std::uint32_t m_currentFrame = 0;
    std::size_t m_playbackCursor = 0;
    std::size_t m_sequence = 0;
    double m_accumulator = 0.0;
    bool m_injecting = false;
    bool m_pendingReset = false;
    bool m_branchResolved = false;
    bool m_framePaused = false;
    int m_stepRequests = 0;
    bool m_trajectoryVisible = true;
};

} // namespace fwa
