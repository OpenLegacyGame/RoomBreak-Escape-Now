#include "DetailElevator.h"
#include "SheetSprite.h"
#include <stdexcept>
using namespace ax;
DetailElevator* DetailElevator::make(const rb::GameData& data, int id, Vec2 origin, int language, int hands)
{
    return createDetail<DetailElevator>(data, id, origin, language, hands);
}
void DetailElevator::setup(const rb::Row&)
{
    makeBoard("e5_s5_det1_1.png");
    for (int i = 0; i < 10; ++i)
        for (int down = 0; down < 2; ++down)
        {
            auto* s = SheetSprite::create(down ? "e5_s5_det1_3.png" : "e5_s5_det1_2.png", 10, 1);
            s->frame(i);
            board_->addChild(s);
            if (!down)
                track(s);
            place(board_, s, {float(96 + 45 * (i % 5)), float(14 + 45 * (i / 5))});
            (down ? pressed_[i] : keys_[i]) = s;
            if (down)
                s->setVisible(false);
        }
}
Vec2 DetailElevator::controlCenter(int i) const
{
    if (i < 0 || i >= 10)
        throw std::runtime_error("Invalid elevator key");
    return keys_[i]->convertToWorldSpace(keys_[i]->getContentSize() / 2);
}
void DetailElevator::press(Vec2 p)
{
    selected_ = -1;
    for (int i = 0; i < 10; ++i)
        if (contains(keys_[i], p))
        {
            selected_ = i;
            pressed_[i]->setVisible(true);
            break;
        }
}
void DetailElevator::cancel()
{
    selected_ = -1;
    for (auto* s : pressed_)
        s->setVisible(false);
}
void DetailElevator::release(Vec2 p)
{
    int i = selected_;
    cancel();
    if (i >= 0 && contains(keys_[i], p))
        enter(i);
}
void DetailElevator::enter(int i)
{
    input_.push_back(i);
    auto* s = SheetSprite::create("e5_s5_det1_4.png", 10, 1);
    s->frame(i);
    board_->addChild(s);
    digits_.push_back(s);
    place(board_, s, {float(25 + 31 * (digits_.size() - 1)), 44});
    playSound("snd_p06");
    if (input_.size() == 2 && !busy_)
    {
        busy_ = true;
        if (input_ == std::vector<int>{2, 8})
            solve();
        else
        {
            for (auto* d : digits_)
                d->runAction(Sequence::create(
                    Repeat::create(Sequence::create(FadeOut::create(.1f), FadeIn::create(.1f), nullptr), 3),
                    FadeOut::create(.3f), nullptr));
            scheduleOnce([this](float) {
                retry();
                busy_ = false;
            }, .9f, "elevator-reset");
            playSound("snd_p29");
        }
    }
}
void DetailElevator::retry()
{
    for (auto* s : digits_)
        s->removeFromParent();
    digits_.clear();
    input_.clear();
}
void DetailElevator::solve()
{
    if (solved_ || closing_)
        return;
    solved_ = true;
    success();
    retry();
    enter(2);
    enter(8);
}
