#pragma once
#include "DetailView.h"
class DetailScreenSafe final : public DetailView
{
public:
    static DetailScreenSafe* make(const rb::GameData&, int, ax::Vec2, int, int);
    void retry() override;
    void solve() override;
    ax::Vec2 controlCenter(int index) const override;
    const std::string& entered() const { return input_; }
    float handleRotation() const { return handle_->getRotation(); }

private:
    void setup(const rb::Row&) override;
    void press(ax::Vec2) override;
    void move(ax::Vec2) override;
    void release(ax::Vec2) override;
    void cancel() override;
    void input(int digit);
    void erase();
    void sortDigits(const std::vector<ax::Sprite*>&);
    void check();
    void wrongPassword();
    void resetHandle();
    void openHandle();
    std::string prefix_, password_, input_;
    std::vector<ax::Sprite*> digits_, failedDigits_;
    ax::Sprite *keys_[11]{}, *pressed_[11]{}, *handle_ = nullptr;
    ax::Vec2 last_;
    int pressedKey_ = -1, safeType_ = 0;
    bool dragging_ = false, resultRunning_ = false, solved_ = false;
};
