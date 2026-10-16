#pragma once
#include "axmol.h"
#include "GameData.h"
#include <functional>
#include <vector>
class InventoryView : public ax::Node
{
public:
    static InventoryView* create(const rb::GameData&, rb::StageState&, int language);
    void show(bool visible);
    void open();
    void close();
    void refresh();
    void refreshAbility();
    void selectMode(int mode);
    int mode() const { return mode_; }
    bool operating() const { return operation_; }
    bool opened() const { return opened_; }
    int selectedIndex() const { return selected_; }
    bool beginTouch(ax::Vec2);
    void moveTouch(ax::Vec2);
    void endTouch(ax::Vec2);
    std::function<bool()> blocked;
    std::function<void(bool)> otherUI;
    std::function<void(int)> selectItem, requestInsight;
    std::function<int()> insightCount;
    std::function<void(std::string_view)> sound;
    std::function<void()> changed;

private:
    void setup();
    void sortUse(float duration);
    void labels();
    void indices();
    void arrows();
    void clearLabels();
    ax::Vec2 gridPosition(int index) const;
    void sortGrid();
    void holdItem();
    void returnItem();
    void combine();
    void separate(ax::Vec2 point);
    void finishSeparate();
    const rb::GameData* data_ = nullptr;
    rb::StageState* state_    = nullptr;
    int language_ = 0, selected_ = 0, pressed_ = -1, mode_ = 1, target_ = -1, holdTicks_ = 0;
    bool held_ = false, splitReady_ = false, splitDragging_ = false, operation_ = false;
    ax::Vec2 start_;
    ax::Sprite *possible_ = nullptr, *impossible_ = nullptr;
    std::vector<int> ids_;
    bool shown_ = false, opened_ = false, dragged_ = false, sliding_ = false;
    float moving_ = 0;
    ax::Vec2 previous_;
    ax::Sprite *bg_ = nullptr, *cell_ = nullptr, *open_ = nullptr, *openP_ = nullptr, *close_ = nullptr,
               *closeP_ = nullptr, *highlight_ = nullptr, *indexSelected_ = nullptr;
    ax::Sprite* tabs_[3]{};
    ax::Sprite* selectedTabs_[3]{};
    ax::Sprite* arrow_[2][3]{};
    std::vector<ax::Sprite*> items_, indexBg_, index_;
    ax::Sprite *insight_ = nullptr, *insightP_ = nullptr;
    ax::Label* insightLabel_ = nullptr;
    ax::Node* labels_        = nullptr;
};
