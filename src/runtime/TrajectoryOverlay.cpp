#include "runtime/TrajectoryOverlay.hpp"

using namespace geode::prelude;

namespace fwa {

void TrajectoryOverlay::ensureNode(PlayLayer* layer) {
    if (m_node && m_node->getParent() == layer) return;
    m_node = CCDrawNode::create();
    m_node->setID("fwa-trajectory-overlay");
    m_node->setVisible(m_visible);
    layer->addChild(m_node, 1000);
}

void TrajectoryOverlay::reset(PlayLayer* layer) {
    m_node = nullptr;
    m_hasP1 = false;
    m_hasP2 = false;
    if (layer && m_visible) ensureNode(layer);
}

void TrajectoryOverlay::setVisible(bool visible) {
    m_visible = visible;
    if (m_node) m_node->setVisible(visible);
}

void TrajectoryOverlay::sample(PlayLayer* layer) {
    if (!m_visible || !layer) return;
    ensureNode(layer);

    auto drawPlayer = [&](PlayerObject* player, CCPoint& last, bool& has, ccColor4F color) {
        if (!player) return;
        auto pos = player->getPosition();
        if (has) m_node->drawSegment(last, pos, 0.65f, color);
        last = pos;
        has = true;
    };

    drawPlayer(layer->m_player1, m_lastP1, m_hasP1, {0.15f, 0.95f, 1.0f, 0.78f});
    drawPlayer(layer->m_player2, m_lastP2, m_hasP2, {1.0f, 0.45f, 0.85f, 0.78f});
}

} // namespace fwa
