#pragma once
#include "DetailView.h"
class DetailMusic final : public DetailView
{
public:
    static DetailMusic* make(const rb::GameData&, int, ax::Vec2, int, int);
    ax::Vec2 controlCenter(int index) const override;
    float notePosition(int index) const { return notes_.at(index)->getPositionX(); }
    bool noteVisible(int index) const { return notes_.at(index)->getOpacity() != 0; }

private:
    void setup(const rb::Row&) override;
    void press(ax::Vec2) override;
    void move(ax::Vec2) override;
    void release(ax::Vec2) override;
    void cancel() override { dragging_ = false; }
    void updateVisibility();
    std::vector<ax::Sprite*> notes_;
    ax::Vec2 last_;
    bool dragging_ = false;
};
