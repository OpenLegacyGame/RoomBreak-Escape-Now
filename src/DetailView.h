#pragma once
#include "axmol.h"
#include "GameData.h"
#include <functional>
class DetailView : public ax::Node
{
public:
    static DetailView* create(const rb::GameData&, int id, ax::Vec2 origin, int language, int hands);
    virtual void open();
    virtual void close();
    virtual void retry() {}
    virtual void solve();
    bool busy() const { return busy_; }
    int id() const { return id_; }
    bool hasResult() const { return hasResult_; }
    void setHands(int count);
    void beginTouch(ax::Vec2);
    void endTouch(ax::Vec2);
    void moveTouch(ax::Vec2);
    void cancelTouch();
    virtual ax::Vec2 controlCenter(int index) const { return {}; }
    std::function<void(bool)> complete;
    std::function<void(std::string_view)> sound;
    std::function<void(bool)> gameUI;
    std::function<void()> handsRequested;
    std::function<bool()> blocked;

protected:
    // Shared two-phase construction for every concrete detail view.
    template <class T>
    static T* createDetail(const rb::GameData& data, int id, ax::Vec2 origin, int language, int hands)
    {
        auto* view = new T;
        if (!view->initialize(data, id, origin, language, hands))
        {
            delete view;
            return nullptr;
        }
        view->autorelease();
        return view;
    }
    bool initialize(const rb::GameData&, int id, ax::Vec2 origin, int language, int hands);
    virtual void setup(const rb::Row&) = 0;
    virtual void press(ax::Vec2) {}
    virtual void release(ax::Vec2) {}
    virtual bool releaseWhileBusy() const { return false; }
    virtual void move(ax::Vec2) {}
    virtual void cancel() {}
    virtual void failedComplete() { busy_ = false; }
    void success();
    void failed();
    void playSound(std::string_view file)
    {
        if (sound)
            sound(file);
    }
    ax::Sprite* makeBoard(std::string_view file);
    ax::Sprite* addImage(std::string_view file, float x, float y, bool fade = true);
    void track(ax::Node* node) { fading_.push_back(node); }
    static bool contains(ax::Node*, ax::Vec2);
    static void place(ax::Node*, ax::Node*, ax::Vec2 topLeft);
    ax::Sprite* board_ = nullptr;
    int id_ = 0, language_ = 0;
    bool busy_ = true, success_ = false, closing_ = false;

private:
    ax::LayerColor* shade_ = nullptr;
    ax::Vec2 origin_;
    bool opened_ = false, hasResult_ = false;
    int pressed_ = -1;
    std::vector<ax::Node*> fading_;
    ax::Sprite *menus_[3]{}, *pressedImages_[3]{};
    ax::Label* handsLabel_ = nullptr;
    void closeMenu();
};
