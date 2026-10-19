#include "Viewport.h"
#include "ClearOverlay.h"
#include "NodeFactory.h"
#include "Assets.h"
using namespace ax;
ClearOverlay* ClearOverlay::create(std::string rank)
{
    auto* value   = rb::make<ClearOverlay>();
    value->rank_  = std::move(rank);
    value->shade_ = rb::screenShade(Color4B(0, 0, 0, 0));
    value->addChild(value->shade_);
    value->bg_ = rb::sprite("clear_bg-1.png");
    value->addChild(value->bg_);
    value->bg_->setPosition(240, 195);
    value->bg_->setOpacity(0);
    value->buttons_[0]       = rb::sprite("det_retry-1.png");
    value->buttons_[1]       = rb::sprite("gameover_ok.png");
    value->pressedImages_[0] = rb::sprite("det_retry_p-1.png");
    value->pressedImages_[1] = rb::sprite("gameover_ok_p.png");
    for (int i = 0; i < 2; ++i)
    {
        auto* button = value->buttons_[i];
        value->bg_->addChild(button);
        const auto size = button->getContentSize();
        button->setPosition(value->bg_->getContentSize().width / 2 + (i == 0 ? -1 : 1) * (size.width / 2 + 5),
                            value->bg_->getContentSize().height - 115 - size.height / 2);
        button->setOpacity(0);
        value->bg_->addChild(value->pressedImages_[i]);
        value->pressedImages_[i]->setPosition(button->getPosition());
        value->pressedImages_[i]->setVisible(false);
    }
    auto* touch = EventListenerTouchOneByOne::create();
    touch->setSwallowTouches(true);
    touch->onTouchBegan = [value](Touch* t, Event*) {
        value->pressed_ = value->ready_ ? value->hit(t->getLocation()) : -1;
        if (value->pressed_ >= 0)
            value->pressedImages_[value->pressed_]->setVisible(true);
        return true;
    };
    touch->onTouchEnded = [value](Touch* t, Event*) {
        const int i     = value->pressed_;
        value->pressed_ = -1;
        for (auto* p : value->pressedImages_)
            p->setVisible(false);
        if (i >= 0 && value->hit(t->getLocation()) == i)
            value->choose(i == 1);
    };
    touch->onTouchCancelled = [value](Touch*, Event*) {
        value->pressed_ = -1;
        for (auto* p : value->pressedImages_)
            p->setVisible(false);
    };
    value->_eventDispatcher->addEventListenerWithSceneGraphPriority(touch, value);
    return value;
}
int ClearOverlay::hit(Vec2 p) const
{
    for (int i = 0; i < 2; ++i)
        if (rb::hitTest(buttons_[i], p))
            return i;
    return -1;
}
void ClearOverlay::open()
{
    if (opened_)
        return;
    opened_ = true;
    if (sound)
        sound("snd_f06");
    shade_->runAction(Sequence::create(FadeTo::create(1, 150), CallFunc::create([this] {
        bg_->runAction(Sequence::create(EaseIn::create(FadeIn::create(.5f), .25f), CallFunc::create([this] {
            rankImage_ = rb::sprite("clear_" + rank_ + "-1.png");
            bg_->addChild(rankImage_);
            const auto size = rankImage_->getContentSize();
            rankImage_->setPosition(175 + size.width / 2, bg_->getContentSize().height - 20 - size.height / 2);
            rankImage_->setOpacity(0);
            rankImage_->setScale(3);
            rankImage_->runAction(
                Sequence::create(Spawn::create(FadeIn::create(.2f), ScaleTo::create(.2f, .9f), nullptr),
                                 ScaleTo::create(.1f, 1), CallFunc::create([this] {
                for (auto* button : buttons_)
                    button->runAction(EaseIn::create(FadeIn::create(.5f), .25f));
                scheduleOnce([this](float) { ready_ = true; }, .5f, "ready");
            }),
                                 nullptr));
        }),
                                        nullptr));
    }),
                                       nullptr));
}
void ClearOverlay::choose(bool exitToMenu)
{
    if (!ready_)
        return;
    ready_ = false;
    if (sound)
        sound("snd_f19");
    auto* fade = rb::screenShade(Color4B(0, 0, 0, 0));
    addChild(fade, 10);
    fade->runAction(Sequence::create(FadeIn::create(.5f), CallFunc::create([this, exitToMenu] {
        auto callback = std::move(complete);
        removeFromParent();
        if (callback)
            callback(exitToMenu);
    }),
                                     nullptr));
}
