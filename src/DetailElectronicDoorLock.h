#pragma once
#include "DetailView.h"
#include <array>
class DetailElectronicDoorLock final : public DetailView
{
public:
    static DetailElectronicDoorLock* make(const rb::GameData&, int, ax::Vec2, int, int);
    void retry() override;
    void solve() override;
    ax::Vec2 controlCenter(int index) const override;
    const std::vector<int>& entered() const { return input_; }

private:
    void setup(const rb::Row&) override;
    void press(ax::Vec2) override;
    void release(ax::Vec2) override;
    void cancel() override;
    void input(int);
    void check();
    std::array<ax::Sprite*, 6> keys_{}, pressed_{};
    std::vector<int> input_;
    std::vector<ax::Sprite*> digits_;
    int selected_ = -1;
};
