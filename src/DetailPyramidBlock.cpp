#include "DetailPyramidBlock.h"
#include "SheetSprite.h"
using namespace ax;
DetailPyramidBlock* DetailPyramidBlock::make(const rb::GameData& data, int id, Vec2 origin, int language, int hands)
{
    return createDetail<DetailPyramidBlock>(data, id, origin, language, hands);
}
void DetailPyramidBlock::setup(const rb::Row&)
{
    makeBoard("e4_s2_det1_1.png");
    int row = 0, col = 0;
    for (int i = 0; i < 18; ++i)
    {
        auto* out = SheetSprite::create("e4_s2_det1_2.png", 18, 1);
        out->frame(i);
        out_[i] = out;
        board_->addChild(out);
        track(out);
        const auto size = out->getContentSize();
        place(board_, out, {82 + (col - row * .5f) * size.width, 53 + row * size.height});
        auto* in = SheetSprite::create("e4_s2_det1_3.png", 18, 1);
        in->frame(i);
        in_[i] = in;
        board_->addChild(in);
        in->setPosition(out->getPosition());
        in->setOpacity(0);
        if (++col == row + 3)
        {
            col = 0;
            ++row;
        }
    }
}
Vec2 DetailPyramidBlock::controlCenter(int i) const
{
    return in_.at(i)->convertToWorldSpace(in_.at(i)->getContentSize() / 2);
}
void DetailPyramidBlock::press(Vec2 p)
{
    for (int i = 0; i < 18; ++i)
        pressed_[i] = contains(in_[i], p);
}
void DetailPyramidBlock::cancel()
{
    pressed_.fill(false);
}
void DetailPyramidBlock::release(Vec2 p)
{
    auto selected = pressed_;
    cancel();
    for (int i = 0; i < 18; ++i)
        if (selected[i] && contains(in_[i], p))
            toggle(i);
}
void DetailPyramidBlock::toggle(int i)
{
    input_[i] = !input_[i];
    in_[i]->runAction(FadeTo::create(.3f, input_[i] ? 255 : 0));
    out_[i]->runAction(
        Spawn::create(ScaleTo::create(.3f, input_[i] ? .5f : 1), FadeTo::create(.3f, input_[i] ? 0 : 255), nullptr));
    playSound("snd_p40");
    if (!busy_ && input_ == password_)
        solve();
}
void DetailPyramidBlock::retry()
{
    if (closing_)
        return;
    for (int i = 0; i < 18; ++i)
        if (input_[i])
            toggle(i);
}
void DetailPyramidBlock::solve()
{
    if (closing_ || solved_)
        return;
    solved_ = true;
    busy_   = true;
    success();
    for (int i = 0; i < 18; ++i)
        if (input_[i] != password_[i])
            toggle(i);
}
