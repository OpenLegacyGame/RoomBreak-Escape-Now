#include "StoryDialog.h"
#include "NodeFactory.h"
#include "Episode5Dialog.h"
#include "PortraitDialog.h"
#include "Episode4Dialog.h"
#include "Episode2Dialog.h"
#include "Assets.h"
#include <stdexcept>
using namespace ax;
StoryDialog* StoryDialog::create(const rb::GameData& data, int episode, int scene, int language, int speed)
{
    auto* value      = rb::make<StoryDialog>();
    value->data_     = &data;
    value->language_ = language;
    value->setup(episode, scene, speed);
    return value;
}
void StoryDialog::setup(int episode, int scene, int speed)
{
    setContentSize({480, 320});
    const auto& master = data_->byId("scene_master", "sceneID", scene);
    rows_              = data_->where("SceneData", "sceneID", scene);
    shade_             = LayerColor::create(Color4B(0, 0, 0, 0), 480, 320);
    addChild(shade_);
    skip_ = rb::sprite("dia_skip.png");
    addChild(skip_);
    skipPressed_ = rb::sprite("dia_skip_p.png");
    addChild(skipPressed_);
    skipPressed_->setVisible(false);
    const auto size = skip_->getContentSize();
    skip_->setPosition(480 + size.width / 2, 320 - size.height / 2);
    skipPressed_->setPosition(skip_->getPosition());
    switch (episode)
    {
    case 0:
    case 5:
        skin_ = Episode5Dialog::create(language_, speed, rb::integer(master, "charactorView") != 0);
        break;
    case 1:
    case 3:
    case 6:
        skin_ = PortraitDialog::create(episode, language_, speed, rb::integer(master, "charactorView") != 0);
        break;
    case 4:
        skin_ = Episode4Dialog::create(language_, speed, rb::integer(master, "charactorView") != 0);
        break;
    case 2:
        skin_ = Episode2Dialog::create(language_, speed, rb::integer(master, "charactorView") != 0);
        break;
    default:
        throw std::runtime_error("Story skin not yet implemented for episode " + std::to_string(episode));
    }
    addChild(skin_);
    auto* touch = EventListenerTouchOneByOne::create();
    touch->setSwallowTouches(true);
    touch->onTouchBegan = [this](Touch* t, Event*) {
        pressed_ = !busy_ && !skin_->busy() && hitSkip(t->getLocation());
        skipPressed_->setVisible(pressed_);
        return true;
    };
    touch->onTouchEnded = [this](Touch* t, Event*) {
        const bool skip = pressed_ && hitSkip(t->getLocation());
        pressed_        = false;
        skipPressed_->setVisible(false);
        if (busy_)
            return;
        if (sound)
            sound("snd_f11", false);
        if (skin_->busy())
            return;
        if (skip)
            close();
        else
            advance();
    };
    touch->onTouchCancelled = [this](Touch*, Event*) {
        pressed_ = false;
        skipPressed_->setVisible(false);
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(touch, this);
}
bool StoryDialog::hitSkip(Vec2 point) const
{
    return rb::hitTest(skip_, point);
}
void StoryDialog::open()
{
    if (gameUI)
        gameUI(false);
    shade_->runAction(FadeTo::create(1, 100));
    const auto size = skip_->getContentSize();
    for (auto* sprite : {skip_, skipPressed_})
        sprite->runAction(EaseIn::create(MoveTo::create(.5f, {480 - size.width / 2 - 5, 320 - size.height / 2}), .25f));
    skin_->open([this] {
        busy_ = false;
        nextLine();
    });
}
void StoryDialog::nextLine()
{
    const rb::Row* row = nullptr;
    for (auto* candidate : rows_)
        if (rb::integer(*candidate, "sequence") == step_)
        {
            row = candidate;
            break;
        }
    if (!row)
    {
        close();
        return;
    }
    if (sound)
    {
        const auto bg = rb::string(*row, "bgSound"), effect = rb::string(*row, "effectSound");
        if (!bg.empty())
            sound(bg, true);
        if (!effect.empty())
            sound(effect, false);
    }
    if (rb::integer(*row, "vibrator") && vibrate)
        vibrate();
    if (rb::integer(*row, "shake") && shake)
        shake();
    skin_->play(data_->text(std::stoi(rb::string(*row, "dialog")), language_), rb::integer(*row, "charactor") != 0);
    ++step_;
}
void StoryDialog::advance()
{
    if (!busy_ && skin_->next())
        nextLine();
}
void StoryDialog::close()
{
    if (closing_)
        return;
    closing_ = true;
    busy_    = true;
    skin_->close([this] { closeOverlay(); });
}
void StoryDialog::closeOverlay()
{
    if (gameUI)
        gameUI(true);
    const auto size = skip_->getContentSize();
    for (auto* sprite : {skip_, skipPressed_})
        sprite->runAction(EaseIn::create(MoveTo::create(.5f, {480 + size.width / 2, 320 - size.height / 2}), .25f));
    shade_->runAction(Sequence::create(FadeTo::create(1, 0), CallFunc::create([this] {
        auto done = complete;
        if (done)
            done();
    }),
                                       nullptr));
}
