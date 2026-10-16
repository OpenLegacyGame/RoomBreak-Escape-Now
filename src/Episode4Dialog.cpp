#include "Episode4Dialog.h"
#include "NodeFactory.h"
#include "Assets.h"
#include "SheetSprite.h"
using namespace ax;
Episode4Dialog* Episode4Dialog::create(int language, int speed, bool showCharacter)
{
    auto* value = rb::make<Episode4Dialog>();
    value->configure(language, speed, showCharacter);
    value->setup();
    return value;
}
void Episode4Dialog::setup()
{
    setContentSize({480, 320});
    portrait_ = rb::place(this, "char_e4.png", 256, 57);
    portrait_->setVisible(false);
    clone_ = SheetSprite::create("char_e4_dir1.png", 5, 2);
    addChild(clone_);
    clone_->setPosition(portrait_->getPosition());
    clone_->setVisible(false);
    eyes_ = SheetSprite::create("char_e4_eyes.png", 4, 1);
    rb::attach(portrait_, eyes_, 56, 84);
    eyes_->setVisible(false);
    mouth_ = SheetSprite::create("char_e4_mouth.png", 7, 1);
    rb::attach(portrait_, mouth_, 74, 115);
    mouth_->setVisible(false);
    heroBox_ = rb::sprite("dia_e4_talkbox1.png");
    addChild(heroBox_);
    heroBox_->setPosition(240, -heroBox_->getContentSize().height / 2);
    charBox_ = rb::place(this, "dia_e4_talkbox2.png", 59, 47);
    charBox_->setVisible(false);
    heroNext_ = SheetSprite::create("dia_e4_next.png", 10, 1);
    rb::attach(heroBox_, heroNext_, 316, 23);
    heroNext_->setVisible(false);
}
void Episode4Dialog::open(std::function<void()> complete)
{
    busy_ = true;
    if (!showCharacter_)
    {
        openBox(std::move(complete));
        return;
    }
    scheduleOnce([this](float) { clone_->setVisible(true); }, .1f, "reveal-clone");
    clone_->playOnce(10, .05f, [this, complete] { openBox(complete); });
}
void Episode4Dialog::openBox(std::function<void()> complete)
{
    if (showCharacter_)
    {
        clone_->setVisible(false);
        portrait_->setVisible(true);
        charBox_->setVisible(true);
        charBox_->setOpacity(0);
        blink(eyes_, "eyes", 4, .1f, 2, true);
    }
    slideHeroBoxIn(std::move(complete));
}
void Episode4Dialog::close(std::function<void()> complete)
{
    busy_ = true;
    blink(eyes_, "eyes", 4, .1f, 2, false);
    mouth(false);
    charBox_->setVisible(false);
    slideHeroBoxOut();
    if (!showCharacter_)
    {
        finish(complete);
        return;
    }
    portrait_->setVisible(false);
    clone_->setVisible(true);
    clone_->playOnce(10, .05f, [this, complete] {
        clone_->setVisible(false);
        finish(complete);
    }, true);
}
EpisodeDialog::TextLayout Episode4Dialog::textLayout(bool character) const
{
    if (character)
        return {charBox_, 40, 15, 80, Color3B::WHITE};
    return {heroBox_, 35, 19, 70, Color3B::WHITE};
}
void Episode4Dialog::mouth(bool start)
{
    mouth_->setVisible(start);
    if (start)
        mouth_->animate(7, .1f);
    else
        mouth_->stopAnimation();
}
void Episode4Dialog::showSpeaker(bool character)
{
    mouth(character);
    charBox_->runAction(FadeTo::create(.3f, character ? 255 : 0));
}
void Episode4Dialog::stopIndicators()
{
    heroNext_->stopAnimation();
    heroNext_->setVisible(false);
    charBox_->stopAllActions();
    if (characterTalk_)
        charBox_->runAction(FadeTo::create(.3f, 255));
}
void Episode4Dialog::textFinished()
{
    mouth(false);
    if (characterTalk_)
        charBox_->runAction(
            RepeatForever::create(Sequence::create(FadeTo::create(.3f, 100), FadeTo::create(.3f, 200), nullptr)));
    else
    {
        heroNext_->setVisible(true);
        heroNext_->animate(10, .1f);
    }
}
