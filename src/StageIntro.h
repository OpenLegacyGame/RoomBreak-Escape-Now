#pragma once
#include "axmol.h"
#include <functional>
#include <array>
class StageIntro : public ax::Node
{
public:
    static StageIntro* create(int stage);
    void start();
    std::function<void()> complete;
    std::function<void(std::string_view)> sound;

private:
    void letter();
    void number();
    void close();
    ax::LayerColor *shade_ = nullptr, *bars_[2]{};
    std::array<ax::Label*, 4> letters_{};
    ax::Label* number_ = nullptr;
    int step_          = 0;
    bool started_      = false;
};
