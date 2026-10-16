#pragma once
#include "DetailView.h"
#include <array>
class DetailFingerScan final : public DetailView
{
public:
    static DetailFingerScan* make(const rb::GameData&, int, ax::Vec2, int, int);
    ax::Vec2 controlCenter(int) const override;
    void retry() override;
    void solve() override;
    const std::array<bool, 10>& entered() const { return input_; }

private:
    void setup(const rb::Row&) override;
    void press(ax::Vec2) override;
    void release(ax::Vec2) override;
    void cancel() override;
    void toggle(int);
    void check();
    std::array<ax::Sprite*, 10> keys_{}, pressed_{}, selectedImages_{};
    std::array<bool, 10> input_{};
    static constexpr std::array<bool, 10> password_ = {1, 1, 0, 0, 0, 0, 1, 1, 0, 0};
    ax::Sprite *cognition_ = nullptr, *scanning_ = nullptr;
    int selected_ = -1, count_ = 0;
};
