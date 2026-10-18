#include "SystemMenu.h"
#include "NodeFactory.h"
#include "Assets.h"
#include "MessageBox.h"
#include "ShopText.h"
#include "ShopButton.h"
#include <stdexcept>
using namespace ax;
SystemMenu* SystemMenu::create(const rb::Progress& progress, int language, Settings settings)
{
    auto* value      = rb::make<SystemMenu>();
    value->progress_ = &progress;
    value->language_ = language;
    value->settings_ = settings;
    value->setup();
    return value;
}
void SystemMenu::setup()
{
    setContentSize({480, 320});
    bg_ = rb::sprite("sys_bg.png");
    addChild(bg_);
    bg_->setPosition(240, 160);
    bg_->setOpacity(0);
    const char* titleNames[] = {"system", "option", "shop"};
    for (int i = 0; i < 3; ++i)
    {
        titles_[i] = rb::place(bg_, std::string("sys_menu_") + titleNames[i] + ".png", 10, 2);
        titles_[i]->setOpacity(0);
    }
    play_           = rb::sprite("ui_play.png");
    playP_          = rb::sprite("ui_play_p.png");
    back_           = rb::sprite("main_back.png");
    backP_          = rb::sprite("main_back_p.png");
    const auto size = play_->getContentSize();
    for (auto* node : {play_, playP_, back_, backP_})
    {
        addChild(node);
        node->setPosition(480 + size.width / 2, 320 - size.height / 2);
    }
    playP_->setVisible(false);
    backP_->setVisible(false);
    const char *mainNames[] = {"retry", "option", "shop", "exit"}, *optionNames[] = {"eft", "bgm", "vib", "text_1"};
    for (int i = 0; i < 4; ++i)
    {
        main_[i] = rb::place(bg_, std::string("sys_") + mainNames[i] + ".png", 65 + 65 * i, 61);
        main_[i]->setOpacity(0);
        mainP_[i] = rb::place(bg_, std::string("sys_") + mainNames[i] + "_p.png", 65 + 65 * i, 61);
        mainP_[i]->setVisible(false);
        options_[i] = rb::place(bg_, std::string("sys_") + optionNames[i] + ".png", 65 + 65 * i, 61);
        options_[i]->setOpacity(0);
        optionsP_[i] = rb::place(bg_, i == 3 ? "sys_text_p.png" : std::string("sys_") + optionNames[i] + "_p.png",
                                 65 + 65 * i, 61);
        optionsP_[i]->setVisible(false);
    }
    for (int i = 0; i < 3; ++i)
    {
        off_[i] = rb::sprite("sys_off.png");
        bg_->addChild(off_[i]);
        off_[i]->setPosition(options_[i]->getPosition());
        off_[i]->setOpacity(0);
        speed_[i] = rb::place(bg_, "sys_text_" + std::to_string(i + 1) + ".png", 260, 61);
        speed_[i]->setOpacity(0);
    }
    for (int i = 0; i < 6; ++i)
    {
        shop_[i] = rb::place(bg_, "sys_shop_item" + std::to_string(i + 1) + ".png", 46 + 51 * i, 72);
        shop_[i]->setOpacity(0);
        shopP_[i] = rb::place(bg_, "sys_shop_item" + std::to_string(i + 1) + "_p.png", 46 + 51 * i, 72);
        shopP_[i]->setOpacity(0);
        auto* price = rb::label(rb::shopFreeText(language_), language_, 11);
        bg_->addChild(price);
        price->setPosition(shop_[i]->getPositionX(), bg_->getContentSize().height - 128);
        shopDecor_.push_back(price);
    }
    const int counts[] = {progress_->eyes, progress_->insight, progress_->hands, progress_->feet};
    for (int i = 0; i < 4; ++i)
    {
        shopDecor_.push_back(rb::place(bg_, "sys_shop_info" + std::to_string(i + 1) + ".png", 77 + 60 * i, 43));
        auto* label = rb::label(std::to_string(counts[i]), 2, 12);
        counts_[i]  = label;
        bg_->addChild(label);
        label->setPosition(rb::topLeft(bg_, label, 119 + 60 * i, 44));
        shopDecor_.push_back(label);
    }
    shopDecor_.push_back(rb::place(bg_, "sys_shop_des.png", 19, 144));
    for (int i = 0; i < 2; ++i)
    {
        description_[i] = rb::label("", language_, 12);
        bg_->addChild(description_[i]);
        description_[i]->setPosition(193, bg_->getContentSize().height - (154 + 15 * i));
        shopDecor_.push_back(description_[i]);
    }
    unlock_ = rb::shopUnlockButton(language_);
    addChild(unlock_);
    unlock_->setPosition(240, 28);
    shopDecor_.push_back(unlock_);
    for (auto* node : shopDecor_)
        node->setOpacity(0);
    shopCursor_ = rb::sprite("sys_shop_sel.png");
    bg_->addChild(shopCursor_);
    shopCursor_->setVisible(false);
    auto* touch = EventListenerTouchOneByOne::create();
    touch->setSwallowTouches(true);
    touch->onTouchBegan     = [this](Touch* t, Event*) { return beginTouch(t->getLocation()); };
    touch->onTouchEnded     = [this](Touch* t, Event*) { endTouch(t->getLocation()); };
    touch->onTouchCancelled = [this](Touch*, Event*) { endTouch({-10000, -10000}); };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(touch, this);
}
void SystemMenu::fade(Node* node, int opacity, float duration)
{
    node->stopAllActions();
    node->runAction(FadeTo::create(duration, opacity));
}
void SystemMenu::open(int mode)
{
    if (opened_)
        return;
    opened_   = true;
    auto size = play_->getContentSize();
    for (auto* node : {play_, playP_})
        node->runAction(EaseIn::create(MoveTo::create(.5f, {480 - size.width / 2 - 5, 320 - size.height / 2}), .25f));
    bg_->runAction(Sequence::create(EaseIn::create(FadeIn::create(.5f), .25f),
                                    CallFunc::create([this, mode] { setMode(mode); }), nullptr));
}
void SystemMenu::setMode(int mode)
{
    if (mode < 1 || mode > 3)
        throw std::runtime_error("Invalid menu mode");
    if (closing_ || mode_ == mode)
        return;
    mode_ = mode;
    for (int i = 0; i < 4; ++i)
    {
        fade(main_[i], mode == 1 ? 255 : 0);
        fade(options_[i], mode == 2 ? 255 : 0);
        mainP_[i]->setVisible(false);
        optionsP_[i]->setVisible(false);
    }
    for (int i = 0; i < 6; ++i)
    {
        fade(shop_[i], mode == 3 ? 255 : 0);
        fade(shopP_[i], mode == 3 && i == selectedShop_ ? 255 : 0);
    }
    for (auto* node : shopDecor_)
        fade(node, mode == 3 ? 255 : 0);
    for (int i = 0; i < 3; ++i)
    {
        auto* title = titles_[i];
        title->stopAllActions();
        if (i == mode - 1)
        {
            title->setPosition(rb::topLeft(bg_, titles_[0], 10, 2));
            title->runAction(
                Spawn::create(FadeIn::create(.5f),
                              EaseIn::create(MoveTo::create(.5f, rb::topLeft(bg_, titles_[0], 20, 2)), .25f), nullptr));
        }
        else
            title->runAction(
                Spawn::create(FadeOut::create(.5f),
                              EaseIn::create(MoveTo::create(.5f, rb::topLeft(bg_, titles_[0], 70, 2)), .25f), nullptr));
    }
    refreshOptions();
    shopCursor_->setVisible(false);
    const auto size = play_->getContentSize();
    for (int i = 0; i < 2; ++i)
    {
        const bool visible = i == 0 ? mode == 1 : mode != 1;
        for (auto* node : i == 0 ? std::vector<Sprite*>{play_, playP_} : std::vector<Sprite*>{back_, backP_})
        {
            node->stopAllActions();
            node->runAction(EaseIn::create(
                MoveTo::create(.5f, {visible ? 480 - size.width / 2 - 5 : 480 + size.width / 2, 320 - size.height / 2}),
                .25f));
        }
    }
    if (sound)
        sound("snd_f19");
}
void SystemMenu::refreshCounts()
{
    const int values[] = {progress_->eyes, progress_->insight, progress_->hands, progress_->feet};
    for (int i = 0; i < 4; ++i)
        counts_[i]->setString(std::to_string(values[i]));
}
void SystemMenu::refreshOptions()
{
    const bool on[] = {settings_.music, settings_.effects, settings_.vibration};
    for (int i = 0; i < 3; ++i)
    {
        fade(off_[i], mode_ == 2 && !on[i] ? 255 : 0);
        fade(speed_[i], mode_ == 2 && settings_.textSpeed == i ? 255 : 0);
    }
}
void SystemMenu::confirm(const char* key, std::function<void()> action)
{
    if (message_)
        return;
    message_ = GameMessageBox::create(rb::otherText(language_, key), language_, true,
                                      [this, action = std::move(action)](bool yes) {
        message_ = nullptr;
        if (yes && action)
            action();
    }, sound);
    addChild(message_, 10);
}
void SystemMenu::selectShop(int index)
{
    selectedShop_ = index;
    for (int i = 0; i < 6; ++i)
        fade(shopP_[i], i == index ? 255 : 0);
    const char* keys[] = {"ABILITY_EYES",  "ABILITY_INSIGHT",  "ABILITY_HANDS",
                          "ABILITY_FOOTS", "ABILITY_BLESSING", "ABILITY_GIFT"};
    auto text          = rb::otherText(language_, keys[index]);
    auto split         = text.find('\n');
    description_[0]->setString(text.substr(0, split));
    description_[1]->setString(split == std::string::npos ? "" : text.substr(split + 1));
}
void SystemMenu::click(int index)
{
    if (!opened_ || closing_ || message_ || mode_ == 0)
        return;
    if (index == 100)
    {
        if (mode_ == 1)
            close();
        else
            setMode(1);
        return;
    }
    if (mode_ == 3 && index == 6)
    {
        message_ = GameMessageBox::create(rb::shopUnlockConfirm(language_), language_, true, [this](bool yes) {
            message_ = nullptr;
            if (yes && unlockAll)
                unlockAll();
        }, sound);
        addChild(message_, 10);
        return;
    }
    if (mode_ == 1)
    {
        if (index == 0)
            confirm("RETRY", retry);
        else if (index == 1)
            setMode(2);
        else if (index == 2)
            setMode(3);
        else if (index == 3)
            confirm("EXIT", exitGame);
    }
    else if (mode_ == 2 && index >= 0 && index < 4)
    {
        if (index == 0)
            settings_.music = !settings_.music;
        else if (index == 1)
            settings_.effects = !settings_.effects;
        else if (index == 2)
            settings_.vibration = !settings_.vibration;
        else
            settings_.textSpeed = (settings_.textSpeed + 1) % 3;
        refreshOptions();
        if (settingsChanged)
            settingsChanged(settings_);
        if (sound)
            sound("snd_f19");
    }
    else if (mode_ == 3 && index >= 0 && index < 6)
    {
        if (selectedShop_ == index)
        {
            if (purchase)
                purchase(index);
            refreshCounts();
        }
        else
            selectShop(index);
    }
}
void SystemMenu::close()
{
    if (closing_ || message_)
        return;
    closing_ = true;
    fade(unlock_, 0);
    for (auto* node : bg_->getChildren())
        fade(node, 0);
    bg_->stopAllActions();
    bg_->runAction(Sequence::create(FadeOut::create(.5f), CallFunc::create([this] {
        auto callback = std::move(resumed);
        removeFromParent();
        if (callback)
            callback();
    }),
                                    nullptr));
}
int SystemMenu::hit(Vec2 point) const
{
    auto inside = [point](Node* node) { return rb::hitTest(node, point); };
    if (mode_ && inside(mode_ == 1 ? play_ : back_))
        return 100;
    if (mode_ == 3 && inside(unlock_))
        return 6;
    const int count = mode_ == 3 ? 6 : 4;
    for (int i = 0; i < count; ++i)
        if (mode_ && inside(mode_ == 1 ? main_[i] : mode_ == 2 ? options_[i] : shop_[i]))
            return i;
    return -1;
}
bool SystemMenu::beginTouch(Vec2 point)
{
    pressed_ = -1;
    if (closing_ || message_ || mode_ == 0)
        return true;
    pressed_ = hit(point);
    rb::pressShopUnlock(unlock_, mode_ == 3 && pressed_ == 6);
    if (pressed_ == 100)
        (mode_ == 1 ? playP_ : backP_)->setVisible(true);
    else if (pressed_ >= 0)
    {
        if (mode_ == 3 && pressed_ < 6)
        {
            shopCursor_->setPosition(shop_[pressed_]->getPosition());
            shopCursor_->setVisible(true);
        }
        else if (mode_ != 3)
            (mode_ == 1 ? mainP_[pressed_] : optionsP_[pressed_])->setVisible(true);
    }
    return true;
}
void SystemMenu::endTouch(Vec2 point)
{
    const int previous = pressed_;
    pressed_           = -1;
    rb::pressShopUnlock(unlock_, false);
    playP_->setVisible(false);
    backP_->setVisible(false);
    for (int i = 0; i < 4; ++i)
    {
        mainP_[i]->setVisible(false);
        optionsP_[i]->setVisible(false);
    }
    shopCursor_->setVisible(false);
    if (previous >= 0 && hit(point) == previous)
        click(previous);
}
