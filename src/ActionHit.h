#pragma once
#include "ActionMode.h"
#include <array>
#include <chrono>
namespace rb
{
class GameData;
}
class ActionHit final : public ActionMode
{
public:
    static ActionHit* create(int level, int hands);
    void solve() override;
    ax::Vec2 targetCenter(int number) const { return targets_.at(number - 1)->getPosition(); }
    int circleCount() const { return static_cast<int>(circles_.size()); }
    int playableTarget() const;
    int firstTarget() const { return circles_.empty() ? 0 : circles_.front().number; }
    int sequenceSize() const { return static_cast<int>(sequence_.size()); }
    std::function<void()> shake;

private:
    friend bool runVisualProbe(ax::Scene*, const rb::GameData&);
    struct Circle
    {
        int number;
        float radius, valid, max;
        ax::DrawNode* drawing;
        std::chrono::steady_clock::time_point born;
    };
    void setup(int) override;
    void start() override;
    void hideContent() override;
    void press(ax::Vec2) override;
    void tick();
    void spawnCircle();
    void hit(int);
    void miss(int);
    void clearCircles();
    std::array<ax::Sprite*, 9> targets_{}, backs_{};
    ax::Sprite *successOuter_ = nullptr, *successCenter_ = nullptr, *failedOuter_ = nullptr, *failedCenter_ = nullptr,
               *item_ = nullptr, *itemAction_ = nullptr;
    ax::ParticleSun* particles_ = nullptr;
    std::vector<int> sequence_;
    std::vector<Circle> circles_;
    float speed_ = 1;
    int step_ = 0, pressedNumber_ = 0;
    bool initialized_ = false, playing_ = false, failed_ = false;
};
