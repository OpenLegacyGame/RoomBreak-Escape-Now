#pragma once
#include "axmol.h"
#include <functional>
class PauseButton : public ax::Node
{
public:
    static PauseButton* create();
    void show(bool visible);
    bool beginTouch(ax::Vec2);
    void endTouch(ax::Vec2);
    std::function<bool()> blocked;
    std::function<void()> activate;

private:
    ax::Sprite *button_ = nullptr, *pressedImage_ = nullptr;
    bool shown_ = false, pressed_ = false;
};
