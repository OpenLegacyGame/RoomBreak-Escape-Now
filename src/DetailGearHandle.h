#pragma once
#include "DetailView.h"
#include <array>
class DetailGearHandle final : public DetailView
{
public:
    static DetailGearHandle* make(const rb::GameData&, int, ax::Vec2, int, int);
    ax::Vec2 controlCenter(int) const override;
    void retry() override;
    void solve() override;
    float angle(int i) const { return gears_.at(i)->getRotation(); }
    bool moving() const { return moving_; }
    int count() const { return count_; }

private:
    void setup(const rb::Row&) override;
    void press(ax::Vec2) override;
    void move(ax::Vec2) override;
    void release(ax::Vec2) override { cancel(); }
    void cancel() override;
    void turn(int, bool);
    void updateStrips();
    std::array<ax::Sprite*, 4> gears_{}, handles_{}, pressed_{};
    std::array<std::array<ax::Sprite*, 4>, 4> strips_{};
    std::array<float, 4> initial_{};
    ax::Vec2 previous_;
    float length_ = 0, handleX_ = 0;
    int count_ = 0, selected_ = -1;
    bool moving_ = false;
};
