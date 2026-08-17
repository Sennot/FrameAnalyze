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

    // FWBot recording is Practice-checkpoint based. Do not call
    // PlayLayer::createCheckpoint() here: that is part of GD's checkpoint
    // placement flow and may legitimately return nullptr when invoked out of
    // that flow. Instead, adopt the checkpoint the player actually placed.
    auto* checkpoint = layer->getLastCheckpoint();
    if (!checkpoint) {
        FileLogger::get().debug("[Practice] no placed Practice checkpoint; Record refused");
        return false;
    }

    clear();
    checkpoint->retain();
    m_checkpoint = checkpoint;
    m_owner = layer;
    m_hasSupplemental = false;
    m_inputs = {};

    FileLogger::get().debug("[Practice] adopted last placed GD checkpoint as FWBot anchor");
    return true;
}

bool PracticeAnchor::captureSupplemental(PlayLayer* layer) {
    if (!validFor(layer)) return false;

    // This is called only after GD has actually loaded the retained Practice
    // checkpoint. Capturing here keeps the supplemental state aligned with the
    // checkpoint instead of accidentally snapshotting a later live position.
    m_gameState = layer->m_gameState;
    if (layer->m_effectManager) {
        layer->m_effectManager->saveToState(m_effectState);
        m_hasSupplemental = true;
    } else {
        m_hasSupplemental = false;
    }

    m_inputs = {};
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

    FileLogger::get().debug("[Practice] captured supplemental state after checkpoint load");
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
    if (!validFor(layer) || !m_hasSupplemental) return;
    layer->m_gameState = m_gameState;
    if (layer->m_effectManager) {
        // Geode/GD 2.2081 binds loadFromState as taking EffectManagerState&,
        // even though restoring should not mutate FWBot's saved anchor. Pass a
        // working copy so this method can remain const and repeated restores
        // always start from the exact same captured effect state.
        auto effectState = m_effectState;
        layer->m_effectManager->loadFromState(effectState);
    }
}

} // namespace fwa
