#pragma once
#include "axmol.h"
#include "GameData.h"
#include "Progress.h"
#include "UserSettings.h"
#include "GameAudio.h"
#include <map>
#include <memory>

class GameSession;
class EpisodeMovie;
class StageSelect;
class TitleShop;
class MainScene : public ax::Scene
{
public:
    bool init() override;
    int musicId() const { return audio_.musicId(); }
#if RB_VISUAL_PROBE
    void startRouteDemo();
    GameSession* routeDemoSession() { return session_; }
    std::shared_ptr<rb::Progress> routeDemoProgress()
    {
        return {&progress_, [](rb::Progress*) {}};
    }
    void prepareDemoRoute();
    int demoIndex_ = 0;
    std::string demoRoot_;
    void finishRouteDemo();
#endif
private:
    ax::Node* canvas_ = nullptr;
    void createTitle();
    void beginLoading(bool tips, std::function<void()> complete);
    void showCredits();
    void showStageSelect(int displayedEpisode = -1);
    void selectStage(int episode, int stage);
    void startStage(int episode, int stage);
    void showMovie(int episode, bool ending, std::function<void()> next);
    void acceptClear(int episode, int stage);
    void clearContent();
    void transition(std::function<void()> next);
    void saveProgress();
    friend bool runMainFlowProbe(MainScene*);
    GameSession* session_ = nullptr;
    EpisodeMovie* movie_  = nullptr;
    std::string savePath_;
    bool transitioning_ = false, loading_ = false;
    void setOptions(bool open);
    void refreshSettings();
    void click(int id);
    int hit(ax::Vec2 point) const;
    bool hasPressedImage(int id) const;
    ax::Sprite* addMenu(int id, const char* file, float x, float y, int parent = -1, bool visible = true);
    rb::GameData data_;
    rb::Progress progress_;
    rb::UserSettings settings_;
    rb::GameAudio audio_{settings_};
    StageSelect* selection_ = nullptr;
    TitleShop* shop_        = nullptr;
    std::map<int, ax::Sprite*> menu_;
    ax::Sprite* background_ = nullptr;
    ax::Node* title_        = nullptr;
    ax::Node* credits_      = nullptr;
    std::vector<ax::Sprite*> creditPages_;
    std::vector<ax::Vec2> creditOrigins_;
    ax::Label* chineseIndicator_ = nullptr;
    int pressed_                 = -1;
    bool options_ = false, draggingCredits_ = false;
};
