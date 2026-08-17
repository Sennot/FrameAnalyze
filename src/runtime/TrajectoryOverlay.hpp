#pragma once

#include <Geode/Geode.hpp>
#include <cstdint>

namespace fwa {

// Visual predictor only. The analyzer never uses these lines as a PASS/FAIL source.
class TrajectoryOverlay {
public:
    ~TrajectoryOverlay();

    void reset(PlayLayer* layer);
    void setVisible(bool visible);
    void update(PlayLayer* layer, float physicsDt, std::uint32_t frame);
    [[nodiscard]] bool drawing() const { return m_drawing; }

private:
    void destroyNodes();
    void ensureNodes(PlayLayer* layer);
    void rebuild(PlayLayer* layer, float physicsDt);
    void simulateBranch(
        PlayLayer* layer,
        PlayerObject* real,
        PlayerObject* fake,
        bool hold,
        cocos2d::ccColor4F color,
        float physicsDt
    );

    cocos2d::CCDrawNode* m_node = nullptr;
    PlayerObject* m_fakeP1 = nullptr;
    PlayerObject* m_fakeP2 = nullptr;
    PlayLayer* m_owner = nullptr;
    bool m_visible = false;
    bool m_drawing = false;
    std::uint32_t m_lastFrame = UINT32_MAX;
};

} // namespace fwa
