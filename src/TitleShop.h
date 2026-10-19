#pragma once
#include "axmol.h"
#include "Progress.h"
class GameMessageBox;
class TitleShop : public ax::ClippingRectangleNode
{
public:
    static TitleShop* create(const rb::Progress&, int language);
    void open();
    void close();
    bool beginTouch(ax::Vec2);
    void endTouch(ax::Vec2);
    ax::Vec2 controlCenter(int index) const;
    GameMessageBox* messageBox() const { return message_; }
    int selectedIndex() const { return selected_; }
    std::string description(int line) const { return std::string(descriptions_[line]->getString()); }
    std::function<void()> closed, unlockAll;
    std::function<void(int)> purchase;
    std::function<void(std::string_view)> sound;

private:
    void setup(const rb::Progress&);
    void select(int);
    void refreshCounts();
    int hit(ax::Vec2) const;
    int language_ = 0, pressed_ = -1, selected_ = -1;
    bool closing_           = false;
    ax::Sprite *background_ = nullptr, *close_ = nullptr, *cursor_ = nullptr, *items_[6]{};
    const rb::Progress* progress_ = nullptr;
    GameMessageBox* message_      = nullptr;
    ax::Label* counts_[4]{};
    ax::Node* unlock_ = nullptr;
    ax::Label* descriptions_[2]{};
    std::vector<ax::Node*> decor_;
};
