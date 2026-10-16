#pragma once
#include "EpisodeDialog.h"
// The matching left-entrance flow of EventSceneEp1/3/6Layer, with their own assets.
class PortraitDialog : public EpisodeDialog
{
public:
    static PortraitDialog* create(int episode, int language, int speed, bool showCharacter);
    void open(std::function<void()> complete) override;
    void close(std::function<void()> complete) override;

private:
    struct Layout
    {
        const char *portrait, *heroBox, *charBox, *heroNext, *charNext;
        int eyeFrames, mouthFrames, boxColumns, boxRows, heroColumns, heroRows, charColumns, charRows;
        ax::Vec2 eyes, mouth, box, heroArrow, charArrow, heroText, charText;
        float eyeInterval, entranceX, heroInset;
    };
    static Layout layoutFor(int episode);
    void setup();
    void openBox(std::function<void()> complete);
    TextLayout textLayout(bool character) const override;
    void stopIndicators() override;
    void mouth(bool start) override;
    void showSpeaker(bool character) override;
    void textFinished() override;
    void eyes(bool start);
    Layout layout_{};
    int episode_          = 0;
    ax::Sprite* portrait_ = nullptr;
    SheetSprite *eyes_ = nullptr, *mouth_ = nullptr, *charBox_ = nullptr, *heroNext_ = nullptr, *charNext_ = nullptr;
    ax::Vec2 heroArrowOrigin_, charArrowOrigin_;
};
