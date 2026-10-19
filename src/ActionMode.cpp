#include "Viewport.h"
#include "ActionMode.h"
#include "Assets.h"
#include "ResultFeedback.h"
using namespace ax;
bool ActionMode::initialize(int level, int hands)
{
    if (!Node::init())
        return false;
    setContentSize({480, 320});
    shade_ = rb::screenShade(Color4B(0, 0, 0, 0));
    addChild(shade_);
    setup(level);
    auto* controls = Node::create();
    addChild(controls, 100);
    rb::anchorToSafeArea(controls, 1, 1);
    const char* files[]   = {"ability_short_hands.png", "det_retry-1.png", "det_close-1.png"};
    const char* pressed[] = {"ability_short_hands_p.png", "det_retry_p-1.png", "det_close_p-1.png"};
    const float tops[]    = {118, 172, 226};
    for (int i = 0; i < 3; ++i)
    {
        menus_[i]         = rb::sprite(files[i]);
        pressedImages_[i] = rb::sprite(pressed[i]);
        for (auto* s : {menus_[i], pressedImages_[i]})
        {
            controls->addChild(s);
            s->setPosition(480 + s->getContentSize().width / 2, 320 - tops[i] - s->getContentSize().height / 2);
        }
        pressedImages_[i]->setVisible(false);
    }
    count_ = rb::label(std::to_string(hands), 2, 10);
    menus_[0]->addChild(count_);
    const auto size = count_->getContentSize();
    count_->setPosition(menus_[0]->getContentSize().width / 2 - 2 + size.width / 2,
                        menus_[0]->getContentSize().height - 33 - size.height / 2);
    auto* touch = EventListenerTouchOneByOne::create();
    touch->setSwallowTouches(true);
    touch->onTouchBegan = [this](Touch* t, Event*) {
        beginTouch(t->getLocation());
        return true;
    };
    touch->onTouchMoved     = [this](Touch* t, Event*) { moveTouch(t->getLocation()); };
    touch->onTouchEnded     = [this](Touch* t, Event*) { endTouch(t->getLocation()); };
    touch->onTouchCancelled = [this](Touch*, Event*) {
        pressed_ = -1;
        for (auto* p : pressedImages_)
            p->setVisible(false);
        cancelGesture();
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(touch, this);
    return true;
}
bool ActionMode::contains(Node* n, Vec2 p)
{
    return rb::hitTest(n, p);
}
void ActionMode::setHands(int value)
{
    count_->setString(std::to_string(value));
}
void ActionMode::open()
{
    if (opened_)
        return;
    opened_ = true;
    if (gameUI)
        gameUI(false);
    shade_->runAction(FadeTo::create(1, 150));
    start();
}
void ActionMode::showMenu(bool show)
{
    menuVisible_ = show;
    for (int i = 0; i < 3; ++i)
        for (auto* n : {menus_[i], pressedImages_[i]})
        {
            n->stopAllActions();
            n->runAction(EaseIn::create(
                MoveTo::create(.5f, {show ? 465 - n->getContentSize().width / 2 : 480 + n->getContentSize().width / 2,
                                     n->getPositionY()}),
                .25f));
        }
}
void ActionMode::beginTouch(Vec2 p)
{
    pressed_ = -1;
    if (closing_ || (blocked && blocked()))
        return;
    if (menuVisible_)
        for (int i = 0; i < 3; ++i)
            if (contains(menus_[i], p))
            {
                pressed_ = i;
                pressedImages_[i]->setVisible(true);
                return;
            }
    if (!busy_)
        press(p);
}
void ActionMode::moveTouch(Vec2 p)
{
    if (!closing_ && !busy_ && pressed_ < 0 && !(blocked && blocked()))
        move(p);
}
void ActionMode::endTouch(Vec2 p)
{
    const int i = pressed_;
    pressed_    = -1;
    for (auto* s : pressedImages_)
        s->setVisible(false);
    if (closing_ || (blocked && blocked()))
        return;
    if (i >= 0)
    {
        if (!contains(menus_[i], p))
            return;
        if (i == 0)
        {
            if (handsRequested)
                handsRequested();
        }
        else if (i == 1)
            retry();
        else
            close();
        return;
    }
    if (!busy_)
        release(p);
}
void ActionMode::fail()
{
    if (closing_)
        return;
    busy_ = true;
    cancelGesture();
    auto* result = ResultFeedback::create(false);
    addChild(result, 99);
    result->sound    = [this](std::string_view name) { playSound(name); };
    result->complete = [this] {
        if (!closing_)
        {
            busy_ = false;
            showMenu(true);
        }
    };
    result->play();
}
void ActionMode::solve()
{
    if (closing_)
        return;
    busy_ = true;
    cancelGesture();
    showMenu(false);
    auto* result = ResultFeedback::create(true);
    addChild(result, 99);
    result->sound    = [this](std::string_view name) { playSound(name); };
    result->complete = [this] {
        success_ = true;
        close();
    };
    result->play();
}
void ActionMode::retry()
{
    if (closing_)
        return;
    cancelGesture();
    showMenu(false);
    start();
}
void ActionMode::close()
{
    if (closing_)
        return;
    closing_ = true;
    busy_    = true;
    cancelGesture();
    hideContent();
    showMenu(false);
    if (gameUI)
        gameUI(true);
    shade_->stopAllActions();
    shade_->runAction(FadeOut::create(1));
    scheduleOnce([this](float) {
        auto callback   = std::move(complete);
        const bool pass = success_;
        removeFromParent();
        if (callback)
            callback(pass);
    }, 1, "action-close");
}
