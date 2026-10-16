#include "DetailDoorLock.h"
#include "SheetSprite.h"
using namespace ax;
DetailDoorLock* DetailDoorLock::make(const rb::GameData& data, int id, Vec2 origin, int language, int hands)
{
    return createDetail<DetailDoorLock>(data, id, origin, language, hands);
}
void DetailDoorLock::setup(const rb::Row&)
{
    makeBoard("e1_s2_det2_1.png");
    for (int i = 0; i < 11; ++i)
    {
        auto* key = SheetSprite::create("e1_s2_det2_2.png", 11, 1);
        key->frame(i);
        keys_[i] = key;
        board_->addChild(key);
        track(key);
        const auto size = key->getContentSize();
        const int col = i == 0 ? 1 : i == 10 ? 2 : (i - 1) % 3, row = i == 0 || i == 10 ? 3 : (i - 1) / 3;
        place(board_, key, {47 + col * size.width, 52 + row * size.height});
        auto* down = SheetSprite::create("e1_s2_det2_3.png", 11, 1);
        down->frame(i);
        pressed_[i] = down;
        board_->addChild(down);
        down->setPosition(key->getPosition());
        down->setVisible(false);
    }
    light_ = addImage("e1_s2_det2_5.png", 86, 206, false);
    light_->setOpacity(0);
}
Vec2 DetailDoorLock::controlCenter(int index) const
{
    return keys_[index]->convertToWorldSpace(keys_[index]->getContentSize() / 2);
}
void DetailDoorLock::press(Vec2 p)
{
    pressedKey_ = -1;
    for (int i = 0; i < 11; ++i)
        if (contains(keys_[i], p))
        {
            pressedKey_ = i;
            pressed_[i]->setVisible(true);
            break;
        }
}
void DetailDoorLock::cancel()
{
    pressedKey_ = -1;
    for (auto* s : pressed_)
        if (s)
            s->setVisible(false);
}
void DetailDoorLock::release(Vec2 p)
{
    const int key = pressedKey_;
    cancel();
    if (key >= 0 && contains(keys_[key], p))
    {
        if (key == 10)
            erase();
        else
            input(key);
    }
}
void DetailDoorLock::sort()
{
    for (std::size_t i = 0; i < digits_.size(); ++i)
        place(board_, digits_[i], {134.f - 17 * (digits_.size() - i - 1), 21});
}
void DetailDoorLock::input(int digit)
{
    input_ += static_cast<char>('0' + digit);
    auto* s = SheetSprite::create("e1_s2_det2_4.png", 10, 1);
    s->frame(digit);
    board_->addChild(s);
    digits_.push_back(s);
    playSound("snd_p02");
    sort();
    if (input_.size() == 4)
        check();
}
void DetailDoorLock::erase()
{
    if (input_.empty())
        return;
    input_.pop_back();
    digits_.back()->removeFromParent();
    digits_.pop_back();
    sort();
}
void DetailDoorLock::retry()
{
    if (!closing_)
        while (!input_.empty())
            erase();
}
void DetailDoorLock::check()
{
    if (busy_)
        return;
    busy_ = true;
    if (input_ == "7230")
        light_->runAction(Sequence::create(FadeIn::create(1.5f), CallFunc::create([this] { solve(); }), nullptr));
    else
    {
        for (std::size_t n = 0; n < digits_.size(); ++n)
        {
            auto* blink = Repeat::create(Sequence::create(FadeOut::create(.1f), FadeIn::create(.1f), nullptr), 3);
            if (n == 3)
                digits_[n]->runAction(Sequence::create(blink, CallFunc::create([this] {
                    busy_ = false;
                    retry();
                }),
                                                       nullptr));
            else
                digits_[n]->runAction(blink);
        }
        playSound("snd_p28");
    }
}
void DetailDoorLock::solve()
{
    if (solved_ || closing_)
        return;
    solved_ = true;
    success();
    retry();
    for (int n : {7, 2, 3, 0})
        input(n);
}
