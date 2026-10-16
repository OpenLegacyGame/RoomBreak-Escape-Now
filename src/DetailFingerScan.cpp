#include "DetailFingerScan.h"
#include "SheetSprite.h"
using namespace ax;
DetailFingerScan* DetailFingerScan::make(const rb::GameData& data, int id, Vec2 origin, int language, int hands)
{
    return createDetail<DetailFingerScan>(data, id, origin, language, hands);
}
void DetailFingerScan::setup(const rb::Row&)
{
    makeBoard("e2_s1_det1_1.png");
    for (int i = 0; i < 10; ++i)
    {
        auto* key = SheetSprite::create("e2_s1_det1_2.png", 10, 1);
        key->frame(i);
        keys_[i] = key;
        board_->addChild(key);
        auto size = key->getContentSize();
        place(board_, key,
              {35 + (i == 0 ? 1 : (i - 1) % 3) * size.width, 29 + (i == 0 ? 3 : (i - 1) / 3) * size.height});
        track(key);
        auto* pressed = SheetSprite::create("e2_s1_det1_7.png", 10, 1);
        pressed->frame(i);
        pressed_[i] = pressed;
        board_->addChild(pressed);
        pressed->setPosition(key->getPosition());
        pressed->setVisible(false);
        auto* selected = SheetSprite::create("e2_s1_det1_3.png", 10, 1);
        selected->frame(i);
        selectedImages_[i] = selected;
        board_->addChild(selected);
        selected->setPosition(key->getPosition());
        selected->setOpacity(0);
    }
    if (id_ == 2)
        addImage("e2_s1_det1_4.png", 35, 29);
    cognition_ = addImage("e2_s1_det1_5.png", 173, 21, false);
    cognition_->setOpacity(0);
    scanning_ = addImage("e2_s1_det1_6.png", 174, 30, false);
    scanning_->setOpacity(0);
}
Vec2 DetailFingerScan::controlCenter(int i) const
{
    return keys_.at(i)->convertToWorldSpace(keys_.at(i)->getContentSize() / 2);
}
void DetailFingerScan::press(Vec2 p)
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
void DetailFingerScan::cancel()
{
    selected_ = -1;
    for (auto* s : pressed_)
        s->setVisible(false);
}
void DetailFingerScan::release(Vec2 p)
{
    int i = selected_;
    cancel();
    if (i >= 0 && contains(keys_[i], p))
        toggle(i);
}
void DetailFingerScan::toggle(int i)
{
    input_[i] = !input_[i];
    count_ += input_[i] ? 1 : -1;
    selectedImages_[i]->runAction(FadeTo::create(.3f, input_[i] ? 255 : 0));
    playSound("snd_p03");
    if (count_ == 4 && !busy_)
        check();
}
void DetailFingerScan::retry()
{
    if (closing_)
        return;
    for (int i = 0; i < 10; ++i)
        if (input_[i])
            toggle(i);
}
void DetailFingerScan::check()
{
    if (input_ == password_)
    {
        solve();
        return;
    }
    busy_ = true;
    for (int i = 0; i < 10; ++i)
        if (input_[i])
            selectedImages_[i]->runAction(
                Repeat::create(Sequence::create(FadeOut::create(.1f), FadeIn::create(.1f), nullptr), 3));
    scheduleOnce([this](float) {
        if (!closing_)
        {
            busy_ = false;
            retry();
        }
    }, 1, "finger-failed");
    playSound("snd_p30");
}
void DetailFingerScan::solve()
{
    if (closing_)
        return;
    busy_ = true;
    for (int i = 0; i < 10; ++i)
        if (input_[i] != password_[i])
            toggle(i);
    cognition_->runAction(Sequence::create(FadeIn::create(.5f), DelayTime::create(1), CallFunc::create([this] {
        scanning_->runAction(Sequence::create(FadeIn::create(.5f),
                                              MoveTo::create(2, scanning_->getPosition() - Vec2(0, 129)),
                                              FadeOut::create(.3f), CallFunc::create([this] { success(); }), nullptr));
        playSound("snd_p23");
    }),
                                           nullptr));
}
