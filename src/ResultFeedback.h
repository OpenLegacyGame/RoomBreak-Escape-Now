#pragma once
#include "axmol.h"
#include <functional>
class ResultFeedback : public ax::Node
{
public:
    static ResultFeedback* create(bool success);
    void play();
    std::function<void()> complete;
    std::function<void(std::string_view)> sound;

private:
    bool success_ = false, played_ = false;
    ax::LayerColor* bar_ = nullptr;
    ax::Sprite* title_   = nullptr;
    std::vector<ax::Sprite*> slices_;
    void finish();
};
