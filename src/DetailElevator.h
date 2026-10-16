#pragma once
#include "DetailView.h"
class DetailElevator final : public DetailView
{
public:
    static DetailElevator* make(const rb::GameData&, int, ax::Vec2, int, int);
    void retry() override;
    void solve() override;
    ax::Vec2 controlCenter(int) const override;
    const std::vector<int>& entered() const { return input_; }

private:
    void setup(const rb::Row&) override;
    void press(ax::Vec2) override;
    void release(ax::Vec2) override;
    void cancel() override;
    void enter(int);
    ax::Sprite *keys_[10]{}, *pressed_[10]{};
    std::vector<ax::Sprite*> digits_;
    std::vector<int> input_;
    int selected_ = -1;
    bool solved_  = false;
};
