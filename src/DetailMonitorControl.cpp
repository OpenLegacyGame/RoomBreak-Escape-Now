#include "DetailMonitorControl.h"
#include "SheetSprite.h"
#include "Assets.h"
using namespace ax;
DetailMonitorControl* DetailMonitorControl::make(const rb::GameData& data, int id, Vec2 origin, int language, int hands)
{
    return createDetail<DetailMonitorControl>(data, id, origin, language, hands);
}
void DetailMonitorControl::setup(const rb::Row&)
{
    makeBoard("e1_s4_det4_1.png");
    for (int i = 0; i < 3; ++i)
    {
        auto* normal = SheetSprite::create("e1_s4_det4_5.png", 3, 1);
        normal->frame(i);
        board_->addChild(normal);
        place(board_, normal, {135 + i * normal->getContentSize().width, 167});
        track(normal);
        auto* marker = SheetSprite::create("e1_s4_det4_6.png", 3, 1);
        marker->frame(i);
        markers_[i] = marker;
        board_->addChild(marker);
        marker->setPosition(normal->getPosition());
        track(marker);
        marker->setOpacity(i == 0 ? 255 : 0);
    }
    for (int i = 0; i < 3; ++i)
    {
        views_[i] = addImage("e1_s4_det4_" + std::to_string(i + 2) + ".png", 87, 27);
        views_[i]->setOpacity(i == 0 ? 255 : 0);
    }
    for (int i = 0; i < 2; ++i)
    {
        keys_[i]    = addImage(i == 0 ? "e1_s4_det4_9.png" : "e1_s4_det4_7.png", i == 0 ? 34 : 236, 79);
        pressed_[i] = rb::sprite(i == 0 ? "e1_s4_det4_10.png" : "e1_s4_det4_8.png");
        board_->addChild(pressed_[i]);
        pressed_[i]->setPosition(keys_[i]->getPosition());
        pressed_[i]->setVisible(false);
    }
}
Vec2 DetailMonitorControl::controlCenter(int i) const
{
    return keys_.at(i)->convertToWorldSpace(keys_.at(i)->getContentSize() / 2);
}
void DetailMonitorControl::press(Vec2 p)
{
    selected_ = -1;
    for (int i = 0; i < 2; ++i)
        if (contains(keys_[i], p))
        {
            selected_ = i;
            pressed_[i]->setVisible(true);
            break;
        }
}
void DetailMonitorControl::cancel()
{
    selected_ = -1;
    for (auto* s : pressed_)
        s->setVisible(false);
}
void DetailMonitorControl::release(Vec2 p)
{
    int i = selected_;
    cancel();
    if (i >= 0 && contains(keys_[i], p))
        select((page_ + (i == 0 ? 2 : 1)) % 3);
}
void DetailMonitorControl::select(int page)
{
    page_ = page;
    for (int i = 0; i < 3; ++i)
    {
        views_[i]->setVisible(true);
        markers_[i]->setVisible(true);
        views_[i]->runAction(FadeTo::create(.3f, 0));
        markers_[i]->runAction(FadeTo::create(.3f, 0));
    }
    views_[page]->runAction(FadeTo::create(.3f, 255));
    markers_[page]->runAction(FadeTo::create(.3f, 255));
}
