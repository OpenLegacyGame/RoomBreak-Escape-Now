#pragma once
#include "DetailView.h"
#include <array>
class DetailCenterSafe final : public DetailView
{
public:
    static DetailCenterSafe* make(const rb::GameData&, int, ax::Vec2, int, int);
    void retry() override;
    void solve() override;
    ax::Vec2 controlCenter(int) const override;
    const std::array<int, 7>& entered() const { return input_; }

private:
    void setup(const rb::Row&) override;
    void press(ax::Vec2) override;
    void move(ax::Vec2) override;
    void release(ax::Vec2) override;
    bool releaseWhileBusy() const override { return true; }
    void cancel() override;
    void animateBlock(int, int, float, bool = true);
    void resetBlocks();
    ax::Sprite *blocks_[7][3]{}, *pressed_[7][3]{};
    std::array<int, 7> input_{};
    ax::Vec2 last_;
    int column_ = 0, row_ = 0;
    float moved_ = 0, checkLength_ = 0;
    bool dragging_ = false, solved_ = false;
};
