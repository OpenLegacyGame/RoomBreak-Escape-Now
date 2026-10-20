#include "Episode5Dialog.h"
#include "NodeFactory.h"
#include "Assets.h"
#include "SheetSprite.h"
using namespace ax;
Episode5Dialog* Episode5Dialog::create(int language, int speed, bool showCharacter)
{
    auto* value = rb::make<Episode5Dialog>();
    value->configure(language, speed, showCharacter);
    value->setup();
    return value;
}
void Episode5Dialog::setup()
{
    setContentSize({480, 320});
    portrait_ = rb::sprite("char_e5.png");
    portrait_->setVisible(showCharacter_);
    addChild(portrait_);
    auto size = portrait_->getContentSize();
    portrait_->setPosition(295 + size.width / 2, 320 + size.height / 2);
    speaker1_ = rb::place(portrait_, "char_e5_obj1.png", 7, 23);
    speaker2_ = rb::place(portrait_, "char_e5_obj2.png", 21, 34);
    charBox_  = SheetSprite::create("dia_e5_talkbox2.png", 3, 1);
    rb::attach(this, charBox_, 46, 20);
    charBox_->setOpacity(0);
    heroBox_ = rb::sprite("dia_e5_talkbox.png");
    addChild(heroBox_);
    heroBox_->setPosition(240, -heroBox_->getContentSize().height / 2);
    heroNext_ = SheetSprite::create("dia_e5_next1.png", 4, 1);
    rb::attach(heroBox_, heroNext_, 310, 26);
    charNext_ = SheetSprite::create("dia_e5_next2.png", 3, 1);
    rb::attach(charBox_, charNext_, 162, 60);
    stopIndicators();
}
void Episode5Dialog::open(std::function<void()> complete)
{
    busy_ = true;
    if (!showCharacter_)
    {
        charBox_->setVisible(false);
        openBox(std::move(complete));
        return;
    }
    charBox_->animate(3, .2f);
    auto size = portrait_->getContentSize();
    portrait_->runAction(
        Sequence::create(EaseIn::create(MoveTo::create(.5f, {295 + size.width / 2, 320 - size.height / 2 - 27}), .25f),
                         CallFunc::create([this, complete] { openBox(complete); }), nullptr));
}
void Episode5Dialog::openBox(std::function<void()> complete)
{
    slideHeroBoxIn(std::move(complete));
}
void Episode5Dialog::close(std::function<void()> complete)
{
    busy_ = true;
    mouth(false);
    charBox_->setVisible(false);
    slideHeroBoxOut();
    // The original completes immediately when there is no character portrait.
    if (!showCharacter_)
    {
        finish(complete);
        return;
    }
    auto size = portrait_->getContentSize();
    portrait_->runAction(
        Sequence::create(EaseIn::create(MoveTo::create(.5f, {295 + size.width / 2, 320 + size.height / 2}), .25f),
                         CallFunc::create([this, complete] { finish(complete); }), nullptr));
}
EpisodeDialog::TextLayout Episode5Dialog::textLayout(bool character) const
{
    if (character)
        return {charBox_, 40, 35, 70, Color3B::BLACK};
    return {heroBox_, 35, 19, 80, Color3B::WHITE};
}
void Episode5Dialog::stopIndicators()
{
    for (auto* arrow : {heroNext_, charNext_})
    {
        arrow->stopAnimation();
        arrow->setVisible(false);
    }
}
void Episode5Dialog::mouth(bool start)
{
    int index = 0;
    for (auto* sprite : {speaker1_, speaker2_})
    {
        sprite->stopAllActions();
        if (!start)
            sprite->runAction(EaseIn::create(ScaleTo::create(.3f, 1), .25f));
        else
        {
            const float dt = index ? .15f : .1f, first = index ? 1.1f : 1.2f, second = index ? 1.2f : 1.4f;
            sprite->runAction(RepeatForever::create(Sequence::create(
                EaseOut::create(ScaleTo::create(dt, first), .25f), EaseOut::create(ScaleTo::create(dt, 1), .25f),
                EaseOut::create(ScaleTo::create(dt, second), .25f), EaseOut::create(ScaleTo::create(dt, 1), .25f),
                nullptr)));
        }
        ++index;
    }
}
void Episode5Dialog::showSpeaker(bool character)
{
    mouth(character);
    charBox_->runAction(FadeTo::create(.3f, character ? 255 : 0));
}
void Episode5Dialog::textFinished()
{
    mouth(false);
    auto* arrow = characterTalk_ ? charNext_ : heroNext_;
    arrow->setVisible(true);
    arrow->animate(characterTalk_ ? 3 : 4, .1f);
}
