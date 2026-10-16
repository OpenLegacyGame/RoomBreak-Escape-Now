#pragma once
#include "EpisodeDialog.h"
// EventSceneEp4Layer: a cloning portrait and a pulsing character box.
class Episode4Dialog : public EpisodeDialog
{
public:
    static Episode4Dialog* create(int language, int speed, bool showCharacter);
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
    ax::Sprite *portrait_ = nullptr, *charBox_ = nullptr;
    SheetSprite *clone_ = nullptr, *eyes_ = nullptr, *mouth_ = nullptr, *heroNext_ = nullptr;
};
