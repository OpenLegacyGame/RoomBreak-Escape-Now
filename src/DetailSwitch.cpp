#include "DetailSwitch.h"
using namespace ax;
DetailSwitch* DetailSwitch::make(const rb::GameData& data, int id, Vec2 origin, int language, int hands)
{
    return createDetail<DetailSwitch>(data, id, origin, language, hands);
}
void DetailSwitch::setup(const rb::Row&)
{
    makeBoard("e1_s5_det2_1.png");
    for (int i = 0; i < 10; ++i)
    {
        off_[i]   = addImage("e1_s5_det2_3.png", 0, 0);
        auto size = off_[i]->getContentSize();
        place(board_, off_[i], {48 + (i % 5) * size.width, 60 + (i / 5) * size.height});
        on_[i] = addImage("e1_s5_det2_2.png", 0, 0);
        on_[i]->setPosition(off_[i]->getPosition());
        on_[i]->setOpacity(0);
    }
}
Vec2 DetailSwitch::controlCenter(int i) const
{
    return on_.at(i)->convertToWorldSpace(on_.at(i)->getContentSize() / 2);
}
void DetailSwitch::press(Vec2 p)
{
    selected_ = -1;
    for (int i = 0; i < 10; ++i)
        if (contains(on_[i], p))
        {
            selected_ = i;
            break;
        }
}
void DetailSwitch::release(Vec2 p)
{
    const int i = selected_;
    cancel();
    if (i >= 0 && contains(on_[i], p))
        toggle(i);
}
void DetailSwitch::toggle(int i)
{
    input_[i] = !input_[i];
    on_[i]->runAction(FadeTo::create(.3f, input_[i] ? 255 : 0));
    off_[i]->runAction(FadeTo::create(.3f, input_[i] ? 0 : 255));
    playSound("snd_p19");
    if (!busy_ && input_ == password_)
        solve();
}
void DetailSwitch::retry()
{
    if (closing_)
        return;
    for (int i = 0; i < 10; ++i)
        if (input_[i])
            toggle(i);
}
void DetailSwitch::solve()
{
    if (closing_)
        return;
    busy_ = true;
    for (int i = 0; i < 10; ++i)
        if (input_[i] != password_[i])
            toggle(i);
    success();
}
