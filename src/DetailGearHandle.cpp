#include "DetailGearHandle.h"
#include "Assets.h"
using namespace ax;
DetailGearHandle* DetailGearHandle::make(const rb::GameData& data, int id, Vec2 origin, int language, int hands)
{
    return createDetail<DetailGearHandle>(data, id, origin, language, hands);
}
void DetailGearHandle::setup(const rb::Row&)
{
    bool four                = id_ == 75;
    count_                   = four ? 4 : 3;
    initial_                 = four ? std::array<float, 4>{80, 160, 290, 120} : std::array<float, 4>{60, 250, 80, 0};
    const std::string prefix = four ? "e3_s4_det1_" : "e1_s5_det1_";
    auto file                = [prefix](int i) { return prefix + std::to_string(i) + ".png"; };
    makeBoard(file(1));
    const Vec2 pos3[] = {{17, 11}, {56, 50}, {79, 73}}, pos4[] = {{20.5f, 18}, {35.5f, 33}, {56.5f, 54}, {76, 73.5f}};
    handleX_            = four ? 249 : 246;
    const float handleY = four ? 32.5f : 37, stripX = four ? 259.5f : 256.5f, stripY = four ? 37 : 41.5f,
                step = four ? 45 : 60;
    for (int i = 0; i < count_; ++i)
    {
        auto pos  = four ? pos4[i] : pos3[i];
        gears_[i] = addImage(file(count_ + 1 - i), pos.x, pos.y);
        gears_[i]->setRotation(initial_[i]);
        handles_[i] = addImage(file(count_ + 3), handleX_, handleY + step * i);
        pressed_[i] = rb::sprite(file(count_ + 4));
        board_->addChild(pressed_[i]);
        pressed_[i]->setPosition(handles_[i]->getPosition());
        pressed_[i]->setVisible(false);
        for (int j = 0; j < 4; ++j)
            strips_[i][j] = addImage(file(count_ + 5), stripX + 19 * j, stripY + step * i);
    }
    addImage(file(count_ + 2), 33, 28, false)->setOpacity(0);
    schedule([this](float) {
        if (!closing_)
            updateStrips();
    }, "gear-strips");
}
Vec2 DetailGearHandle::controlCenter(int i) const
{
    return handles_.at(i)->convertToWorldSpace(handles_.at(i)->getContentSize() / 2);
}
void DetailGearHandle::press(Vec2 p)
{
    previous_ = p;
    selected_ = -1;
    for (int i = 0; i < count_; ++i)
        if (contains(handles_[i], p))
        {
            selected_ = i;
            pressed_[i]->setVisible(true);
            break;
        }
}
void DetailGearHandle::cancel()
{
    selected_ = -1;
    for (int i = 0; i < count_; ++i)
        pressed_[i]->setVisible(false);
}
void DetailGearHandle::move(Vec2 p)
{
    float dx  = p.x - previous_.x;
    previous_ = p;
    if (selected_ < 0)
        return;
    for (auto* strip : strips_[selected_])
        strip->setPositionX(strip->getPositionX() + dx);
    length_ += dx;
    if (length_ > 10 || length_ < -10)
    {
        turn(selected_, length_ > 0);
        length_ = 0;
    }
}
void DetailGearHandle::turn(int i, bool clockwise)
{
    if (moving_)
        return;
    moving_        = true;
    auto* gear     = gears_[i];
    float rotation = gear->getRotation();
    if (clockwise && rotation == 360)
        rotation = 0;
    else if (!clockwise && rotation == 0)
        rotation = 360;
    gear->setRotation(rotation);
    float delta = clockwise ? 10 : -10;
    gear->runAction(
        Sequence::create(EaseOut::create(RotateTo::create(.1f, rotation + delta + (clockwise ? 1 : -1)), .25f),
                         RotateTo::create(.1f, rotation + delta), CallFunc::create([this] {
        moving_  = false;
        bool all = true;
        for (int j = 0; j < count_; ++j)
            if (gears_[j]->getRotation() != 0)
                all = false;
        if (all)
            solve();
    }),
                         nullptr));
    playSound(id_ == 75 ? "snd_p17" : "snd_p18");
}
void DetailGearHandle::retry()
{
    if (closing_)
        return;
    busy_ = true;
    for (int i = 0; i < count_; ++i)
        gears_[i]->runAction(Sequence::create(EaseIn::create(RotateTo::create(.5f, initial_[i]), .25f),
                                              CallFunc::create([this] { busy_ = false; }), nullptr));
}
void DetailGearHandle::solve()
{
    if (closing_)
        return;
    success();
    for (int i = 0; i < count_; ++i)
        gears_[i]->runAction(EaseIn::create(RotateTo::create(.5f, 0), .25f));
}
void DetailGearHandle::updateStrips()
{
    const float left = handleX_ + 10, right = handleX_ + handles_[0]->getContentSize().width;
    for (int i = 0; i < count_; ++i)
        for (auto* strip : strips_[i])
        {
            float x = strip->getPositionX(), w = strip->getContentSize().width;
            if (x - w < left - w / 2)
                x += 76;
            if (x + w > right + w * 1.5f)
                x -= 76;
            strip->setPositionX(x);
            float outside = x < left ? left - x : (x > right ? x - right : 0);
            strip->setOpacity(255 - std::clamp(int(outside * 35), 0, 255));
        }
}
