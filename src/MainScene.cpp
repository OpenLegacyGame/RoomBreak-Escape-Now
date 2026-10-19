#include "MainScene.h"
#include "Viewport.h"
#include "TitleShop.h"
#include "LoadingScreen.h"
#include "SaveStorage.h"
#include "Assets.h"
#include "StageSelect.h"
#include "GameSession.h"
#include "EpisodeMovie.h"
#include "ClearOverlay.h"
#include <cstdlib>
#include <stdexcept>
#if AX_TARGET_PLATFORM == AX_PLATFORM_IOS
#    include <AudioToolbox/AudioToolbox.h>
#elif AX_TARGET_PLATFORM == AX_PLATFORM_ANDROID
#    include "platform/android/jni/JniHelper.h"
#endif
using namespace ax;
namespace
{
void originalVibration()
{
#if AX_TARGET_PLATFORM == AX_PLATFORM_IOS
    AudioServicesPlaySystemSound(kSystemSoundID_Vibrate);
#elif AX_TARGET_PLATFORM == AX_PLATFORM_ANDROID
    ax::JniHelper::callStaticVoidMethod("dev/axmol/app/AppActivity", "vibrateDefault");
#endif
}
}  // namespace

#if RB_VISUAL_PROBE
bool runVisualProbe(ax::Scene*, const rb::GameData&);
bool runViewportProbe(ax::Scene*, ax::Node*, const rb::GameData&);
bool runMainFlowProbe(MainScene*);
#endif

