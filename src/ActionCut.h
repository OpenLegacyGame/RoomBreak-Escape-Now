#pragma once
#include "ActionMode.h"
#include <array>
#include <chrono>
class ActionCut final : public ActionMode
{
public:
    static ActionCut* create(int level, int hands);
    int step() const { return step_; }
    int stepCount() const { return static_cast<int>(directions_.size()); }
    int direction() const { return directions_.at(step_); }
    void solve() override;

private:
    void setup(int) override;
    void start() override;
    void hideContent() override;
    void press(ax::Vec2) override;
    void move(ax::Vec2) override;
    void release(ax::Vec2) override;
    void cancelGesture() override;
    void reveal();
    void startBanner();
    void cutSuccess();
    std::vector<int> directions_;
    std::array<ax::Sprite*, 8> arrows_{};
    std::vector<ax::Sprite*> boxes_, copies_;
    ax::Sprite *startTop_ = nullptr, *startBottom_ = nullptr, *successText_ = nullptr;
    ax::Node* trails_ = nullptr;
    ax::Vec2 checkpoint_, last_;
    std::chrono::steady_clock::time_point started_;
    float total_ = 0;
    int step_ = 0, revealStep_ = 0;
    bool playing_ = false, slicing_ = false, right_ = false, wrong_ = false;
};
