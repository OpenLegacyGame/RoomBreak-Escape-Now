#include "Viewport.h"
#include "EpisodeMovie.h"
#include "NodeFactory.h"
#include "Assets.h"
#include "Progress.h"
#include <stdexcept>
using namespace ax;
EpisodeMovie* EpisodeMovie::create(int episode, bool ending, int language)
{
    if (episode < 1 || episode > 6 || language < 0 || language > 3)
        throw std::runtime_error("Invalid episode movie");
    auto* value     = rb::make<EpisodeMovie>();
    value->episode_ = episode;
    value->ending_  = ending;
    value->addChild(rb::screenShade(Color4B::BLACK));
    const int introCounts[] = {5, 6, 3, 4, 3, 14}, endingCounts[] = {4, 6, 4, 3, 6, 8};
    const char* suffix[] = {"chi", "kor", "eng", "jap"};
    float x              = 0;
    for (int i = 1; i <= (ending ? endingCounts : introCounts)[episode - 1]; ++i)
    {
        const bool localized = ending ? (episode == 2 && (i == 1 || i == 2 || i == 3 || i == 5))
                                      : ((episode == 1 || episode == 2) && (i == 2 || i == 3)) ||
                                            (episode == 4 && i == 4) || (episode == 5 && (i == 1 || i == 3));
        std::string file     = "e" + std::to_string(episode) + (ending ? "_ending" : "_intro") + std::to_string(i);
        if (localized)
            file += "_" + std::string(suffix[language]);
        auto* panel = rb::sprite(file + ".png");
        value->addChild(panel);
        panel->setPosition(x + panel->getContentSize().width / 2, 160);
        x += panel->getContentSize().width;
        value->panels_.push_back(panel);
    }
    value->fast_        = rb::sprite("event_fast.png");
    value->fastPressed_ = rb::sprite("event_fast_p.png");
    value->skip_        = rb::sprite("event_skip.png");
    value->skipPressed_ = rb::sprite("event_skip_p.png");
    for (auto* s : {value->fast_, value->fastPressed_})
    {
        value->addChild(s);
        s->setPosition(470 - s->getContentSize().width / 2, 10 + s->getContentSize().height / 2);
    }
    for (auto* s : {value->skip_, value->skipPressed_})
    {
        value->addChild(s);
        s->setPosition(10 + s->getContentSize().width / 2, 10 + s->getContentSize().height / 2);
    }
    value->fastPressed_->setVisible(false);
    value->skipPressed_->setVisible(false);
    value->fade_ = rb::screenShade(Color4B::BLACK);
    value->addChild(value->fade_);
    auto* touch = EventListenerTouchOneByOne::create();
    touch->setSwallowTouches(true);
    touch->onTouchBegan = [value](Touch* t, Event*) {
        if (value->playing_)
        {
            value->setFast(rb::hitTest(value->fast_, t->getLocation()));
            value->skipPressed_->setVisible(rb::hitTest(value->skip_, t->getLocation()));
        }
        return true;
    };
    touch->onTouchEnded = [value](Touch* t, Event*) {
        if (value->playing_ && rb::hitTest(value->skip_, t->getLocation()))
            value->skip();
        value->setFast(false);
        value->skipPressed_->setVisible(false);
    };
    touch->onTouchCancelled = [value](Touch*, Event*) {
        value->setFast(false);
        value->skipPressed_->setVisible(false);
    };
    value->_eventDispatcher->addEventListenerWithSceneGraphPriority(touch, value);
    return value;
}
void EpisodeMovie::open()
{
    if (opened_)
        return;
    opened_ = true;
    fade_->runAction(Sequence::create(FadeOut::create(1), CallFunc::create([this] {
        playing_              = true;
        const char* endings[] = {"EP1_1", "EP3_3", "EP2_3", "EP4_2", "EP5_3", "snd_b1"};
        if (music)
            music(ending_ ? endings[rb::displayedEpisode(episode_) - 1] : "snd_b1");
        schedule([this](float) { step(); }, .05f, "scroll");
    }),
                                      nullptr));
}
void EpisodeMovie::setFast(bool value)
{
    speed_ = value && playing_ ? 10 : 2;
    fastPressed_->setVisible(value && playing_);
}
void EpisodeMovie::step()
{
    if (!playing_)
        return;
    offset_ += speed_;
    for (auto* panel : panels_)
        panel->setPositionX(panel->getPositionX() - speed_);
    if (panels_.back()->getPositionX() < 240)
        skip();
}
void EpisodeMovie::skip()
{
    if (!playing_)
        return;
    playing_ = false;
    unschedule("scroll");
    setFast(false);
    if (music)
        music("");
    fade_->runAction(Sequence::create(FadeIn::create(1), CallFunc::create([this] {
        auto callback = std::move(complete);
        removeFromParent();
        if (callback)
            callback();
    }),
                                      nullptr));
}
