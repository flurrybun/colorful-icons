#pragma once

#include <Geode/cocos/include/ccTypes.h>
using namespace geode::prelude;

namespace color {
    ccHSVValue rgb2hsv(const ccColor3B&);
    ccColor3B hsv2rgb(const ccHSVValue&);
}
