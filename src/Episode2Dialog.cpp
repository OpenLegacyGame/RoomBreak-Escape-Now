#include "Episode2Dialog.h"
#include "NodeFactory.h"
#include "Assets.h"
#include "SheetSprite.h"
using namespace ax;
Episode2Dialog* Episode2Dialog::create(int language, int speed, bool showCharacter)
{
    auto* value = rb::make<Episode2Dialog>();
    value->configure(language, speed, showCharacter);
    value->setup();
    return value;
}
void Episode2Dialog::setup()
{
    setContentSize({480, 320});
    portrait_ = rb::sprite("char_e2_bg.png");
    addChild(portrait_);
    portrait_->setPosition(240, 320 - portrait_->getContentSize().height / 2);
    portrait_->setOpacity(0);
    direction1_ = rb::place(portrait_, "char_e2_next_1.png", 315, 188);
    direction1_->setOpacity(0);
    direction2_ = rb::place(portrait_, "char_e2_next_2.png", 309, 181);
    direction2_->setOpacity(0);
    heroBox_ = rb::sprite("dia_e2_talkbox.png");
    addChild(heroBox_);
    heroBox_->setPosition(240, -heroBox_->getContentSize().height / 2);
    heroBox_->setOpacity(150);
    heroNext_ = rb::place(heroBox_, "dia_e2_next.png", 304, 25);
    heroNext_->setOpacity(150);
    heroArrowOrigin_ = heroNext_->getPosition();
    heroNext_->setVisible(false);
    eyes_ = SheetSprite::create("char_e2_eyes.png", 2, 1);
    rb::attach(portrait_, eyes_, 155.5f, 98.5f);
    eyes_->setVisible(false);
    mouth_ = SheetSprite::create("char_e2_mouth.png", 3, 1);
    rb::attach(portrait_, mouth_, 178, 129);
    mouth_->setVisible(false);
    noiseFrames_.pushBack(rb::sprite("char_e2_bg.png"));
    for (int i = 1; i <= 3; ++i)
        noiseFrames_.pushBack(rb::sprite("char_e2_dir1_" + std::to_string(i) + ".png"));
    charTextBox_ = Node::create();
    charTextBox_->setContentSize(portrait_->getContentSize());
    addChild(charTextBox_);
    charTextBox_->setPosition(portrait_->getPosition() - portrait_->getContentSize() / 2);
}
void Episode2Dialog::open(std::function<void()> complete)
{
    busy_ = true;
    if (!showCharacter_)
    {
        openBox(std::move(complete));
        return;
    }
    portrait_->runAction(Sequence::create(EaseIn::create(FadeIn::create(.5f), .5f),
                                          CallFunc::create([this, complete] { openBox(complete); }), nullptr));
}
void Episode2Dialog::openBox(std::function<void()> complete)
{
    if (showCharacter_)
    {
        direction1_->setOpacity(100);
        direction2_->setOpacity(100);
    }
    slideHeroBoxIn(std::move(complete));
}
void Episode2Dialog::close(std::function<void()> complete)
{
    busy_    = true;
    closing_ = true;
    charTextBox_->setVisible(false);
    direction1_->setVisible(false);
    direction2_->setVisible(false);
    slideHeroBoxOut();
    if (!showCharacter_)
    {
        finish(complete);
        return;
    }
    noise(false);
    eyes(false);
    mouth(false);
    portrait_->runAction(Sequence::create(EaseIn::create(FadeOut::create(.5f), .25f),
                                          CallFunc::create([this, complete] { finish(complete); }), nullptr));
}
void Episode2Dialog::mouth(bool start)
{
    if (start && closing_)
        return;
    blink(mouth_, "mouth", 3, .1f, .5f, start);
}
void Episode2Dialog::eyes(bool start)
{
    blink(eyes_, "eyes", 2, .2f, 2, start);
}
void Episode2Dialog::showSpeaker(bool character)
{
    mouth(character);
    eyes(character);
    noise(!character);
}
void Episode2Dialog::noiseFrame(int index)
{
    auto* source = noiseFrames_.at(index);
    portrait_->setTexture(source->getTexture());
    portrait_->setTextureRect(source->getTextureRect());
    portrait_->setContentSize(source->getContentSize());
    noiseFrame_ = index;
}
void Episode2Dialog::noise(bool start)
{
    if (!start)
    {
        unschedule("noise");
        return;
    }
    if (isScheduled("noise"))
        return;
    schedule([this](float) {
        direction1_->setVisible(false);
        direction2_->setVisible(false);
        noiseFrame(1);
        schedule([this](float) {
            if (noiseFrame_ < 3)
                noiseFrame(noiseFrame_ + 1);
            else
            {
                unschedule("noise-frames");
                noiseFrame(0);
                direction1_->setVisible(true);
                direction2_->setVisible(true);
            }
        }, .1f, "noise-frames");
    }, 2.5f, "noise");
}
void Episode2Dialog::stopIndicators()
{
    direction1_->stopAllActions();
    direction2_->stopAllActions();
    heroNext_->stopAllActions();
    heroNext_->setVisible(false);
    heroNext_->setPosition(heroArrowOrigin_);
}
void Episode2Dialog::textFinished()
{
    mouth(false);
    if (characterTalk_)
    {
        rb::rotateGear(direction1_, .5f, false);
        rb::rotateGear(direction2_, .5f, true);
    }
    else
    {
        heroNext_->setVisible(true);
        heroNext_->runAction(RepeatForever::create(Sequence::create(
            MoveTo::create(.3f, heroArrowOrigin_), MoveTo::create(.3f, heroArrowOrigin_ + Vec2(0, 2)), nullptr)));
    }
}
EpisodeDialog::TextLayout Episode2Dialog::textLayout(bool character) const
{
    if (character)
        return {charTextBox_, 50, 25, 100, Color3B::WHITE};
    return {heroBox_, 35, 12, 70, Color3B::WHITE};
}
