#pragma once
#include "DetailView.h"
#include <array>
class DetailWomanStatue final : public DetailView
{
public:
    static DetailWomanStatue* make(const rb::GameData&, int, ax::Vec2, int, int);
    void retry() override;
    void solve() override;
    ax::Vec2 controlCenter(int) const override;
    const std::array<int, 4>& entered() const { return input_; }
    float digitWidth() const { return numbers_[0][0]->getContentSize().width; }

private:
    void setup(const rb::Row&) override;
    void press(ax::Vec2) override;
    void move(ax::Vec2) override;
    void release(ax::Vec2) override;
    void cancel() override;
    void snap(int, int);
    void updateNumbers(float);
    ax::Sprite *numbers_[4][10]{}, *handles_[4]{};
    std::array<int, 4> input_{};
    ax::Vec2 last_;
    float travel_  = 0;
    int selected_  = 0;
    bool dragging_ = false, solved_ = false;
};
