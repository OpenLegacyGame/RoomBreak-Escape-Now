#include "DetailSafe.h"
#include "SheetSprite.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
using namespace ax;
DetailSafe* DetailSafe::make(const rb::GameData& data, int id, Vec2 origin, int language, int hands)
{
    return createDetail<DetailSafe>(data, id, origin, language, hands);
}
void DetailSafe::setup(const rb::Row&)
{
    std::string prefix = "e1_s1_det2_";
    Vec2 start(37, 89), cover(37, 39);
    switch (id_)
    {
    case 15:
        password_ = {21, 0, 18, 4};
        prefix    = "e2_s4_det4_";
        start     = {49, 102};
        max_      = 26;
        break;
    case 24:
        password_ = {0, 2, 1, 3};
        break;
    case 43:
        password_ = {4, 11, 4, 11};
        prefix    = "e5_s2_det5_";
        start     = {50, 103};
        cover     = {0, 66};
        max_      = 26;
        break;
    case 57:
        password_ = {1, 8, 5, 6};
        break;
    case 58:
        password_ = {7, 5, 5, 3};
        break;
    case 59:
        password_ = {6, 4, 9, 4};
        break;
    default:
        throw std::runtime_error("Invalid rolling safe");
    }
    makeBoard(prefix + "1.png");
    input_.resize(password_.size());
    soundDistance_.resize(password_.size());
    for (std::size_t column = 0; column < password_.size(); ++column)
    {
        std::vector<Sprite*> wheel;
        for (int digit = 0; digit < max_; ++digit)
        {
            auto* s = SheetSprite::create(prefix + "2.png", 1, max_);
            s->frame(digit);
            board_->addChild(s);
            track(s);
            place(board_, s, start + Vec2(60 * column, 50 * digit));
            wheel.push_back(s);
        }
        wheels_.push_back(std::move(wheel));
    }
    addImage(prefix + "3.png", cover.x, cover.y);
    schedule([this](float dt) { updateWheels(dt); }, "safe-wheels");
}
Vec2 DetailSafe::controlCenter(int column) const
{
    return board_->convertToWorldSpace({62.f + 60 * column, 126});
}
void DetailSafe::press(Vec2 p)
{
    last_            = p;
    const auto local = board_->convertToNodeSpace(p);
    for (std::size_t column = 0; column < wheels_.size(); ++column)
        if (Rect(37 + 60 * column, 51, 50, 150).containsPoint(local))
        {
            if (selected_ == static_cast<int>(column))
                for (auto* s : wheels_[column])
                    s->stopAllActions();
            selected_ = static_cast<int>(column);
            dragging_ = true;
        }
}
void DetailSafe::move(Vec2 p)
{
    const float dy = p.y - last_.y;
    last_          = p;
    if (!dragging_)
        return;
    for (auto* s : wheels_[selected_])
        s->setPositionY(s->getPositionY() + dy);
    updateWheels(0);
    auto& distance = soundDistance_[selected_];
    distance += std::abs(dy);
    if (distance >= 50)
    {
        distance -= 50;
        playSound(id_ == 15 ? "snd_p18" : "snd_p08");
    }
}
void DetailSafe::release(Vec2)
{
    if (!dragging_)
        return;
    dragging_          = false;
    const float center = board_->getContentSize().height / 2 + 6;
    int closest        = 0;
    float distance     = wheels_[selected_][0]->getPositionY() - center;
    for (int n = 1; n < max_; ++n)
    {
        float delta = wheels_[selected_][n]->getPositionY() - center;
        if (std::abs(delta) < std::abs(distance))
        {
            closest  = n;
            distance = delta;
        }
    }
    input_[selected_] = closest;
    snap(selected_, closest);
}
void DetailSafe::snap(int column, int digit)
{
    const float delta = wheels_[column][digit]->getPositionY() - board_->getContentSize().height / 2 - 6;
    for (int n = 0; n < max_; ++n)
    {
        auto* s      = wheels_[column][n];
        auto* action = EaseIn::create(MoveTo::create(.3f, s->getPosition() - Vec2(0, delta)), .5f);
        if (n == max_ - 1)
            s->runAction(Sequence::create(action, CallFunc::create([this] { checkPassword(); }), nullptr));
        else
            s->runAction(action);
    }
}
void DetailSafe::checkPassword()
{
    if (!busy_ && input_ == password_)
        solve();
}
void DetailSafe::updateWheels(float)
{
    if (closing_)
        return;
    for (auto& wheel : wheels_)
        for (auto* s : wheel)
        {
            const float y = s->getPositionY(), h = s->getContentSize().height;
            if (y - h > 162 + h / 2)
                s->setPositionY(s->getPositionY() - 50 * max_);
            if (s->getPositionY() + h < 78 - h / 2)
                s->setPositionY(s->getPositionY() + 50 * max_);
            // A wrapped digit must use its final position for this frame's fade.
            const float displayedY = s->getPositionY();
            const int fade         = static_cast<int>(displayedY > 174  ? (displayedY - 174) * 10
                                                      : displayedY < 78 ? (78 - displayedY) * 10
                                                                        : 0);
            s->setOpacity(255 - std::clamp(fade, 0, 255));
        }
}
void DetailSafe::retry()
{
    if (!closing_)
        for (std::size_t n = 0; n < wheels_.size(); ++n)
            snap(n, 0);
}
void DetailSafe::solve()
{
    if (solved_ || closing_)
        return;
    solved_ = true;
    for (std::size_t n = 0; n < wheels_.size(); ++n)
        snap(n, password_[n]);
    success();
}
