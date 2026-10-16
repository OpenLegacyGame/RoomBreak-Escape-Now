#pragma once
#include "DetailView.h"
#include <array>
class DetailPyramidBlock final : public DetailView
{
public:
    static DetailPyramidBlock* make(const rb::GameData&, int, ax::Vec2, int, int);
    void retry() override;
    void solve() override;
    ax::Vec2 controlCenter(int) const override;
    const auto& entered() const { return input_; }

private:
    void setup(const rb::Row&) override;
    void press(ax::Vec2) override;
    void release(ax::Vec2) override;
    void cancel() override;
    void toggle(int);
    std::array<ax::Sprite*, 18> in_{}, out_{};
    std::array<bool, 18> pressed_{}, input_{};
    const std::array<bool, 18> password_{{true, false, false, false, false, true, false, false, true, false, false,
                                          false, false, false, false, true, false, false}};
    bool solved_ = false;
};
