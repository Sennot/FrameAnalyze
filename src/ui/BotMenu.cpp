#include "ui/BotMenu.hpp"

#include "runtime/BotController.hpp"

using namespace geode::prelude;

namespace fwa {
namespace {

CCMenuItemSpriteExtra* makeButton(char const* text, CCObject* target, SEL_MenuHandler callback) {
    auto sprite = ButtonSprite::create(text, "bigFont.fnt", "GJ_button_01.png");
    sprite->setScale(0.72f);
    return CCMenuItemSpriteExtra::create(sprite, target, callback);
}

} // namespace

BotMenu* BotMenu::create() {
    auto ret = new BotMenu();
    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool BotMenu::init() {
    if (!CCLayerColor::initWithColor({0, 0, 0, 135})) return false;
    setID("fwa-bot-menu");

    auto win = CCDirector::sharedDirector()->getWinSize();
    auto panel = CCScale9Sprite::create("GJ_square01.png");
    panel->setContentSize({330.f, 245.f});
    panel->setPosition({win.width / 2.f, win.height / 2.f});
    addChild(panel);

    auto title = CCLabelBMFont::create("Frame Window Analyzer", "bigFont.fnt");
    title->setScale(0.62f);
    title->setPosition({win.width / 2.f, win.height / 2.f + 96.f});
    addChild(title);

    m_status = CCLabelBMFont::create("Idle", "chatFont.fnt");
    m_status->setScale(0.72f);
    m_status->setPosition({win.width / 2.f, win.height / 2.f + 67.f});
    addChild(m_status);

    auto menu = CCMenu::create();
    menu->setPosition({win.width / 2.f, win.height / 2.f});
    addChild(menu);

    struct Spec { char const* name; SEL_MenuHandler cb; float x; float y; };
    Spec specs[] = {
        {"Record", menu_selector(BotMenu::onRecord), -72.f, 30.f},
        {"Playback", menu_selector(BotMenu::onPlayback), 72.f, 30.f},
        {"Analyze", menu_selector(BotMenu::onAnalyze), -72.f, -15.f},
        {"Pause", menu_selector(BotMenu::onPause), 72.f, -15.f},
        {"Frame Step", menu_selector(BotMenu::onStep), -72.f, -60.f},
        {"Trajectory", menu_selector(BotMenu::onTrajectory), 72.f, -60.f},
    };
    for (auto const& spec : specs) {
        auto item = makeButton(spec.name, this, spec.cb);
        item->setPosition({spec.x, spec.y});
        menu->addChild(item);
    }

    auto close = CCMenuItemSpriteExtra::create(
        CCSprite::createWithSpriteFrameName("GJ_closeBtn_001.png"),
        this,
        menu_selector(BotMenu::onClose)
    );
    close->setPosition({-151.f, 105.f});
    menu->addChild(close);

    auto hint = CCLabelBMFont::create("Binds: Geode > Frame Window Analyzer settings", "chatFont.fnt");
    hint->setScale(0.55f);
    hint->setPosition({win.width / 2.f, win.height / 2.f - 101.f});
    addChild(hint);

    refreshStatus();
    return true;
}

void BotMenu::toggle() {
    auto scene = CCDirector::sharedDirector()->getRunningScene();
    if (!scene) return;
    if (auto existing = scene->getChildByID("fwa-bot-menu")) {
        existing->removeFromParentAndCleanup(true);
        return;
    }
    if (auto menu = BotMenu::create()) scene->addChild(menu, 100000);
}

void BotMenu::onClose(CCObject*) { removeFromParentAndCleanup(true); }
void BotMenu::onRecord(CCObject*) { BotController::get().toggleRecording(PlayLayer::get()); refreshStatus(); }
void BotMenu::onPlayback(CCObject*) { BotController::get().startPlayback(PlayLayer::get()); refreshStatus(); }
void BotMenu::onAnalyze(CCObject*) { BotController::get().startAnalysis(PlayLayer::get()); refreshStatus(); }
void BotMenu::onPause(CCObject*) { BotController::get().togglePause(); refreshStatus(); }
void BotMenu::onStep(CCObject*) { BotController::get().requestFrameStep(); refreshStatus(); }
void BotMenu::onTrajectory(CCObject*) { BotController::get().toggleTrajectory(); refreshStatus(); }

void BotMenu::refreshStatus() {
    if (m_status) m_status->setString(BotController::get().statusText().c_str());
}

} // namespace fwa
