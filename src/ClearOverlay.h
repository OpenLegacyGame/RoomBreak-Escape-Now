#pragma once
#include "axmol.h"
#include <functional>
class ClearOverlay : public ax::Node
{
public:
    static ClearOverlay* create(std::string rank);
    void open();
    const std::string& rank() const { return rank_; }
    void choose(bool exitToMenu);
    bool ready() const { return ready_; }
    std::function<void(bool)> complete;
    std::function<void(std::string_view)> sound;

private:
    std::string rank_;
    ax::Sprite* rankImage_ = nullptr;
    ax::LayerColor* shade_ = nullptr;
    ax::Sprite *bg_        = nullptr, *buttons_[2]{}, *pressedImages_[2]{};
    bool ready_ = false, opened_ = false;
    int pressed_ = -1;
    int hit(ax::Vec2) const;
};
