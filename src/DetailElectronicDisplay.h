#pragma once
#include "DetailView.h"
class DetailElectronicDisplay final : public DetailView
{
public:
    static DetailElectronicDisplay* make(const rb::GameData&, int, ax::Vec2, int, int);
    void close() override;
    int selectedPage() const { return selected_; }
    float pagePosition(int index) const { return pages_.at(index)->getPositionX(); }
    bool displayBusy() const { return displayBusy_; }
    ax::Vec2 controlCenter(int index) const override { return pages_.at(index)->getPosition(); }

private:
    void setup(const rb::Row&) override;
    void press(ax::Vec2) override;
    void move(ax::Vec2) override;
    void release(ax::Vec2) override;
    void cancel() override { dragging_ = false; }
    void sortPages();
    std::vector<ax::Sprite*> pages_;
    int selected_     = 0;
    bool displayBusy_ = true, dragging_ = false, hiding_ = false;
    ax::Vec2 last_;
};
