#include "AbilityInfo.h"
#include "NodeFactory.h"
#include "Assets.h"
using namespace ax;
AbilityInfo* AbilityInfo::create(const rb::Progress& progress)
{
    auto* value      = rb::make<AbilityInfo>();
    value->progress_ = &progress;
    value->setup();
    return value;
}
void AbilityInfo::setup()
{
    setContentSize({480, 320});
    const char* names[] = {"eyes", "foots"};
    for (int i = 0; i < 2; ++i)
    {
        buttons_[i] = rb::sprite(std::string("ability_") + names[i] + "_s.png");
        addChild(buttons_[i]);
        const auto size = buttons_[0]->getContentSize();
        buttons_[i]->setPosition(size.width / 2 + (i == 0 ? size.width + 10 : 5), 320 + size.height / 2);
        pressedImages_[i] = rb::sprite(std::string("ability_") + names[i] + "_p.png");
        addChild(pressedImages_[i]);
        pressedImages_[i]->setPosition(buttons_[i]->getPosition());
        pressedImages_[i]->setVisible(false);
        labels_[i] = rb::label("0", 2, 10);
        buttons_[i]->addChild(labels_[i]);
        const auto ls = labels_[i]->getContentSize();
        labels_[i]->setPosition(buttons_[i]->getContentSize().width / 2 - 2 + ls.width / 2,
                                buttons_[i]->getContentSize().height - 24 - ls.height / 2);
    }
    refresh();
    auto* touch = EventListenerTouchOneByOne::create();
    touch->setSwallowTouches(true);
    touch->onTouchBegan     = [this](Touch* t, Event*) { return beginTouch(t->getLocation()); };
    touch->onTouchEnded     = [this](Touch* t, Event*) { endTouch(t->getLocation()); };
    touch->onTouchCancelled = [this](Touch*, Event*) {
        pressed_ = -1;
        for (auto* p : pressedImages_)
            p->setVisible(false);
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(touch, this);
}
void AbilityInfo::refresh()
{
    labels_[0]->setString(std::to_string(progress_->eyes));
    labels_[1]->setString(std::to_string(progress_->feet));
}
void AbilityInfo::show(bool visible)
{
    shown_          = visible;
    pressed_        = -1;
    const auto size = buttons_[0]->getContentSize();
    for (int i = 0; i < 2; ++i)
    {
        pressedImages_[i]->setVisible(false);
        const Vec2 pos(size.width / 2 + (i == 0 ? size.width + 10 : 5),
                       visible ? 320 - size.height / 2 - 7 : 320 + size.height / 2);
        for (auto* node : {buttons_[i], pressedImages_[i]})
        {
            node->stopAllActions();
            node->runAction(EaseIn::create(MoveTo::create(.5f, pos), .25f));
        }
    }
}
bool AbilityInfo::beginTouch(Vec2 point)
{
    pressed_ = -1;
    if (!shown_ || (blocked && blocked()))
        return false;
    for (int i = 0; i < 2; ++i)
        if (rb::hitTest(buttons_[i], point))
        {
            pressed_ = i;
            pressedImages_[i]->setVisible(true);
            return true;
        }
    return false;
}
void AbilityInfo::endTouch(Vec2 point)
{
    const int i = pressed_;
    pressed_    = -1;
    for (auto* p : pressedImages_)
        p->setVisible(false);
    if (i >= 0 && rb::hitTest(buttons_[i], point) && request)
        request(i == 0 ? 0 : 3);
}
