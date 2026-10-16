#include "DetailPad.h"
#include "Assets.h"
#include "SheetSprite.h"
#include <stdexcept>
using namespace ax;
DetailPad* DetailPad::make(const rb::GameData& data, int id, Vec2 origin, int language, int hands)
{
    return createDetail<DetailPad>(data, id, origin, language, hands);
}
void DetailPad::setup(const rb::Row&)
{
    Vec2 words, keyOrigin(22, 123);
    switch (id_)
    {
    case 5:
        prefix_      = "e2_s3_det2_";
        password_    = "4710";
        words        = {37, 35};
        inputOrigin_ = {227, 70};
        break;
    case 12:
        prefix_      = "e2_s5_det1_";
        password_    = "1111";
        words        = {106, 45};
        inputOrigin_ = {227, 79};
        break;
    case 38:
        prefix_      = "e5_s1_det8_";
        password_    = "645933";
        words        = {38, 33};
        inputOrigin_ = {238, 54};
        break;
    case 47:
        prefix_      = "e5_s3_det2_";
        password_    = "7979";
        words        = {92, 15};
        inputOrigin_ = {226, 40};
        break;
    case 71:
        prefix_      = "e5_s4_det5_";
        password_    = "3846";
        words        = {70, 25};
        inputOrigin_ = {201, 51};
        keyOrigin    = {18, 117};
        break;
    default:
        throw std::runtime_error("Invalid numeric pad detail");
    }
    makeBoard(prefix_ + "1.png");
    words_      = addImage(prefix_ + "2.png", words.x, words.y);
    retryWords_ = addImage(prefix_ + "3.png", words.x, words.y, false);
    retryWords_->setOpacity(0);
    for (int i = 0; i < 10; ++i)
    {
        auto* key = SheetSprite::create(prefix_ + "4.png", 10, 1);
        key->frame(i);
        keys_[i] = key;
        board_->addChild(key);
        track(key);
        const auto size = key->getContentSize();
        place(board_, key, {keyOrigin.x + i % 5 * size.width, keyOrigin.y + i / 5 * size.height});
        auto* down = SheetSprite::create(prefix_ + "5.png", 10, 1);
        down->frame(i);
        pressed_[i] = down;
        board_->addChild(down);
        down->setPosition(key->getPosition());
        down->setVisible(false);
    }
}
Vec2 DetailPad::controlCenter(int index) const
{
    if (index < 0 || index > 9)
        throw std::runtime_error("Invalid pad key");
    return keys_[index]->convertToWorldSpace(keys_[index]->getContentSize() / 2);
}
void DetailPad::press(Vec2 point)
{
    pressedKey_ = -1;
    for (int i = 0; i < 10; ++i)
        if (contains(keys_[i], point))
        {
            pressedKey_ = i;
            pressed_[i]->setVisible(true);
            break;
        }
}
void DetailPad::release(Vec2 point)
{
    const int index = pressedKey_;
    cancel();
    if (index >= 0 && contains(keys_[index], point))
        input(index);
}
void DetailPad::cancel()
{
    pressedKey_ = -1;
    for (auto* image : pressed_)
        if (image)
            image->setVisible(false);
}
void DetailPad::clear()
{
    input_.clear();
    for (auto* digit : digits_)
        digit->removeFromParent();
    digits_.clear();
}
void DetailPad::retry()
{
    if (!closing_)
        clear();
}
void DetailPad::input(int digit, bool check)
{
    if (input_.size() >= password_.size())
        return;
    input_ += static_cast<char>('0' + digit);
    auto* image = SheetSprite::create(prefix_ + "6.png", 10, 1);
    image->frame(digit);
    board_->addChild(image);
    digits_.push_back(image);
    const int step = 216 / static_cast<int>(password_.size());
    for (std::size_t i = 0; i < digits_.size(); ++i)
        place(board_, digits_[i], {inputOrigin_.x - (password_.size() - 1) * step + i * step, inputOrigin_.y});
    playSound(id_ == 38 || id_ == 71 ? "snd_p10" : id_ == 47 ? "snd_p02" : "snd_p03");
    if (check && input_.size() == password_.size())
    {
        busy_ = true;
        if (input_ == password_)
            solve();
        else
            wrongPassword();
    }
}
void DetailPad::wrongPassword()
{
    words_->runAction(FadeOut::create(.1f));
    retryWords_->runAction(Sequence::create(
        FadeIn::create(.1f), Repeat::create(Sequence::create(FadeOut::create(.1f), FadeIn::create(.1f), nullptr), 3),
        CallFunc::create([this] {
        clear();
        busy_ = false;
        words_->runAction(FadeIn::create(.1f));
        retryWords_->runAction(FadeOut::create(.1f));
    }),
        nullptr));
    playSound(id_ == 38 || id_ == 71 ? "snd_p26" : id_ == 12 ? "snd_p29" : "snd_p30");
}
void DetailPad::solve()
{
    if (solved_ || closing_)
        return;
    solved_ = true;
    success();
    clear();
    for (char digit : password_)
        input(digit - '0', false);
}
