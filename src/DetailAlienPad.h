#pragma once
#include "DetailView.h"
#include <array>
class DetailAlienPad final : public DetailView
{
public:
    static DetailAlienPad* make(const rb::GameData&, int, ax::Vec2, int, int);
    void retry() override;
    void solve() override;
    ax::Vec2 controlCenter(int) const override;
    const auto& entered() const { return input_; }
    int selected() const { return selected_; }

private:
    void setup(const rb::Row&) override;
    void press(ax::Vec2) override;
    void move(ax::Vec2) override;
    void release(ax::Vec2) override;
    void cancel() override;
    void updateLetters();
    void snap();
    void input(int);
    void clearInput();
    std::string prefix_, password_;
    std::array<ax::Sprite*, 26> letters_{};
    std::vector<ax::Sprite*> cells_, displays_;
    std::vector<int> input_;
    ax::Sprite *selection_ = nullptr, *successEyes_ = nullptr, *failedEyes_ = nullptr;
    ax::Vec2 last_;
    float travel_ = 0;
    int selected_ = 0;
    bool pressed_ = false, sliding_ = false, solved_ = false;
};
