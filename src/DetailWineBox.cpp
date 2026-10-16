#include "DetailWineBox.h"
using namespace ax;
DetailWineBox* DetailWineBox::make(const rb::GameData& data, int id, Vec2 origin, int language, int hands)
{
    return createDetail<DetailWineBox>(data, id, origin, language, hands);
}
void DetailWineBox::setup(const rb::Row&)
{
    makeBoard("e2_s5_det2_1.png");
    const Vec2 positions[] = {{101, 6},  {152, 20}, {186, 53}, {197, 102}, {186, 147}, {152, 178}, {101, 196},
                              {52, 182}, {19, 147}, {6, 102},  {19, 53},   {52, 20},   {52, 102},  {152, 102}};
    for (int i = 0; i < 14; ++i)
    {
        buttons_[i] = addImage("e2_s5_det2_4.png", positions[i].x, positions[i].y);
        red_[i]     = addImage("e2_s5_det2_6.png", 0, 0, false);
        red_[i]->setPosition(buttons_[i]->getPosition());
        red_[i]->setOpacity(0);
        green_[i] = addImage("e2_s5_det2_5.png", 0, 0, false);
        green_[i]->setPosition(buttons_[i]->getPosition());
        green_[i]->setOpacity(0);
    }
    main_ = addImage("e2_s5_det2_2.png", 93, 94);
    down_ = addImage("e2_s5_det2_3.png", 0, 0, false);
    down_->setPosition(main_->getPosition());
    down_->setVisible(false);
}
Vec2 DetailWineBox::controlCenter(int i) const
{
    auto* s = i == 14 ? main_ : buttons_.at(i);
    return s->convertToWorldSpace(s->getContentSize() / 2);
}
void DetailWineBox::press(Vec2 p)
{
    pressedButton_ = -1;
    for (int i = 0; i < 14; ++i)
        if (contains(buttons_[i], p))
        {
            pressedButton_ = i;
            break;
        }
    if (contains(main_, p))
    {
        pressedButton_ = 14;
        down_->setVisible(true);
    }
}
void DetailWineBox::cancel()
{
    pressedButton_ = -1;
    if (down_)
        down_->setVisible(false);
}
void DetailWineBox::release(Vec2 p)
{
    int i = pressedButton_;
    cancel();
    if (i == 14 && contains(main_, p))
        retry();
    else if (i >= 0 && i < 14 && contains(buttons_[i], p))
    {
        convert(i, (input_[i] + 1) % 3);
        playSound("snd_p13");
    }
}
void DetailWineBox::convert(int i, int next)
{
    if (input_[i])
        (input_[i] == 1 ? red_[i] : green_[i])->runAction(FadeTo::create(.3f, 0));
    if (next)
        (next == 1 ? red_[i] : green_[i])->runAction(FadeTo::create(.3f, 255));
    input_[i] = next;
    if (!busy_ && input_ == password_)
        solve();
}
void DetailWineBox::retry()
{
    if (closing_)
        return;
    for (int i = 0; i < 14; ++i)
        convert(i, 0);
}
void DetailWineBox::solve()
{
    if (closing_ || solved_)
        return;
    solved_ = true;
    busy_   = true;
    success();
    for (int i = 0; i < 14; ++i)
        if (input_[i] != password_[i])
            convert(i, password_[i]);
}
