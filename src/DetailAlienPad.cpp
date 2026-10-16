#include "DetailAlienPad.h"
#include "SheetSprite.h"
#include <cmath>
using namespace ax;
DetailAlienPad* DetailAlienPad::make(const rb::GameData& data, int id, Vec2 origin, int language, int hands)
{
    return createDetail<DetailAlienPad>(data, id, origin, language, hands);
}
void DetailAlienPad::setup(const rb::Row&)
{
    prefix_ = id_ == 73 ? "e5_s5_det2_" : "e4_s5_det2_";
    switch (id_)
    {
    case 22:
        password_ = "pendant";
        break;
    case 27:
        password_ = "earth";
        break;
    case 28:
        password_ = "alien";
        break;
    case 29:
        password_ = "crystal";
        break;
    case 30:
        password_ = "english";
        break;
    case 31:
        password_ = "peace";
        break;
    case 73:
        password_ = "push";
        break;
    }
    makeBoard(prefix_ + "1.png");
    selection_ = addImage(prefix_ + "4.png", 11, 138, false);
    selection_->setOpacity(0);
    for (int i = 0; i < 26; ++i)
    {
        auto* s = SheetSprite::create(prefix_ + "5.png", 26, 1);
        s->frame(i);
        letters_[i] = s;
        board_->addChild(s);
        track(s);
        place(board_, s, {(board_->getContentSize().width - s->getContentSize().width) / 2 + 91 * i, 158});
    }
    for (size_t i = 0; i < password_.size(); ++i)
        cells_.push_back(addImage(prefix_ + "2.png", 137 - 18 * static_cast<int>(password_.size()) + 45 * i, 11));
    successEyes_ = addImage(prefix_ + "7.png", 120, 115, false);
    failedEyes_  = addImage(prefix_ + "6.png", 120, 115, false);
    failedEyes_->setPosition(successEyes_->getPosition());
    successEyes_->setOpacity(0);
    failedEyes_->setOpacity(0);
    schedule([this](float) { updateLetters(); }, "alien-letters");
}
void DetailAlienPad::updateLetters()
{
    float center = board_->getContentSize().width / 2;
    for (auto* s : letters_)
    {
        float x = s->getPositionX();
        s->setOpacity(static_cast<uint8_t>(std::max(0, 255 - static_cast<int>(255 * std::abs(x - center) / center))));
        if (x > 2366)
            s->setPositionX(x - 2366);
        else if (x < board_->getContentSize().width - 2366)
            s->setPositionX(x + 2366);
    }
}
Vec2 DetailAlienPad::controlCenter(int i) const
{
    auto* s = i == 26 ? selection_ : letters_.at(i);
    return s->convertToWorldSpace(s->getContentSize() / 2);
}
void DetailAlienPad::press(Vec2 p)
{
    last_    = p;
    pressed_ = contains(selection_, p);
    sliding_ = false;
    if (pressed_)
    {
        for (auto* s : letters_)
            s->stopAllActions();
        selection_->runAction(FadeIn::create(.3f));
    }
}
void DetailAlienPad::move(Vec2 p)
{
    if (pressed_)
    {
        float dx = p.x - last_.x;
        for (auto* s : letters_)
            s->setPositionX(s->getPositionX() + dx * 2);
        travel_ += std::abs(dx);
        if (travel_ >= 91)
        {
            travel_ -= 91;
            playSound(id_ == 73 ? "snd_p13" : "snd_p15");
        }
        sliding_ = true;
    }
    last_ = p;
}
void DetailAlienPad::snap()
{
    int center = static_cast<int>(board_->getContentSize().width / 2),
        delta  = static_cast<int>(center - letters_[0]->getPositionX());
    selected_  = 0;
    for (int i = 1; i < 26; ++i)
    {
        float d = center - letters_[i]->getPositionX();
        if (std::abs(d) < std::abs(delta))
        {
            delta     = static_cast<int>(d);
            selected_ = i;
        }
    }
    for (auto* s : letters_)
    {
        s->stopAllActions();
        s->runAction(EaseIn::create(MoveTo::create(.5f, s->getPosition() + Vec2(delta, 0)), .25f));
    }
}
void DetailAlienPad::release(Vec2 p)
{
    if (pressed_ && !sliding_ && contains(selection_, p))
        input(selected_);
    snap();
    cancel();
}
void DetailAlienPad::cancel()
{
    pressed_ = sliding_ = false;
    selection_->runAction(FadeOut::create(.3f));
}
void DetailAlienPad::clearInput()
{
    for (auto* s : displays_)
        s->removeFromParent();
    displays_.clear();
    input_.clear();
}
void DetailAlienPad::retry()
{
    for (auto* s : cells_)
        s->runAction(FadeIn::create(.3f));
    clearInput();
}
void DetailAlienPad::input(int i)
{
    auto* s = SheetSprite::create(prefix_ + "3.png", 26, 1);
    s->frame(i);
    board_->addChild(s);
    place(board_, s, {137 - 18 * static_cast<int>(password_.size()) + 45.f * input_.size(), 35});
    cells_.at(input_.size())->runAction(FadeOut::create(.3f));
    displays_.push_back(s);
    input_.push_back(i);
    playSound(id_ == 73 ? "snd_p11" : "snd_p13");
    if (input_.size() != password_.size() || busy_)
        return;
    busy_     = true;
    bool good = true;
    for (size_t j = 0; j < input_.size(); ++j)
        if (input_[j] != password_[j] - 'a')
            good = false;
    auto* eyes = good ? successEyes_ : failedEyes_;
    if (!good)
        playSound(id_ == 73 ? "snd_p22" : "snd_p41");
    eyes->runAction(Sequence::create(FadeIn::create(.5f), DelayTime::create(.5f), FadeOut::create(.5f),
                                     CallFunc::create([this, good] {
        if (good)
            solve();
        else
        {
            busy_ = false;
            retry();
        }
    }),
                                     nullptr));
}
void DetailAlienPad::solve()
{
    if (closing_ || solved_)
        return;
    solved_ = true;
    busy_   = true;
    success();
    clearInput();
    for (char c : password_)
        input(c - 'a');
}
