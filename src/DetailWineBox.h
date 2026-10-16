#pragma once
#include "DetailView.h"
#include <array>
class DetailWineBox final : public DetailView
{
public:
    static DetailWineBox* make(const rb::GameData&, int, ax::Vec2, int, int);
    void retry() override;
    void solve() override;
    ax::Vec2 controlCenter(int) const override;
    const std::array<int, 14>& entered() const { return input_; }

private:
    void setup(const rb::Row&) override;
    void press(ax::Vec2) override;
    void release(ax::Vec2) override;
    void cancel() override;
    void convert(int, int);
    std::array<ax::Sprite*, 14> buttons_{}, red_{}, green_{};
    ax::Sprite *main_ = nullptr, *down_ = nullptr;
    std::array<int, 14> input_{};
    static constexpr std::array<int, 14> password_ = {2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 2};
    int pressedButton_                             = -1;
    bool solved_                                   = false;
};
