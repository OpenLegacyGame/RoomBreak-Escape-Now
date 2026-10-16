#include "InsightOverlay.h"
#include "NodeFactory.h"
#include "GameSession.h"
#include "RoomView.h"
#include "Assets.h"
#include <algorithm>
#include <stdexcept>
using namespace ax;
InsightOverlay* InsightOverlay::create(const rb::GameData& data, GameSession& session, int item, int language)
{
    auto* value = rb::make<InsightOverlay>();
    value->setContentSize({480, 320});
    value->data_         = &data;
    value->session_      = &session;
    value->item_         = item;
    value->originalRoom_ = session.state().room;
    for (auto* row : data.where("ItemUse", "itemID", item))
        value->targets_.push_back(rb::integer(*row, "useObjectID"));
    value->shade_ = LayerColor::create(Color4B(0, 0, 0, 0), 480, 320);
    value->addChild(value->shade_);
    value->effect_ = ParticleSystemQuad::create("par_ability_insight.plist");
    value->marker_ = ParticleSystemQuad::create("par_ability_itempos.plist");
    if (!value->effect_ || !value->marker_)
        throw std::runtime_error("Missing insight particle");
    for (auto* p : {value->effect_, value->marker_})
    {
        value->addChild(p);
        p->stopSystem();
        p->setVisible(false);
    }
    value->effect_->setPosition(240, 160);
    const char* suffix = language == 1 ? "kor" : language == 3 ? "jap" : "eng";
    value->info_       = rb::sprite(std::string("think_bg_") + suffix + ".png");
    value->addChild(value->info_);
    value->info_->setPosition(240, 160);
    value->info_->setOpacity(0);
    auto* touch = EventListenerTouchOneByOne::create();
    touch->setSwallowTouches(true);
    touch->onTouchBegan = [](Touch*, Event*) { return true; };
    touch->onTouchEnded = [value](Touch*, Event*) { value->close(); };
    value->_eventDispatcher->addEventListenerWithSceneGraphPriority(touch, value);
    return value;
}
void InsightOverlay::start()
{
    if (started_)
        throw std::runtime_error("Insight already started");
    started_ = true;
    if (sound)
        sound("snd_f46");
    shade_->runAction(Sequence::create(FadeTo::create(1, 150), CallFunc::create([this] {
        effect_->setVisible(true);
        effect_->resetSystem();
    }),
                                       DelayTime::create(2), FadeTo::create(.5f, 0),
                                       CallFunc::create([this] { nextTarget(); }), nullptr));
}
void InsightOverlay::nextTarget()
{
    markedObject_ = 0;
    if (target_ == targets_.size())
    {
        showInfo();
        return;
    }
    const auto& object = data_->byId("ObjectData", "objectID", targets_[target_]);
    const int room     = rb::integer(object, "roomID");
    if (session_->state().room != room)
    {
        session_->moveRoom(room, 0, [this] { nextTarget(); });
        return;
    }
    auto* bg         = session_->room()->background();
    const auto point = session_->room()->objectPosition(targets_[target_]);
    const float x    = std::clamp(240 - point.x, 480 - bg->getContentSize().width, 0.f);
    if (bg->getPositionX() == x)
    {
        mark();
        return;
    }
    bg->runAction(Sequence::create(EaseIn::create(MoveTo::create(.5f, {x, bg->getPositionY()}), .25f),
                                   CallFunc::create([this] { mark(); }), nullptr));
}
void InsightOverlay::mark()
{
    markedObject_ = targets_.at(target_);
    marker_->setPosition(session_->room()->background()->getPosition() +
                         session_->room()->objectPosition(markedObject_));
    marker_->setVisible(true);
    marker_->resetSystem();
    scheduleOnce([this](float) {
        marker_->setVisible(false);
        ++target_;
        nextTarget();
    }, 1.5f, "insight-target");
}
void InsightOverlay::showInfo()
{
    const auto& row = data_->byId("ItemData", "itemID", item_);
    auto* item      = rb::place(info_, rb::string(row, "imageName"), -25, -10);
    item->setScale(.55f);
    fading_.push_back(item);
    const bool flags[] = {!data_->where("ItemUse", "itemID", item_).empty(),
                          !data_->where("ItemCombine", "itemID", item_).empty(), !rb::string(row, "separate").empty()};
    const float xs[]   = {109, 162, 220};
    for (int i = 0; i < 3; ++i)
        fading_.push_back(rb::place(info_, flags[i] ? "think_yes.png" : "think_no.png", xs[i], 47));
    fading_.push_back(info_);
    for (auto* node : fading_)
    {
        node->setOpacity(0);
        node->runAction(EaseOut::create(FadeIn::create(.5f), .25f));
    }
    scheduleOnce([this](float) { ready_ = true; }, .5f, "insight-info");
}
void InsightOverlay::close()
{
    if (!ready_)
        return;
    ready_ = false;
    for (auto* node : fading_)
        node->runAction(EaseOut::create(FadeOut::create(.5f), .25f));
    scheduleOnce([this](float) {
        if (originalRoom_ != session_->state().room)
            session_->moveRoom(originalRoom_, 0, [this] { finish(); });
        else
            finish();
    }, .5f, "insight-close");
}
void InsightOverlay::finish()
{
    auto callback = std::move(complete);
    if (callback)
        callback();
}
