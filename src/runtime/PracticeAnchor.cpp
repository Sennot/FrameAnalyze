#include "runtime/PracticeAnchor.hpp"

#include "runtime/FileLogger.hpp"

using namespace geode::prelude;

namespace fwa {

PracticeAnchor::~PracticeAnchor() {
    clear();
}

void PracticeAnchor::clear() {
    if (m_checkpoint) {
        m_checkpoint->release();
        m_checkpoint = nullptr;
    }
    m_owner = nullptr;
    m_inputs = {};
    m_hasSupplemental = false;
}

bool PracticeAnchor::capture(PlayLayer* layer) {
    if (!layer) return false;
    clear();

    auto* checkpoint = layer->createCheckpoint();
    if (!checkpoint) {
        FileLogger::get().debug("[Practice] createCheckpoint returned null");
        return false;
    }

    checkpoint->retain();
    m_checkpoint = checkpoint;
    m_owner = layer;
    m_gameState = layer->m_gameState;
    if (layer->m_effectManager) {
        layer->m_effectManager->saveToState(m_effectState);
        m_hasSupplemental = true;
    }

    // Store held button state explicitly. CheckpointObject is the primary world state,
    // while these flags let replay validation know which RELEASE inputs are legal at frame 0.
    if (layer->m_player1) {
        auto const& held = layer->m_player1->m_holdingButtons;
        if (auto it = held.find(1); it != held.end()) m_inputs.p1[1] = it->second;
        m_inputs.p1[2] = layer->m_player1->m_holdingLeft;
        m_inputs.p1[3] = layer->m_player1->m_holdingRight;
    }
    if (layer->m_player2) {
        auto const& held = layer->m_player2->m_holdingButtons;
        if (auto it = held.find(1); it != held.end()) m_inputs.p2[1] = it->second;
        m_inputs.p2[2] = layer->m_player2->m_holdingLeft;
        m_inputs.p2[3] = layer->m_player2->m_holdingRight;
    }

    FileLogger::get().debug("[Practice] captured current gameplay checkpoint as FWBot anchor");
    return true;
}

bool PracticeAnchor::validFor(PlayLayer* layer) const {
    return layer && m_checkpoint && m_owner == layer;
}

bool PracticeAnchor::restoreNow(PlayLayer* layer) const {
    if (!validFor(layer)) return false;
    layer->loadFromCheckpoint(m_checkpoint);
    applySupplemental(layer);
    layer->m_extraDelta = 0.0f;
    return true;
}

void PracticeAnchor::applySupplemental(PlayLayer* layer) const {
    if (!validFor(layer)) return;
    layer->m_gameState = m_gameState;
    if (m_hasSupplemental && layer->m_effectManager) {
        layer->m_effectManager->loadFromState(m_effectState);
    }
}

} // namespace fwa
