#pragma once
#include "DetailView.h"
class DetailFurnitureLeft final : public DetailView
{
public:
    static DetailFurnitureLeft* make(const rb::GameData&, int, ax::Vec2, int, int);
    void retry() override;
    void solve() override;
    ax::Vec2 controlCenter(int) const override;
    const std::string& entered() const { return input_; }
    bool firstActive() const { return first_; }

private:
    void setup(const rb::Row&) override;
    void press(ax::Vec2) override;
    void release(ax::Vec2) override;
    void cancel() override;
    void active(bool);
    void input(int, bool = true);
    void clear();
    ax::Sprite *keys_[10]{}, *pressed_[10]{}, *inactive_[10]{};
    std::vector<ax::Sprite*> digits_;
    std::string input_;
    bool first_ = true, solved_ = false;
    int pressedKey_ = -1;
};
