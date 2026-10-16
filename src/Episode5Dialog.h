#pragma once
#include "EpisodeDialog.h"

// EventSceneEp5Layer (also used by tutorial episode 0).
class Episode5Dialog : public EpisodeDialog
{
public:
    static Episode5Dialog* create(int language, int textSpeed, bool showCharacter);
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
    ax::Sprite *portrait_ = nullptr, *speaker1_ = nullptr, *speaker2_ = nullptr;
    SheetSprite *charBox_ = nullptr, *heroNext_ = nullptr, *charNext_ = nullptr;
};
