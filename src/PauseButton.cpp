#include "PauseButton.h"
#include "NodeFactory.h"
#include "Assets.h"
using namespace ax;
PauseButton* PauseButton::create()
{
    auto* value          = rb::make<PauseButton>();
    value->button_       = rb::sprite("ui_pause.png");
    value->pressedImage_ = rb::sprite("ui_pause_p.png");
    const auto size      = value->button_->getContentSize();
    for (auto* node : {value->button_, value->pressedImage_})
    {
        value->addChild(node);
        node->setPosition(480 + size.width / 2, 320 - size.height / 2);
    }
    value->pressedImage_->setVisible(false);
    auto* touch = EventListenerTouchOneByOne::create();
    touch->setSwallowTouches(true);
    touch->onTouchBegan     = [value](Touch* t, Event*) { return value->beginTouch(t->getLocation()); };
    touch->onTouchEnded     = [value](Touch* t, Event*) { value->endTouch(t->getLocation()); };
    touch->onTouchCancelled = [value](Touch*, Event*) {
        value->pressed_ = false;
        value->pressedImage_->setVisible(false);
    };
    value->_eventDispatcher->addEventListenerWithSceneGraphPriority(touch, value);
    return value;
}
void PauseButton::show(bool visible)
{
    shown_   = visible;
    pressed_ = false;
    pressedImage_->setVisible(false);
    const auto size = button_->getContentSize();
    const Vec2 pos(visible ? 480 - size.width / 2 - 5 : 480 + size.width / 2, 320 - size.height / 2);
    for (auto* node : {button_, pressedImage_})
    {
        node->stopAllActions();
        node->runAction(EaseIn::create(MoveTo::create(.5f, pos), .25f));
    }
}
bool PauseButton::beginTouch(Vec2 point)
{
    pressed_ = false;
    if (!shown_ || (blocked && blocked()))
        return false;
    pressed_ = rb::hitTest(button_, point);
    pressedImage_->setVisible(pressed_);
    return pressed_;
}
void PauseButton::endTouch(Vec2 point)
{
    const bool hit = pressed_ && rb::hitTest(button_, point);
    pressed_       = false;
    pressedImage_->setVisible(false);
    if (hit && activate)
        activate();
}
