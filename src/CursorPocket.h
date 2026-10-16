#pragma once
#include "axmol.h"
#include "GameData.h"
#include <array>
#include <functional>
class CursorPocket : public ax::Node
{
public:
    static CursorPocket* create(const rb::GameData& data);
    void show(bool visible);
    void open();
    void close();
    void select(int cursor);
    void setItem(int id);
    void consumeItem();
    bool opened() const { return opened_; }
    int selected() const { return selected_; }
    int item() const { return item_; }
    std::function<void(int, int)> changed;
    std::function<bool()> blocked;

private:
    void setup();
    int hit(ax::Vec2 point) const;
    ax::Sprite* makeItem();
    const rb::GameData* data_ = nullptr;
    ax::Sprite *box_ = nullptr, *pressedBox_ = nullptr, *underbar_ = nullptr, *itemCell_ = nullptr;
    std::array<ax::Sprite*, 4> choices_{}, selectedIcons_{};
    std::array<ax::Vec2, 4> positions_{};
    ax::Vec2 underbarPosition_;
    int selected_ = 0, item_ = 0, pressed_ = -1;
    bool opened_ = false, shown_ = false;
};
