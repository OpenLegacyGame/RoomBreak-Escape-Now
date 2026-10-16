#include "DetailScreenSafe.h"
#include "SheetSprite.h"
#include <stdexcept>
using namespace ax;
DetailScreenSafe* DetailScreenSafe::make(const rb::GameData& data, int id, Vec2 origin, int language, int hands)
{
    return createDetail<DetailScreenSafe>(data, id, origin, language, hands);
}
void DetailScreenSafe::setup(const rb::Row&)
{
    safeType_ = (id_ == 68 || id_ == 76 || id_ == 77) ? 65 : id_;
    switch (id_)
    {
    case 4:
        prefix_   = "e2_s2_det1_";
        password_ = "4545";
        break;
    case 40:
        prefix_   = "e5_s1_det7_";
        password_ = "2146";
        break;
    case 65:
        prefix_   = "e3_s3_det1_";
        password_ = "0817";
        break;
    case 68:
        prefix_   = "e3_s3_det1_";
        password_ = "6614";
        break;
    case 76:
        prefix_   = "e3_s3_det1_";
        password_ = "0327";
        break;
    case 77:
        prefix_   = "e3_s3_det1_";
        password_ = "1963";
        break;
    case 93:
        prefix_   = "e0_s1_det2_";
        password_ = "1234";
        break;
    case 95:
        prefix_   = "e6_s1_det1_";
        password_ = "9764";
        break;
    default:
        throw std::runtime_error("Invalid screen safe");
    }
    makeBoard(prefix_ + "1.png");
    for (int i = 0; i < 11; ++i)
    {
        auto* key = SheetSprite::create(prefix_ + "4.png", 11, 1);
        key->frame(i);
        keys_[i] = key;
        board_->addChild(key);
        track(key);
        const auto size = key->getContentSize();
        const int col = i == 0 ? 1 : i == 10 ? 2 : (i - 1) % 3, row = i == 0 || i == 10 ? 3 : (i - 1) / 3;
        place(board_, key, {185 + col * size.width, 37 + row * size.height});
        auto* down = SheetSprite::create(prefix_ + "5.png", 11, 1);
        down->frame(i);
        pressed_[i] = down;
        board_->addChild(down);
        down->setPosition(key->getPosition());
        down->setVisible(false);
    }
    handle_ = addImage(prefix_ + "2.png", 0, 0);
    handle_->setAnchorPoint({.85f, .5f});
    const auto size = handle_->getContentSize();
    place(board_, handle_, {11 + size.width * .85f * .5f, 86 - size.height * .25f});
}
Vec2 DetailScreenSafe::controlCenter(int index) const
{
    auto* image = index == 11 ? handle_ : keys_[index];
    return image->convertToWorldSpace(image->getContentSize() / 2);
}
void DetailScreenSafe::press(Vec2 p)
{
    last_          = p;
    resultRunning_ = false;
    pressedKey_    = -1;
    for (int i = 0; i < 11; ++i)
        if (contains(keys_[i], p))
        {
            pressedKey_ = i;
            pressed_[i]->setVisible(true);
            break;
        }
    dragging_ = contains(handle_, p);
}
void DetailScreenSafe::cancel()
{
    pressedKey_ = -1;
    dragging_   = false;
    for (auto* image : pressed_)
        if (image)
            image->setVisible(false);
}
void DetailScreenSafe::release(Vec2 p)
{
    const int key = pressedKey_;
    cancel();
    if (key >= 0 && contains(keys_[key], p))
    {
        if (key == 10)
            erase();
        else
            input(key);
    }
    resetHandle();
}
void DetailScreenSafe::move(Vec2 p)
{
    const float dy = p.y - last_.y;
    last_          = p;
    if (!dragging_)
        return;
    const float angle = handle_->getRotation() + dy;
    if (angle > 0)
        return;
    if (angle < -30)
    {
        handle_->setRotation(-30);
        check();
    }
    else
        handle_->setRotation(angle);
}
void DetailScreenSafe::sortDigits(const std::vector<Sprite*>& images)
{
    for (std::size_t i = 0; i < images.size(); ++i)
        place(board_, images[i], {136.f - 21 * (images.size() - i), 37});
}
void DetailScreenSafe::input(int digit)
{
    if (input_.size() == password_.size())
        return;
    input_ += static_cast<char>('0' + digit);
    auto* image = SheetSprite::create(prefix_ + "3.png", 10, 1);
    image->frame(digit);
    board_->addChild(image);
    digits_.push_back(image);
    sortDigits(digits_);
    playSound(safeType_ == 65 ? "snd_p08" : safeType_ == 40 ? "snd_p05" : "snd_p06");
}
void DetailScreenSafe::erase()
{
    if (input_.empty())
        return;
    input_.pop_back();
    digits_.back()->removeFromParent();
    digits_.pop_back();
    sortDigits(digits_);
}
void DetailScreenSafe::retry()
{
    if (!closing_)
        while (!input_.empty())
            erase();
}
void DetailScreenSafe::resetHandle()
{
    if (!resultRunning_ && handle_->getRotation() != 0)
        handle_->runAction(Sequence::create(RotateTo::create(.1f, 5), RotateTo::create(.1f, 0), nullptr));
}
void DetailScreenSafe::openHandle()
{
    handle_->runAction(Sequence::create(RotateTo::create(.2f, -95), RotateTo::create(.1f, -90), nullptr));
}
void DetailScreenSafe::check()
{
    if (busy_)
        return;
    busy_     = true;
    dragging_ = false;
    if (input_ == password_)
    {
        resultRunning_ = true;
        openHandle();
        playSound(safeType_ == 65 ? "snd_p21" : "snd_p20");
        scheduleOnce([this](float) { solve(); }, .8f, "safe-success");
    }
    else
        wrongPassword();
}
void DetailScreenSafe::wrongPassword()
{
    resetHandle();
    resultRunning_ = true;
    for (char digit : input_)
    {
        auto* image = SheetSprite::create(prefix_ + "6.png", 10, 1);
        image->frame(digit - '0');
        board_->addChild(image);
        failedDigits_.push_back(image);
    }
    sortDigits(failedDigits_);
    retry();
    for (auto* image : failedDigits_)
        image->runAction(
            Sequence::create(Repeat::create(Sequence::create(FadeOut::create(.1f), FadeIn::create(.1f), nullptr), 3),
                             FadeOut::create(.3f), nullptr));
    scheduleOnce([this](float) {
        for (auto* image : failedDigits_)
            image->removeFromParent();
        failedDigits_.clear();
        busy_ = false;
    }, failedDigits_.empty() ? .3f : .9f, "safe-failed");
    playSound("snd_p21");
}
void DetailScreenSafe::solve()
{
    if (solved_ || closing_)
        return;
    solved_        = true;
    resultRunning_ = true;
    success();
    retry();
    for (char digit : password_)
        input(digit - '0');
    openHandle();
}
