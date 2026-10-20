#include "PortraitDialog.h"
#include "NodeFactory.h"
#include "Assets.h"
#include "SheetSprite.h"
#include <stdexcept>
using namespace ax;
PortraitDialog* PortraitDialog::create(int episode, int language, int speed, bool showCharacter)
{
    auto* value     = rb::make<PortraitDialog>();
    value->episode_ = episode;
    value->configure(language, speed, showCharacter);
    value->setup();
    return value;
}
PortraitDialog::Layout PortraitDialog::layoutFor(int episode)
{
    switch (episode)
    {
    case 1:
        return {.portrait    = "char_e1.png",
                .heroBox     = "dia_talkbox.png",
                .charBox     = "dia_charbox.png",
                .heroNext    = "dia_hero_next.png",
                .charNext    = "dia_char_next.png",
                .eyeFrames   = 2,
                .mouthFrames = 4,
                .boxColumns  = 3,
                .boxRows     = 1,
                .heroColumns = 1,
                .heroRows    = 1,
                .charColumns = 1,
                .charRows    = 1,
                .eyes        = {63, 44},
                .mouth       = {75.5f, 64},
                .box         = {150, 0},
                .heroArrow   = {334, 25},
                .charArrow   = {212, 49},
                .heroText    = {40, 20},
                .charText    = {40, 30},
                .eyeInterval = .2f,
                .entranceX   = 23,
                .heroInset   = 80};
    case 3:
        return {.portrait    = "char_e3.png",
                .heroBox     = "dia_e3_talkbox1.png",
                .charBox     = "dia_e3_talkbox2.png",
                .heroNext    = "dia_e3_next1.png",
                .charNext    = "dia_e3_next2.png",
                .eyeFrames   = 2,
                .mouthFrames = 3,
                .boxColumns  = 3,
                .boxRows     = 1,
                .heroColumns = 5,
                .heroRows    = 1,
                .charColumns = 5,
                .charRows    = 1,
                .eyes        = {35, 34},
                .mouth       = {54, 58},
                .box         = {100, 25},
                .heroArrow   = {315, 28},
                .charArrow   = {230, 38},
                .heroText    = {30, 20},
                .charText    = {40, 25},
                .eyeInterval = .1f,
                .entranceX   = 45,
                .heroInset   = 60};
    case 6:
        return {.portrait    = "char_e6.png",
                .heroBox     = "dia_e6_talkbox1.png",
                .charBox     = "dia_e6_talkbox2.png",
                .heroNext    = "dia_e6_next2.png",
                .charNext    = "dia_e6_next1.png",
                .eyeFrames   = 4,
                .mouthFrames = 6,
                .boxColumns  = 2,
                .boxRows     = 2,
                .heroColumns = 3,
                .heroRows    = 2,
                .charColumns = 4,
                .charRows    = 1,
                .eyes        = {64.5f, 44.5f},
                .mouth       = {72, 71},
                .box         = {181, -11},
                .heroArrow   = {323, 27},
                .charArrow   = {203, 55},
                .heroText    = {30, 19},
                .charText    = {40, 30},
                .eyeInterval = .1f,
                .entranceX   = 24,
                .heroInset   = 80};
    default:
        throw std::runtime_error("Unsupported portrait dialog episode");
    }
}
void PortraitDialog::setup()
{
    layout_ = layoutFor(episode_);
    setContentSize({480, 320});
    portrait_ = rb::sprite(layout_.portrait);
    portrait_->setVisible(showCharacter_);
    addChild(portrait_);
    const auto size = portrait_->getContentSize();
    portrait_->setPosition(-size.width / 2, size.height / 2);
    const std::string prefix = "char_e" + std::to_string(episode_);
    eyes_                    = SheetSprite::create(prefix + "_eyes.png", layout_.eyeFrames, 1);
    rb::attach(portrait_, eyes_, layout_.eyes.x, layout_.eyes.y);
    eyes_->setVisible(false);
    mouth_ = SheetSprite::create(prefix + "_mouth.png", layout_.mouthFrames, 1);
    rb::attach(portrait_, mouth_, layout_.mouth.x, layout_.mouth.y);
    mouth_->setVisible(false);
    charBox_ = SheetSprite::create(layout_.charBox, layout_.boxColumns, layout_.boxRows);
    rb::attach(portrait_, charBox_, layout_.box.x, layout_.box.y);
    charBox_->setOpacity(0);
    heroBox_ = rb::sprite(layout_.heroBox);
    addChild(heroBox_);
    heroBox_->setPosition(240, -heroBox_->getContentSize().height / 2);
    heroNext_ = SheetSprite::create(layout_.heroNext, layout_.heroColumns, layout_.heroRows);
    rb::attach(heroBox_, heroNext_, layout_.heroArrow.x, layout_.heroArrow.y);
    heroArrowOrigin_ = heroNext_->getPosition();
    charNext_        = SheetSprite::create(layout_.charNext, layout_.charColumns, layout_.charRows);
    rb::attach(charBox_, charNext_, layout_.charArrow.x, layout_.charArrow.y);
    charArrowOrigin_ = charNext_->getPosition();
    stopIndicators();
}
void PortraitDialog::open(std::function<void()> complete)
{
    busy_ = true;
    if (!showCharacter_)
    {
        charBox_->setVisible(false);
        openBox(std::move(complete));
        return;
    }
    charBox_->animate(layout_.boxColumns * layout_.boxRows, .2f);
    const auto size = portrait_->getContentSize();
    portrait_->runAction(Sequence::create(
        EaseIn::create(MoveTo::create(.5f, {size.width / 2 + layout_.entranceX, size.height / 2}), .25f),
        CallFunc::create([this, complete] {
        eyes(true);
        openBox(complete);
    }),
        nullptr));
}
void PortraitDialog::openBox(std::function<void()> complete)
{
    slideHeroBoxIn(std::move(complete));
}
void PortraitDialog::close(std::function<void()> complete)
{
    busy_ = true;
    charBox_->stopAnimation();
    charBox_->stopAllActions();
    eyes(false);
    mouth(false);
    charBox_->setVisible(false);
    slideHeroBoxOut();
    if (!showCharacter_)
    {
        finish(complete);
        return;
    }
    const auto size = portrait_->getContentSize();
    portrait_->runAction(Sequence::create(EaseIn::create(MoveTo::create(.5f, {-size.width / 2, size.height / 2}), .25f),
                                          CallFunc::create([this, complete] { finish(complete); }), nullptr));
}
void PortraitDialog::eyes(bool start)
{
    blink(eyes_, "eyes", layout_.eyeFrames, layout_.eyeInterval, 3.5f, start);
}
void PortraitDialog::mouth(bool start)
{
    blink(mouth_, "mouth", layout_.mouthFrames, .1f, .5f, start);
}
void PortraitDialog::showSpeaker(bool character)
{
    mouth(character);
    charBox_->runAction(FadeTo::create(.3f, character ? 255 : 0));
}
void PortraitDialog::stopIndicators()
{
    for (auto* arrow : {heroNext_, charNext_})
    {
        arrow->stopAnimation();
        arrow->stopAllActions();
        arrow->setVisible(false);
    }
}
void PortraitDialog::textFinished()
{
    mouth(false);
    auto* arrow = characterTalk_ ? charNext_ : heroNext_;
    arrow->setVisible(true);
    if (episode_ == 1)
    {
        const auto origin = characterTalk_ ? charArrowOrigin_ : heroArrowOrigin_;
        arrow->setPosition(origin);
        arrow->runAction(RepeatForever::create(
            Sequence::create(MoveTo::create(.3f, origin), MoveTo::create(.3f, origin + Vec2(0, 3)), nullptr)));
    }
    else if (episode_ == 3 && characterTalk_)
    {
        // Original 0x53bce..0x53bfe: animation uses 4 columns, while creation uses 5.
        arrow->setGrid(4, 1);
        arrow->animate(4, .1f);
    }
    else
        arrow->animate(characterTalk_ ? layout_.charColumns * layout_.charRows : layout_.heroColumns * layout_.heroRows,
                       .1f);
}
EpisodeDialog::TextLayout PortraitDialog::textLayout(bool character) const
{
    if (character)
        return {charBox_, layout_.charText.x, layout_.charText.y, 80, Color3B::BLACK};
    return {heroBox_, layout_.heroText.x, layout_.heroText.y, layout_.heroInset, Color3B::WHITE};
}
