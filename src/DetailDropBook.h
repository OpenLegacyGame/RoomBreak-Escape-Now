#pragma once
#include "DetailView.h"
#include <array>
class DetailDropBook final : public DetailView
{
public:
    static DetailDropBook* make(const rb::GameData&, int, ax::Vec2, int, int);
    void retry() override;
    void solve() override;
    ax::Vec2 controlCenter(int) const override;
    int count() const { return count_; }

private:
    void setup(const rb::Row&) override;
    void press(ax::Vec2) override;
    void release(ax::Vec2) override;
    void cancel() override;
    ax::Sprite *keys_[5]{}, *pressed_[5]{}, *good_ = nullptr, *bad_ = nullptr;
    std::array<int, 5> counts_{};
    int count_ = 0, pressedKey_ = -1;
    bool solved_ = false;
};
