#pragma once
#include "axmol.h"
#include <functional>
class EpisodeMovie : public ax::Node
{
public:
    static EpisodeMovie* create(int databaseEpisode, bool ending, int language);
    void open();
    void setFast(bool value);
    void skip();
    bool playing() const { return playing_; }
    std::size_t panelCount() const { return panels_.size(); }
    float offset() const { return offset_; }
    std::function<void()> complete;
    // Empty filename stops the soundtrack.
    std::function<void(std::string_view)> music;

private:
    void step();
    std::vector<ax::Sprite*> panels_;
    ax::Sprite *fast_ = nullptr, *fastPressed_ = nullptr, *skip_ = nullptr, *skipPressed_ = nullptr;
    ax::LayerColor* fade_ = nullptr;
    int episode_          = 0;
    bool ending_ = false, playing_ = false, opened_ = false;
    float speed_ = 2, offset_ = 0;
};
