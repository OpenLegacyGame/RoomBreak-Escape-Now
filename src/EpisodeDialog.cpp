#include "EpisodeDialog.h"
#include "DialogText.h"
#include "SheetSprite.h"
using namespace ax;
void EpisodeDialog::configure(int language, int speed, bool showCharacter)
{
    // Dialogue artwork enters and exits through the original canvas edges.
    // Wider room views must not expose sprites parked outside those edges.
    setClippingRegion({0, 0, 480, 320});
    language_      = language;
    speed_         = speed;
    showCharacter_ = showCharacter;
}
void EpisodeDialog::play(std::string_view value, bool character)
{
    if (text_)
        text_->removeFromParent();
    characterTalk_ = character;
    stopIndicators();
    const auto layout = textLayout(character);
    text_ = DialogText::create(language_, speed_, layout.box->getContentSize().width - layout.inset, 2, layout.color);
    layout.box->addChild(text_);
    text_->setPosition(layout.x, layout.box->getContentSize().height - layout.y);
    text_->finished = [this] { textFinished(); };
    if (showCharacter_)
        showSpeaker(character);
    text_->setText(value);
}
bool EpisodeDialog::next()
{
    if (busy_ || !text_)
        return false;
    if (text_->animating())
    {
        text_->skip();
        return false;
    }
    if (text_->hasMore())
    {
        stopIndicators();
        if (characterTalk_)
            mouth(true);
        text_->nextPage();
        return false;
    }
    return true;
}
void EpisodeDialog::slideHeroBoxIn(std::function<void()> complete)
{
    heroBox_->runAction(
        Sequence::create(EaseIn::create(MoveTo::create(.5f, {240, heroBox_->getContentSize().height / 2 + 8}), .25f),
                         CallFunc::create([this, complete = std::move(complete)] { finish(complete); }), nullptr));
}
void EpisodeDialog::slideHeroBoxOut()
{
    heroBox_->runAction(EaseIn::create(MoveTo::create(.5f, {240, -heroBox_->getContentSize().height / 2}), .25f));
}
void EpisodeDialog::finish(const std::function<void()>& complete)
{
    busy_ = false;
    if (complete)
        complete();
}
void EpisodeDialog::blink(SheetSprite* sprite,
                          const std::string& key,
                          int frames,
                          float interval,
                          float period,
                          bool start)
{
    if (!start)
    {
        unschedule(key);
        sprite->stopAnimation();
        sprite->setVisible(false);
        return;
    }
    if (isScheduled(key))
        return;
    schedule([sprite, frames, interval](float) {
        sprite->setVisible(true);
        sprite->playOnce(frames, interval, [sprite] { sprite->setVisible(false); });
    }, period, key);
}
