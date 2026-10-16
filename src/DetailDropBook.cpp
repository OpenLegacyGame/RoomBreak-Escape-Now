#include "DetailDropBook.h"
#include "Assets.h"
#include "SheetSprite.h"
#include <stdexcept>
using namespace ax;
DetailDropBook* DetailDropBook::make(const rb::GameData& data, int id, Vec2 origin, int language, int hands)
{
    return createDetail<DetailDropBook>(data, id, origin, language, hands);
}
void DetailDropBook::setup(const rb::Row&)
{
    makeBoard("e5_s3_det1_1.png");
    const Vec2 positions[] = {{122, 52}, {5, 5}, {165, 5}, {5, 95}, {165, 95}};
    for (int i = 0; i < 5; ++i)
        for (int down = 0; down < 2; ++down)
        {
            Sprite* s;
            if (i == 0)
                s = rb::sprite(down ? "e5_s3_det1_5.png" : "e5_s3_det1_4.png");
            else
            {
                auto* sheet = SheetSprite::create(down ? "e5_s3_det1_3.png" : "e5_s3_det1_2.png", 4, 1);
                sheet->frame(i - 1);
                s = sheet;
            }
            board_->addChild(s);
            track(s);
            place(board_, s, positions[i]);
            (down ? pressed_[i] : keys_[i]) = s;
            if (down)
                s->setVisible(false);
        }
    good_ = addImage("e5_s3_det1_7.png", 95, 25, false);
    bad_  = addImage("e5_s3_det1_6.png", 95, 25, false);
    good_->setOpacity(0);
    bad_->setOpacity(0);
}
Vec2 DetailDropBook::controlCenter(int i) const
{
    if (i < 0 || i >= 5)
        throw std::runtime_error("Invalid book control");
    return keys_[i]->convertToWorldSpace(keys_[i]->getContentSize() / 2);
}
void DetailDropBook::press(Vec2 p)
{
    pressedKey_ = -1;
    for (int i = 0; i < 5; ++i)
        if (contains(keys_[i], p))
        {
            pressedKey_ = i;
            pressed_[i]->setVisible(true);
            break;
        }
}
void DetailDropBook::cancel()
{
    pressedKey_ = -1;
    for (auto* s : pressed_)
        s->setVisible(false);
}
void DetailDropBook::release(Vec2 p)
{
    int i = pressedKey_;
    cancel();
    if (i < 0 || !contains(keys_[i], p))
        return;
    ++counts_[i];
    ++count_;
    playSound("snd_p07");
    if (count_ == 9)
    {
        busy_ = true;
        if (counts_ == std::array<int, 5>{3, 1, 2, 2, 1})
            good_->runAction(Sequence::create(FadeIn::create(.5f), DelayTime::create(.5f),
                                              CallFunc::create([this] { solve(); }), nullptr));
        else
        {
            bad_->runAction(Sequence::create(FadeIn::create(.5f), DelayTime::create(.5f), FadeOut::create(.5f),
                                             CallFunc::create([this] {
                retry();
                busy_ = false;
            }),
                                             nullptr));
            playSound("snd_p22");
        }
    }
}
void DetailDropBook::retry()
{
    if (!closing_)
    {
        counts_.fill(0);
        count_ = 0;
    }
}
void DetailDropBook::solve()
{
    if (solved_ || closing_)
        return;
    solved_ = true;
    success();
}
