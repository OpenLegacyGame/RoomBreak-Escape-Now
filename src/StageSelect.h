#pragma once
#include "axmol.h"
#include "GameData.h"
#include "Progress.h"
#include <array>
#include <functional>

class StageSelect : public ax::Node
{
public:
    static StageSelect* create(const rb::GameData& data,
                               rb::Progress& progress,
                               int language,
                               int displayedEpisode = -1);
    int episode() const { return episode_; }
    bool opened() const { return opened_; }
    std::function<void(std::string_view)> sound;
    std::function<void()> back;
    std::function<void(int, int)> play;
    std::function<void(int, bool)> movie;

private:
    friend class MainScene;
    friend bool runVisualProbe(ax::Scene*, const rb::GameData&);
    void setup();
    void buildSymbols();
    void selectEpisode();
    void showStages(bool animate = true);
    void closeEpisode();
    void sortSymbols(int episode);
    void updateArrows();
    void pulseArrows(bool left);
    void notify(const char* key);
    int hit(ax::Vec2 point) const;
    void activate(int id);
    void moveMechanism(float delta);
    void playSound(const char* file)
    {
        if (sound)
            sound(file);
    }
    float dragDistance_       = 0;
    const rb::GameData* data_ = nullptr;
    rb::Progress* progress_   = nullptr;
    int language_ = 0, episode_ = 0, pressed_ = -1;
    bool opened_ = false, moving_ = false, dragged_ = false, dragCommitted_ = false;
    ax::Sprite *back_ = nullptr, *backPressed_ = nullptr, *stagePressed_ = nullptr;
    ax::Node* stageGroup_    = nullptr;
    ax::Label* notification_ = nullptr;
    std::array<ax::Sprite*, 7> symbols_{}, titles_{};
    std::array<std::array<ax::Sprite*, 2>, 7> doors_{};
    std::array<std::array<ax::Vec2, 2>, 7> doorOrigins_{};
    std::array<ax::Sprite*, 5> stages_{};
    std::array<ax::Sprite*, 3> gears_{};
    std::array<std::vector<ax::Sprite*>, 3> chains_;
    std::array<std::vector<ax::Vec2>, 3> chainOrigins_;
    std::array<ax::Sprite*, 8> arrows_{};
    std::array<ax::Sprite*, 2> movies_{};
};
