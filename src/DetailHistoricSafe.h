#pragma once
#include "DetailView.h"
#include <array>
class DetailHistoricSafe final : public DetailView
{
public:
    static DetailHistoricSafe* make(const rb::GameData&, int, ax::Vec2, int, int);
    void retry() override;
    void solve() override;
    ax::Vec2 controlCenter(int) const override;
    const auto& entered() const { return input_; }

private:
    void setup(const rb::Row&) override;
    void press(ax::Vec2) override;
    void move(ax::Vec2) override;
    void release(ax::Vec2) override;
    void cancel() override;
    void updateNumbers();
    void snap();
    void input(int);
    std::array<ax::Sprite*, 10> numbers_{};
    std::vector<ax::Sprite*> displays_;
    std::vector<int> input_;
    ax::Rect touchSpace_;
    ax::Vec2 last_;
    float moveLength_  = 0;
    int pressedNumber_ = -1;
    bool spacePressed_ = false, sliding_ = false, solved_ = false;
};
