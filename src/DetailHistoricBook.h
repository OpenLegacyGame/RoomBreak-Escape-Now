#pragma once
#include "DetailView.h"
#include <array>
class DetailHistoricBook final : public DetailView
{
public:
    static DetailHistoricBook* make(const rb::GameData&, int, ax::Vec2, int, int);
    void solve() override {}
    ax::Vec2 controlCenter(int) const override;
    int page() const { return page_; }

private:
    void setup(const rb::Row&) override;
    void press(ax::Vec2) override;
    void move(ax::Vec2) override;
    void release(ax::Vec2) override;
    void cancel() override;
    void sort();
    std::array<ax::Sprite*, 4> pages_{}, dots_{};
    ax::Sprite* selected_ = nullptr;
    ax::Vec2 last_;
    int page_     = 0;
    bool pressed_ = false;
};
