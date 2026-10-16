#pragma once
#include "EpisodeDialog.h"
// EventSceneEp2Layer: a monitor portrait with static noise and rotating dials.
class Episode2Dialog : public EpisodeDialog
{
public:
    static Episode2Dialog* create(int language, int speed, bool showCharacter);
    void open(std::function<void()> complete) override;
    void close(std::function<void()> complete) override;

private:
    void setup();
    void openBox(std::function<void()> complete);
    TextLayout textLayout(bool character) const override;
    void stopIndicators() override;
    void mouth(bool start) override;
    void showSpeaker(bool character) override;
    void textFinished() override;
    void eyes(bool start);
    void noise(bool start);
    void noiseFrame(int index);
    int noiseFrame_       = 0;
    bool closing_         = false;
    ax::Sprite *portrait_ = nullptr, *heroNext_ = nullptr, *direction1_ = nullptr, *direction2_ = nullptr;
    ax::Node* charTextBox_ = nullptr;
    SheetSprite *eyes_ = nullptr, *mouth_ = nullptr;
    ax::Vector<ax::Sprite*> noiseFrames_;
    ax::Vec2 heroArrowOrigin_;
};
