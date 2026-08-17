#pragma once

#include <Geode/Geode.hpp>
#include <cstdint>

namespace fwa {

// Predictive trajectory is deliberately isolated from the frame-window verdict engine.
// It uses copied fake players for visual prediction; Analyze still uses real PlayLayer branches.
class TrajectoryOverlay {
public:
    void reset(PlayLayer* layer);
    void setVisible(bool visible);
    void update(PlayLayer* layer, float physicsDt, std::uint32_t frame);

private:
    void ensureNodes(PlayLayer* layer);
    void rebuild(PlayLayer* layer, float physicsDt);
    void simulateBranch(PlayLayer* layer, PlayerObject* real, PlayerObject* fake, bool hold, cocos2d::ccColor4F color, float physicsDt);

    cocos2d::CCDrawNode* m_node = nullptr;
    PlayerObject* m_fakeP1 = nullptr;
    PlayerObject* m_fakeP2 = nullptr;
    PlayLayer* m_owner = nullptr;
    bool m_visible = true;
    std::uint32_t m_lastFrame = UINT32_MAX;
};

} // namespace fwa
