#include "DetailWatchBox.h"
#include "Assets.h"
#include "SheetSprite.h"
using namespace ax;
DetailWatchBox* DetailWatchBox::make(const rb::GameData& data, int id, Vec2 origin, int language, int hands)
{
    return createDetail<DetailWatchBox>(data, id, origin, language, hands);
}
void DetailWatchBox::setup(const rb::Row&)
{
    makeBoard("e2_s4_det2_1.png");
    const float angles[] = {60, 330, 150, 240};
    for (int i = 0; i < 4; ++i)
    {
        auto* needle = addImage("e2_s4_det2_4.png", 0, 0);
        needles_[i]  = needle;
        needle->setAnchorPoint({.5f, 0});
        place(board_, needle, {44.f + 80 * i, 36 + needle->getContentSize().height / 2});
        needle->setRotation(angles[i]);
    }
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
        {
            auto* letter = SheetSprite::create("e2_s4_det2_5.png", 1, 4);
            letter->frame(j);
            letters_[i][j] = letter;
            board_->addChild(letter);
            track(letter);
            place(board_, letter, {40.f + 76 * i, 124});
            letter->setVisible(j == 0);
        }
    for (int i = 0; i < 4; ++i)
    {
        buttons_[i]       = addImage("e2_s4_det2_2.png", 37 + 76 * i, 188);
        pressedImages_[i] = addImage("e2_s4_det2_3.png", 37 + 76 * i, 188, false);
        pressedImages_[i]->setVisible(false);
    }
}
Vec2 DetailWatchBox::controlCenter(int i) const
{
    return buttons_.at(i)->convertToWorldSpace(buttons_.at(i)->getContentSize() / 2);
}
void DetailWatchBox::press(Vec2 p)
{
    pressedButton_ = -1;
    for (int i = 0; i < 4; ++i)
        if (contains(buttons_[i], p))
        {
            pressedButton_ = i;
            pressedImages_[i]->setVisible(true);
            break;
        }
}
void DetailWatchBox::cancel()
{
    pressedButton_ = -1;
    for (auto* s : pressedImages_)
        if (s)
            s->setVisible(false);
}
void DetailWatchBox::release(Vec2 p)
{
    int i = pressedButton_;
    cancel();
    if (i >= 0 && contains(buttons_[i], p))
        rotate(i, (input_[i] + 1) % 4);
}
void DetailWatchBox::rotate(int i, int next)
{
    if (input_[i] == next)
        return;
    busy_     = true;
    auto* old = letters_[i][input_[i]];
    auto* now = letters_[i][next];
    old->setVisible(true);
    old->runAction(Spawn::create(EaseIn::create(MoveTo::create(.5f, old->getPosition() + Vec2(0, 25)), .5f),
                                 FadeOut::create(.5f), nullptr));
    now->setVisible(true);
    now->setOpacity(0);
    now->setPositionY(now->getPositionY() - 25);
    now->runAction(
        Sequence::create(Spawn::create(EaseIn::create(MoveTo::create(.5f, now->getPosition() + Vec2(0, 25)), .5f),
                                       FadeIn::create(.5f), nullptr),
                         CallFunc::create([this, old] {
        if (closing_)
            return;
        busy_ = false;
        old->setPositionY(old->getPositionY() - 25);
        check();
    }),
                         nullptr));
    input_[i] = next;
    playSound("snd_p08");
}
void DetailWatchBox::check()
{
    if (!busy_ && !assistance_ && !successStarted_ && input_ == password_)
        successAction();
}
void DetailWatchBox::successAction()
{
    if (successStarted_)
        return;
    successStarted_      = true;
    busy_                = true;
    const float angles[] = {90, 180, 270, 360};
    for (int i = 0; i < 4; ++i)
    {
        auto* turn = RotateTo::create(.5f, angles[i]);
        if (i < 3)
            needles_[i]->runAction(turn);
        else
            needles_[i]->runAction(Sequence::create(turn, DelayTime::create(.5f), CallFunc::create([this] {
                if (!closing_)
                    success();
            }),
                                                    nullptr));
    }
}
void DetailWatchBox::retry()
{
    if (closing_)
        return;
    for (int i = 0; i < 4; ++i)
        rotate(i, 0);
}
void DetailWatchBox::solve()
{
    if (closing_ || successStarted_)
        return;
    assistance_ = true;
    successAction();
    for (int i = 0; i < 4; ++i)
        rotate(i, password_[i]);
}
