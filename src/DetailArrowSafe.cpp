#include "DetailArrowSafe.h"
#include "SheetSprite.h"
using namespace ax;
DetailArrowSafe* DetailArrowSafe::make(const rb::GameData& data, int id, Vec2 origin, int language, int hands)
{
    return createDetail<DetailArrowSafe>(data, id, origin, language, hands);
}
void DetailArrowSafe::setup(const rb::Row&)
{
    makeBoard("e1_s3_det3_1.png");
    const Vec2 positions[] = {{83, 62}, {83, 132}, {48, 97}, {118, 97}};
    for (int i = 0; i < 4; ++i)
    {
        auto* s = SheetSprite::create("e1_s3_det3_2.png", 4, 1);
        s->frame(i);
        keys_[i] = s;
        board_->addChild(s);
        place(board_, s, positions[i]);
        track(s);
        auto* down = SheetSprite::create("e1_s3_det3_3.png", 4, 1);
        down->frame(i);
        pressed_[i] = down;
        board_->addChild(down);
        down->setPosition(s->getPosition());
        down->setVisible(false);
    }
    correct_ = addImage("e1_s3_det3_4.png", 48, 21, false);
    wrong_   = addImage("e1_s3_det3_5.png", 48, 21, false);
    light_   = addImage("e1_s3_det3_6.png", 82, 205, false);
    for (auto* s : {correct_, wrong_, light_})
        s->setOpacity(0);
}
Vec2 DetailArrowSafe::controlCenter(int i) const
{
    return keys_[i]->convertToWorldSpace(keys_[i]->getContentSize() / 2);
}
void DetailArrowSafe::press(Vec2 p)
{
    selected_ = -1;
    for (int i = 0; i < 4; ++i)
        if (contains(keys_[i], p))
        {
            selected_ = i;
            pressed_[i]->setVisible(true);
            break;
        }
}
void DetailArrowSafe::cancel()
{
    selected_ = -1;
    for (auto* s : pressed_)
        if (s)
            s->setVisible(false);
}
void DetailArrowSafe::release(Vec2 p)
{
    const int i = selected_;
    cancel();
    if (i >= 0 && contains(keys_[i], p))
    {
        input_.push_back(i);
        playSound("snd_p01");
        if (input_.size() == 6)
            check();
    }
}
void DetailArrowSafe::retry()
{
    if (!closing_)
        input_.clear();
}
void DetailArrowSafe::check()
{
    busy_       = true;
    auto* blink = Repeat::create(Sequence::create(FadeIn::create(.1f), FadeOut::create(.1f), nullptr), 3);
    if (input_ == std::vector<int>{0, 1, 2, 3, 0, 3})
    {
        correct_->runAction(Sequence::create(blink, FadeIn::create(.3f), nullptr));
        light_->runAction(Sequence::create(FadeIn::create(1), CallFunc::create([this] { solve(); }), nullptr));
    }
    else
    {
        wrong_->runAction(Sequence::create(blink, CallFunc::create([this] {
            busy_ = false;
            retry();
        }),
                                           nullptr));
        playSound("snd_p28");
    }
}
