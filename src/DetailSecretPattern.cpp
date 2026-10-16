#include "DetailSecretPattern.h"
using namespace ax;
DetailSecretPattern* DetailSecretPattern::make(const rb::GameData& data, int id, Vec2 origin, int language, int hands)
{
    return createDetail<DetailSecretPattern>(data, id, origin, language, hands);
}
void DetailSecretPattern::setup(const rb::Row&)
{
    makeBoard("e2_s4_det3_1.png");
    for (int i = 0; i < 9; ++i)
    {
        buttons_[i]  = addImage("e2_s4_det3_2.png", 14 + 70 * (i % 3), 15 + 71 * (i / 3));
        selected_[i] = addImage("e2_s4_det3_3.png", 0, 0, false);
        selected_[i]->setPosition(buttons_[i]->getPosition());
        selected_[i]->setOpacity(0);
        down_[i] = addImage("e2_s4_det3_4.png", 0, 0, false);
        down_[i]->setPosition(buttons_[i]->getPosition());
        down_[i]->setVisible(false);
    }
}
Vec2 DetailSecretPattern::controlCenter(int i) const
{
    return buttons_.at(i)->convertToWorldSpace(buttons_.at(i)->getContentSize() / 2);
}
void DetailSecretPattern::press(Vec2 p)
{
    pressedButton_ = -1;
    for (int i = 0; i < 9; ++i)
        if (contains(buttons_[i], p))
        {
            pressedButton_ = i;
            down_[i]->setVisible(true);
            break;
        }
}
void DetailSecretPattern::cancel()
{
    pressedButton_ = -1;
    for (auto* s : down_)
        if (s)
            s->setVisible(false);
}
void DetailSecretPattern::release(Vec2 p)
{
    int i = pressedButton_;
    cancel();
    if (i >= 0 && contains(buttons_[i], p))
        toggle(i);
}
void DetailSecretPattern::toggle(int i)
{
    input_[i] = !input_[i];
    selected_[i]->runAction(FadeTo::create(.3f, input_[i] ? 255 : 0));
    playSound("snd_p39");
    if (!busy_ && input_ == password_)
        solve();
}
void DetailSecretPattern::retry()
{
    if (closing_)
        return;
    for (int i = 0; i < 9; ++i)
        if (input_[i])
            toggle(i);
}
void DetailSecretPattern::solve()
{
    if (closing_ || solved_)
        return;
    solved_ = true;
    busy_   = true;
    success();
    for (int i = 0; i < 9; ++i)
        if (input_[i] != password_[i])
            toggle(i);
}
