#pragma once
#include "DetailView.h"
#include <chrono>
class DetailTimeControl final : public DetailView
{
public:
    static DetailTimeControl* make(const rb::GameData&, int, ax::Vec2, int, int);
    void retry() override;
    void solve() override;
    ax::Vec2 controlCenter(int) const override;
    int targetIndex() const { return target_; }

private:
    void setup(const rb::Row&) override;
    void press(ax::Vec2) override;
    void release(ax::Vec2) override;
    void cancel() override;
    void showTarget(float y);
    ax::Sprite *finger_ = nullptr, *fingerP_ = nullptr, *failure_ = nullptr;
    ax::Label *targetLabel_ = nullptr, *resultLabel_ = nullptr;
    std::chrono::system_clock::time_point start_;
    bool pressed_ = false;
    int target_   = 0;
};
