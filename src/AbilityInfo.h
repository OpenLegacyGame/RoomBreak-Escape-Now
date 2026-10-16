#pragma once
#include "axmol.h"
#include "Progress.h"
#include <functional>
class AbilityInfo : public ax::Node
{
public:
    static AbilityInfo* create(const rb::Progress& progress);
    void show(bool visible);
    void refresh();
    bool beginTouch(ax::Vec2);
    void endTouch(ax::Vec2);
    std::function<void(int)> request;
    std::function<bool()> blocked;

private:
    void setup();
    const rb::Progress* progress_ = nullptr;
    ax::Sprite *buttons_[2]{}, *pressedImages_[2]{};
    ax::Label* labels_[2]{};
    int pressed_ = -1;
    bool shown_  = false;
};
