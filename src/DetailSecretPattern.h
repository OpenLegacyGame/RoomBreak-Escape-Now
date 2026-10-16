#pragma once
#include "DetailView.h"
#include <array>
class DetailSecretPattern final : public DetailView
{
public:
    static DetailSecretPattern* make(const rb::GameData&, int, ax::Vec2, int, int);
    void retry() override;
    void solve() override;
    ax::Vec2 controlCenter(int) const override;
    const std::array<bool, 10>& entered() const { return input_; }

private:
    void setup(const rb::Row&) override;
    void press(ax::Vec2) override;
    void release(ax::Vec2) override;
    void cancel() override;
    void toggle(int);
    std::array<ax::Sprite*, 9> buttons_{}, selected_{}, down_{};
    // Original init contains ten password entries, with the last always false.
    std::array<bool, 10> input_{};
    static constexpr std::array<bool, 10> password_ = {1, 0, 1, 0, 1, 0, 0, 0, 1, 0};
    int pressedButton_                              = -1;
    bool solved_                                    = false;
};
