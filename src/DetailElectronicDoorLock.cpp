#include "DetailElectronicDoorLock.h"
#include "SheetSprite.h"
using namespace ax;
DetailElectronicDoorLock* DetailElectronicDoorLock::make(const rb::GameData& data,
                                                         int id,
                                                         Vec2 origin,
                                                         int language,
                                                         int hands)
{
    return createDetail<DetailElectronicDoorLock>(data, id, origin, language, hands);
}
void DetailElectronicDoorLock::setup(const rb::Row&)
{
    makeBoard("e1_s4_det2_1.png");
    for (int i = 0; i < 6; ++i)
    {
        auto* s = SheetSprite::create("e1_s4_det2_2.png", 6, 1);
        s->frame(i);
        keys_[i] = s;
        board_->addChild(s);
        const auto size = s->getContentSize();
        place(board_, s, {26 + (i % 3) * size.width, 118 + (i / 3) * size.height});
        track(s);
        auto* down = SheetSprite::create("e1_s4_det2_3.png", 6, 1);
        down->frame(i);
        pressed_[i] = down;
        board_->addChild(down);
        down->setPosition(s->getPosition());
        down->setVisible(false);
    }
}
Vec2 DetailElectronicDoorLock::controlCenter(int i) const
{
    return keys_[i]->convertToWorldSpace(keys_[i]->getContentSize() / 2);
}
void DetailElectronicDoorLock::press(Vec2 p)
{
    selected_ = -1;
    for (int i = 0; i < 6; ++i)
        if (contains(keys_[i], p))
        {
            selected_ = i;
            pressed_[i]->setVisible(true);
            break;
        }
}
void DetailElectronicDoorLock::cancel()
{
    selected_ = -1;
    for (auto* s : pressed_)
        if (s)
            s->setVisible(false);
}
void DetailElectronicDoorLock::release(Vec2 p)
{
    const int i = selected_;
    cancel();
    if (i >= 0 && contains(keys_[i], p))
        input(i);
}
void DetailElectronicDoorLock::input(int i)
{
    input_.push_back(i);
    auto* digit = SheetSprite::create("e1_s4_det2_4.png", 6, 1);
    digit->frame(i);
    digits_.push_back(digit);
    board_->addChild(digit);
    place(board_, digit, {30.f + 35 * (digits_.size() - 1), 53});
    digit->setOpacity(0);
    digit->runAction(FadeTo::create(.3f, 255));
    playSound("snd_p08");
    if (input_.size() == 4)
        check();
}
void DetailElectronicDoorLock::retry()
{
    if (closing_)
        return;
    for (auto* s : digits_)
        s->removeFromParent();
    digits_.clear();
    input_.clear();
}
void DetailElectronicDoorLock::solve()
{
    if (closing_)
        return;
    success();
    retry();
    for (int i : {3, 5, 2, 0})
        input(i);
}
void DetailElectronicDoorLock::check()
{
    if (busy_)
        return;
    busy_              = true;
    const bool correct = input_ == std::vector<int>{3, 5, 2, 0};
    // Original performSelector delay is double .5 at 0x17E70/0x17E86.
    scheduleOnce([this, correct](float) {
        if (closing_)
            return;
        if (correct)
        {
            solve();
            return;
        }
        for (std::size_t n = 0; n < digits_.size(); ++n)
        {
            auto* blink = Repeat::create(Sequence::create(FadeOut::create(.1f), FadeIn::create(.1f), nullptr), 3);
            if (n == 3)
                digits_[n]->runAction(Sequence::create(blink, FadeOut::create(.5f), CallFunc::create([this] {
                    busy_ = false;
                    retry();
                }),
                                                       nullptr));
            else
                digits_[n]->runAction(Sequence::create(blink, FadeOut::create(.5f), nullptr));
        }
        playSound("snd_p24");
    }, .5f, "symbol-check");
}
