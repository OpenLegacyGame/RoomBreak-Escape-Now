#pragma once
#include "axmol.h"
#include "GameData.h"
#include <functional>
class EpisodeDialog;
class StoryDialog : public ax::Node
{
public:
    static StoryDialog* create(const rb::GameData& data, int episode, int scene, int language, int textSpeed);
    void open();
    void advance();
    void close();
    std::function<void(std::string_view, bool)> sound;
    std::function<void()> shake, vibrate, complete;
    std::function<void(bool)> gameUI;

private:
    void setup(int episode, int scene, int textSpeed);
    void nextLine();
    void closeOverlay();
    bool hitSkip(ax::Vec2 point) const;
    const rb::GameData* data_ = nullptr;
    std::vector<const rb::Row*> rows_;
    int language_ = 0, step_ = 1;
    bool busy_ = true, closing_ = false, pressed_ = false;
    EpisodeDialog* skin_   = nullptr;
    ax::LayerColor* shade_ = nullptr;
    ax::Sprite *skip_ = nullptr, *skipPressed_ = nullptr;
};
