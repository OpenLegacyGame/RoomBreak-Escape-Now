#pragma once
#include "DetailView.h"
#include <array>
class DetailDustingFrame final : public DetailView
{
public:
    static DetailDustingFrame* make(const rb::GameData&, int, ax::Vec2, int, int);
    void retry() override;
    void solve() override;
    ax::Vec2 controlCenter(int) const override;
    int remaining() const { return static_cast<int>(std::count(hitPoints_.begin(), hitPoints_.end(), 1)); }

private:
    void setup(const rb::Row&) override;
    void press(ax::Vec2) override;
    void move(ax::Vec2) override;
    void release(ax::Vec2) override { cancel(); }
    void cancel() override
    {
        touching_ = false;
        distance_ = 0;
    }
    void consume(int);
    void particle(ax::Vec2);
    std::array<ax::Sprite*, 6> dust_{};
    std::array<int, 6> hitPoints_ = {1, 1, 1, 1, 1, 1};
    std::vector<ax::ParticleSystemQuad*> particles_;
    ax::Vec2 last_;
    float distance_ = 0;
    int selected_   = 0;
    bool touching_ = false, solved_ = false;
};
