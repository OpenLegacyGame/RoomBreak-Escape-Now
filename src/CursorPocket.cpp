#include "CursorPocket.h"
#include "NodeFactory.h"
#include "Assets.h"
#include <stdexcept>
using namespace ax;
CursorPocket* CursorPocket::create(const rb::GameData& data)
{
    auto* value  = rb::make<CursorPocket>();
    value->data_ = &data;
    value->setup();
    return value;
}
void CursorPocket::setup()
{
    setContentSize({480, 320});
    box_ = rb::sprite("cur_box.png");
    addChild(box_);
    const auto size = box_->getContentSize();
    box_->setPosition(480 + size.width / 2, 270 - size.height / 2);
    pressedBox_ = rb::sprite("cur_box_p.png");
    box_->addChild(pressedBox_);
    pressedBox_->setAnchorPoint(Vec2::ZERO);
    pressedBox_->setVisible(false);
    underbar_ = rb::sprite("cur_underbar.png");
    box_->addChild(underbar_);
    underbarPosition_ = {underbar_->getContentSize().width / 2, -underbar_->getContentSize().height / 2};
    underbar_->setPosition(underbarPosition_);
    underbar_->setOpacity(0);
    const char* names[] = {"hands", "search", "foots", "item"};
    for (int i = 0; i < 4; ++i)
    {
        choices_[i] = rb::sprite(std::string("cur_") + names[i] + "_p.png");
        box_->addChild(choices_[i]);
        const auto cs = choices_[i]->getContentSize();
        positions_[i] = {cs.width / 2, size.height - cs.height / 2 - 37 - 42 * i};
        choices_[i]->setPosition(positions_[i]);
        choices_[i]->setOpacity(0);
    }
    const auto cell = choices_[0]->getContentSize();
    for (int i = 0; i < 3; ++i)
    {
        selectedIcons_[i] = rb::sprite(std::string("cur_") + names[i] + "_s.png");
        box_->addChild(selectedIcons_[i]);
        selectedIcons_[i]->setPosition(cell.width / 2, cell.height / 2 - 3);
        selectedIcons_[i]->setOpacity(0);
    }
    select(0);
    auto* touch = EventListenerTouchOneByOne::create();
    touch->setSwallowTouches(true);
    touch->onTouchBegan = [this](Touch* t, Event*) {
        if (!shown_ || (blocked && blocked()))
            return false;
        pressed_ = hit(t->getLocation());
        pressedBox_->setVisible(pressed_ == 4);
        return pressed_ >= 0;
    };
    touch->onTouchEnded = [this](Touch* t, Event*) {
        const int release = hit(t->getLocation()), pressed = pressed_;
        pressed_ = -1;
        pressedBox_->setVisible(false);
        if (pressed < 0 || release != pressed)
            return;
        if (!opened_)
        {
            open();
            return;
        }
        if (release < 3 || (release == 3 && item_))
            select(release);
        close();
    };
    touch->onTouchCancelled = [this](Touch*, Event*) {
        pressed_ = -1;
        pressedBox_->setVisible(false);
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(touch, this);
}
int CursorPocket::hit(Vec2 point) const
{
    auto inside = [point](Sprite* s) { return rb::hitTest(s, point); };
    if (opened_)
        for (int i = 0; i < 4; ++i)
            if (inside(choices_[i]))
                return i;
    return inside(box_) ? 4 : -1;
}
void CursorPocket::show(bool visible)
{
    shown_ = visible;
    box_->stopAllActions();
    const auto size = box_->getContentSize();
    box_->runAction(EaseIn::create(
        MoveTo::create(.5f, {visible ? 465 - size.width / 2 : 480 + size.width / 2, 270 - size.height / 2}), .25f));
}
void CursorPocket::open()
{
    if (opened_)
        return;
    opened_ = true;
    underbar_->stopAllActions();
    underbar_->setPosition(underbarPosition_ + Vec2(0, 20));
    underbar_->runAction(
        Spawn::create(EaseIn::create(MoveTo::create(.3f, underbarPosition_), .25f), FadeIn::create(.3f), nullptr));
    for (int i = 0; i < 4; ++i)
    {
        choices_[i]->stopAllActions();
        choices_[i]->setPosition(positions_[i] + Vec2(0, 20));
        choices_[i]->runAction(Spawn::create(MoveTo::create(.3f, positions_[i]), FadeIn::create(.3f), nullptr));
    }
    if (item_)
    {
        if (itemCell_)
            itemCell_->removeFromParent();
        itemCell_ = makeItem();
        box_->addChild(itemCell_);
        itemCell_->setPosition(positions_[3] + Vec2(0, 20));
        itemCell_->runAction(Spawn::create(MoveTo::create(.3f, positions_[3]), FadeIn::create(.3f), nullptr));
    }
}
void CursorPocket::close()
{
    if (!opened_)
        return;
    opened_ = false;
    underbar_->stopAllActions();
    underbar_->runAction(Spawn::create(EaseIn::create(MoveTo::create(.3f, underbarPosition_ + Vec2(0, 20)), .25f),
                                       FadeOut::create(.3f), nullptr));
    for (int i = 0; i < 4; ++i)
    {
        choices_[i]->stopAllActions();
        choices_[i]->runAction(
            Spawn::create(MoveTo::create(.3f, positions_[i] + Vec2(0, 20)), FadeOut::create(.3f), nullptr));
    }
    if (itemCell_)
    {
        itemCell_->stopAllActions();
        itemCell_->runAction(
            Spawn::create(MoveTo::create(.3f, positions_[3] + Vec2(0, 20)), FadeOut::create(.3f), nullptr));
    }
}
Sprite* CursorPocket::makeItem()
{
    auto* icon = rb::sprite(rb::string(data_->byId("ItemData", "itemID", item_), "imageName"));
    icon->setScale(.2f);
    return icon;
}
void CursorPocket::select(int cursor)
{
    if (cursor < 0 || cursor > 3 || (cursor == 3 && !item_))
        throw std::runtime_error("Invalid pocket cursor");
    selected_ = cursor;
    for (auto* icon : selectedIcons_)
        if (icon)
        {
            icon->stopAllActions();
            icon->setOpacity(0);
        }
    if (cursor == 3)
    {
        if (selectedIcons_[3])
            selectedIcons_[3]->removeFromParent();
        selectedIcons_[3] = makeItem();
        box_->addChild(selectedIcons_[3]);
        const auto size = choices_[0]->getContentSize();
        selectedIcons_[3]->setPosition(size.width / 2, size.height / 2 - 3);
        selectedIcons_[3]->setOpacity(0);
    }
    selectedIcons_[cursor]->runAction(FadeIn::create(.3f));
    if (changed)
        changed(cursor, cursor == 3 ? item_ : 0);
}
void CursorPocket::setItem(int id)
{
    data_->byId("ItemData", "itemID", id);
    item_ = id;
    select(3);
}
void CursorPocket::consumeItem()
{
    for (auto** icon : {&itemCell_, &selectedIcons_[3]})
        if (*icon)
        {
            (*icon)->removeFromParent();
            *icon = nullptr;
        }
    item_ = 0;
    select(0);
}
