#include "main.hpp"

// GJItemIcon::create is inlined and hooking GJItemIcon::init crashes even when doing nothing
// not ideal but we can work around it

bool ModSimplePlayer::init(int p0) {
    if (!SimplePlayer::init(p0)) return false;

    retain();

    queueInMainThread([this] {
        // it's possible for this node's ref count to be 0 when navigating away from a menu before it loads
        // if that happens, calling getParent() will crash
        // originally i just checked if the ref count was 0, but that would still crash every now and then

        auto parent = getParent();
        if (parent && typeinfo_cast<GJItemIcon*>(parent)) {
            if (!m_fields->m_isLocked) changeToPlayerColors();
        }

        release();
    });

    return true;
}

void ModSimplePlayer::changeToPlayerColors() {
    if (auto spr = getChildByType<CCSprite>(0)) {
        if (spr->getOpacity() != 255) return;
    }

    auto gm = GameManager::get();

    ccColor3B primary = gm->colorForIdx(gm->getPlayerColor());
    ccColor3B secondary = gm->colorForIdx(gm->getPlayerColor2());
    ccColor3B glow = gm->colorForIdx(gm->getPlayerGlowColor());
    bool hasGlow = gm->getPlayerGlow();

    setColors(primary, secondary);
    if (hasGlow) setGlowOutline(glow);
    else disableGlowOutline();
}

CCSprite* ModSimplePlayer::renderIcon() {
    setOpacity(255);
    m_secondLayer->setVisible(true);
    setColors({41, 41, 41}, {80, 80, 80});

    bool hasDetailSprite = m_fields->m_hasDetailSprite;
    bool isUFO = m_fields->m_hasUFODome;
    bool isRobot = m_robotSprite != nullptr;
    bool isSpider = m_spiderSprite != nullptr;

    m_detailSprite->setVisible(hasDetailSprite);
    m_detailSprite->setColor({110, 110, 110});

    m_birdDome->setVisible(isUFO);
    m_birdDome->setColor({110, 110, 110});

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
    }

    CCSize iconSize = spriteToRender->getContentSize() + CCSize(10, 10);
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

    if (isUFO) renderedSprite->setPositionY(8);

    return renderedSprite;
}

void ModGJItemIcon::changeToLockedState(float p0) {
    // we can't assume that m_player is a SimplePlayer, because locked path icons set m_player to a CCSprite

    if (auto player = typeinfo_cast<SimplePlayer*>(m_player)) {
        auto modPlayer = static_cast<ModSimplePlayer*>(player);

        // if an icon doesn't have a detail sprite or ufo dome, it defaults to the default cube's inner square set to invisible
        // since GJItemIcon::changeToLockedState sets the detail sprite and ufo dome to invisible regardless,
        // we need to store their visibility before calling the original method

        modPlayer->m_fields->m_hasUFODome = m_player->m_birdDome->isVisible();

        if (modPlayer->m_robotSprite) modPlayer->m_fields->m_hasDetailSprite = m_player->m_robotSprite->m_extraSprite->isVisible();
        else if (modPlayer->m_spiderSprite) modPlayer->m_fields->m_hasDetailSprite = m_player->m_spiderSprite->m_extraSprite->isVisible();
        else modPlayer->m_fields->m_hasDetailSprite = m_player->m_detailSprite->isVisible();
    }

    GJItemIcon::changeToLockedState(p0);

    if (Mod::get()->getSettingValue<bool>("hide-locks")) {
        if (auto lock = getChildByType<CCSprite>(1)) lock->setVisible(false);
        
        if (auto player = typeinfo_cast<SimplePlayer*>(m_player)) {
            auto modPlayer = static_cast<ModSimplePlayer*>(player);
            modPlayer->m_fields->m_isLocked = true;

            addChild(modPlayer->renderIcon());
            player->setVisible(false);
        }
    }
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

        auto player = itemIcon->m_player;
        if (!player) continue;

        static_cast<ModSimplePlayer*>(player)->changeToPlayerColors();
    }
}
