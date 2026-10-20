#pragma once
#include "axmol.h"
#include <functional>
#include <string>
class DialogText;
class SheetSprite;

// Shared flow of the original EventScene layers: a hero text box that slides
// up from the bottom, an optional character box, and paged typewriter text.
// Each episode skin supplies its artwork and the hooks below.
class EpisodeDialog : public ax::ClippingRectangleNode
{
public:
    virtual void open(std::function<void()> complete)  = 0;
    virtual void close(std::function<void()> complete) = 0;
    void play(std::string_view text, bool character);
    // Skips typing, then turns pages; true once the current line is finished.
    bool next();
    bool busy() const { return busy_; }

protected:
    struct TextLayout
    {
        ax::Node* box;
        float x, y, inset;
        ax::Color3B color;
    };
    void configure(int language, int speed, bool showCharacter);
    virtual TextLayout textLayout(bool character) const = 0;
    virtual void stopIndicators()                       = 0;
    virtual void mouth(bool start)                      = 0;
    virtual void showSpeaker(bool character)            = 0;
    virtual void textFinished()                         = 0;
    // Moves the hero box on screen, then reports the dialog ready.
    void slideHeroBoxIn(std::function<void()> complete);
    void slideHeroBoxOut();
    void finish(const std::function<void()>& complete);
    // Repeats a one-shot sheet animation every period seconds until stopped.
    void blink(SheetSprite* sprite, const std::string& key, int frames, float interval, float period, bool start);
    int language_ = 0, speed_ = 1;
    bool showCharacter_ = false, characterTalk_ = false, busy_ = true;
    ax::Sprite* heroBox_ = nullptr;
    DialogText* text_    = nullptr;
};
