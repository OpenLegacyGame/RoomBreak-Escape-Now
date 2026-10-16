#pragma once
#include "DetailView.h"
class DetailPad final : public DetailView
{
public:
    static DetailPad* make(const rb::GameData&, int id, ax::Vec2 origin, int language, int hands);
    void retry() override;
    void solve() override;
    ax::Vec2 controlCenter(int index) const override;
    std::string entered() const { return input_; }

private:
    void setup(const rb::Row&) override;
    void press(ax::Vec2) override;
    void release(ax::Vec2) override;
    void cancel() override;
    void input(int digit, bool check = true);
    void wrongPassword();
    void clear();
    std::string prefix_, password_, input_;
    ax::Vec2 inputOrigin_;
    ax::Sprite *words_ = nullptr, *retryWords_ = nullptr, *keys_[10]{}, *pressed_[10]{};
    std::vector<ax::Sprite*> digits_;
    int pressedKey_ = -1;
    bool solved_    = false;
};
