#include "DetailHistoricBook.h"
#include "Assets.h"
using namespace ax;
DetailHistoricBook* DetailHistoricBook::make(const rb::GameData& data, int id, Vec2 origin, int language, int hands)
{
    return createDetail<DetailHistoricBook>(data, id, origin, language, hands);
}
void DetailHistoricBook::setup(const rb::Row&)
{
    board_ = Sprite::create();
    board_->setContentSize({480, 320});
    addChild(board_);
    for (int i = 0; i < 4; ++i)
    {
        auto* p   = rb::sprite("e4_s4_det3_" + std::to_string(i + 1) + ".png");
        pages_[i] = p;
        board_->addChild(p);
        track(p);
        p->setPosition(240 + 480 * i, 160);
        auto* d  = rb::sprite("e4_s4_det3_5.png");
        dots_[i] = d;
        board_->addChild(d);
        track(d);
        d->setPosition(210 + 20 * i + d->getContentSize().width / 2, 160 - p->getContentSize().height / 2);
    }
    selected_ = rb::sprite("e4_s4_det3_6.png");
    board_->addChild(selected_);
    track(selected_);
    selected_->setPosition(dots_[0]->getPosition());
}
Vec2 DetailHistoricBook::controlCenter(int) const
{
    return pages_[page_]->convertToWorldSpace(pages_[page_]->getContentSize() / 2);
}
void DetailHistoricBook::press(Vec2 p)
{
    last_    = p;
    pressed_ = false;
    for (auto* s : pages_)
        if (contains(s, p))
        {
            pressed_ = true;
            break;
        }
    if (pressed_)
        for (auto* s : pages_)
            s->stopAllActions();
}
void DetailHistoricBook::move(Vec2 p)
{
    if (pressed_)
    {
        for (auto* s : pages_)
            s->setPositionX(s->getPositionX() + p.x - last_.x);
        float x = pages_[page_]->getPositionX();
        if (x < 190)
        {
            if (page_ < 3)
                ++page_;
            sort();
            pressed_ = false;
        }
        else if (x > 290)
        {
            if (page_ > 0)
                --page_;
            sort();
            pressed_ = false;
        }
    }
    last_ = p;
}
void DetailHistoricBook::sort()
{
    for (int i = 0; i < 4; ++i)
    {
        pages_[i]->stopAllActions();
        pages_[i]->runAction(EaseIn::create(MoveTo::create(.5f, {240.f + 480 * (i - page_), 160}), .25f));
    }
    selected_->setPosition(dots_[page_]->getPosition());
}
void DetailHistoricBook::release(Vec2)
{
    sort();
    pressed_ = false;
}
void DetailHistoricBook::cancel()
{
    pressed_ = false;
}
