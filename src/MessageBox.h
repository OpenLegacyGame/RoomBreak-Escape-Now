#pragma once
#include "axmol.h"
#include <functional>
// Original GameMessageBox (0x78194–0x79364), in logical iPhone coordinates.
class GameMessageBox : public ax::Node
{
public:
    static GameMessageBox* create(std::string_view text, int language, bool choice, std::function<void(bool)> complete);
    bool ready() const { return ready_; }
    void answer(bool yes);
    bool beginTouch(ax::Vec2 point);
    void endTouch(ax::Vec2 point);

private:
    void setup(std::string_view text, int language);
    int hit(ax::Vec2 point) const;
    ax::Sprite* bg_ = nullptr;
    ax::Sprite* buttons_[3]{};
    ax::Sprite* pressedImages_[3]{};
    bool choice_ = false, ready_ = false;
    int pressed_ = -1;
    std::function<void(bool)> complete_;
};
