#include "StageIntro.h"
#include "NodeFactory.h"
#include "Assets.h"
#include <stdexcept>
using namespace ax;
StageIntro* StageIntro::create(int stage)
{
    auto* value = rb::make<StageIntro>();
    value->setContentSize({480, 320});
    value->shade_ = LayerColor::create(Color4B(0, 0, 0, 0), 480, 320);
    value->addChild(value->shade_);
    for (int i = 0; i < 2; ++i)
    {
        value->bars_[i] = LayerColor::create(Color4B(0, 0, 0, 150), 480, 1);
        value->addChild(value->bars_[i]);
        value->bars_[i]->setPosition(i == 0 ? -480 : 720, i == 0 ? 166 : 182);
    }
    const char* title = "ROOM";
    for (int i = 0; i < 4; ++i)
    {
        auto* label = rb::label(std::string(1, title[i]), 2, 15);
        label->setDimensions(15, 0);
        label->setAlignment(TextHAlignment::CENTER);
        value->letters_[i] = label;
        if (i == 0)
        {
            value->addChild(label);
            label->setPosition(190, 175);
        }
        else
        {
            value->letters_[i - 1]->addChild(label);
            label->setAnchorPoint(Vec2::ZERO);
            label->setPosition(Vec2::ZERO);
        }
        label->setOpacity(0);
    }
    value->number_ = rb::label(std::to_string(stage), 2, 25);
    value->number_->setDimensions(50, 0);
    value->number_->setAlignment(TextHAlignment::CENTER);
    value->addChild(value->number_);
    value->number_->setPosition(250, 176);
    value->number_->setOpacity(0);
    auto* touch = EventListenerTouchOneByOne::create();
    touch->setSwallowTouches(true);
    touch->onTouchBegan = [](Touch*, Event*) { return true; };
    value->_eventDispatcher->addEventListenerWithSceneGraphPriority(touch, value);
    return value;
}
void StageIntro::start()
{
    if (started_)
        throw std::runtime_error("Stage intro already started");
    started_ = true;
    if (sound)
        sound("snd_f05");
    bars_[0]->runAction(EaseIn::create(MoveTo::create(2.5f, {-190, 166}), .25f));
    bars_[1]->runAction(EaseIn::create(MoveTo::create(2.5f, {190, 182}), .25f));
    shade_->runAction(FadeTo::create(1, 150));
    schedule([this](float) { letter(); }, .3f, "room-letters");
}
void StageIntro::letter()
{
    auto* label  = letters_.at(step_++);
    auto* action = Spawn::create(EaseIn::create(MoveBy::create(.5f, {15, 0}), .25f), FadeIn::create(.5f), nullptr);
    if (step_ == 4)
    {
        unschedule("room-letters");
        label->runAction(Sequence::create(action, CallFunc::create([this] { number(); }), nullptr));
    }
    else
        label->runAction(action);
}
void StageIntro::number()
{
    number_->runAction(
        Sequence::create(Spawn::create(EaseIn::create(MoveTo::create(1, {270, 176}), .25f), FadeIn::create(1), nullptr),
                         DelayTime::create(1), CallFunc::create([this] { close(); }), nullptr));
}
void StageIntro::close()
{
    bars_[0]->runAction(EaseIn::create(MoveTo::create(1.5f, {-480, 166}), .25f));
    bars_[1]->runAction(EaseIn::create(MoveTo::create(1.5f, {720, 182}), .25f));
    shade_->runAction(FadeOut::create(1));
    for (auto* label : letters_)
        label->runAction(FadeOut::create(1));
    number_->runAction(Sequence::create(FadeOut::create(1.5f), CallFunc::create([this] {
        auto callback = std::move(complete);
        if (callback)
            callback();
    }),
                                        nullptr));
}
