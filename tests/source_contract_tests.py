from pathlib import Path
import json

root = Path(__file__).resolve().parents[1]

def text(path: str) -> str:
    return (root / path).read_text(encoding='utf-8')

# Every Geode Modify target must include its class-specific modifier header.
modify_contracts = {
    'src/hooks/GameHooks.cpp': [
        '#include <Geode/modify/CCScheduler.hpp>',
        '#include <Geode/modify/GJBaseGameLayer.hpp>',
        '#include <Geode/modify/PlayLayer.hpp>',
    ],
    'src/ui/ImGuiOverlay.cpp': [
        '#include <Geode/modify/CCEGLView.hpp>',
    ],
}
for path, required in modify_contracts.items():
    src = text(path)
    for include in required:
        assert include in src, f'{path}: missing required modifier header {include}'

# Timing architecture: scheduler owns freeze/speed. Never manually drive gameplay update.
hooks = text('src/hooks/GameHooks.cpp')
assert 'class $modify(FWBotSchedulerHook, CCScheduler)' in hooks
assert 'CCScheduler::update(bot.fixedDt());' in hooks
assert 'CCScheduler::update(dt * speed);' in hooks
assert 'int ticks = std::clamp(static_cast<int>(bot.schedulerSpeed()), 1, 16);' in hooks
assert 'CCScheduler::update(bot.fixedDt());' in hooks
assert 'GJBaseGameLayer::update(bot.fixedDt())' not in hooks
assert 'void processCommands(float dt, bool isHalfTick, bool isLastTick)' in hooks
assert 'if (bot.waitingForBranchReset()) return;' in hooks

# GJEffectManager::loadFromState is bound as EffectManagerState& on GD 2.2081.
# Keep the captured anchor immutable and load through a working copy.
practice = text('src/runtime/PracticeAnchor.cpp')
assert 'auto effectState = m_effectState;' in practice
assert 'loadFromState(effectState);' in practice
assert 'loadFromState(m_effectState);' not in practice

# Keybind callback must remain const-invocable in Geode EventV3.
main = text('src/main.cpp')
assert 'bool operator()(Keybind const&, bool down, bool repeat, double) const' in main
assert 'std::is_invocable_r_v<bool, KeybindPressListener const&' in main

# Trajectory regression: reset must remove old Cocos nodes instead of just forgetting pointers.
analysis = text('src/runtime/AnalysisSession.cpp')
assert '+ 1u + extra' in analysis

trajectory = text('src/runtime/TrajectoryOverlay.cpp')
assert 'removeFromParentAndCleanup(true)' in trajectory
assert 'm_node->clear();' in trajectory
assert 'layer->checkCollisions(fake, playerDelta, false)' in trajectory
assert 'layer->m_gameState = gameState;' in trajectory

# Audio speedhack is actively re-synchronized, so respawn cannot leave stale pitch.
speedhack = text('src/runtime/Speedhack.cpp')
bot = text('src/runtime/BotController.cpp')
assert 'master->setPitch(target);' in speedhack
assert 'void BotController::onVisualFrame' in bot
assert 'm_pendingReset = m_anchor.validFor(PlayLayer::get());' in bot
assert '+ 1u + post' in bot
assert 'syncAudio();' in bot

# Trajectory must be opt-in at runtime. Old persisted show-trajectory settings caused surprise lines.
assert 'm_trajectoryVisible = false;' in bot
assert 'if (!play || play->m_isPaused)' in bot
assert 'if (!play || play->m_isPaused) return;' in bot

# ImGui menu must have safe positioning + persistence.
ui = text('src/ui/ImGuiOverlay.cpp')
assert 'fwbot_imgui.ini' in ui
assert 'clampWindowToSafeArea' in ui
assert 'ImGui::SetNextWindowPos(initial, ImGuiCond_FirstUseEver);' in ui
assert 'ImGui::BeginTabBar("##fwbot-tabs")' in ui
assert 'fwbotFunctionKey = wParam >= VK_F3 && wParam <= VK_F8' in ui

mod = json.loads(text('mod.json'))
assert mod['version'] == 'v0.2.2'
assert mod['geode'] == '5.8.2'
assert mod['gd']['win'] == '2.2081'
assert 'show-trajectory' not in mod['settings']
assert mod['settings']['analysis-speed']['max'] <= 16
assert mod['settings']['trajectory-length']['default'] <= 30

print('source contract tests: OK')
