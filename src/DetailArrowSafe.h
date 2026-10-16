#pragma once
#include "DetailView.h"
#include <array>
class DetailArrowSafe final : public DetailView
{
public:
    static DetailArrowSafe* make(const rb::GameData&, int, ax::Vec2, int, int);
    void retry() override;
    ax::Vec2 controlCenter(int index) const override;
    const std::vector<int>& entered() const { return input_; }

private:
    void setup(const rb::Row&) override;
    void press(ax::Vec2) override;
    void release(ax::Vec2) override;
    void cancel() override;
    void check();
    std::array<ax::Sprite*, 4> keys_{}, pressed_{};
    ax::Sprite *correct_ = nullptr, *wrong_ = nullptr, *light_ = nullptr;
    std::vector<int> input_;
    int selected_ = -1;
};
