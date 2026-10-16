#pragma once
#include "DetailView.h"
#include <array>
class DetailSwitch final : public DetailView
{
public:
    static DetailSwitch* make(const rb::GameData&, int, ax::Vec2, int, int);
    ax::Vec2 controlCenter(int) const override;
    void retry() override;
    void solve() override;
    const std::array<bool, 10>& entered() const { return input_; }

private:
    void setup(const rb::Row&) override;
    void press(ax::Vec2) override;
    void release(ax::Vec2) override;
    void cancel() override { selected_ = -1; }
    void toggle(int);
    std::array<ax::Sprite*, 10> on_{}, off_{};
    std::array<bool, 10> input_{};
    static constexpr std::array<bool, 10> password_ = {0, 1, 0, 0, 1, 0, 1, 0, 1, 1};
    int selected_                                   = -1;
};
