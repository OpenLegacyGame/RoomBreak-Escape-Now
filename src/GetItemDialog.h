#pragma once
#include "axmol.h"
#include "GameData.h"
#include <functional>
#include <vector>
class GetItemDialog : public ax::Node
{
public:
    static GetItemDialog* create(const rb::GameData&, int item, int language);
    void open();
    void close();
    bool ready() const { return ready_; }
    std::function<void()> complete;
    std::function<void(std::string_view)> sound;

private:
    ax::Sprite* light_ = nullptr;
    std::vector<ax::Node*> fading_;
    bool ready_ = false, opened_ = false;
};
