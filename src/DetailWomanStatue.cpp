#include "DetailWomanStatue.h"
#include "SheetSprite.h"
#include <cmath>
#include <stdexcept>
using namespace ax;
namespace
{
constexpr std::array<int, 4> password = {2, 6, 1, 3};
}
DetailWomanStatue* DetailWomanStatue::make(const rb::GameData& data, int id, Vec2 origin, int language, int hands)
{
    return createDetail<DetailWomanStatue>(data, id, origin, language, hands);
}
void DetailWomanStatue::setup(const rb::Row&)
{
    makeBoard("e5_s4_det4_1.png");
    for (int c = 0; c < 4; ++c)
    {
        for (int i = 0; i < 10; ++i)
        {
            auto* s = SheetSprite::create("e5_s4_det4_2.png", 10, 1);
            s->frame(i);
            numbers_[c][i] = s;
            board_->addChild(s);
            track(s);
            place(board_, s, {29 + 80 * c + s->getContentSize().width * i, 40});
        }
        handles_[c] = addImage("e5_s4_det4_4.png", 14 + 80 * c, 87);
    }
    schedule([this](float dt) { updateNumbers(dt); }, "woman-update");
}
Vec2 DetailWomanStatue::controlCenter(int i) const
{
    if (i < 0 || i >= 4)
        throw std::runtime_error("Invalid woman wheel");
    return handles_[i]->convertToWorldSpace(handles_[i]->getContentSize() / 2);
}
void DetailWomanStatue::press(Vec2 p)
{
    last_ = p;
    for (int c = 0; c < 4; ++c)
        if (contains(handles_[c], p))
        {
            selected_ = c;
            dragging_ = true;
        }
}
void DetailWomanStatue::move(Vec2 p)
{
    float dx = p.x - last_.x;
    last_    = p;
    if (!dragging_)
        return;
    for (auto* s : numbers_[selected_])
        s->setPositionX(s->getPositionX() + dx);
    travel_ += std::abs(dx);
    if (travel_ >= digitWidth())
    {
        travel_ -= digitWidth();
        playSound("snd_p15");
    }
}
void DetailWomanStatue::release(Vec2)
{
    if (dragging_)
    {
        float center = 29 + 80 * selected_ + digitWidth() / 2;
        int closest = 0, distance = int(numbers_[selected_][0]->getPositionX() - center);
        for (int i = 1; i < 10; ++i)
        {
            float delta = numbers_[selected_][i]->getPositionX() - center;
            if (std::abs(delta) < std::abs(distance))
            {
                closest  = i;
                distance = int(delta);
            }
        }
        input_[selected_] = closest;
        snap(selected_, closest);
    }
    dragging_ = false;
}
void DetailWomanStatue::cancel()
{
    dragging_ = false;
}
void DetailWomanStatue::snap(int c, int i)
{
    int delta = int(numbers_[c][i]->getPositionX() - (29 + 80 * c + digitWidth() / 2));
    for (auto* s : numbers_[c])
    {
        s->stopAllActions();
        s->runAction(EaseIn::create(MoveTo::create(.3f, s->getPosition() - Vec2(delta, 0)), .5f));
    }
    scheduleOnce([this](float) {
        if (!busy_ && !solved_ && input_ == password)
            solve();
    }, .3f, "woman-check-" + std::to_string(c));
}
void DetailWomanStatue::updateNumbers(float)
{
    for (int c = 0; c < 4; ++c)
    {
        float origin = 29 + 80 * c, w = digitWidth();
        for (auto* s : numbers_[c])
        {
            float x = s->getPositionX(), distance = x < origin + 6       ? origin + 6 - x
                                                    : x > origin + w - 6 ? x - (origin + w - 6)
                                                                         : 0;
            s->setOpacity(255 - std::clamp(int(distance * 35), 0, 255));
            if (x - w < origin - w / 2)
                x += 10 * w;
            if (x + w > origin + 2.5f * w)
                x -= 10 * w;
            s->setPositionX(x);
        }
    }
}
void DetailWomanStatue::retry()
{
    if (!closing_)
        for (int c = 0; c < 4; ++c)
            snap(c, 0);
}
void DetailWomanStatue::solve()
{
    if (solved_ || closing_)
        return;
    solved_ = true;
    success();
    for (int c = 0; c < 4; ++c)
        if (input_[c] != password[c])
            snap(c, password[c]);
}
