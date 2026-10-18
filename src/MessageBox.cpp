#include "MessageBox.h"
#include "NodeFactory.h"
#include "Assets.h"
using namespace ax;
GameMessageBox* GameMessageBox::create(std::string_view text,
                                       int language,
                                       bool choice,
                                       std::function<void(bool)> complete,
                                       std::function<void(std::string_view)> sound)
{
    auto* value      = rb::make<GameMessageBox>();
    value->choice_   = choice;
    value->complete_ = std::move(complete);
    if (sound)
        sound("snd_f12");
    value->setup(text, language);
    return value;
}
void GameMessageBox::setup(std::string_view text, int language)
{
    setContentSize({480, 320});
    bg_ = rb::sprite("mes_bg.png");
    addChild(bg_);
    bg_->setPosition(240, 160);
    bg_->setScale(0);
    auto line = [&](std::string_view value, float y) {
        if (!value.empty() && value.back() == '\r')
            value.remove_suffix(1);
        auto* label = rb::label(value, language, 11);
        label->setColor(Color3B::BLACK);
        bg_->addChild(label);
        const auto size = label->getContentSize();
        label->setPosition(58 + size.width / 2, bg_->getContentSize().height - y - size.height / 2);
    };
    const auto newline = text.find('\n');
    if (newline == std::string_view::npos)
        line(text, 31);
    else
    {
        line(text.substr(0, newline), 24);
        auto rest = text.substr(newline + 1);
        line(rest.substr(0, rest.find('\n')), 39);
    }
    const char* names[] = {"yes", "no", "ok"};
    const float x[]     = {163, 223, 103};
    for (int i = 0; i < 3; ++i)
    {
        buttons_[i]       = rb::place(bg_, std::string("mes_") + names[i] + ".png", x[i], 73);
        pressedImages_[i] = rb::place(bg_, std::string("mes_") + names[i] + "_p.png", x[i], 73);
        buttons_[i]->setVisible(choice_ ? i < 2 : i == 2);
        pressedImages_[i]->setVisible(false);
    }
    auto* touch = EventListenerTouchOneByOne::create();
    touch->setSwallowTouches(true);
    touch->onTouchBegan     = [this](Touch* t, Event*) { return beginTouch(t->getLocation()); };
    touch->onTouchEnded     = [this](Touch* t, Event*) { endTouch(t->getLocation()); };
    touch->onTouchCancelled = [this](Touch*, Event*) {
        pressed_ = -1;
        for (auto* p : pressedImages_)
            p->setVisible(false);
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(touch, this);
    bg_->runAction(Sequence::create(EaseOut::create(ScaleTo::create(.3f, 1.1f), .25f), ScaleTo::create(.1f, 1),
                                    CallFunc::create([this] { ready_ = true; }), nullptr));
}
int GameMessageBox::hit(Vec2 point) const
{
    for (int i = 0; i < 3; ++i)
        if (buttons_[i]->isVisible() && rb::hitTest(buttons_[i], point))
            return i;
    return -1;
}
bool GameMessageBox::beginTouch(Vec2 point)
{
    pressed_ = -1;
    if (ready_)
    {
        pressed_ = hit(point);
        if (pressed_ >= 0)
            pressedImages_[pressed_]->setVisible(true);
    }
    return true;
}
void GameMessageBox::endTouch(Vec2 point)
{
    const int previous = pressed_;
    pressed_           = -1;
    for (auto* p : pressedImages_)
        p->setVisible(false);
    if (ready_ && previous >= 0 && hit(point) == previous)
        answer(previous == 0);
}
void GameMessageBox::answer(bool yes)
{
    if (!ready_)
        return;
    ready_ = false;
    bg_->runAction(Sequence::create(ScaleTo::create(.1f, 1.1f), EaseOut::create(ScaleTo::create(.3f, 0), .25f),
                                    CallFunc::create([this, yes] {
        const bool accepted = choice_ && yes;
        auto callback       = std::move(complete_);
        removeFromParent();
        if (callback)
            callback(accepted);
    }),
                                    nullptr));
}
