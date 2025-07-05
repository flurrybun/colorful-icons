#include "main.hpp"
#include "color.hpp"
#include <random>
#include <Geode/Geode.hpp>

// trying to access the last parameter of GJItemIcon::init in any way crashes the game
// genuinely how the fuck does that even happen

bool ModGJItemIcon::init(
    UnlockType unlockType, int itemID, cocos2d::ccColor3B primaryColor, cocos2d::ccColor3B secondaryColor,
    bool b1, bool b2, bool b3, cocos2d::ccColor3B _
) {
    if (!GJItemIcon::init(unlockType, itemID, primaryColor, secondaryColor, b1, b2, b3, {})) return false;

    if (auto player = typeinfo_cast<SimplePlayer*>(m_player)) {
        auto modPlayer = static_cast<ModSimplePlayer*>(player);

        if (!isAprilFools()) modPlayer->changeToPlayerColors();
        else modPlayer->makeRainbow();
    }

    return true;
}

void ModGJItemIcon::changeToLockedState(float p0) {
    if (auto player = typeinfo_cast<SimplePlayer*>(m_player)) {
        auto modPlayer = static_cast<ModSimplePlayer*>(player);

        // if an icon doesn't have a detail sprite or ufo dome, it defaults to the default cube's inner square set to invisible
        // since GJItemIcon::changeToLockedState sets the detail sprite and ufo dome to invisible regardless,
        // we need to store their visibility before calling the original method

        modPlayer->m_fields->m_hasUFODome = player->m_birdDome->isVisible();

        if (modPlayer->m_robotSprite) {
            modPlayer->m_fields->m_hasDetailSprite = player->m_robotSprite->m_extraSprite->isVisible();
        } else if (modPlayer->m_spiderSprite) {
            modPlayer->m_fields->m_hasDetailSprite = player->m_spiderSprite->m_extraSprite->isVisible();
        } else {
            modPlayer->m_fields->m_hasDetailSprite = player->m_detailSprite->isVisible();
        }
    }

    GJItemIcon::changeToLockedState(p0);

    // unlock state is 0 when the icon is unobtainable until 2.21
    bool isUnobtainable = GameStatsManager::get()->getItemUnlockState(m_unlockID, m_unlockType) == 0;

    if (Mod::get()->getSettingValue<bool>("hide-locks")) {
        if (auto lock = getChildByType<CCSprite>(1)) lock->setVisible(false);

        if (auto player = typeinfo_cast<SimplePlayer*>(m_player)) {
            addChild(static_cast<ModSimplePlayer*>(player)->renderIcon(isUnobtainable));
            player->setVisible(false);
        }
    }
}

void ModSimplePlayer::changeToPlayerColors() {
    if (auto spr = getChildByType<CCSprite>(0)) {
        // if an icon is locked, its opacity is set to 120
        if (spr->getOpacity() == 120) return;
    }

    auto gm = GameManager::get();

    if (tryChangeSeparateDualIconsColor()) return;

    ccColor3B primary = gm->colorForIdx(gm->getPlayerColor());
    ccColor3B secondary = gm->colorForIdx(gm->getPlayerColor2());
    ccColor3B glow = gm->colorForIdx(gm->getPlayerGlowColor());
    bool hasGlow = gm->getPlayerGlow();

    setColors(primary, secondary);
    if (hasGlow) setGlowOutline(glow);
    else disableGlowOutline();
}

bool ModSimplePlayer::tryChangeSeparateDualIconsColor() {
    auto scene = CCDirector::sharedDirector()->getRunningScene();
    if (!scene) return false;

    auto garageLayer = scene->getChildByType<GJGarageLayer>(0);
    if (!garageLayer) return false;

    bool isP2Selected = static_cast<ModGJGarageLayer*>(garageLayer)->m_fields->m_isP2Selected;
    if (!isP2Selected) return false;

    auto gm = GameManager::get();
    auto mod = Loader::get()->getLoadedMod("weebify.separate_dual_icons");

    ccColor3B primary = gm->colorForIdx(mod->getSavedValue<int64_t>("color1"));
    ccColor3B secondary = gm->colorForIdx(mod->getSavedValue<int64_t>("color2"));
    ccColor3B glow = gm->colorForIdx(mod->getSavedValue<int64_t>("colorglow"));
    bool hasGlow = mod->getSavedValue<bool>("glow");

    setColors(primary, secondary);
    if (hasGlow) setGlowOutline(glow);
    else disableGlowOutline();

    return true;
}

