#pragma once
#include "DetailView.h"
class DetailDoorLock final : public DetailView
{
public:
    static DetailDoorLock* make(const rb::GameData&, int, ax::Vec2, int, int);
    void retry() override;
    void solve() override;
    ax::Vec2 controlCenter(int index) const override;
    const std::string& entered() const { return input_; }

private:
    void setup(const rb::Row&) override;
    void press(ax::Vec2) override;
    void release(ax::Vec2) override;
    void cancel() override;
    void input(int);
    void erase();
    void sort();
    void check();
    ax::Sprite *keys_[11]{}, *pressed_[11]{}, *light_ = nullptr;
    std::vector<ax::Sprite*> digits_;
    std::string input_;
    int pressedKey_ = -1;
    bool solved_    = false;
};
