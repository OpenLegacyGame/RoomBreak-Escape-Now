#pragma once
#include "DetailView.h"
class DetailSafe final : public DetailView
{
public:
    static DetailSafe* make(const rb::GameData&, int, ax::Vec2, int, int);
    void retry() override;
    void solve() override;
    ax::Vec2 controlCenter(int column) const override;
    const std::vector<int>& entered() const { return input_; }

private:
    friend bool runVisualProbe(ax::Scene*, const rb::GameData&);
    void setup(const rb::Row&) override;
    void press(ax::Vec2) override;
    void move(ax::Vec2) override;
    void release(ax::Vec2) override;
    void cancel() override { dragging_ = false; }
    void updateWheels(float);
    void snap(int column, int digit);
    void checkPassword();
    std::vector<int> password_, input_;
    std::vector<std::vector<ax::Sprite*>> wheels_;
    std::vector<float> soundDistance_;
    ax::Vec2 last_;
    int selected_ = 0, max_ = 10;
    bool dragging_ = false, solved_ = false;
};
