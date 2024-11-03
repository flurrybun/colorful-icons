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
    };
    
    $override
    bool init(int);

    CCSprite* renderIcon();
    void changeToPlayerColors();
};

class $modify(ModGJItemIcon, GJItemIcon) {
    void changeToLockedState(float);
};

class $modify(ModGJGarageLayer, GJGarageLayer) {
    $override
    virtual void playerColorChanged();
};