void ModSimplePlayer::makeRainbow() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distr(0, 360);

    ccColor3B primary = color::hsv2rgb({static_cast<float>(distr(gen)), 1, 1});
    ccColor3B secondary = color::hsv2rgb({static_cast<float>(distr(gen)), 1, 1});
    ccColor3B glow = color::hsv2rgb({static_cast<float>(distr(gen)), 1, 1});
    m_fields->m_rainbowPrimary = primary;
    m_fields->m_rainbowSecondary = secondary;
    m_fields->m_rainbowGlow = glow;

    setColors(primary, secondary);
    setGlowOutline(glow);

    m_firstLayer->runAction(CCRepeatForever::create(CCSequence::create(
        CCCallFunc::create(this, callfunc_selector(ModSimplePlayer::tintPlayerSprites)),
        CCDelayTime::create(0.3f),
        nullptr
    )));

    if (m_robotSprite || m_spiderSprite) schedule(schedule_selector(ModSimplePlayer::updateRobotSprite));
}

void ModSimplePlayer::tintPlayerSprites() {
    tintToRandomColor(ColorType::Primary, 0.3f);
    tintToRandomColor(ColorType::Secondary, 0.3f);
    tintToRandomColor(ColorType::Glow, 0.3f);
}

void ModSimplePlayer::tintToRandomColor(ColorType colorType, float duration) {
    CCSprite* sprite;
    ccColor3B baseColor;

    if (colorType == ColorType::Primary) {
        sprite = m_firstLayer;
        baseColor = m_fields->m_rainbowPrimary;
    } else if (colorType == ColorType::Secondary) {
        sprite = m_secondLayer;
        baseColor = m_fields->m_rainbowSecondary;
    } else {
        sprite = m_outlineSprite;
        baseColor = m_fields->m_rainbowGlow;
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distr(-30, 30);

    ccHSVValue hsv = color::rgb2hsv(baseColor);
    hsv.h += distr(gen);
    hsv.s = 1;
    hsv.v = 1;
    ccColor3B color = color::hsv2rgb(hsv);

    sprite->runAction(CCTintTo::create(duration, color.r, color.g, color.b));

    if (colorType == ColorType::Primary) m_fields->m_rainbowPrimary = color;
    else if (colorType == ColorType::Secondary) m_fields->m_rainbowSecondary = color;
    else m_fields->m_rainbowGlow = color;
}

void ModSimplePlayer::updateRobotSprite(float dt) {
    GJRobotSprite* robotSprite = m_robotSprite ? m_robotSprite : m_spiderSprite;
    ccColor3B primary = m_firstLayer->getColor();
    ccColor3B secondary = m_secondLayer->getColor();
    ccColor3B glow = m_outlineSprite->getColor();

    setColors(primary, secondary);
    setGlowOutline(glow);
}

CCSprite* ModSimplePlayer::renderIcon(bool isUnobtainable) {
    if (!Mod::get()->getSettingValue<bool>("dim-unobtainable")) isUnobtainable = false;

    setOpacity(255);
    m_secondLayer->setVisible(true);
    setColors({41, 41, 41}, {80, 80, 80});
    if (isUnobtainable) setColors({1, 1, 1}, {1, 1, 1});

    bool hasDetailSprite = m_fields->m_hasDetailSprite;
    bool isUFO = m_fields->m_hasUFODome;
    bool isRobot = m_robotSprite != nullptr;
    bool isSpider = m_spiderSprite != nullptr;

    m_detailSprite->setVisible(hasDetailSprite);
    m_detailSprite->setColor({110, 110, 110});
    if (isUnobtainable) m_detailSprite->setColor({1, 1, 1});

    m_birdDome->setVisible(isUFO);
    m_birdDome->setColor({110, 110, 110});
    if (isUnobtainable) m_birdDome->setColor({1, 1, 1});

    CCSprite* spriteToRender = m_firstLayer;
    GJRobotSprite* robotSpiderSprite = nullptr;
    if (isRobot) {
        spriteToRender = m_robotSprite->getChildByType<CCPartAnimSprite>(0);
        robotSpiderSprite = m_robotSprite;
    } else if (isSpider) {
        spriteToRender = m_spiderSprite->getChildByType<CCPartAnimSprite>(0);
        robotSpiderSprite = m_spiderSprite;
    }

    if (isRobot || isSpider) {
        for (auto& child : static_cast<CCArrayExt<CCSprite*>>(robotSpiderSprite->m_secondArray)) {
            child->setVisible(true);
        }

        robotSpiderSprite->m_extraSprite->setVisible(m_fields->m_hasDetailSprite);
        robotSpiderSprite->m_extraSprite->setColor({110, 110, 110});
        if (isUnobtainable) robotSpiderSprite->m_extraSprite->setColor({13, 13, 13});
    }

    CCSize iconSize = CCSize(60, 40);
    if (isUFO) iconSize = CCSize(iconSize.width, iconSize.height + 40);
    if (isRobot || isSpider) iconSize = CCSize(60, 55);

    setPosition(ccp(15, 15) - iconSize / 2);
    spriteToRender->setPosition(iconSize / 2);
    setAnchorPoint({0, 0});
    setContentSize(iconSize);

    auto renderTexture = CCRenderTexture::create(iconSize.width, iconSize.height);
    renderTexture->begin();

    if (isRobot || isSpider) {
        for (auto& child : static_cast<CCArrayExt<CCNode*>>(robotSpiderSprite->getChildren())) {
            child->visit();
        }
    }
    else spriteToRender->visit();

    renderTexture->end();

    auto renderedSprite = CCSprite::createWithTexture(renderTexture->getSprite()->getTexture());
    renderedSprite->setFlipY(true);
    renderedSprite->setPosition({15, 15});
    renderedSprite->setOpacity(120);
    if (isUnobtainable) renderedSprite->setOpacity(30);

    if (isUFO) renderedSprite->setPositionY(8);

    return renderedSprite;
}

void ModGJGarageLayer::playerColorChanged() {
    GJGarageLayer::playerColorChanged();

    // most of these null checks are almost certainly unnecessary, but better safe than sorry

    auto page = typeinfo_cast<ListButtonPage*>(m_iconSelection->m_pages->firstObject());
    if (!page) return;

    auto pageMenu = page->getChildByType<CCMenu>(0);
    if (!pageMenu) return;

    CCArrayExt<CCMenuItemSpriteExtra*> iconBtns = pageMenu->getChildren();

    for (auto& iconBtn : iconBtns) {
        auto itemIcon = iconBtn->getChildByType<GJItemIcon>(0);
        if (!itemIcon) continue;

        // m_player may be a CCSprite*, e.g. trail icons
        auto player = itemIcon->m_player;
        if (!player || !typeinfo_cast<SimplePlayer*>(itemIcon->m_player)) continue;

        static_cast<ModSimplePlayer*>(player)->changeToPlayerColors();
    }
}

bool ModGJGarageLayer::init() {
    if (!GJGarageLayer::init()) return false;

    if (Loader::get()->isModLoaded("weebify.separate_dual_icons")) {
        schedule(schedule_selector(ModGJGarageLayer::updateSeparateDualIcons));
    }

    return true;
}

void ModGJGarageLayer::updateSeparateDualIcons(float dt) {
    auto mod = Loader::get()->getLoadedMod("weebify.separate_dual_icons");
    auto isP2Selected = mod->getSavedValue<bool>("2pselected");

    // only continue if the value has changed
    if (m_fields->m_isP2Selected == isP2Selected) return;
    m_fields->m_isP2Selected = isP2Selected;

    playerColorChanged();
}

bool isAprilFools() {
    time_t now = time(nullptr);
    tm* time = localtime(&now);

    return time->tm_mon == 3 && time->tm_mday == 1;
}
