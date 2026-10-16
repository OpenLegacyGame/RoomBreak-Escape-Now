#pragma once
#include "DetailView.h"
#include <array>
class DetailWatchBox final : public DetailView
{
public:
    static DetailWatchBox* make(const rb::GameData&, int, ax::Vec2, int, int);
    void retry() override;
    void solve() override;
    ax::Vec2 controlCenter(int) const override;
    const std::array<int, 4>& entered() const { return input_; }

private:
    void setup(const rb::Row&) override;
    void press(ax::Vec2) override;
    void release(ax::Vec2) override;
    void cancel() override;
    void rotate(int, int);
    void check();
    void successAction();
    std::array<ax::Sprite*, 4> needles_{}, buttons_{}, pressedImages_{};
    std::array<std::array<ax::Sprite*, 4>, 4> letters_{};
    std::array<int, 4> input_{};
    static constexpr std::array<int, 4> password_ = {1, 2, 3, 0};
    int pressedButton_                            = -1;
    bool assistance_ = false, successStarted_ = false;
};
