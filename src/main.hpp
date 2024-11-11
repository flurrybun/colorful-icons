#pragma once

#include <Geode/modify/SimplePlayer.hpp>
#include <Geode/modify/GJItemIcon.hpp>
#include <Geode/modify/GJGarageLayer.hpp>
#include <Geode/Geode.hpp>
using namespace geode::prelude;

class $modify(ModSimplePlayer, SimplePlayer) {
    struct Fields {
        bool m_isLocked = false;
        bool m_hasDetailSprite;
        bool m_hasUFODome;

        ccColor3B m_rainbowPrimary;
        ccColor3B m_rainbowSecondary;
        ccColor3B m_rainbowGlow;
    };

    enum ColorType {
        Primary, Secondary, Glow
    };
    
    $override
    bool init(int);

    void changeToPlayerColors();
    void makeRainbow();
    void tintPlayerSprites();
    void tintToRandomColor(ColorType, float);
    void updateRobotSprite(float);
    CCSprite* renderIcon(bool);
};

class $modify(ModGJItemIcon, GJItemIcon) {
    void changeToLockedState(float);
};

class $modify(ModGJGarageLayer, GJGarageLayer) {
    $override
    virtual void playerColorChanged();
};

bool isAprilFools();
