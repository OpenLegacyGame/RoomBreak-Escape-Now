#pragma once
#include "axmol.h"
#include <functional>
class GameOverOverlay : public ax::Node
{
public:
    static GameOverOverlay* create();
    void open();
    void choose(bool exitToMenu);
    bool ready() const { return ready_; }
    std::function<void(bool)> complete;
    std::function<void(std::string_view)> sound;

private:
    ax::LayerColor* shade_ = nullptr;
    ax::Sprite *bg_        = nullptr, *buttons_[2]{}, *pressedImages_[2]{};
    bool ready_ = false, opened_ = false;
    int pressed_ = -1;
    int hit(ax::Vec2) const;
};
