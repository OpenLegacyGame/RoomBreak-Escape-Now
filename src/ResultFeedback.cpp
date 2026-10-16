#include "ResultFeedback.h"
#include "NodeFactory.h"
#include "Assets.h"
#include "SheetSprite.h"
using namespace ax;
ResultFeedback* ResultFeedback::create(bool success)
{
    auto* value     = rb::make<ResultFeedback>();
    value->success_ = success;
    if (success)
    {
        value->bar_ = LayerColor::create(Color4B(0, 0, 0, 0), 480, 30);
        value->addChild(value->bar_);
        value->bar_->setPosition(0, 145);
        value->title_ = rb::sprite("res_success.png");
        value->addChild(value->title_);
        value->title_->setPosition(240, 160);
        value->title_->setOpacity(0);
    }
    for (int i = 0; i < 15; ++i)
    {
        auto* slice = SheetSprite::create(success ? "res_success_shinee.png" : "res_failed.png", 15, 1);
        slice->frame(i);
        value->addChild(slice);
        value->slices_.push_back(slice);
        const float width = slice->getContentSize().width;
        slice->setPosition(240 - width * 7.5f + width * i + (success ? width / 2 : 0), success ? 160 : 130);
        slice->setOpacity(0);
    }
    return value;
}
void ResultFeedback::play()
{
    if (played_)
        return;
    played_ = true;
    if (sound)
        sound(success_ ? "snd_f28" : "snd_f29");
    if (success_)
    {
        bar_->runAction(
            Sequence::create(FadeTo::create(.5f, 150), DelayTime::create(2.5f), FadeOut::create(.5f), nullptr));
        title_->runAction(Sequence::create(FadeIn::create(.5f), DelayTime::create(2.5f), FadeOut::create(.5f),
                                           CallFunc::create([this] { finish(); }), nullptr));
        for (int i = 0; i < 15; ++i)
            slices_[i]->runAction(
                Sequence::create(DelayTime::create(.5f + i * .1f), FadeIn::create(.5f), FadeOut::create(.5f), nullptr));
    }
    else
    {
        for (int i = 0; i < 15; ++i)
        {
            auto* slice  = slices_[i];
            const auto p = slice->getPosition();
            slice->runAction(Sequence::create(
                DelayTime::create(i * .1f),
                Spawn::create(EaseIn::create(MoveTo::create(.5f, p + Vec2(0, 30)), .25f), FadeIn::create(.5f), nullptr),
                DelayTime::create(1.5f),
                Spawn::create(EaseOut::create(MoveTo::create(.5f, p + Vec2(0, 60)), .25f), FadeOut::create(.5f),
                              nullptr),
                nullptr));
        }
        scheduleOnce([this](float) { finish(); }, 4, "finish");
    }
}
void ResultFeedback::finish()
{
    auto callback = std::move(complete);
    removeFromParent();
    if (callback)
        callback();
}
