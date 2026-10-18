#pragma once
#include "axmol.h"
#include "EventRunner.h"
#include "SystemMenu.h"
#include <memory>
class RoomView;
class StoryDialog;
class CursorPocket;
class CursorFeedback;
class GetItemDialog;
class InventoryView;
class InsightOverlay;
class AbilityEffect;
class AbilityInfo;
class HeroHealth;
class PauseButton;
class StageIntro;
class GameOverOverlay;
class ClearOverlay;
class DetailView;
class ActionMode;
class GameMessageBox;
namespace rb
{
class Progress;
}
// Owns the stage state and the original shared EventHandle completion barrier.
// Specialized overlays install their command dispatcher before begin().
class GameSession : public ax::Node
{
public:
    static GameSession* create(const rb::GameData&, int episode, int stage, int language, int speed, bool goldKey);
    void begin();
    std::string defaultMusic() const;
    void startWithIntro();
    void showGameOver();
    void showClear();
    ClearOverlay* clearOverlay() const { return clear_; }
    std::function<void()> clearAccepted;
    GameOverOverlay* gameOverOverlay() const { return gameOver_; }
    StageIntro* stageIntro() const { return intro_; }
    void showUI(bool visible);
    void setMenuOpen(bool open);
    PauseButton* pauseButton() const { return pauseButton_; }
    bool useAbility(rb::Progress&, int type, std::function<void()> complete = {});
    HeroHealth* heroHealth() const { return heroHealth_; }
    void fillHealth();
    AbilityInfo* abilityInfo() const { return abilityInfo_; }
    AbilityEffect* abilityEffect() const { return ability_; }
    bool useInsight(rb::Progress&, int item);
    void bindProgress(rb::Progress&, std::function<void()> shop = {});
    void openSystemMenu(int mode = 1);
    SystemMenu* systemMenu() const { return systemMenu_; }
    SystemMenu::Settings menuSettings;
    std::function<void(SystemMenu::Settings)> settingsChanged;
    std::function<void(bool)> menuAudio;
    std::function<void(int)> purchaseRequested;
    std::function<void()> unlockAllRequested;
    std::function<void()> retryRequested, exitRequested;
    void requestInsight(int item);
    void requestAbility(int type, int item = 0, std::function<void()> complete = {});
    GameMessageBox* messageBox() const { return message_; }
    InsightOverlay* insight() const { return insight_; }
    void runEvent(int id);
    void moveRoom(int room, int position, std::function<void()> complete);
    void tap(ax::Vec2 world);
    void selectCursor(int cursor, int item = 0);
    void setCursorOutcome(int outcome) { cursorOutcome_ = outcome; }
    void onExit() override;
    rb::StageState& state() { return state_; }
    const rb::StageState& state() const { return state_; }
    RoomView* room() const { return room_; }
    StoryDialog* story() const { return story_; }
    DetailView* detail() const { return detail_; }
    ActionMode* actionMode() const { return action_; }
    InventoryView* inventory() const { return inventory_; }
    GetItemDialog* itemDialog() const { return itemDialog_; }
    CursorPocket* pocket() const { return pocket_; }
    bool busy() const { return events_->busy(); }
    bool touchProtected() const { return events_->protectsTouch(); }
    int touchCount() const { return touchCount_; }
    int touchObject() const { return touchObject_; }
    std::function<void(const rb::Command&, rb::EventRunner::Complete)> overlay;
    std::function<void(std::string_view, bool)> sound;
    std::function<void(bool)> gameUI;
    std::function<void()> vibrate, healthChanged, gameOver, inventoryChanged, roomChanged, progressChanged,
        pauseRequested;
    std::function<void(ax::Vec2, int)> cursorAction;

private:
    void setup(const rb::GameData&, int episode, int stage, int language, int speed, bool goldKey);
    void setupHud();
    void setupTouch();
    void dispatch(const rb::Command&, rb::EventRunner::Complete);
    void openActionMode(const rb::Command&, rb::EventRunner::Complete);
    void openDetail(int id, rb::EventRunner::Complete);
    void receiveItem(int id, bool removeObject, rb::EventRunner::Complete);
    void playStory(int scene, rb::EventRunner::Complete);
    // Shared wiring for detail views and action modes, which may open the shop on close.
    template <class Overlay>
    void presentPuzzle(Overlay*& slot, bool& shopAfterClose, rb::EventRunner::Complete);
    void checkConditions();
    void after(float seconds, rb::EventRunner::Complete);
    void shake(int strength, float duration);
    void basicDialog(std::string_view text);
    void collectGoldKey(rb::EventRunner::Complete);
    void removeTouchedObject();
    // Common modal state that blocks room taps and HUD controls.
    bool interfaceBlocked() const;
    bool abilityBlocked(int type) const;
    std::function<void(std::string_view)> effectSound();
    const rb::GameData* data_ = nullptr;
    rb::StageState state_;
    std::unique_ptr<rb::EventRunner> events_;
    RoomView* room_            = nullptr;
    CursorPocket* pocket_      = nullptr;
    CursorFeedback* feedback_  = nullptr;
    StoryDialog* story_        = nullptr;
    GetItemDialog* itemDialog_ = nullptr;
    InventoryView* inventory_  = nullptr;
    DetailView* detail_        = nullptr;
    bool detailShop_ = false, actionShop_ = false;
    ActionMode* action_        = nullptr;
    ClearOverlay* clear_       = nullptr;
    GameOverOverlay* gameOver_ = nullptr;
    bool ended_                = false;
    StageIntro* intro_         = nullptr;
    SystemMenu* systemMenu_    = nullptr;
    PauseButton* pauseButton_  = nullptr;
    bool menuOpen_             = false;
    HeroHealth* heroHealth_    = nullptr;
    int maxHealth_             = 100;
    AbilityInfo* abilityInfo_  = nullptr;
    AbilityEffect* ability_    = nullptr;
    InsightOverlay* insight_   = nullptr;
    GameMessageBox* message_   = nullptr;
    rb::Progress* progress_    = nullptr;
    std::function<void()> shop_;
    ax::Node* basicDialog_ = nullptr;
    int language_ = 0, speed_ = 1, cursor_ = 0, item_ = 0, touchCount_ = 0, touchObject_ = 0, lastType_ = 0;
    int cursorOutcome_ = 4;
    bool begun_ = false, dragged_ = false, changingRoom_ = false;
    ax::Vec2 touchPoint_;
    ax::Vec2 gestureStart_;
    ax::Vec2 shakeOrigin_;
    bool shaking_ = false;
};
