#pragma once

#include <Geode/Geode.hpp>

namespace fwa {

class BotMenu : public cocos2d::CCLayerColor {
public:
    static BotMenu* create();
    static void toggle();

    bool init() override;

private:
    void onClose(cocos2d::CCObject*);
    void onRecord(cocos2d::CCObject*);
    void onPlayback(cocos2d::CCObject*);
    void onAnalyze(cocos2d::CCObject*);
    void onPause(cocos2d::CCObject*);
    void onStep(cocos2d::CCObject*);
    void onTrajectory(cocos2d::CCObject*);
    void refreshStatus();

    cocos2d::CCLabelBMFont* m_status = nullptr;
};

} // namespace fwa
