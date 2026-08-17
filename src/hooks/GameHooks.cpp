#include "runtime/BotController.hpp"

#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>

#include <algorithm>

using namespace geode::prelude;

namespace fwa {

class $modify(FWBotGameLayerHook, GJBaseGameLayer) {
    void handleButton(bool pressed, int button, bool player1) {
        auto& bot = BotController::get();
        if (!bot.isInjecting() && bot.suppressUserInput()) return;

        GJBaseGameLayer::handleButton(pressed, button, player1);
        if (!bot.isInjecting()) bot.recordInput(pressed, button, player1);
    }

    void update(float dt) override {
        auto& bot = BotController::get();
        auto* play = PlayLayer::get();
        bool currentPlayLayer = play &&
            static_cast<GJBaseGameLayer*>(play) == static_cast<GJBaseGameLayer*>(this);

        if (!currentPlayLayer || !bot.shouldInterceptUpdate()) {
            GJBaseGameLayer::update(dt);
            return;
        }

        // Silicate-style frame stepping: no PauseLayer. When enabled, gameplay physics
        // simply does not receive an update until a step request is consumed.
        if (bot.frameStepperEnabled()) {
            if (!bot.consumeFrameStep()) return;
            play->m_extraDelta = 0.0f;
            bot.beforePhysicsStep(play);
            GJBaseGameLayer::update(bot.fixedDt());
            bot.afterPhysicsStep(play);
            if (bot.consumeResetRequest()) bot.restartCurrentBranch(play);
            return;
        }

        // Recording/playback/analyze use deterministic fixed physics ticks.
        if (bot.mode() != BotMode::Idle) {
            int steps = bot.automatedStepsForUpdate(dt);
            for (int i = 0; i < steps; ++i) {
                bot.beforePhysicsStep(play);
                GJBaseGameLayer::update(bot.fixedDt());
                bot.afterPhysicsStep(play);
                if (bot.consumeResetRequest()) {
                    bot.restartCurrentBranch(play);
                    break;
                }
            }
            return;
        }

        // Manual FWBot speedhack intentionally affects normal gameplay dt. Analysis
        // never uses this path; it always uses the fixed-tick branch above.
        GJBaseGameLayer::update(dt * bot.gameplaySpeed());
    }

    double getModifiedDelta(float dt) {
        auto& bot = BotController::get();
        if (!PlayLayer::get() || !bot.fixedDeltaActive()) {
            return GJBaseGameLayer::getModifiedDelta(dt);
        }
        float timeWarp = std::min(this->m_gameState.m_timeWarp, 1.0f);
        return static_cast<double>(bot.fixedDt() * timeWarp);
    }
};

class $modify(FWBotPlayLayerHook, PlayLayer) {
    void resetLevel() {
        auto& bot = BotController::get();
        bot.beforeReset(this);
        PlayLayer::resetLevel();
        bot.onReset(this);
    }

    void loadFromCheckpoint(CheckpointObject* object) {
        auto& bot = BotController::get();
        if (auto* forced = bot.forcedCheckpoint(this)) {
            PlayLayer::loadFromCheckpoint(forced);
            bot.onForcedCheckpointLoaded(this);
            return;
        }
        PlayLayer::loadFromCheckpoint(object);
    }

    void destroyPlayer(PlayerObject* player, GameObject* cause) {
        auto& bot = BotController::get();
        if (bot.isAnalyzing()) {
            bot.onDeath(bot.currentFrame(), cause);
            return;
        }
        PlayLayer::destroyPlayer(player, cause);
    }

    void levelComplete() {
        auto& bot = BotController::get();
        if (bot.isAnalyzing()) {
            bot.onLevelComplete();
            return;
        }
        if (bot.mode() == BotMode::Recording) bot.stopRecording(this, false);
        PlayLayer::levelComplete();
    }

    void onExit() override {
        BotController::get().onPlayLayerExit(this);
        PlayLayer::onExit();
    }
};

} // namespace fwa
