#include "runtime/Speedhack.hpp"

#include <Geode/Geode.hpp>

#include <algorithm>
#include <cmath>

using namespace geode::prelude;

namespace fwa {

void Speedhack::setEnabled(bool value) {
    m_enabled = value;
}

void Speedhack::setSpeed(float value) {
    m_speed = std::clamp(value, 0.05f, 10.0f);
}

void Speedhack::setAudioFollow(bool value) {
    m_audioFollow = value;
}

void Speedhack::syncAudio(bool allowFollow) const {
    auto* engine = FMODAudioEngine::get();
    if (!engine || !engine->m_system) return;

    FMOD::ChannelGroup* master = nullptr;
    if (engine->m_system->getMasterChannelGroup(&master) != FMOD_OK || !master) return;

    float target = (allowFollow && m_enabled && m_audioFollow) ? m_speed : 1.0f;
    if (!std::isfinite(target) || target <= 0.0f) target = 1.0f;

    // Setting every scheduler frame is intentional: GD can recreate/reconfigure
    // audio state on respawn. This keeps the current attempt in sync immediately.
    master->setPitch(target);
}

void Speedhack::resetAudio() const {
    auto* engine = FMODAudioEngine::get();
    if (!engine || !engine->m_system) return;
    FMOD::ChannelGroup* master = nullptr;
    if (engine->m_system->getMasterChannelGroup(&master) == FMOD_OK && master) {
        master->setPitch(1.0f);
    }
}

} // namespace fwa
