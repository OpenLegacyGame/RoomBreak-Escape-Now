#include "TitleShop.h"
#include "NodeFactory.h"
#include "Assets.h"
#include "ShopText.h"
#include "ShopButton.h"
#include "MessageBox.h"
using namespace ax;
namespace
{
void fade(Node* node, int opacity, float duration)
{
    node->stopAllActions();
    node->runAction(FadeTo::create(duration, opacity));
}
}  // namespace
TitleShop* TitleShop::create(const rb::Progress& progress, int language)
{
    auto* v      = rb::make<TitleShop>();
    v->language_ = language;
    v->setup(progress);
    return v;
}
void TitleShop::setup(const rb::Progress& progress)
{
    progress_ = &progress;
    setContentSize({480, 320});
    background_ = rb::sprite("title_shop_bg.png");
    addChild(background_);
    background_->setPosition(240, 160);
    background_->setOpacity(0);
    for (int i = 0; i < 6; ++i)
    {
        items_[i] = rb::place(background_, "sys_shop_item" + std::to_string(i + 1) + "_p.png", 41.5f + 50 * i, 49.5f);
        items_[i]->setOpacity(0);
        auto* price = rb::label(rb::shopFreeText(language_), language_, 11);
        background_->addChild(price);
        price->setPosition(items_[i]->getPositionX(), background_->getContentSize().height - 107);
        decor_.push_back(price);
    }
    close_ = rb::place(background_, "title_shop_close.png", 159.5f, 158.5f);
    decor_.push_back(close_);
    const int counts[] = {progress.eyes, progress.insight, progress.hands, progress.feet};
    for (int i = 0; i < 4; ++i)
    {
        auto* label = rb::label(std::to_string(counts[i]), 2, 12);
        counts_[i]  = label;
        background_->addChild(label);
        auto size = label->getContentSize();
        label->setPosition(153 + 60 * i + size.width / 2, background_->getContentSize().height - 22 - size.height / 2);
        decor_.push_back(label);
    }
    decor_.push_back(rb::place(background_, "sys_shop_des.png", 14, 122));
    for (int i = 0; i < 2; ++i)
    {
        descriptions_[i] = rb::label("", language_, 12);
        background_->addChild(descriptions_[i]);
        descriptions_[i]->setPosition(193, background_->getContentSize().height - 132 - 15 * i);
        decor_.push_back(descriptions_[i]);
    }
    unlock_ = rb::shopUnlockButton(language_);
    background_->addChild(unlock_);
    unlock_->setPosition(160, close_->getPositionY());
    close_->setPositionX(close_->getPositionX() + 106);
    decor_.push_back(unlock_);
    for (auto* node : decor_)
        node->setOpacity(0);
    cursor_ = rb::sprite("sys_shop_sel.png");
    background_->addChild(cursor_);
    cursor_->setVisible(false);
    auto* touch = EventListenerTouchOneByOne::create();
    touch->setSwallowTouches(true);
    touch->onTouchBegan     = [this](Touch* t, Event*) { return beginTouch(t->getLocation()); };
    touch->onTouchEnded     = [this](Touch* t, Event*) { endTouch(t->getLocation()); };
    touch->onTouchCancelled = [this](Touch*, Event*) { endTouch({-10000, -10000}); };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(touch, this);
}
void TitleShop::open()
{
    fade(background_, 255, .3f);
    for (auto* node : decor_)
        fade(node, 255, .3f);
}
void TitleShop::select(int index)
{
    if (index == selected_)
    {
        if (purchase)
            purchase(index);
        refreshCounts();
        return;
    }
    selected_ = index;
    for (int i = 0; i < 6; ++i)
        fade(items_[i], i == index ? 255 : 0, .3f);
    const char* keys[] = {"ABILITY_EYES",  "ABILITY_INSIGHT",  "ABILITY_HANDS",
                          "ABILITY_FOOTS", "ABILITY_BLESSING", "ABILITY_GIFT"};
    const auto text    = rb::otherText(language_, keys[index]);
    const auto split   = text.find('\n');
    descriptions_[0]->setString(text.substr(0, split));
    descriptions_[1]->setString(split == std::string::npos ? "" : text.substr(split + 1));
}
void TitleShop::refreshCounts()
{
    const int values[] = {progress_->eyes, progress_->insight, progress_->hands, progress_->feet};
    for (int i = 0; i < 4; ++i)
        counts_[i]->setString(std::to_string(values[i]));
}
void TitleShop::close()
{
    if (closing_ || message_)
        return;
    closing_  = true;
    selected_ = -1;
    cursor_->setVisible(false);
    fade(background_, 0, .5f);
    for (auto* node : decor_)
        fade(node, 0, .3f);
    for (auto* node : items_)
        fade(node, 0, .3f);
    if (sound)
        sound("snd_f19");
    scheduleOnce([this](float) {
        auto callback = std::move(closed);
        removeFromParent();
        if (callback)
            callback();
    }, .5f, "title-shop-close");
}
Vec2 TitleShop::controlCenter(int index) const
{
    Node* node = index == 7 ? static_cast<Node*>(unlock_) : index == 6 ? static_cast<Node*>(close_) : items_[index];
    return node->convertToWorldSpace(node->getContentSize() / 2);
}
int TitleShop::hit(Vec2 point) const
{
    if (rb::hitTest(unlock_, point))
        return 7;
    for (int i = 0; i < 7; ++i)
    {
        auto* node = i == 6 ? close_ : items_[i];
        if (rb::hitTest(node, point))
            return i;
    }
    return -1;
}
bool TitleShop::beginTouch(Vec2 point)
{
    pressed_ = (closing_ || message_) ? -1 : hit(point);
    rb::pressShopUnlock(unlock_, pressed_ == 7);
    if (pressed_ >= 0 && pressed_ < 6)
    {
        cursor_->setVisible(true);
        cursor_->setPosition(items_[pressed_]->getPosition());
    }
    return true;
}
void TitleShop::endTouch(Vec2 point)
{
    const int index = pressed_;
    pressed_        = -1;
    rb::pressShopUnlock(unlock_, false);
    cursor_->setVisible(false);
    if (closing_ || index < 0 || hit(point) != index)
        return;
    if (index == 7)
    {
        message_ = GameMessageBox::create(rb::shopUnlockConfirm(language_), language_, true, [this](bool yes) {
            message_ = nullptr;
            if (yes && unlockAll)
                unlockAll();
        }, sound);
        addChild(message_, 10);
    }
    else if (index == 6)
        close();
    else
        select(index);
}
