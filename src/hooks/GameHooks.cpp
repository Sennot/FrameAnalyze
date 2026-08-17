#include "runtime/BotController.hpp"

#include <Geode/Geode.hpp>
#include <Geode/modify/CCScheduler.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>

#include <algorithm>

using namespace geode::prelude;

namespace fwa {

// Time control belongs at the scheduler level. This mirrors the stable part of
// Silicate's architecture and avoids manually calling GJBaseGameLayer::update.
class $modify(FWBotSchedulerHook, CCScheduler) {
    void update(float dt) override {
        auto& bot = BotController::get();
        auto* play = PlayLayer::get();

        if (!play || play->m_isPaused) {
            bot.syncAudio();
            CCScheduler::update(dt);
            return;
        }

        if (bot.frameStepperEnabled()) {
            if (!bot.consumeFrameStep()) {
                // Do not call the scheduler at all. The render loop still swaps
                // buffers, so ImGui remains responsive while gameplay is frozen.
                bot.syncAudio();
                bot.onVisualFrame(play);
                return;
            }

            play->m_extraDelta = 0.0f;
            CCScheduler::update(bot.fixedDt());
        } else if (bot.isAnalyzing()) {
            // Analysis uses exact 240 Hz scheduler ticks rather than one giant
            // variable dt. The setting is ticks per visual frame, not physics TPS.
            int ticks = std::clamp(static_cast<int>(bot.schedulerSpeed()), 1, 16);
            for (int i = 0; i < ticks && bot.isAnalyzing(); ++i) {
                CCScheduler::update(bot.fixedDt());

                if (bot.consumeResetRequest()) {
                    bot.restartCurrentBranch(play);
                    play = PlayLayer::get();
                    if (!play) break;
                }
            }
        } else {
            float speed = std::clamp(bot.schedulerSpeed(), 0.05f, 10.0f);
            CCScheduler::update(dt * speed);
        }

        bot.onVisualFrame(play);

        // Final analysis completion also requests one restore so the user never
        // gets left inside the last simulated candidate branch.
        if (bot.consumeResetRequest() && play) {
            bot.restartCurrentBranch(play);
        }
    }
};

class $modify(FWBotGameLayerHook, GJBaseGameLayer) {
    void handleButton(bool pressed, int button, bool player1) {
        auto& bot = BotController::get();
        if (!bot.isInjecting() && bot.suppressUserInput()) return;

        // Record at the same relative physics-frame counter that playback uses.
        if (!bot.isInjecting()) bot.recordInput(pressed, button, player1);
        GJBaseGameLayer::handleButton(pressed, button, player1);
    }

    void processCommands(float dt, bool isHalfTick, bool isLastTick) {
        auto& bot = BotController::get();
        auto* play = PlayLayer::get();
        bool currentPlayLayer = play &&
            static_cast<GJBaseGameLayer*>(play) == static_cast<GJBaseGameLayer*>(this);

        if (!currentPlayLayer) {
            GJBaseGameLayer::processCommands(dt, isHalfTick, isLastTick);
            return;
        }

        // A branch can resolve in the middle of one accelerated scheduler update.
        // Do not let the newly prepared candidate run on the old branch world
        // before the scheduler returns and restores the Practice anchor.
        if (bot.waitingForBranchReset()) return;

        // processCommands is inside GD's physics loop. At high speed the game can
        // perform many of these in one rendered frame; every one still gets exact
        // replay injection / frame accounting.
        bot.beforePhysicsStep(play);
        GJBaseGameLayer::processCommands(dt, isHalfTick, isLastTick);
        bot.afterPhysicsStep(play);
    }

    void destroyObject(GameObject* object) {
        // Prediction may touch simulated objects. Never let a visual fake-player
        // trajectory permanently destroy real level objects.
        if (BotController::get().trajectoryDrawing()) return;
        GJBaseGameLayer::destroyObject(object);
    }

    void gameEventTriggered(GJGameEvent event, int p1, int p2) {
        if (BotController::get().trajectoryDrawing()) return;
        GJBaseGameLayer::gameEventTriggered(event, p1, p2);
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

        // Fake trajectory players must never kill/reset the real attempt.
        if (bot.trajectoryDrawing() && player != m_player1 && player != m_player2) {
            return;
        }

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
