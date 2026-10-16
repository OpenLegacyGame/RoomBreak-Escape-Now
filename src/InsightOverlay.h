#pragma once
#include "axmol.h"
#include "GameData.h"
#include <functional>
class GameSession;
class InsightOverlay : public ax::Node
{
public:
    static InsightOverlay* create(const rb::GameData&, GameSession&, int item, int language);
    void start();
    void close();
    bool ready() const { return ready_; }
    int markedObject() const { return markedObject_; }
    std::function<void()> complete;
    std::function<void(std::string_view)> sound;

private:
    void nextTarget();
    void mark();
    void showInfo();
    void finish();
    const rb::GameData* data_ = nullptr;
    GameSession* session_     = nullptr;
    int item_ = 0, originalRoom_ = 0, markedObject_ = 0;
    std::size_t target_ = 0;
    bool ready_ = false, started_ = false;
    std::vector<int> targets_;
    ax::LayerColor* shade_          = nullptr;
    ax::ParticleSystemQuad *effect_ = nullptr, *marker_ = nullptr;
    ax::Sprite* info_ = nullptr;
    std::vector<ax::Node*> fading_;
};
