from pathlib import Path
import json

root = Path(__file__).resolve().parents[1]

def text(path: str) -> str:
    return (root / path).read_text(encoding='utf-8')

# Geode's Modify system requires the class-specific generated modifier header.
modify_contracts = {
    'src/hooks/GameHooks.cpp': [
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

# GJEffectManager::loadFromState is bound as EffectManagerState& on GD 2.2081.
# Keep the captured anchor immutable and load through a working copy.
practice = text('src/runtime/PracticeAnchor.cpp')
assert 'auto effectState = m_effectState;' in practice
assert 'loadFromState(effectState);' in practice
assert 'loadFromState(m_effectState);' not in practice

# The keybind callback must remain const-invocable in Geode EventV3.
main = text('src/main.cpp')
assert 'bool operator()(Keybind const&, bool down, bool repeat, double) const' in main
assert 'std::is_invocable_r_v<bool, KeybindPressListener const&' in main

mod = json.loads(text('mod.json'))
assert mod['version'] == 'v0.2.1'
assert mod['geode'] == '5.8.2'
assert mod['gd']['win'] == '2.2081'

print('source contract tests: OK')
