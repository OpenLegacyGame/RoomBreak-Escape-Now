#pragma once
#include "axmol.h"
#include <functional>
class ActionMode : public ax::Node
{
public:
    void open();
    void close();
    void retry();
    virtual void solve();
    void setHands(int);
    void beginTouch(ax::Vec2);
    void moveTouch(ax::Vec2);
    void endTouch(ax::Vec2);
    bool busy() const { return busy_; }
    bool menuVisible() const { return menuVisible_; }
    ax::Vec2 menuCenter(int i) const { return menus_[i]->getPosition(); }
    std::function<void(bool)> complete, gameUI;
    std::function<void(std::string_view)> sound;
    std::function<void()> handsRequested;
    std::function<bool()> blocked;

protected:
    // Shared two-phase construction for every concrete action mode.
    template <class T>
    static T* createAction(int level, int hands)
    {
        auto* mode = new T;
        if (!mode->initialize(level, hands))
        {
            delete mode;
            return nullptr;
        }
        mode->autorelease();
        return mode;
    }
    bool initialize(int level, int hands);
    virtual void setup(int)    = 0;
    virtual void start()       = 0;
    virtual void hideContent() = 0;
    virtual void press(ax::Vec2) {}
    virtual void move(ax::Vec2) {}
    virtual void release(ax::Vec2) {}
    virtual void cancelGesture() {}
    void fail();
    void showMenu(bool);
    bool busy_ = true, closing_ = false;
    void playSound(std::string_view name)
    {
        if (sound)
            sound(name);
    }

private:
    bool menuVisible_ = false, opened_ = false, success_ = false;
    int pressed_           = -1;
    ax::LayerColor* shade_ = nullptr;
    ax::Sprite *menus_[3]{}, *pressedImages_[3]{};
    ax::Label* count_ = nullptr;
    static bool contains(ax::Node*, ax::Vec2);
};
