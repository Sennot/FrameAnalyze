#pragma once

#include <Geode/Geode.hpp>

namespace fwa {

class TrajectoryOverlay {
public:
    void reset(PlayLayer* layer);
    void setVisible(bool visible);
    void sample(PlayLayer* layer);

private:
    void ensureNode(PlayLayer* layer);

    cocos2d::CCDrawNode* m_node = nullptr;
    cocos2d::CCPoint m_lastP1{};
    cocos2d::CCPoint m_lastP2{};
    bool m_hasP1 = false;
    bool m_hasP2 = false;
    bool m_visible = true;
};

} // namespace fwa
