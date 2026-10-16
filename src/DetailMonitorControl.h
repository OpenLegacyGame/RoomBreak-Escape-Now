#pragma once
#include "DetailView.h"
#include <array>
class DetailMonitorControl final : public DetailView
{
public:
    static DetailMonitorControl* make(const rb::GameData&, int, ax::Vec2, int, int);
    void solve() override {}
    ax::Vec2 controlCenter(int) const override;
    int selectedPage() const { return page_; }

private:
    void setup(const rb::Row&) override;
    void press(ax::Vec2) override;
    void release(ax::Vec2) override;
    void cancel() override;
    void select(int);
    std::array<ax::Sprite*, 3> views_{}, markers_{};
    std::array<ax::Sprite*, 2> keys_{}, pressed_{};
    int page_ = 0, selected_ = -1;
};
