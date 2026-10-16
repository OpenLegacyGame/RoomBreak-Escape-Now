#pragma once
#include "DetailView.h"
class DetailTelevision final : public DetailView
{
public:
    static DetailTelevision* make(const rb::GameData&, int, ax::Vec2, int, int);
    void close() override;
    void solve() override {}

private:
    void setup(const rb::Row&) override;
};
