#pragma once
#include "DetailView.h"
#include <array>
class DetailCircuit final : public DetailView
{
public:
    static DetailCircuit* make(const rb::GameData&, int, ax::Vec2, int, int);
    void open() override;
    void retry() override;
    void solve() override;
    ax::Vec2 controlCenter(int) const override;
    int currentPoint() const { return current_; }
    const std::vector<std::pair<int, int>>& ways() const { return ways_; }

private:
    void setup(const rb::Row&) override;
    void press(ax::Vec2) override;
    void release(ax::Vec2) override;
    void cancel() override { pressed_ = -1; }
    void lightPoint(int);
    void lightLine(int);
    void checkWays();
    void failure();
    void start();
    void finishReset();
    ax::Sprite* overlay(int, int, int);
    static const std::array<int, 105> grid_;
    std::array<ax::Sprite*, 105> cells_{};
    std::array<bool, 105> visited_{};
    std::vector<ax::Sprite*> points_, lines_, wrong_;
    std::vector<std::pair<int, int>> ways_;
    int current_ = 30, pressed_ = -1;
};
