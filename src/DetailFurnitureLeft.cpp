#include "DetailFurnitureLeft.h"
#include "SheetSprite.h"
#include <stdexcept>
using namespace ax;
DetailFurnitureLeft* DetailFurnitureLeft::make(const rb::GameData& data, int id, Vec2 origin, int language, int hands)
{
    return createDetail<DetailFurnitureLeft>(data, id, origin, language, hands);
}
void DetailFurnitureLeft::setup(const rb::Row&)
{
    makeBoard("e5_s2_det4_1.png");
    for (int i = 0; i < 10; ++i)
    {
        auto add = [this, i](int file, int columns, int frame) {
            auto* s = SheetSprite::create("e5_s2_det4_" + std::to_string(file) + ".png", columns, 1);
            s->frame(frame);
            board_->addChild(s);
            place(board_, s, i < 2 ? Vec2(20 + 150 * i, 97) : Vec2(20 + 75 * ((i - 2) % 4), 142 + 36 * ((i - 2) / 4)));
            return s;
        };
        keys_[i] = add(i < 2 ? i + 2 : 6, i < 2 ? 3 : 8, i < 2 ? 2 : i - 2);
        track(keys_[i]);
        pressed_[i] = add(i < 2 ? i + 2 : 5, i < 2 ? 3 : 8, i < 2 ? 1 : i - 2);
        pressed_[i]->setVisible(false);
        inactive_[i] = add(i < 2 ? i + 2 : 4, i < 2 ? 3 : 8, i < 2 ? 0 : i - 2);
        if (i < 2)
            inactive_[i]->setOpacity(0);
        else
            track(inactive_[i]);
    }
}
Vec2 DetailFurnitureLeft::controlCenter(int i) const
{
    if (i < 0 || i >= 10)
        throw std::runtime_error("Invalid furniture key");
    return keys_[i]->convertToWorldSpace(keys_[i]->getContentSize() / 2);
}
void DetailFurnitureLeft::press(Vec2 p)
{
    pressedKey_ = -1;
    for (int i = first_ ? 0 : 2; i < (first_ ? 2 : 10); ++i)
        if (contains(keys_[i], p))
        {
            pressedKey_ = i;
            pressed_[i]->setVisible(true);
            break;
        }
}
void DetailFurnitureLeft::release(Vec2 p)
{
    int i = pressedKey_;
    cancel();
    if (i >= 0 && contains(keys_[i], p))
    {
        input(i);
        active(!first_);
    }
}
void DetailFurnitureLeft::cancel()
{
    pressedKey_ = -1;
    for (auto* s : pressed_)
        s->setVisible(false);
}
void DetailFurnitureLeft::active(bool first)
{
    first_ = first;
    for (int i = 0; i < 10; ++i)
        inactive_[i]->runAction(FadeTo::create(.3f, (i < 2) == first ? 0 : 255));
}
void DetailFurnitureLeft::clear()
{
    input_.clear();
    for (auto* s : digits_)
        s->removeFromParent();
    digits_.clear();
}
void DetailFurnitureLeft::retry()
{
    if (!closing_)
        clear();
}
void DetailFurnitureLeft::input(int value, bool check)
{
    if (input_.size() >= 8)
        return;
    int index = static_cast<int>(input_.size());
    input_ += char('0' + value);
    auto* s = SheetSprite::create("e5_s2_det4_7.png", 10, 1);
    s->frame(value);
    board_->addChild(s);
    place(board_, s, {float(29 + 25 * (index + index / 2)), 42});
    digits_.push_back(s);
    playSound("snd_p08");
    if (check && input_.size() == 8)
    {
        busy_ = true;
        if (input_ == "18141814")
            solve();
        else
        {
            for (auto* digit : digits_)
                digit->runAction(
                    Repeat::create(Sequence::create(FadeOut::create(.1f), FadeIn::create(.1f), nullptr), 3));
            scheduleOnce([this](float) {
                clear();
                busy_ = false;
            }, .6f, "furniture-failed");
        }
    }
}
void DetailFurnitureLeft::solve()
{
    if (solved_ || closing_)
        return;
    solved_ = true;
    success();
    clear();
    first_ = true;
    for (char c : std::string("18141814"))
        input(c - '0', false);
}
