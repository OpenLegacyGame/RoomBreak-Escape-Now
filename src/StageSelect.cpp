#include "StageSelect.h"
#include "NodeFactory.h"
#include "Assets.h"
#include <algorithm>
#include <cmath>
using namespace ax;
namespace
{
bool contains(Node* n, Vec2 p)
{
    return n && n->isVisible() && n->getScale() != 0 && rb::hitTest(n, p);
}
}  // namespace
StageSelect* StageSelect::create(const rb::GameData& data, rb::Progress& progress, int language, int displayedEpisode)
{
    auto* node      = rb::make<StageSelect>();
    node->data_     = &data;
    node->progress_ = &progress;
    node->language_ = language;
    if (displayedEpisode >= 0)
    {
        node->episode_ = displayedEpisode;
        node->opened_  = true;
    }
    node->setup();
    return node;
}
void StageSelect::setup()
{
    setContentSize({480, 320});
    rb::place(this, "main_gear_bg1.png", 18, 172);
    const float angle[] = {29, -20, -60}, startX[] = {0, 126, 10}, startY[] = {200, 320, 320};
    for (int g = 0; g < 3; ++g)
        for (int i = 0; i < 75; ++i)
        {
            auto* link = rb::sprite("main_chain.png");
            addChild(link);
            const auto size = link->getContentSize();
            const float rad = angle[g] * 3.14f / 180;
            link->setPosition(startX[g] + i * size.width * std::cos(rad) + size.width / 2,
                              320 - startY[g] - i * size.width * std::sin(rad) - size.height / 2);
            link->setRotation(angle[g]);
            chains_[g].push_back(link);
            chainOrigins_[g].push_back(link->getPosition());
        }
    const Vec2 gearPos[] = {{41, 175}, {76, 215}, {34, 224}};
    for (int i = 0; i < 3; ++i)
        gears_[i] = rb::place(this, "main_gear_" + std::to_string(i + 1) + ".png", gearPos[i].x, gearPos[i].y);
    stageGroup_ = Node::create();
    stageGroup_->setContentSize({480, 320});
    addChild(stageGroup_);
    buildSymbols();
    for (int i = 0; i < 7; ++i)
    {
        titles_[i] =
            rb::place(this, i == 0 ? "main_tutorial.png" : "main_ep" + std::to_string(i) + "_title.png", 20, 15);
        titles_[i]->setOpacity(i == episode_ ? 255 : 0);
    }
    const float xs[] = {75, 60, 30, 15, 375, 390, 420, 435};
    for (int i = 0; i < 8; ++i)
    {
        arrows_[i] = rb::place(this, "main_arrow.png", xs[i], 140);
        arrows_[i]->setFlippedX(i < 4);
        arrows_[i]->setOpacity(i < 4 ? 0 : 50);
    }
    back_ = rb::sprite("main_back.png");
    addChild(back_, 20);
    back_->setPosition(470 - back_->getContentSize().width / 2, 310 - back_->getContentSize().height / 2);
    backPressed_ = rb::sprite("main_back_p.png");
    addChild(backPressed_, 21);
    backPressed_->setPosition(back_->getPosition());
    backPressed_->setVisible(false);
    notification_ = rb::label(" ", 0, 11);
    addChild(notification_, 20);
    notification_->setPosition(240, 25);
    notify("SELECT_EPISODE");
    auto* listener = EventListenerTouchOneByOne::create();
    listener->setSwallowTouches(true);
    listener->onTouchBegan = [this](Touch* t, Event*) {
        if (!isVisible() || moving_)
            return false;
        pressed_       = hit(t->getLocation());
        dragged_       = false;
        dragCommitted_ = false;
        dragDistance_  = 0;
        backPressed_->setVisible(pressed_ == 100);
        if (pressed_ >= 1 && pressed_ <= 5 && opened_)
        {
            stagePressed_->setPosition(stages_[pressed_ - 1]->getPosition());
            stagePressed_->setVisible(true);
        }
        return true;
    };
    listener->onTouchMoved = [this](Touch* t, Event*) {
        if (opened_ || pressed_ == 100 || moving_ || dragCommitted_)
            return;
        float dx = t->getDelta().x;
        dragged_ = true;
        for (auto* s : symbols_)
            s->setPositionX(s->getPositionX() + dx);
        moveMechanism(dx);
        dragDistance_ += dx;
        if (std::abs(dragDistance_) >= 150)
        {
            const int next = std::clamp(episode_ + (dragDistance_ < 0 ? 1 : -1), 0, 6);
            dragCommitted_ = true;
            sortSymbols(next);
            playSound("snd_f25.wav");
            dragDistance_ = 0;
        }
    };
    listener->onTouchEnded = [this](Touch* t, Event*) {
        backPressed_->setVisible(false);
        if (stagePressed_)
            stagePressed_->setVisible(false);
        if (dragged_)
        {
            if (!dragCommitted_ && !moving_)
                sortSymbols(episode_);
        }
        else if (pressed_ == hit(t->getLocation()))
            activate(pressed_);
        pressed_ = -1;
        dragged_ = false;
    };
    listener->onTouchCancelled = [this](Touch*, Event*) {
        backPressed_->setVisible(false);
        if (stagePressed_)
            stagePressed_->setVisible(false);
        if (dragged_ && !dragCommitted_ && !moving_)
            sortSymbols(episode_);
        pressed_ = -1;
        dragged_ = false;
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(listener, this);
    schedule([this](float) {
        if (!dragged_)
            moveMechanism(.1f);
    }, "mechanism");
    if (opened_)
    {
        symbols_[episode_]->setOpacity(0);
        for (auto* door : doors_[episode_])
            door->setOpacity(0);
        for (auto* arrow : arrows_)
            arrow->setOpacity(0);
        showStages(false);
    }
    updateArrows();
}
void StageSelect::buildSymbols()
{
    const float positions[7][4] = {{-6, 18, 90, 18}, {0, 17, 79, 20},     {14, 26, 111, 30}, {0, 0, 92, 0},
                                   {21, 18, 94, 18}, {-10, -23, 79, -25}, {0, 14, 118, 14}};
    const float locks[6][6]     = {{0, 0, 0, 0, 0, 0},      {107, 104, 16, 118, 132, 72}, {75, 90, -2, 47, 140, 125},
                                   {24, 85, 0, 58, 71, 63}, {68, 91, -8, 90, 115, 46},    {50, 106, -19, 92, 83, 68}};
    for (int e = 0; e < 7; ++e)
    {
        auto prefix  = "main_ep" + std::to_string(e);
        auto* symbol = rb::sprite(prefix + "_1.png");
        addChild(symbol);
        symbols_[e] = symbol;
        symbol->setPosition(240 + 480 * (e - episode_), 160);
        for (int d = 0; d < 2; ++d)
        {
            auto* door         = rb::place(symbol, prefix + "_" + std::to_string(d + 2) + ".png", positions[e][2 * d],
                                           positions[e][2 * d + 1], e == 6 ? 2 - d : 1 + d);
            doors_[e][d]       = door;
            doorOrigins_[e][d] = door->getPosition();
        }
        if (e && !progress_->episodeOpened(e))
        {
            symbol->setColor(Color3B(120, 120, 120));
            for (auto* door : doors_[e])
                door->setColor(Color3B(120, 120, 120));
            if (e < 6)
                for (int n = 0; n < 3; ++n)
                    rb::place(symbol, prefix + "_loc" + std::to_string(n + 1) + ".png", locks[e][2 * n],
                              locks[e][2 * n + 1], 3);
        }
    }
}
void StageSelect::notify(const char* key)
{
    notification_->setString(rb::findOtherText(language_, key).value_or(""));
}
int StageSelect::hit(Vec2 p) const
{
    if (contains(back_, p))
        return 100;
    if (opened_)
    {
        for (int i = 0; i < (episode_ ? 5 : 1); ++i)
            if (contains(stages_[i], p))
                return i + 1;
        for (int i = 0; i < 2; ++i)
            if (contains(movies_[i], p))
                return 200 + i;
    }
    else
    {
        if (contains(symbols_[episode_], p))
            return 10;
        if (p.x < 100 && episode_ > 0)
            return 11;
        if (p.x > 370 && episode_ < 6)
            return 12;
    }
    return -1;
}
void StageSelect::activate(int id)
{
    if (id == 100)
    {
        if (opened_)
            closeEpisode();
        else
        {
            playSound("snd_f19.wav");
            if (back)
                back();
        }
    }
    else if (id == 11)
        sortSymbols(episode_ - 1);
    else if (id == 12)
        sortSymbols(episode_ + 1);
    else if (id == 10)
        selectEpisode();
    else if (opened_ && id >= 1 && id <= 5)
    {
        const int dbEpisode = rb::databaseEpisode(episode_);
        if (progress_->stageOpened(*data_, dbEpisode, id))
        {
            playSound("snd_f19.wav");
            if (play)
                play(dbEpisode, id);
        }
        else
        {
            playSound("snd_f20.wav");
            notify("LOCK_STAGE");
        }
    }
    else if (opened_ && id >= 200 && id <= 201)
    {
        const bool ending    = id == 201;
        const int dbEpisode  = rb::databaseEpisode(episode_);
        const bool available = (ending ? progress_->seenEndings : progress_->seenIntros).contains(dbEpisode);
        if (available)
        {
            playSound("snd_f19.wav");
            if (movie)
                movie(dbEpisode, ending);
        }
        else
        {
            playSound("snd_f20.wav");
            notify(ending ? "LOCK_ENDING" : "LOCK_INTRO");
        }
    }
}
void StageSelect::sortSymbols(int e)
{
    episode_ = e;
    moving_  = true;
    for (int i = 0; i < 7; ++i)
    {
        symbols_[i]->stopAllActions();
        symbols_[i]->runAction(EaseIn::create(MoveTo::create(.5f, {240.f + 480 * (i - e), 160}), .5f));
        titles_[i]->runAction(FadeTo::create(.3f, i == e ? 255 : 0));
    }
    updateArrows();
    scheduleOnce([this](float) { moving_ = false; }, .5f, "sort-complete");
}
void StageSelect::selectEpisode()
{
    if (episode_ && !progress_->episodeOpened(episode_))
    {
        playSound("snd_f20.wav");
        notify("CLEAR_EPISODE");
        return;
    }
    moving_ = true;
    opened_ = true;
    playSound("snd_f22.wav");
    updateArrows();
    for (int i = 0; i < 2; ++i)
    {
        auto* door      = doors_[episode_][i];
        float direction = i ? 1 : -1;
        auto p          = doorOrigins_[episode_][i];
        door->runAction(
            Sequence::create(EaseIn::create(MoveTo::create(.5f, p + Vec2(direction * 10, 0)), .25f),
                             EaseOut::create(Spawn::create(MoveTo::create(.5f, p + Vec2(direction * 280, 0)),
                                                           FadeOut::create(.5f), ScaleTo::create(.5f, 3), nullptr),
                                             .25f),
                             nullptr));
    }
    symbols_[episode_]->runAction(
        Sequence::create(DelayTime::create(.5f), CallFunc::create([this] { showStages(); }),
                         EaseOut::create(Spawn::create(FadeOut::create(.5f), ScaleTo::create(.5f, 3), nullptr), .25f),
                         CallFunc::create([this] { moving_ = false; }), nullptr));
}
void StageSelect::showStages(bool animate)
{
    stageGroup_->removeAllChildren();
    stages_.fill(nullptr);
    movies_.fill(nullptr);
    notify("SELECT_STAGE");
    const int count = episode_ ? 5 : 1, dbEpisode = rb::databaseEpisode(episode_);
    for (int i = 0; i < count; ++i)
    {
        auto* stage = rb::sprite("main_room" + std::to_string(i + 1) + ".png");
        stageGroup_->addChild(stage);
        stages_[i] = stage;
        stage->setPosition(240, 160);
        stage->setScale(0);
        auto* rank        = rb::place(stage, "main_rank_bg.png", 12, 82);
        auto rows         = data_->stage("StageTable", dbEpisode, i + 1);
        const auto& saved = progress_->stages.at(rb::integer(*rows.at(0), "stageID"));
        if (!saved.rank.empty())
            rb::place(rank, "main_rank_" + saved.rank + ".png", 0, 0);
        rb::place(stage, "main_get_goldkey_bg.png", 20, -35);
        if (saved.goldKey)
            rb::place(stage, "main_get_goldkey.png", 20, -35);
        if (!saved.opened)
        {
            auto* locked = rb::sprite("main_room_n.png");
            stage->addChild(locked);
            locked->setPosition(stage->getContentSize() / 2);
        }
        const Vec2 target(240 - (stage->getContentSize().width + 10) * (count / 2 - i), 190);
        if (animate)
            stage->runAction(
                Sequence::create(Spawn::create(EaseOut::create(ScaleTo::create(.5f, 1.1f), .25f),
                                               EaseOut::create(MoveTo::create(.5f, target), .25f), nullptr),
                                 ScaleTo::create(.1f, 1), nullptr));
        else
        {
            stage->setPosition(target);
            stage->setScale(1);
        }
    }
    stagePressed_ = rb::sprite("main_room_p.png");
    stageGroup_->addChild(stagePressed_, 5);
    stagePressed_->setVisible(false);
    if (episode_)
    {
        auto* bg = rb::place(stageGroup_, "main_movie_bg.png", 224, 224);
        for (int i = 0; i < 2; ++i)
        {
            movies_[i] = rb::place(bg, "main_movie_p.png", i ? 126 : 30, 0);
            movies_[i]->setOpacity(0);
            const bool seen = (i ? progress_->seenEndings : progress_->seenIntros).contains(dbEpisode);
            if (!seen)
                rb::place(bg, "main_movie_n.png", i ? 126 : 30, 0);
        }
    }
}
void StageSelect::closeEpisode()
{
    playSound("snd_f23.wav");
    moving_ = true;
    opened_ = false;
    notify("SELECT_EPISODE");
    for (auto* child : stageGroup_->getChildren())
    {
        child->stopAllActions();
        child->runAction(Spawn::create(EaseIn::create(ScaleTo::create(.5f, 0), .25f),
                                       EaseIn::create(MoveTo::create(.5f, {240, 160}), .25f), nullptr));
    }
    scheduleOnce([this](float) {
        stageGroup_->removeAllChildren();
        stages_.fill(nullptr);
        movies_.fill(nullptr);
        stagePressed_ = nullptr;
    }, .5f, "hide-stages");
    auto* symbol = symbols_[episode_];
    symbol->stopAllActions();
    symbol->setScale(3);
    symbol->setOpacity(0);
    const auto center = symbol->getPosition();
    auto* shake       = Repeat::create(
        Sequence::create(MoveTo::create(.05f, center + Vec2(-1, 0)), MoveTo::create(.05f, center), nullptr), 2);
    symbol->runAction(
        Sequence::create(EaseIn::create(Spawn::create(FadeIn::create(.5f), ScaleTo::create(.5f, 1), nullptr), .25f),
                         DelayTime::create(.5f), shake, CallFunc::create([this] { moving_ = false; }), nullptr));
    for (int i = 0; i < 2; ++i)
    {
        auto* door            = doors_[episode_][i];
        const auto origin     = doorOrigins_[episode_][i];
        const float direction = i ? 1 : -1;
        door->stopAllActions();
        door->setScale(3);
        door->setOpacity(0);
        door->setPosition(origin + Vec2(direction * 280, 0));
        auto* settle = Repeat::create(
            Sequence::create(MoveTo::create(.05f, origin + Vec2(-1, 0)), MoveTo::create(.05f, origin), nullptr), 2);
        door->runAction(
            Sequence::create(EaseIn::create(Spawn::create(MoveTo::create(.5f, origin + Vec2(direction * 10, 0)),
                                                          FadeIn::create(.5f), ScaleTo::create(.5f, 1), nullptr),
                                            .25f),
                             EaseOut::create(MoveTo::create(.5f, origin), .25f), settle, nullptr));
    }
    updateArrows();
}
void StageSelect::updateArrows()
{
    unschedule("left-arrow");
    unschedule("right-arrow");
    for (auto* arrow : arrows_)
    {
        arrow->stopAllActions();
        arrow->runAction(FadeTo::create(.3f, 0));
    }
    if (opened_)
        return;
    if (episode_ > 0)
        schedule([this](float) { pulseArrows(true); }, 1.5f, "left-arrow");
    if (episode_ < 6)
        schedule([this](float) { pulseArrows(false); }, 1.5f, "right-arrow");
}
void StageSelect::pulseArrows(bool left)
{
    for (int i = 0; i < 4; ++i)
    {
        auto* arrow = arrows_[(left ? 0 : 4) + i];
        arrow->stopAllActions();
        arrow->runAction(FadeTo::create(.3f, 0));
        arrow->runAction(
            Sequence::create(DelayTime::create(i * .1f), FadeTo::create(.3f, 200), FadeTo::create(.3f, 50), nullptr));
    }
}
void StageSelect::moveMechanism(float dx)
{
    const float angles[] = {29, -20, -60}, multiplier[] = {1, -.5f, -1.2f};
    for (int g = 0; g < 3; ++g)
    {
        gears_[g]->setRotation(gears_[g]->getRotation() + dx * (g == 1 ? -.8f : g == 2 ? 1.2f : 1));
        const float radians = angles[g] * 3.14f / 180;
        float offset        = chains_[g].front()->getPositionX() - chainOrigins_[g].front().x + dx * multiplier[g];
        const float period  = chains_[g].front()->getContentSize().width * std::cos(radians);
        offset              = std::fmod(offset, period);
        for (size_t i = 0; i < chains_[g].size(); ++i)
            chains_[g][i]->setPosition(chainOrigins_[g][i] + Vec2(offset, -offset * std::tan(radians)));
    }
}
