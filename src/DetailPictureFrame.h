#pragma once
#include "DetailView.h"
#include <array>
class DetailPictureFrame final : public DetailView
{
public:
    static DetailPictureFrame* make(const rb::GameData&, int, ax::Vec2, int, int);
    void close() override;
    void solve() override {}
    int eyeDirection() const { return direction_; }

private:
    void setup(const rb::Row&) override;
    void animateEyes();
    std::array<ax::Sprite*, 5> eyes_{};
    int step_ = 0, direction_ = -1;
};
