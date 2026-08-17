#include "runtime/TrajectoryOverlay.hpp"

#include <Geode/loader/Mod.hpp>

#include <algorithm>
#include <cmath>

using namespace geode::prelude;

namespace fwa {

namespace {

struct DrawingGuard {
    bool& flag;
    explicit DrawingGuard(bool& value) : flag(value) { flag = true; }
    ~DrawingGuard() { flag = false; }
};

bool finitePoint(CCPoint const& p) {
    return std::isfinite(p.x) && std::isfinite(p.y);
}

} // namespace

TrajectoryOverlay::~TrajectoryOverlay() {
    destroyNodes();
}

void TrajectoryOverlay::destroyNodes() {
    // v0.2.1 only nulled these pointers. The old CCDrawNodes stayed attached to
    // PlayLayer, which is exactly why multiple long stale lines accumulated.
    if (m_node) m_node->removeFromParentAndCleanup(true);
    if (m_fakeP1) m_fakeP1->removeFromParentAndCleanup(true);
    if (m_fakeP2) m_fakeP2->removeFromParentAndCleanup(true);

    m_node = nullptr;
    m_fakeP1 = nullptr;
    m_fakeP2 = nullptr;
    m_owner = nullptr;
    m_lastFrame = UINT32_MAX;
    m_drawing = false;
}

void TrajectoryOverlay::ensureNodes(PlayLayer* layer) {
    if (!layer) return;
    if (m_owner == layer && m_node && m_fakeP1) return;

    destroyNodes();
    m_owner = layer;

    m_node = CCDrawNode::create();
    if (!m_node) return;
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
    destroyNodes();
    if (layer && m_visible) ensureNodes(layer);
}

void TrajectoryOverlay::setVisible(bool visible) {
    m_visible = visible;
    if (!visible) {
        // Hidden means fully detached, not just alpha/visibility off. This makes
        // Ctrl+F5 an immediate hard cleanup even before the next reset.
        destroyNodes();
        return;
    }
    if (m_node) m_node->setVisible(true);
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

    // Save the portions of world state that collision prediction can legitimately
    // touch. Silicate follows the same save/simulate/restore principle.
    GJGameState gameState = layer->m_gameState;
    EffectManagerState effectState{};
    bool hasEffects = layer->m_effectManager != nullptr;
    if (hasEffects) layer->m_effectManager->saveToState(effectState);

    DrawingGuard guard(m_drawing);

    fake->copyAttributes(real);
    fake->setPosition(real->getPosition());
    fake->setRotation(real->getRotation());
    fake->m_playEffects = false;
    fake->m_maybeReducedEffects = true;
    fake->setVisible(false);

    if (hold) {
        fake->pushButton(PlayerButton::Jump);
    } else {
        fake->releaseButton(PlayerButton::Jump);
        fake->m_jumpBuffered = false;
    }

    int length = 30;
    try {
        length = static_cast<int>(Mod::get()->getSettingValue<std::int64_t>("trajectory-length"));
    } catch (...) {}
    length = std::clamp(length, 15, 180);

    float playerDelta = physicsDt * 60.0f;
    CCPoint start = fake->getPosition();
    CCPoint previous = start;

    for (int i = 0; i < length; ++i) {
        fake->update(playerDelta);
        fake->updateRotation(playerDelta);

        int collision = layer->checkCollisions(fake, playerDelta, false);
        auto current = fake->getPosition();

        if (!finitePoint(current)) break;
        if (std::abs(current.x - start.x) > 900.0f || std::abs(current.y - start.y) > 700.0f) break;

        m_node->drawSegment(previous, current, 0.45f, color);
        previous = current;

        if (collision == 1 || fake->m_isDead) break;
        if (layer->m_effectManager) layer->m_effectManager->postCollisionCheck();
    }

    fake->setVisible(false);
    layer->m_gameState = gameState;
    if (hasEffects && layer->m_effectManager) {
        layer->m_effectManager->loadFromState(effectState);
    }
}

void TrajectoryOverlay::rebuild(PlayLayer* layer, float physicsDt) {
    ensureNodes(layer);
    if (!m_node || !m_fakeP1 || !layer->m_player1) return;

    m_node->clear();

    simulateBranch(layer, layer->m_player1, m_fakeP1, true,  {0.20f, 0.82f, 1.0f, 0.72f}, physicsDt);
    simulateBranch(layer, layer->m_player1, m_fakeP1, false, {0.82f, 0.84f, 0.90f, 0.52f}, physicsDt);

    if (layer->m_player2 && layer->m_gameState.m_isDualMode && m_fakeP2) {
        simulateBranch(layer, layer->m_player2, m_fakeP2, true,  {0.92f, 0.42f, 0.88f, 0.66f}, physicsDt);
        simulateBranch(layer, layer->m_player2, m_fakeP2, false, {0.70f, 0.50f, 0.84f, 0.48f}, physicsDt);
    }
}

void TrajectoryOverlay::update(PlayLayer* layer, float physicsDt, std::uint32_t frame) {
    if (!m_visible || !layer || layer->m_isPaused) return;
    ensureNodes(layer);
    if (!m_node) return;

    // At 240 physics Hz this caps visual prediction rebuilds to ~60 Hz.
    if (frame == m_lastFrame) return;
    if (frame != 0 && (frame % 4u) != 0u) return;
    m_lastFrame = frame;
    rebuild(layer, physicsDt);
}

} // namespace fwa
