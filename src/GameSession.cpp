#include "GameSession.h"
#include "NodeFactory.h"
#include "RoomView.h"
#include "StoryDialog.h"
#include "Assets.h"
#include "CursorPocket.h"
#include "CursorFeedback.h"
#include "GetItemDialog.h"
#include "InventoryView.h"
#include "InsightOverlay.h"
#include "Progress.h"
#include "MessageBox.h"
#include "AbilityEffect.h"
#include "AbilityInfo.h"
#include "HeroHealth.h"
#include "PauseButton.h"
#include "StageIntro.h"
#include "GameOverOverlay.h"
#include "ClearOverlay.h"
#include "DetailView.h"
#include "ActionCut.h"
#include "ActionHit.h"
#include <stdexcept>
using namespace ax;
namespace
{
// Finger taps include small move events; a drag starts only beyond six logical points.
constexpr float tapSlopSquared = 36;
}  // namespace
GameSession* GameSession::create(const rb::GameData& data,
                                 int episode,
                                 int stage,
                                 int language,
                                 int speed,
                                 bool goldKey)
{
    auto* value = rb::make<GameSession>();
    value->setup(data, episode, stage, language, speed, goldKey);
    return value;
}
void GameSession::setup(const rb::GameData& data, int episode, int stage, int language, int speed, bool goldKey)
{
    setContentSize({480, 320});
    data_     = &data;
    language_ = language;
    speed_    = speed;
    state_.reset(data, episode, stage, goldKey);
    // HeroHealthLayer::init overwrites both globals after DataControl loads the stage (0x654d2..654de).
    maxHealth_    = 100;
    state_.health = 100;
    heroHealth_   = HeroHealth::create(maxHealth_, state_.health);
    addChild(heroHealth_, 7);
    room_ = RoomView::create(data, state_);
    addChild(room_);
    events_ = std::make_unique<rb::EventRunner>(
        data, [this](const rb::Command& command, auto done) { dispatch(command, std::move(done)); });
    events_->afterGroup = [this] { checkConditions(); };
    setupHud();
    setupTouch();
}
void GameSession::setupHud()
{
    feedback_ = CursorFeedback::create(*data_);
    addChild(feedback_, 5);
    feedback_->sound = effectSound();
    pocket_          = CursorPocket::create(*data_);
    addChild(pocket_, 7);
    pocket_->changed = [this](int cursor, int item) { selectCursor(cursor, item); };
    pocket_->blocked = [this] { return interfaceBlocked(); };
    inventory_       = InventoryView::create(*data_, state_, language_);
    addChild(inventory_, 8);
    inventory_->blocked = [this] { return interfaceBlocked(); };
    inventory_->otherUI = [this](bool visible) {
        pocket_->show(visible);
        pauseButton_->show(visible);
        heroHealth_->show(visible);
        if (abilityInfo_)
            abilityInfo_->show(visible);
        if (gameUI)
            gameUI(visible);
    };
    inventory_->changed = [this] {
        if (inventoryChanged)
            inventoryChanged();
    };
    inventory_->selectItem = [this](int item) { selectCursor(3, item); };
    inventory_->sound      = effectSound();
    pauseButton_           = PauseButton::create();
    addChild(pauseButton_, 7);
    pauseButton_->blocked  = [this] { return interfaceBlocked() || inventory_->opened(); };
    pauseButton_->activate = [this] {
        if (pauseRequested)
            pauseRequested();
        else
            openSystemMenu();
    };
}
void GameSession::setupTouch()
{
    auto* touch = EventListenerTouchOneByOne::create();
    touch->setSwallowTouches(true);
    touch->onTouchBegan = [this](Touch* t, Event*) {
        if (interfaceBlocked() || inventory_->opened())
            return false;
        dragged_      = false;
        gestureStart_ = t->getLocation();
        room_->beginDrag();
        return true;
    };
    touch->onTouchMoved = [this](Touch* t, Event*) {
        const auto displacement = t->getLocation() - gestureStart_;
        if (!dragged_)
        {
            // Commit the drag with the displacement accumulated so far.
            if (displacement.lengthSquared() <= tapSlopSquared)
                return;
            dragged_ = true;
            room_->drag(displacement.x);
        }
        else
            room_->drag(t->getDelta().x);
    };
    touch->onTouchEnded = [this](Touch* t, Event*) {
        if (!dragged_ && (t->getLocation() - gestureStart_).lengthSquared() <= tapSlopSquared)
            tap(t->getLocation());
        room_->settle();
        pocket_->close();
    };
    touch->onTouchCancelled = [this](Touch*, Event*) { room_->settle(); };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(touch, this);
}
void GameSession::showUI(bool visible)
{
    pocket_->show(visible);
    inventory_->show(visible);
    pauseButton_->show(visible);
    heroHealth_->show(visible);
    if (abilityInfo_)
        abilityInfo_->show(visible);
    if (gameUI)
        gameUI(visible);
}
bool GameSession::interfaceBlocked() const
{
    return ended_ || detail_ || action_ || intro_ || menuOpen_ || ability_ || message_ || insight_ || changingRoom_ ||
           touchProtected() || story_ || itemDialog_;
}
bool GameSession::abilityBlocked(int type) const
{
    // Hands may solve an open detail or action mode while its event is still pending.
    const bool handsInOverlay = (detail_ || action_) && type == 2;
    return ended_ || ((detail_ || action_) && type != 2) || intro_ || menuOpen_ || ability_ || message_ || insight_ ||
           changingRoom_ || (busy() && !handsInOverlay) || story_ || itemDialog_;
}
std::function<void(std::string_view)> GameSession::effectSound()
{
    return [this](std::string_view file) {
        if (sound)
            sound(file, false);
    };
}
void GameSession::setMenuOpen(bool open)
{
    menuOpen_ = open;
    showUI(!open);
}
void GameSession::showClear()
{
    if (ended_)
        return;
    if (!progress_)
        throw std::runtime_error("Clear requires bound progress");
    const auto rank = progress_->clear(*data_, state_.episode, state_.stage, touchCount_);
    ended_          = true;
    showUI(false);
    if (progressChanged)
        progressChanged();
    if (menuAudio)
        menuAudio(true);
    clear_ = ClearOverlay::create(rank);
    addChild(clear_, 20);
    clear_->sound    = effectSound();
    clear_->complete = [this](bool accepted) {
        clear_ = nullptr;
        if (accepted)
        {
            if (clearAccepted)
                clearAccepted();
        }
        else if (retryRequested)
            retryRequested();
    };
    clear_->open();
}
void GameSession::showGameOver()
{
    if (ended_)
        return;
    ended_ = true;
    showUI(false);
    gameOver_  = GameOverOverlay::create();
    auto* over = gameOver_;
    addChild(over, 20);
    over->sound    = effectSound();
    over->complete = [this](bool exitToMenu) {
        gameOver_ = nullptr;
        if (exitToMenu)
        {
            if (exitRequested)
                exitRequested();
        }
        else if (retryRequested)
            retryRequested();
    };
    over->open();
}
void GameSession::startWithIntro()
{
    if (begun_ || intro_)
        throw std::runtime_error("Stage has already started");
    intro_      = StageIntro::create(state_.stage);
    auto* intro = intro_;
    addChild(intro, 20);
    intro->sound    = effectSound();
    intro->complete = [this, intro] {
        intro_ = nullptr;
        intro->removeFromParent();
        begin();
    };
    intro->start();
}
std::string GameSession::defaultMusic() const
{
    const auto& stage = *data_->stage("StageTable", state_.episode, state_.stage).front();
    const int id      = rb::integer(stage, "soundID");
    return id ? rb::string(data_->byId("SoundData", "soundID", id), "fileName") : std::string();
}
void GameSession::begin()
{
    if (begun_)
        throw std::runtime_error("GameSession already started");
    begun_ = true;
    pocket_->show(true);
    inventory_->show(true);
    pauseButton_->show(true);
    heroHealth_->show(true);
    if (abilityInfo_)
        abilityInfo_->show(true);
    const auto& stage = *data_->stage("StageTable", state_.episode, state_.stage).front();
    if (sound)
        sound(defaultMusic(), true);
    if (rb::integer(stage, "isSetEvent"))
        runEvent(rb::integer(stage, "eventID"));
}
void GameSession::fillHealth()
{
    state_.health = maxHealth_;
    heroHealth_->fill();
    if (healthChanged)
        healthChanged();
}
void GameSession::onExit()
{
    events_->cancel();
    Node::onExit();
}
void GameSession::runEvent(int id)
{
    events_->start(id);
}
void GameSession::openSystemMenu(int mode)
{
    if (systemMenu_)
        return;
    if (!progress_)
        throw std::runtime_error("Menu requires persistent progress");
    systemMenu_ = SystemMenu::create(*progress_, language_, menuSettings);
    auto* menu  = systemMenu_;
    addChild(menu, 20);
    setMenuOpen(true);
    if (menuAudio)
        menuAudio(true);
    menu->sound           = effectSound();
    menu->settingsChanged = [this](SystemMenu::Settings settings) {
        menuSettings = settings;
        speed_       = settings.textSpeed;
        if (settingsChanged)
            settingsChanged(settings);
    };
    menu->resumed = [this] {
        systemMenu_ = nullptr;
        setMenuOpen(false);
        if (menuAudio)
            menuAudio(false);
    };
    menu->retry = [this] {
        if (retryRequested)
            retryRequested();
    };
    menu->exitGame = [this] {
        if (exitRequested)
            exitRequested();
    };
    menu->unlockAll = [this] {
        if (unlockAllRequested)
            unlockAllRequested();
    };
    menu->purchase = [this](int product) {
        if (purchaseRequested)
            purchaseRequested(product);
    };
    menu->open(mode);
}
void GameSession::bindProgress(rb::Progress& progress, std::function<void()> shop)
{
    if (progress_)
        throw std::runtime_error("Progress already bound");
    progress_    = &progress;
    shop_        = shop ? std::move(shop) : std::function<void()>([this] { openSystemMenu(3); });
    abilityInfo_ = AbilityInfo::create(progress);
    addChild(abilityInfo_, 7);
    abilityInfo_->request = [this](int type) { requestAbility(type); };
    abilityInfo_->blocked = [this] { return interfaceBlocked() || inventory_->opened(); };
    if (begun_)
        abilityInfo_->show(true);
    inventory_->insightCount   = [this] { return progress_->insight; };
    inventory_->requestInsight = [this](int item) { requestInsight(item); };
    inventory_->refreshAbility();
}
void GameSession::requestInsight(int item)
{
    requestAbility(1, item);
}
void GameSession::requestAbility(int type, int item, std::function<void()> complete)
{
    if (!progress_)
        throw std::runtime_error("Ability progress is not bound");
    if (abilityBlocked(type))
        return;
    const int count = type == 0   ? progress_->eyes
                      : type == 1 ? progress_->insight
                      : type == 2 ? progress_->hands
                      : type == 3 ? progress_->feet
                                  : -1;
    if (count < 0)
        throw std::runtime_error("Invalid ability count or type");
    const bool tutorial = state_.episode == 0, enough = count > 0;
    const char* key = tutorial ? "NO_TUTO" : enough ? "IS_USE" : "CASHWARNING";
    message_ = GameMessageBox::create(rb::otherText(language_, key), language_, !tutorial,
                                      [this, tutorial, enough, item, type, complete = std::move(complete)](bool yes) {
        message_ = nullptr;
        if (!yes || tutorial)
            return;
        if (enough)
        {
            if (type == 1)
                useInsight(*progress_, item);
            else
                useAbility(*progress_, type, std::move(complete));
        }
        else if (detail_)
        {
            detailShop_ = true;
            detail_->close();
        }
        else if (action_)
        {
            actionShop_ = true;
            action_->close();
        }
        else
            shop_();
    }, effectSound());
    addChild(message_, 20);
}
bool GameSession::useAbility(rb::Progress& progress, int type, std::function<void()> complete)
{
    int* count = type == 0 ? &progress.eyes : type == 2 ? &progress.hands : type == 3 ? &progress.feet : nullptr;
    if (!count)
        throw std::runtime_error("Invalid ability type");
    if (state_.episode == 0 || *count < 1 || abilityBlocked(type))
        return false;
    auto* effect = AbilityEffect::create(type);
    ability_     = effect;
    addChild(effect, 9);
    --*count;
    if (abilityInfo_)
        abilityInfo_->refresh();
    if (detail_)
        detail_->setHands(progress.hands);
    if (action_)
        action_->setHands(progress.hands);
    if (progressChanged)
        progressChanged();
    effect->sound    = effectSound();
    effect->complete = [this, effect, type, &progress, complete = std::move(complete)] {
        if (type == 0)
        {
            for (auto& [id, state] : state_.objects)
                if (rb::integer(data_->byId("ObjectData", "objectID", id), "objectType") == 1)
                    state.shining = true;
            room_->refreshShining();
        }
        ability_ = nullptr;
        effect->removeFromParent();
        progress.achievements.insert(12);
        if (progressChanged)
            progressChanged();
        if (type == 3)
            dispatch(rb::Command{rb::Command::Type::Clear, ""}, [](bool) {});
        if (complete)
            complete();
    };
    effect->start();
    return true;
}
bool GameSession::useInsight(rb::Progress& progress, int item)
{
    if (state_.episode == 0 || progress.insight < 1)
        return false;
    if (abilityBlocked(1))
        return false;
    if (!state_.items.contains(item) || !state_.items.at(item))
        throw std::runtime_error("Insight item is not held");
    auto* value = InsightOverlay::create(*data_, *this, item, language_);
    --progress.insight;
    inventory_->refreshAbility();
    if (progressChanged)
        progressChanged();
    inventory_->close();
    insight_ = value;
    addChild(value, 9);
    value->sound    = effectSound();
    value->complete = [this, value, &progress] {
        progress.achievements.insert(12);
        if (progressChanged)
            progressChanged();
        insight_ = nullptr;
        value->removeFromParent();
    };
    value->start();
    return true;
}
void GameSession::moveRoom(int room, int position, std::function<void()> complete)
{
    if (changingRoom_)
        throw std::runtime_error("Concurrent room transitions");
    const auto& row = data_->byId("RoomData", "roomID", room);
    if (rb::integer(row, "episodeNumber") != state_.episode || rb::integer(row, "stageNumber") != state_.stage)
        throw std::runtime_error("Room transition leaves current stage");
    changingRoom_  = true;
    state_.room    = room;
    cursorOutcome_ = 2;
    auto* shade    = LayerColor::create(Color4B(0, 0, 0, 0), 480, 320);
    addChild(shade, 6);
    shade->runAction(
        Sequence::create(EaseOut::create(FadeIn::create(.5f), .25f), CallFunc::create([this, room, position] {
        room_->loadRoom(room, position >= 0 && position <= 4 ? position : 0);
        if (roomChanged)
            roomChanged();
    }),
                         EaseOut::create(FadeOut::create(.5f), .25f), CallFunc::create([this, shade, complete] {
        changingRoom_ = false;
        shade->removeFromParent();
        complete();
    }),
                         nullptr));
}
void GameSession::selectCursor(int cursor, int item)
{
    if (cursor < 0 || cursor > 3 || (cursor == 3 && !item))
        throw std::runtime_error("Invalid cursor selection");
    if (cursor == 3 && (!state_.items.contains(item) || !state_.items.at(item)))
        throw std::runtime_error("Selected item is not in inventory");
    cursor_ = cursor;
    item_   = cursor == 3 ? item : 0;
    if (cursor == 3 && (pocket_->item() != item || pocket_->selected() != 3))
        pocket_->setItem(item);
    else if (pocket_->selected() != cursor)
        pocket_->select(cursor);
}
void GameSession::tap(Vec2 point)
{
    if (interfaceBlocked() || inventory_->opened())
        return;
    touchPoint_        = point;
    cursorOutcome_     = 4;
    const int usedItem = item_;
    touchObject_       = room_->hitObject(point);
    if (touchObject_)
    {
        const int id = state_.eventFor(*data_, touchObject_, cursor_, item_);
        if (id)
        {
            if (item_)
            {
                const auto& item = data_->byId("ItemData", "itemID", item_);
                if (rb::integer(item, "consume"))
                {
                    state_.removeItem(item_);
                    pocket_->consumeItem();
                    if (inventoryChanged)
                        inventoryChanged();
                }
                selectCursor(0);
                cursorOutcome_ = 3;
            }
            runEvent(id);
        }
    }
    feedback_->play(point, cursor_, cursorOutcome_, usedItem);
    if (cursorAction)
        cursorAction(point, cursorOutcome_);
    // useHeroHealth exempts the tutorial and the clear command, and checks
    // exhaustion before decrementing (0x65848).
    if (state_.episode != 0 && lastType_ != 4)
    {
        if (state_.health < 1)
        {
            if (gameOver)
                gameOver();
            else
                showGameOver();
        }
        else
        {
            --state_.health;
            heroHealth_->updateHealth(state_.health);
            if (healthChanged)
                healthChanged();
        }
    }
    ++touchCount_;
}
void GameSession::after(float seconds, rb::EventRunner::Complete done)
{
    runAction(Sequence::create(DelayTime::create(seconds), CallFunc::create([done = std::move(done)] { done(true); }),
                               nullptr));
}
void GameSession::checkConditions()
{
    for (auto* row : data_->stage("ConditionEventTable", state_.episode, state_.stage))
    {
        const int id = rb::integer(*row, "conID");
        if (!state_.conditionsMatch(*data_, id) || (!rb::integer(*row, "isContinue") && state_.firedConditions[id]))
            continue;
        const int event = rb::integer(*row, "eventID");
        if (events_->linked())
            events_->append(event);
        else
            events_->start(event);
        state_.firedConditions[id] = true;
    }
}
void GameSession::shake(int strength, float duration)
{
    auto* background = room_->background();
    if (shaking_)
        background->setPosition(shakeOrigin_);
    shakeOrigin_          = background->getPosition();
    shaking_              = true;
    const float amplitude = strength == 0 ? 5 : strength == 1 ? 10 : strength == 2 ? 15 : 0;
    auto* cycle           = Sequence::create(MoveTo::create(.05f, shakeOrigin_ - Vec2(amplitude, 0)),
                                             MoveTo::create(.05f, shakeOrigin_), nullptr);
    background->runAction(Sequence::create(Repeat::create(cycle, static_cast<unsigned>(duration / .1f)),
                                           CallFunc::create([this] { shaking_ = false; }), nullptr));
}
void GameSession::basicDialog(std::string_view text)
{
    if (basicDialog_)
        basicDialog_->removeFromParent();
    basicDialog_ = Node::create();
    addChild(basicDialog_, 4);
    std::string clean;
    for (std::size_t i = 0; i < text.size(); ++i)
    {
        if (text[i] == '#' && i + 1 < text.size())
        {
            ++i;
            continue;
        }
        clean += text[i];
    }
    for (int shadow = 1; shadow >= 0; --shadow)
    {
        auto* label = rb::label(clean, language_, 14);
        basicDialog_->addChild(label, 1 - shadow);
        const Vec2 position(240 + shadow, 16 - shadow);
        label->setPosition(position);
        label->setColor(shadow ? Color3B::BLACK : Color3B::WHITE);
        label->setOpacity(0);
        label->runAction(Sequence::create(
            Spawn::create(EaseIn::create(MoveTo::create(1, position + Vec2(0, 10)), 3), FadeIn::create(1), nullptr),
            DelayTime::create(1.5f),
            Spawn::create(EaseOut::create(MoveTo::create(1, position + Vec2(0, 20)), 3), FadeOut::create(1), nullptr),
            nullptr));
    }
}
void GameSession::removeTouchedObject()
{
    auto& object       = state_.objects.at(touchObject_);
    object.disappeared = true;
    room_->refreshShining();
    if (auto* sprite = room_->stateImage(object.state))
        sprite->runAction(FadeOut::create(.3f));
}
void GameSession::collectGoldKey(rb::EventRunner::Complete done)
{
    if (!progress_)
        throw std::runtime_error("Gold key requires bound progress");
    const auto rows = data_->stage("StageTable", state_.episode, state_.stage);
    if (rows.size() != 1)
        throw std::runtime_error("Invalid gold key stage");
    auto& saved = progress_->stages.at(rb::integer(*rows.front(), "stageID"));
    if (!saved.goldKey)
    {
        saved.goldKey = true;
        if (progressChanged)
            progressChanged();
    }
    if (sound)
        sound("snd_f10", false);
    if (touchObject_)
        removeTouchedObject();
    auto* overlay = Node::create();
    addChild(overlay, 6);
    auto* shade = LayerColor::create(Color4B(0, 0, 0, 0), 480, 16);
    overlay->addChild(shade);
    shade->setPosition(0, 153);
    auto* label = rb::label(rb::otherText(language_, "GOLDKEY"), language_, 14);
    overlay->addChild(label);
    label->setPosition(270, 160);
    label->setOpacity(0);
    label->setColor(Color3B(255, 255, 0));
    shade->runAction(Sequence::create(EaseIn::create(FadeTo::create(1, 150), .25f), DelayTime::create(1),
                                      EaseIn::create(FadeOut::create(1), .25f), nullptr));
    label->runAction(Sequence::create(
        Spawn::create(FadeIn::create(1), EaseIn::create(MoveTo::create(1, {240, 160}), .25f), nullptr),
        DelayTime::create(1),
        Spawn::create(FadeOut::create(1), EaseIn::create(MoveTo::create(1, {210, 160}), .25f), nullptr),
        CallFunc::create([overlay, done] {
        overlay->removeFromParent();
        done(true);
    }),
        nullptr));
}
template <class Overlay>
void GameSession::presentPuzzle(Overlay*& slot, bool& shopAfterClose, rb::EventRunner::Complete done)
{
    addChild(slot, 8);
    slot->sound          = effectSound();
    slot->gameUI         = [this](bool visible) { showUI(visible); };
    slot->blocked        = [this] { return ability_ || message_ || systemMenu_; };
    slot->handsRequested = [this, &slot] {
        requestAbility(2, 0, [&slot] {
            if (slot)
                slot->solve();
        });
    };
    slot->complete = [this, &slot, &shopAfterClose, done](bool pass) {
        slot            = nullptr;
        const bool shop = shopAfterClose;
        shopAfterClose  = false;
        done(pass);
        if (shop)
            shop_();
    };
    slot->open();
}
void GameSession::openActionMode(const rb::Command& command, rb::EventRunner::Complete done)
{
    if (action_)
        throw std::runtime_error("Concurrent action modes");
    const auto separator = command.argument.find('-');
    const auto kind      = command.argument.substr(0, separator);
    const auto tail      = separator == std::string::npos ? std::string() : command.argument.substr(separator + 1);
    const int level = std::stoi(tail), hands = progress_ ? progress_->hands : 0;
    if (kind == "C")
        action_ = ActionCut::create(level, hands);
    else if (kind == "H")
    {
        auto* hit  = ActionHit::create(level, hands);
        hit->shake = [this] { shake(0, .2f); };
        action_    = hit;
    }
    else
        throw std::runtime_error("Unknown action mode: " + command.source);
    presentPuzzle(action_, actionShop_, std::move(done));
}
void GameSession::openDetail(int id, rb::EventRunner::Complete done)
{
    if (detail_)
        throw std::runtime_error("Concurrent detail views");
    const auto origin =
        touchObject_ ? room_->background()->convertToWorldSpace(room_->objectPosition(touchObject_)) : touchPoint_;
    detail_ = DetailView::create(*data_, id, origin, language_, progress_ ? progress_->hands : 0);
    presentPuzzle(detail_, detailShop_, std::move(done));
}
void GameSession::receiveItem(int id, bool removeObject, rb::EventRunner::Complete done)
{
    if (itemDialog_)
        throw std::runtime_error("Concurrent item dialogs");
    auto* dialog = GetItemDialog::create(*data_, id, language_);
    state_.addItem(id);
    if (inventoryChanged)
        inventoryChanged();
    if (cursorOutcome_ != 3)
        cursorOutcome_ = 0;
    itemDialog_ = dialog;
    addChild(dialog, 6);
    showUI(false);
    dialog->sound    = effectSound();
    dialog->complete = [this, dialog, done] {
        itemDialog_ = nullptr;
        dialog->removeFromParent();
        showUI(true);
        done(true);
    };
    dialog->open();
    if (removeObject && touchObject_)
        removeTouchedObject();
}
void GameSession::playStory(int scene, rb::EventRunner::Complete done)
{
    if (story_)
        throw std::runtime_error("Concurrent story overlays");
    story_ = StoryDialog::create(*data_, state_.episode, scene, language_, speed_);
    addChild(story_, 6);
    auto* story  = story_;
    story->sound = [this](std::string_view file, bool music) {
        if (sound)
            sound(file, music);
    };
    story->shake   = [this] { shake(0, 1); };
    story->vibrate = [this] {
        if (vibrate)
            vibrate();
    };
    story->gameUI   = [this](bool visible) { showUI(visible); };
    story->complete = [this, story, done] {
        // EventSceneLayer::_exitLayer restores defSound before completing the event.
        if (sound)
            sound(defaultMusic(), true);
        story_ = nullptr;
        story->removeFromParent();
        done(true);
    };
    story->open();
}
void GameSession::dispatch(const rb::Command& command, rb::EventRunner::Complete done)
{
    lastType_            = static_cast<int>(command.type);
    const auto separator = command.argument.find('-');
    const auto number    = [&] { return std::stoi(command.argument); };
    const auto tail      = separator == std::string::npos ? std::string() : command.argument.substr(separator + 1);
    auto complete        = [done] { done(true); };
    switch (command.type)
    {
    case rb::Command::Type::Action:
        openActionMode(command, std::move(done));
        break;
    case rb::Command::Type::Detail:
        openDetail(number(), std::move(done));
        break;
    case rb::Command::Type::Clear:
        showClear();
        break;
    case rb::Command::Type::Crystal:
        collectGoldKey(std::move(done));
        break;
    case rb::Command::Type::Vibrate:
        if (vibrate)
            vibrate();
        after(.5f, std::move(done));
        break;
    case rb::Command::Type::GetItem:
    {
        const int id = number();
        receiveItem(id, !tail.empty() && std::stoi(tail) != 0, std::move(done));
        break;
    }
    case rb::Command::Type::Scene:
        playStory(number(), std::move(done));
        break;
    case rb::Command::Type::Dialog:
        basicDialog(data_->text(number(), language_));
        after(.3f, std::move(done));
        break;
    case rb::Command::Type::MoveRoom:
        moveRoom(number(), tail.empty() ? 0 : std::stoi(tail), complete);
        break;
    case rb::Command::Type::MovePosition:
        room_->movePosition(number(), tail.empty() ? 1 : std::stof(tail), complete);
        break;
    case rb::Command::Type::Direction:
        if (room_->direction(number(), complete))
            break;
        if (!overlay)
            throw std::runtime_error("Missing specialized direction controller: " + command.source);
        overlay(command, std::move(done));
        break;
    case rb::Command::Type::Particle:
        cursorOutcome_ = 6;
        room_->particle(number(), touchPoint_, complete);
        break;
    case rb::Command::Type::Effect:
        room_->objectEffect(number(), std::stoi(tail), complete);
        break;
    case rb::Command::Type::ChangeState:
        room_->changeState(number(), std::stoi(tail), complete);
        break;
    case rb::Command::Type::Show:
        room_->setActive(number(), true, complete);
        break;
    case rb::Command::Type::Hide:
        room_->setActive(number(), false, complete);
        break;
    case rb::Command::Type::Sound:
    {
        const auto file = rb::string(data_->byId("SoundData", "soundID", number()), "fileName");
        if (file.rfind("snd_b", 0) != 0 && sound)
            sound(file, false);
        after(.3f, std::move(done));
        break;
    }
    case rb::Command::Type::Shake:
    {
        const float duration = std::stof(tail);
        shake(number(), duration);
        after(duration, std::move(done));
        break;
    }
    default:
        if (!overlay)
            throw std::runtime_error("Missing specialized event controller: " + command.source);
        overlay(command, std::move(done));
        break;
    }
}
