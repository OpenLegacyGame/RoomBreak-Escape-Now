#pragma once
#include "DetailView.h"
#include <array>
class DetailFittedPieces final : public DetailView
{
public:
    static DetailFittedPieces* make(const rb::GameData&, int, ax::Vec2, int, int);
    void retry() override;
    void solve() override;
    ax::Vec2 controlCenter(int index) const override;
    ax::Vec2 targetCenter(int index) const;
    bool fitted(int index) const { return fitted_.at(index); }

private:
    void setup(const rb::Row&) override;
    void press(ax::Vec2) override;
    void move(ax::Vec2) override;
    void release(ax::Vec2) override;
    void cancel() override { selected_ = -1; }
    void animate(int, bool);
    std::array<ax::Sprite*, 9> pieces_{};
    std::array<ax::Vec2, 9> initial_{}, targets_{};
    std::array<bool, 9> fitted_{};
    ax::Vec2 last_;
    int selected_ = -1;
};
