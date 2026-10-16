#include "DetailHistoricSafe.h"
#include "SheetSprite.h"
#include <cmath>
using namespace ax;
DetailHistoricSafe* DetailHistoricSafe::make(const rb::GameData& data, int id, Vec2 origin, int language, int hands)
{
    return createDetail<DetailHistoricSafe>(data, id, origin, language, hands);
}
void DetailHistoricSafe::setup(const rb::Row&)
{
    makeBoard("e4_s3_det2_1.png");
    auto size   = board_->getContentSize();
    touchSpace_ = {0, size.height - 81 - 150, size.width, 150};
    for (int i = 0; i < 10; ++i)
    {
        auto* s = SheetSprite::create("e4_s3_det2_2.png", 10, 1);
        s->frame(i);
        numbers_[i] = s;
        board_->addChild(s);
        track(s);
        place(board_, s, {(size.width - s->getContentSize().width) / 2 + 70 * i, 91});
    }
    schedule([this](float) { updateNumbers(); }, "historic-numbers");
}
void DetailHistoricSafe::updateNumbers()
{
    const float center = board_->getContentSize().width / 2;
    for (auto* s : numbers_)
    {
        float x        = s->getPositionX();
        float distance = std::abs(x - center) / center;
        s->setScale(1 - std::min(1.f, distance));
        s->setOpacity(static_cast<uint8_t>(std::max(0, 255 - static_cast<int>(255 * distance))));
        if (x > 700)
            s->setPositionX(x - 700);
        else if (x < board_->getContentSize().width - 700)
            s->setPositionX(x + 700);
    }
}
Vec2 DetailHistoricSafe::controlCenter(int i) const
{
    if (i == 10)
        return board_->convertToWorldSpace({board_->getContentSize().width / 2, touchSpace_.getMidY()});
    return numbers_.at(i)->convertToWorldSpace(numbers_.at(i)->getContentSize() / 2);
}
void DetailHistoricSafe::press(Vec2 p)
{
    last_          = p;
    pressedNumber_ = -1;
    sliding_       = false;
    spacePressed_  = touchSpace_.containsPoint(board_->convertToNodeSpace(p));
    if (!spacePressed_)
        return;
    for (auto* s : numbers_)
        s->stopAllActions();
    for (int i = 0; i < 10; ++i)
        if (contains(numbers_[i], p))
        {
            pressedNumber_ = i;
            break;
        }
}
void DetailHistoricSafe::move(Vec2 p)
{
    if (spacePressed_)
    {
        float dx = p.x - last_.x;
        for (auto* s : numbers_)
            s->setPositionX(s->getPositionX() + dx);
        moveLength_ += std::abs(dx);
        if (moveLength_ >= 70)
        {
            moveLength_ -= 70;
            playSound("snd_p15");
        }
        sliding_ = true;
    }
    last_ = p;
}
void DetailHistoricSafe::snap()
{
    int delta = static_cast<int>(board_->getContentSize().width / 2) - static_cast<int>(numbers_[0]->getPositionX());
    for (auto* s : numbers_)
    {
        float d = static_cast<int>(board_->getContentSize().width / 2) - s->getPositionX();
        if (std::abs(d) < std::abs(delta))
            delta = static_cast<int>(d);
    }
    for (auto* s : numbers_)
        s->runAction(EaseIn::create(MoveTo::create(.5f, s->getPosition() + Vec2(delta, 0)), .25f));
}
void DetailHistoricSafe::release(Vec2 p)
{
    if (pressedNumber_ >= 0 && !sliding_ && contains(numbers_[pressedNumber_], p))
        input(pressedNumber_);
    cancel();
    snap();
}
void DetailHistoricSafe::cancel()
{
    pressedNumber_ = -1;
    spacePressed_ = sliding_ = false;
}
void DetailHistoricSafe::retry()
{
    for (auto* s : displays_)
        s->removeFromParent();
    displays_.clear();
    input_.clear();
}
void DetailHistoricSafe::input(int value)
{
    input_.push_back(value);
    auto* s = SheetSprite::create("e4_s3_det2_3.png", 10, 1);
    s->frame(value);
    board_->addChild(s);
    displays_.push_back(s);
    for (size_t i = 0; i < displays_.size(); ++i)
        place(board_, displays_[i], {66 + displays_[i]->getContentSize().width * i, 18});
    playSound("snd_p13");
    if (input_.size() != 4 || busy_)
        return;
    busy_ = true;
    if (input_ == std::vector<int>{1, 1, 0, 0})
    {
        solve();
        return;
    }
    for (auto* d : displays_)
        d->runAction(Repeat::create(Sequence::create(FadeOut::create(.1f), FadeIn::create(.1f), nullptr), 3));
    playSound("snd_p41");
    scheduleOnce([this](float) {
        retry();
        busy_ = false;
    }, .6f, "historic-failed");
}
void DetailHistoricSafe::solve()
{
    if (closing_ || solved_)
        return;
    solved_ = true;
    busy_   = true;
    success();
    retry();
    for (int i : {1, 1, 0, 0})
        input(i);
}