bool MainScene::init()
{
    if (!Scene::init())
        return false;
    canvas_ = Node::create();
    canvas_->setContentSize({480, 320});
    addChild(canvas_);
    rb::onViewport(this, [this] {
        setContentSize(Director::getInstance()->getWinSize());
        canvas_->setPosition(rb::canvasOrigin());
    });
    schedule([](float) { rb::updateViewport(); }, "viewport");
#if (AX_TARGET_PLATFORM == AX_PLATFORM_WIN32) || (AX_TARGET_PLATFORM == AX_PLATFORM_MAC)
    auto windowed = std::make_shared<Size>(960, 640);
    auto* keys = EventListenerKeyboard::create();
    keys->onKeyReleased = [windowed](EventKeyboard::KeyCode code, Event*) {
        auto* view = static_cast<RenderViewImpl*>(Director::getInstance()->getRenderView());
        if (code != EventKeyboard::KeyCode::KEY_F11 &&
            !(code == EventKeyboard::KeyCode::KEY_ESCAPE && view->isFullscreen()))
            return;
        if (view->isFullscreen())
            view->setWindowed(static_cast<int>(windowed->width), static_cast<int>(windowed->height));
        else
        {
            int width, height;
            view->getWindowSize(&width, &height);
            *windowed = Size(width, height);
            view->setFullscreen();
        }
        rb::updateViewport();
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(keys, this);
#endif
    data_.load(FileUtils::getInstance()->getStringFromFile("GameData.json"));
#if RB_VISUAL_PROBE
    if (runViewportProbe(this, canvas_, data_))
        return true;
    if (!std::getenv("RB_ROUTE_DEMO") && !std::getenv("RB_PROBE_FLOW") && runVisualProbe(this, data_))
        return true;
#endif
    progress_.initialize(data_);
    savePath_ = FileUtils::getInstance()->getWritablePath() + "roombreak-rebuilt-progress-v1.json";
#if RB_VISUAL_PROBE
    if (const char* path = std::getenv("RB_PROBE_SAVE"))
        savePath_ = path;
#endif
    if (FileUtils::getInstance()->isFileExist(savePath_))
        progress_.load(FileUtils::getInstance()->getStringFromFile(savePath_));
    settings_ = rb::UserSettings::load();
#if RB_VISUAL_PROBE
    if (std::getenv("RB_ROUTE_DEMO"))
    {
        settings_.language = 0;
        settings_.music = settings_.effects = true;
    }
#endif
    beginLoading(false, [this] {
        createTitle();
        audio_.playMusic("EP0");
    });
    auto* touch         = EventListenerTouchOneByOne::create();
    touch->onTouchBegan = [this](Touch* t, Event*) {
        if (loading_ || transitioning_ || (!title_->isVisible() && !credits_))
            return false;
        pressed_         = hit(t->getLocation());
        draggingCredits_ = credits_ && pressed_ != 100;
        if (hasPressedImage(pressed_))
            menu_.at(pressed_ + 1)->setVisible(true);
        return true;
    };
    touch->onTouchMoved = [this](Touch* t, Event*) {
        if (draggingCredits_)
        {
            for (auto* page : creditPages_)
                page->setPositionY(page->getPositionY() + t->getDelta().y);
            if (creditPages_.front()->getPositionY() <= creditOrigins_.front().y)
                for (size_t i = 0; i < creditPages_.size(); ++i)
                    creditPages_[i]->setPosition(creditOrigins_[i]);
        }
    };
    touch->onTouchEnded = [this](Touch* t, Event*) {
        const int id     = pressed_;
        pressed_         = -1;
        draggingCredits_ = false;
        if (hasPressedImage(id))
            menu_.at(id + 1)->setVisible(false);
        if (id >= 0 && hit(t->getLocation()) == id)
            click(id);
    };
    touch->onTouchCancelled = [this](Touch*, Event*) {
        pressed_         = -1;
        draggingCredits_ = false;
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(touch, this);
    schedule([this](float) {
        if (!credits_ || draggingCredits_)
            return;
        for (auto* page : creditPages_)
            page->setPositionY(page->getPositionY() + .25f);
        if (creditPages_.back()->getPositionY() >= 320 + creditPages_.back()->getContentSize().height / 2)
            for (size_t i = 0; i < creditPages_.size(); ++i)
                creditPages_[i]->setPosition(creditOrigins_[i]);
    }, "credits-scroll");
#if RB_VISUAL_PROBE
    if (std::getenv("RB_PROBE_FLOW"))
        runMainFlowProbe(this);
    if (std::getenv("RB_ROUTE_DEMO"))
        startRouteDemo();
#endif
    return true;
}
bool MainScene::hasPressedImage(int id) const
{
    // Buttons whose pressed sprite is registered under id + 1.
    const bool pressable = id == 5 || id == 29 || id == 31 || id == 33 || id == 35 || id == 100;
    return pressable && menu_.contains(id + 1);
}
Sprite* MainScene::addMenu(int id, const char* file, float x, float y, int parent, bool visible)
{
    auto* value = rb::place(parent < 0 ? background_ : menu_.at(parent), file, x, y);
    value->setVisible(visible);
    menu_[id] = value;
    return value;
}
void MainScene::createTitle()
{
    title_ = ClippingRectangleNode::create({0, 0, 480, 320});
    title_->setContentSize({480, 320});
    canvas_->addChild(title_);
    const Vec2 gears[] = {{357, -26}, {241, 32}, {345, 93}};
    for (int i = 0; i < 3; ++i)
    {
        auto* gear = rb::place(title_, "title_bg_gear_" + std::to_string(i + 1) + ".png", gears[i].x, gears[i].y);
        rb::rotateGear(gear, i == 0 ? 10 : i == 1 ? 8 : 15, i == 0);
    }
    rb::place(title_, "title_bg_1.png", 268, 22);
    background_ = rb::sprite("title_bg.png");
    title_->addChild(background_);
    background_->setPosition(240, 160);
    struct Entry
    {
        int id;
        const char* name;
        float x, y;
        int parent;
        bool visible;
    };
    const Entry entries[] = {
        {29, "title_shop.png", 159, 178, -1, true},       {30, "title_shop_p.png", 159, 178, -1, false},
        {33, "title_rank.png", 208, 178, -1, true},       {34, "title_rank_p.png", 208, 178, -1, false},
        {35, "title_trophy.png", 257, 178, -1, true},     {36, "title_trophy_p.png", 257, 178, -1, false},
        {4, "title_update_ep6.png", 52, 173, -1, true},   {31, "title_info.png", 306, 178, -1, true},
        {32, "title_info_p.png", 306, 178, -1, false},    {7, "title_play_gear.png", 357, 205, -1, true},
        {5, "title_play.png", 388, 233, -1, true},        {6, "title_play_p.png", 388, 233, -1, false},
        {0, "title_symbol.png", 110, 44, -1, true},       {1, "title_symbol_kor.png", 121, 44, -1, true},
        {2, "title_company.png", 40, 310, -1, true},      {3, "title_company_tap.png", 40, 310, -1, true},
        {42, "title_facebook.png", 195, 310, -1, true},   {10, "title_op_bar.png", 120, 235, -1, true},
        {11, "title_op_lan_p.png", 81, 243, -1, false},   {22, "title_op_lan_ko.png", 19, 15, 10, true},
        {23, "title_op_lan_en.png", 19, 15, 10, true},    {24, "title_op_lan_ja.png", 19, 15, 10, true},
        {12, "title_op_eft_p.png", 175, 243, -1, false},  {13, "title_op_off.png", 105, 8, 10, true},
        {14, "title_op_bgm_p.png", 128, 243, -1, false},  {15, "title_op_off.png", 58, 8, 10, true},
        {16, "title_op_vib_p.png", 218, 243, -1, false},  {17, "title_op_off.png", 148, 8, 10, true},
        {18, "title_op_text_p.png", 263, 245, -1, false}, {19, "title_op_text_1.png", 194, 16, 10, true},
        {20, "title_op_text_2.png", 194, 16, 10, true},   {21, "title_op_text_3.png", 194, 16, 10, true},
        {26, "title_op_gear.png", 290, 231, -1, true}};
    for (auto e : entries)
        addMenu(e.id, e.name, e.x, e.y, e.parent, e.visible);
    chineseIndicator_ = rb::label("中", 0, 14);
    menu_.at(10)->addChild(chineseIndicator_);
    chineseIndicator_->setPosition(menu_.at(23)->getPosition());
    menu_.at(10)->setOpacity(0);
    rb::rotateGear(menu_.at(7), 10, true);
    menu_.at(3)->setOpacity(0);
    menu_.at(3)->runAction(RepeatForever::create(Sequence::create(
        EaseOut::create(FadeIn::create(.3f), .25f), EaseOut::create(FadeOut::create(.3f), .25f), nullptr)));
    refreshSettings();
}
void MainScene::refreshSettings()
{
    menu_.at(0)->setOpacity(settings_.language == 1 ? 0 : 255);
    menu_.at(1)->setOpacity(settings_.language == 1 ? 255 : 0);
    for (int id : {13, 15, 17, 19, 20, 21, 22, 23, 24})
    {
        bool on = (id == 13 && !settings_.effects) || (id == 15 && !settings_.music) ||
                  (id == 17 && !settings_.vibration) || id == 19 + settings_.textSpeed ||
                  (id == 22 && settings_.language == 1) || (id == 23 && settings_.language == 2) ||
                  (id == 24 && settings_.language == 3);
        menu_.at(id)->stopAllActions();
        menu_.at(id)->runAction(EaseIn::create(FadeTo::create(.5f, options_ && on ? 255 : 0), .25f));
    }
    chineseIndicator_->stopAllActions();
    chineseIndicator_->runAction(
        EaseIn::create(FadeTo::create(.5f, options_ && settings_.language == 0 ? 255 : 0), .25f));
}
void MainScene::setOptions(bool open)
{
    options_  = open;
    auto* bar = menu_.at(10);
    bar->stopAllActions();
    auto size = bar->getContentSize();
    auto target =
        Vec2((open ? 68 : 120) + size.width / 2, background_->getContentSize().height - 235 - size.height / 2);
    bar->runAction(Spawn::create(EaseIn::create(MoveTo::create(.5f, target), .25f),
                                 EaseIn::create(FadeTo::create(.5f, open ? 255 : 0), .25f), nullptr));
    menu_.at(26)->stopAllActions();
    menu_.at(26)->runAction(EaseIn::create(RotateTo::create(.5f, open ? 180 : 0), .5f));
    refreshSettings();
}
int MainScene::hit(Vec2 point) const
{
    const auto inside = [&](int id) { return rb::hitTest(menu_.at(id), point); };
    if (credits_)
        return inside(100) ? 100 : -1;
    if (options_)
        for (int id : {11, 12, 14, 16, 18})
            if (inside(id))
                return id;
    for (int id : {5, 29, 31, 33, 35, 26})
        if (inside(id))
            return id;
    return -1;
}
void MainScene::click(int id)
{
    if (loading_)
        return;
    if (id != 12 && id != 100)
        audio_.play(id == 5 ? "snd_f27" : id == 26 ? "snd_f21" : "snd_f19", false);
    if (id == 5)
    {
        showStageSelect();
        return;
    }
    if (id == 29)
    {
        if (shop_)
            return;
        shop_ = TitleShop::create(progress_, settings_.language);
        canvas_->addChild(shop_, 20);
        shop_->unlockAll = [this] {
            progress_.unlockAllStages(data_);
            saveProgress();
        };
        shop_->purchase = [this](int product) {
            progress_.claimShopProduct(product);
            saveProgress();
        };
        shop_->closed = [this] { shop_ = nullptr; };
        shop_->sound  = [this](std::string_view file) { audio_.play(file, false); };
        shop_->open();
        return;
    }
    if (id == 31)
    {
        showCredits();
        return;
    }
    if (id == 100)
    {
        credits_->removeFromParent();
        credits_ = nullptr;
        creditPages_.clear();
        creditOrigins_.clear();
        menu_.erase(100);
        menu_.erase(101);
        title_->setVisible(true);
        audio_.playMusic("EP0");
        return;
    }
    if (id == 26)
    {
        setOptions(!options_);
        return;
    }
    if (id == 11)
        settings_.language = (settings_.language + 1) % 4;
    if (id == 12)
    {
        settings_.effects = !settings_.effects;
        audio_.play("snd_f19", false);
    }
    if (id == 14)
    {
        settings_.music = !settings_.music;
        audio_.setMenuAudio(!settings_.music, "EP0");
    }
    if (id == 16)
    {
        settings_.vibration = !settings_.vibration;
        if (settings_.vibration)
            originalVibration();
    }
    if (id == 18)
        settings_.textSpeed = (settings_.textSpeed + 1) % 3;
    settings_.save();
    refreshSettings();
}
void MainScene::saveProgress()
{
    auto* files                 = FileUtils::getInstance();
    const std::string temporary = savePath_ + ".tmp";
    if (!files->writeStringToFile(progress_.encode(), temporary) || !rb::replaceSaveFile(temporary, savePath_))
        throw std::runtime_error("Could not save game progress");
}
void MainScene::beginLoading(bool tips, std::function<void()> complete)
{
    loading_     = true;
    auto* screen = LoadingScreen::create(settings_.language, tips, [this, complete = std::move(complete)] {
        complete();
        loading_ = false;
    });
    canvas_->addChild(screen, 100);
}
void MainScene::transition(std::function<void()> next)
{
    if (transitioning_)
        return;
    transitioning_ = true;
    scheduleOnce([this, next = std::move(next)](float) {
        next();
        transitioning_ = false;
    }, 0, "screen-transition");
}
void MainScene::clearContent()
{
    if (session_)
    {
        session_->removeFromParent();
        session_ = nullptr;
    }
    if (selection_)
    {
        selection_->removeFromParent();
        selection_ = nullptr;
    }
    if (movie_)
    {
        movie_->removeFromParent();
        movie_ = nullptr;
    }
}
void MainScene::showStageSelect(int displayedEpisode)
{
    clearContent();
    title_->setVisible(false);
    audio_.playMusic("");
    beginLoading(true, [this, displayedEpisode] {
        auto* select = StageSelect::create(data_, progress_, settings_.language, displayedEpisode);
        selection_   = select;
        canvas_->addChild(select, 10);
        select->sound = [this](std::string_view file) { audio_.play(file, false); };
        select->back  = [this] {
            transition([this] {
                clearContent();
                beginLoading(false, [this] {
                    title_->setVisible(true);
                    audio_.playMusic("EP0");
                });
            });
        };
        select->play = [this](int episode, int stage) {
            transition([this, episode, stage] { selectStage(episode, stage); });
        };
        select->movie = [this](int episode, bool ending) {
            transition([this, episode, ending] {
                showMovie(episode, ending, [this, episode] { showStageSelect(rb::displayedEpisode(episode)); });
            });
        };
    });
}
void MainScene::selectStage(int episode, int stage)
{
    if (!progress_.stageOpened(data_, episode, stage))
        throw std::runtime_error("Selected stage is locked");
    if (episode && stage == 1 && !progress_.seenIntros.contains(episode))
    {
        progress_.seenIntros.insert(episode);
        saveProgress();
        showMovie(episode, false, [this, episode, stage] { startStage(episode, stage); });
    }
    else
        startStage(episode, stage);
}
void MainScene::startStage(int episode, int stage)
{
    clearContent();
    title_->setVisible(false);
    audio_.playMusic("");
    beginLoading(true, [this, episode, stage] {
        const int stageID = rb::integer(*data_.stage("StageTable", episode, stage).front(), "stageID");
        auto* game        = GameSession::create(data_, episode, stage, settings_.language, settings_.textSpeed,
                                                progress_.stages.at(stageID).goldKey);
        session_          = game;
        canvas_->addChild(game, 10);
        game->bindProgress(progress_);
        game->unlockAllRequested = [this] {
            progress_.unlockAllStages(data_);
            saveProgress();
        };
        game->purchaseRequested = [this](int product) {
            progress_.claimShopProduct(product);
            saveProgress();
        };
        game->progressChanged = [this] { saveProgress(); };
        game->sound           = [this](std::string_view file, bool music) { audio_.play(file, music); };
        game->menuSettings    = {settings_.effects, settings_.music, settings_.vibration, settings_.textSpeed};
        game->vibrate         = [this] {
            if (settings_.vibration)
                originalVibration();
        };
        game->settingsChanged = [this](SystemMenu::Settings s) {
            if (!settings_.vibration && s.vibration)
                originalVibration();
            settings_.effects   = s.effects;
            settings_.music     = s.music;
            settings_.vibration = s.vibration;
            settings_.textSpeed = s.textSpeed;
            settings_.save();
            refreshSettings();
        };
        game->menuAudio      = [this, game](bool stop) { audio_.setMenuAudio(stop, game->defaultMusic()); };
        game->retryRequested = [this, episode, stage] {
            transition([this, episode, stage] { startStage(episode, stage); });
        };
        game->exitRequested = [this, episode] {
            transition([this, episode] {
                audio_.setMenuAudio(true);
                showStageSelect(rb::displayedEpisode(episode));
            });
        };
        game->clearAccepted = [this, episode, stage] {
            transition([this, episode, stage] { acceptClear(episode, stage); });
        };
        game->startWithIntro();
    });
}
void MainScene::showMovie(int episode, bool ending, std::function<void()> next)
{
    clearContent();
    title_->setVisible(false);
    auto* movie = EpisodeMovie::create(episode, ending, settings_.language);
    movie_      = movie;
    canvas_->addChild(movie, 10);
    movie->music    = [this](std::string_view file) { audio_.playMusic(file); };
    movie->complete = [this, next = std::move(next)] {
        movie_ = nullptr;
        transition(next);
    };
    movie->open();
}
void MainScene::acceptClear(int episode, int stage)
{
    if (episode && stage == 5 && !progress_.seenEndings.contains(episode))
    {
        progress_.seenEndings.insert(episode);
        saveProgress();
        showMovie(episode, true, [this, episode] { showStageSelect(rb::displayedEpisode(episode)); });
    }
    else
        showStageSelect(rb::displayedEpisode(episode));
}
void MainScene::showCredits()
{
    title_->setVisible(false);
    credits_ = ClippingRectangleNode::create({0, 0, 480, 320});
    credits_->setContentSize({480, 320});
    canvas_->addChild(credits_, 20);
    auto* bg = rb::sprite("title_info_bg.png");
    credits_->addChild(bg);
    bg->setPosition(240, 160);
    const std::vector<std::string> files =
        settings_.language == 1
            ? std::vector<std::string>{"title_info_kor_1.png", "title_info_kor_2.png"}
            : std::vector<std::string>{settings_.language == 3 ? "title_info_jap.png" : "title_info_eng.png"};
    float y = 0;
    for (const auto& file : files)
    {
        auto* page = rb::sprite(file);
        credits_->addChild(page);
        Vec2 initial(creditPages_.empty() ? 240 : 250,
                     y - page->getContentSize().height / 2 + (creditPages_.empty() ? 0 : 1));
        creditOrigins_.push_back(initial);
        page->setPosition(initial + Vec2(0, 50));
        creditPages_.push_back(page);
        y -= page->getContentSize().height;
    }
    auto* back = rb::sprite("main_back.png");
    credits_->addChild(back, 1);
    back->setPosition(470 - back->getContentSize().width / 2, 310 - back->getContentSize().height / 2);
    menu_[100]    = back;
    auto* pressed = rb::sprite("main_back_p.png");
    credits_->addChild(pressed, 2);
    pressed->setPosition(back->getPosition());
    pressed->setVisible(false);
    menu_[101]    = pressed;
    auto* version = rb::label("ver 1.0.7", 2, 11);
    credits_->addChild(version, 2);
    version->setAnchorPoint({0, 1});
    version->setPosition(10, 295);
    scheduleOnce([this](float) {
        if (credits_)
        {
            audio_.playMusic("EP1_1");
        }
    }, .3f, "credits-music");
}

#if RB_VISUAL_PROBE
void MainScene::prepareDemoRoute()
{
    const int episodes[] = {3, 1, 2, 4, 5, 6};
    const int episode = episodes[demoIndex_ / 5], stage = demoIndex_ % 5 + 1;
    for (int e = 1; e <= 6; ++e)
        for (int n = 1; n <= 5; ++n)
            unsetenv(("RB_PROBE_EPISODE" + std::to_string(e) + "_STAGE" + std::to_string(n)).c_str());
    setenv(("RB_PROBE_EPISODE" + std::to_string(episode) + "_STAGE" + std::to_string(stage)).c_str(), "1", 1);
    const std::string folder =
        demoRoot_ + "/episode" + std::to_string(demoIndex_ / 5 + 1) + "-stage" + std::to_string(stage);
    FileUtils::getInstance()->createDirectories(folder);
    setenv("RB_VISUAL_PROBE_OUTPUT", folder.c_str(), 1);
    setenv("RB_PROBE_LANGUAGE", "0", 1);
    AXLOGI("RB_MENU_DEMO_START episode={} stage={}", demoIndex_ / 5 + 1, stage);
}
void MainScene::startRouteDemo()
{
    if (demoRoot_.empty())
        demoRoot_ = std::getenv("RB_VISUAL_PROBE_OUTPUT");
    prepareDemoRoute();
    schedule([this](float) {
        if (loading_ || transitioning_ || movie_)
            return;
        if (title_->isVisible())
        {
            click(5);
            return;
        }
        if (selection_)
        {
            if (selection_->moving_)
                return;
            const int wanted = demoIndex_ / 5 + 1;
            if (selection_->opened_)
            {
                if (selection_->episode_ != wanted)
                {
                    selection_->activate(100);
                    return;
                }
                selection_->activate(demoIndex_ % 5 + 1);
                return;
            }
            if (selection_->episode_ != wanted)
            {
                selection_->activate(selection_->episode_ < wanted ? 12 : 11);
                return;
            }
            selection_->activate(10);
            return;
        }
        if (session_ && !session_->stageIntro())
        {
            unschedule("demo-menu");
            if (!runVisualProbe(this, data_))
                throw std::runtime_error("Demo route missing");
        }
    }, 1.5f, "demo-menu");
}
void MainScene::finishRouteDemo()
{
    unscheduleAllCallbacks();
    schedule([this](float) {
        if (session_ && session_->clearOverlay() && session_->clearOverlay()->ready())
            session_->clearOverlay()->choose(true);
        if (selection_ && !loading_ && !transitioning_)
        {
            unschedule("demo-completion");
            scheduleOnce([this](float) {
                utils::captureScreen([this](bool ok, std::string_view) {
                    if (!ok)
                        throw std::runtime_error("Demo final screen capture failed");
                    AXLOGI("RB_MENU_DEMO_PASS episode={} stage={}", demoIndex_ / 5 + 1, demoIndex_ % 5 + 1);
                    if (++demoIndex_ == 30)
                    {
                        AXLOGI("RB_ROUTE_DEMO_COMPLETE 30/30");
                        Director::getInstance()->end();
                    }
                    else
                        startRouteDemo();
                }, std::string(std::getenv("RB_VISUAL_PROBE_OUTPUT")) + "/post-clear.png");
            }, 2, "demo-final-screen");
        }
    }, .1f, "demo-completion");
}
#endif
