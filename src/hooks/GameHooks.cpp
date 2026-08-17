#include "runtime/BotController.hpp"

#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>

#include <algorithm>

using namespace geode::prelude;

namespace fwa {

class $modify(FWAGameLayerHook, GJBaseGameLayer) {
    void handleButton(bool pressed, int button, bool player1) {
        auto& bot = BotController::get();
        if (!bot.isInjecting() && bot.suppressUserInput()) {
            return;
        }
        GJBaseGameLayer::handleButton(pressed, button, player1);
        if (!bot.isInjecting()) bot.recordInput(pressed, button, player1);
    }

    void update(float dt) override {
        auto& bot = BotController::get();
        auto* play = PlayLayer::get();
        if (!play || play != this || !bot.shouldUseFixedStep()) {
            GJBaseGameLayer::update(dt);
            return;
        }

        int steps = bot.stepsForUpdate(dt);
        for (int i = 0; i < steps; ++i) {
            bot.beforePhysicsStep(play);
            GJBaseGameLayer::update(bot.fixedDt());
            bot.afterPhysicsStep(play);
            if (bot.consumeResetRequest()) {
                play->resetLevel();
                break;
            }
        }
    }

    double getModifiedDelta(float dt) {
        auto& bot = BotController::get();
        if (!PlayLayer::get() || !bot.shouldUseFixedStep()) {
            return GJBaseGameLayer::getModifiedDelta(dt);
        }

        // Lock physics delta to the analyzer TPS while preserving GD's slow-time
        // component. This follows the same fixed-delta principle used by bot tools
        // such as Silicate without copying their implementation.
        float timeWarp = std::min(this->m_gameState.m_timeWarp, 1.0f);
        return static_cast<double>(bot.fixedDt() * timeWarp);
    }
};

class $modify(FWAPlayLayerHook, PlayLayer) {
    void resetLevel() {
        PlayLayer::resetLevel();
        BotController::get().onReset(this);
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
        if (bot.mode() == BotMode::Recording) {
            bot.stopRecording(this, false);
        }
        PlayLayer::levelComplete();
    }
};

} // namespace fwa
