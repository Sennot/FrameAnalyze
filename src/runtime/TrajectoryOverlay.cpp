#include "runtime/TrajectoryOverlay.hpp"

#include <Geode/loader/Mod.hpp>
#include <algorithm>

using namespace geode::prelude;

namespace fwa {

void TrajectoryOverlay::ensureNodes(PlayLayer* layer) {
    if (!layer) return;
    if (m_owner == layer && m_node && m_fakeP1) return;

    m_owner = layer;
    m_node = CCDrawNode::create();
    m_node->setID("fwbot-trajectory");
    m_node->setVisible(m_visible);
    layer->addChild(m_node, 1000);

    m_fakeP1 = PlayerObject::create(1, 1, layer, layer, true);
    if (m_fakeP1) {
        m_fakeP1->setVisible(false);
        layer->addChild(m_fakeP1, -1000);
    }
    m_fakeP2 = PlayerObject::create(2, 1, layer, layer, true);
    if (m_fakeP2) {
        m_fakeP2->setVisible(false);
        layer->addChild(m_fakeP2, -1000);
    }
}

void TrajectoryOverlay::reset(PlayLayer* layer) {
    m_node = nullptr;
    m_fakeP1 = nullptr;
    m_fakeP2 = nullptr;
    m_owner = nullptr;
    m_lastFrame = UINT32_MAX;
    if (layer && m_visible) ensureNodes(layer);
}

void TrajectoryOverlay::setVisible(bool visible) {
    m_visible = visible;
    if (m_node) m_node->setVisible(visible);
}

void TrajectoryOverlay::simulateBranch(
    PlayLayer* layer,
    PlayerObject* real,
    PlayerObject* fake,
    bool hold,
    ccColor4F color,
    float physicsDt
) {
    if (!layer || !real || !fake || !m_node) return;

    fake->copyAttributes(real);
    fake->setPosition(real->getPosition());
    fake->setRotation(real->getRotation());
    fake->m_playEffects = false;
    fake->setVisible(false);

    if (hold) fake->pushButton(PlayerButton::Jump);
    else {
        fake->releaseButton(PlayerButton::Jump);
        fake->m_jumpBuffered = false;
    }

    int length = 90;
    try {
        length = static_cast<int>(Mod::get()->getSettingValue<std::int64_t>("trajectory-length"));
    } catch (...) {}
    length = std::clamp(length, 15, 360);

    // PlayerObject::update expects GD's 60Hz-scaled delta in this physics path,
    // same convention used by Silicate's trajectory iteration.
    float playerDelta = physicsDt * 60.0f;
    CCPoint previous = fake->getPosition();
    for (int i = 0; i < length; ++i) {
        fake->update(playerDelta);
        fake->updateRotation(playerDelta);
        auto current = fake->getPosition();
        m_node->drawSegment(previous, current, 0.7f, color);
        previous = current;
    }
    fake->setVisible(false);
}

void TrajectoryOverlay::rebuild(PlayLayer* layer, float physicsDt) {
    ensureNodes(layer);
    if (!m_node) return;
    m_node->clear();

    // Hold/release alternatives are shown separately. The trajectory backend is visual;
    // exact hazards and frame-window verdicts are evaluated by real gameplay branches.
    simulateBranch(layer, layer->m_player1, m_fakeP1, true,  {0.24f, 0.84f, 1.0f, 0.78f}, physicsDt);
    simulateBranch(layer, layer->m_player1, m_fakeP1, false, {0.78f, 0.78f, 0.82f, 0.62f}, physicsDt);

    if (layer->m_player2 && layer->m_gameState.m_isDualMode) {
        simulateBranch(layer, layer->m_player2, m_fakeP2, true,  {0.92f, 0.42f, 0.88f, 0.72f}, physicsDt);
        simulateBranch(layer, layer->m_player2, m_fakeP2, false, {0.68f, 0.50f, 0.82f, 0.54f}, physicsDt);
    }
}

void TrajectoryOverlay::update(PlayLayer* layer, float physicsDt, std::uint32_t frame) {
    if (!m_visible || !layer) return;
    ensureNodes(layer);
    if (frame == m_lastFrame) return;
    m_lastFrame = frame;
    rebuild(layer, physicsDt);
}

} // namespace fwa
