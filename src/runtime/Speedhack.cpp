#include "runtime/Speedhack.hpp"

#include <Geode/Geode.hpp>
#include <algorithm>

using namespace geode::prelude;

namespace fwa {

void Speedhack::setEnabled(bool value) {
    m_enabled = value;
    applyAudio();
}

void Speedhack::setSpeed(float value) {
    m_speed = std::clamp(value, 0.05f, 10.0f);
    applyAudio();
}

void Speedhack::setAudioFollow(bool value) {
    m_audioFollow = value;
    applyAudio();
}

void Speedhack::applyAudio() const {
    auto* engine = FMODAudioEngine::get();
    if (!engine || !engine->m_system) return;
    FMOD::ChannelGroup* master = nullptr;
    if (engine->m_system->getMasterChannelGroup(&master) != FMOD_OK || !master) return;
    master->setPitch((m_enabled && m_audioFollow) ? m_speed : 1.0f);
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
