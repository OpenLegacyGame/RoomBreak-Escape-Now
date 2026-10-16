#pragma once
#include "axmol.h"
#include <array>
class HeroHealth : public ax::Node
{
public:
    static HeroHealth* create(int maximum, int health);
    void show(bool visible);
    void updateHealth(int health);
    void fill();
    int opacity(int index) const { return bars_.at(index)->getOpacity(); }

private:
    std::array<ax::Sprite*, 5> bars_{};
    int maximum_ = 100;
};
