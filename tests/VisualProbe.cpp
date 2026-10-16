#include "DetailElevator.h"
#include "MainScene.h"
#include "LoadingScreen.h"
#include "StageSelect.h"
#include <chrono>
#include "TitleShop.h"
#include "DetailWomanStatue.h"
#include "DetailCenterSafe.h"
#include "DetailDropBook.h"
#include "DetailFurnitureLeft.h"
#include "DetailCircuit.h"
#include "DetailWatchBox.h"
#include "DetailSecretPattern.h"
#include "DetailPyramidBlock.h"
#include "DetailHistoricSafe.h"
#include "DetailTimeControl.h"
#include "DetailHistoricBook.h"
#include "DetailAlienPad.h"
#include "DetailWineBox.h"
#include "DetailDustingFrame.h"
#include "DetailFingerScan.h"
#include "DetailGearHandle.h"
#include "DetailSwitch.h"
#include "DetailMonitorControl.h"
#include "DetailElectronicDoorLock.h"
#include "DetailArrowSafe.h"
#include "DetailPictureFrame.h"
#include "axmol.h"
#include "GameData.h"
#include "RoomView.h"
#include "GameSession.h"
#include "CursorPocket.h"
#include "CursorFeedback.h"
#include "GetItemDialog.h"
#include "InventoryView.h"
#include "InsightOverlay.h"
#include "Progress.h"
#include "MessageBox.h"
#include "AbilityInfo.h"
#include "HeroHealth.h"
#include "PauseButton.h"
#include "SystemMenu.h"
#include "GameOverOverlay.h"
#include "ClearOverlay.h"
#include "EpisodeMovie.h"
#include "DetailView.h"
#include "DetailPad.h"
#include "DetailMusic.h"
#include "DetailElectronicDisplay.h"
#include "DetailScreenSafe.h"
#include "DetailSafe.h"
#include "DetailDoorLock.h"
#include "DetailTutorial.h"
#include "DetailFittedPieces.h"
#include "ActionCut.h"
#include "ActionHit.h"
#include "ResultFeedback.h"
#include "StoryDialog.h"
#include "SheetSprite.h"
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <memory>
#include <stdexcept>
using namespace ax;
namespace
{
MainScene* routeDemoOwner = nullptr;
void advanceRouteStory(GameSession* session)
{
    if (!routeDemoOwner)
    {
        session->story()->advance();
        return;
    }
    static auto previous = std::chrono::steady_clock::now();
    auto now             = std::chrono::steady_clock::now();
    if (std::chrono::duration<float>(now - previous).count() >= 3)
    {
        previous = now;
        session->story()->advance();
    }
}
void finishRoute()
{
    if (routeDemoOwner)
        routeDemoOwner->finishRouteDemo();
    else
        Director::getInstance()->end();
}
}  // namespace
bool runVisualProbe(Scene* scene, const rb::GameData& data)
{
    const char* directory = std::getenv("RB_VISUAL_PROBE_OUTPUT");
    if (!directory)
        return false;
    const int language = std::getenv("RB_PROBE_LANGUAGE") ? std::atoi(std::getenv("RB_PROBE_LANGUAGE")) : 0;
    if (std::getenv("RB_PROBE_SPHINX_CLUE"))
    {
        auto* session = GameSession::create(data, 4, 2, language, 1, false);
        scene->addChild(session);
        auto* room = session->room();
        room->direction(26, [scene, room, directory] {
            room->direction(24, [scene, room, directory] {
                SheetSprite* clue = nullptr;
                for (auto* child : room->background()->getChildren())
                    if (auto* sheet = dynamic_cast<SheetSprite*>(child))
                        if (sheet->getTexture()->getPath().find("e4_r2_dir5") != std::string::npos)
                            clue = sheet;
                if (!clue)
                    throw std::runtime_error("Sphinx numeric clue missing");
                utils::captureScreen([scene, room, clue, directory](bool ok, std::string_view) {
                    if (!ok)
                        throw std::runtime_error("Sphinx capture failed");
                    const auto width = clue->getTexture()->getContentSizeInPixels().width / 6;
                    if (std::abs(clue->getTextureRect().origin.x - width * 5) > .01f ||
                        std::abs(clue->getTextureRect().size.width - width) > .01f)
                        throw std::runtime_error("Sphinx clue did not retain complete 1324 frame");
                    const int home = 28;
                    // RoomData identifies this single-room sphinx level.
                    room->loadRoom(home);
                    scene->scheduleOnce([room, clue, width](float) {
                        if (std::abs(clue->getTextureRect().origin.x - width * 5) > .01f)
                            throw std::runtime_error("Sphinx clue lost after room reload");
                        room->direction(25, [room] {
                            if (room->directionAnimationCount() != 0)
                                throw std::runtime_error("Sphinx light did not turn off");
                            AXLOGI("RB_PROBE_SPHINX_CLUE PASS");
                            Director::getInstance()->end();
                        });
                    }, .3f, "sphinx-reload");
                }, std::string(directory) + "/sphinx.png");
            });
        });
        return true;
    }
    if (std::getenv("RB_PROBE_TOUCH_JITTER"))
    {
        auto index = std::make_shared<int>(0);
        scene->schedule([scene, index, &data, language](float) {
            if (*index == 9)
            {
                scene->unschedule("touch-jitter");
                AXLOGI("RB_PROBE_TOUCH_JITTER PASS: 5 taps, 3 drags, 1 cancellation");
                Director::getInstance()->end();
                return;
            }
            auto* session = GameSession::create(data, 1, 1, language, 1, false);
            scene->addChild(session);
            auto* room = session->room();
            auto p     = room->background()->convertToWorldSpace(room->objectPosition(866));
            if (room->hitObject(p) != 866)
                throw std::runtime_error("Touch probe target mismatch");
            Touch touch;
            auto send = [&](EventTouch::EventCode code, Vec2 point) {
                auto view = Director::getInstance()->convertToUI(point);
                touch.setTouchInfo(0, view.x, view.y);
                EventTouch event;
                event.setEventCode(code);
                event.setTouches({&touch});
                Director::getInstance()->getEventDispatcher()->dispatchEvent(&event);
            };
            const Vec2 deltas[] = {{0, 0}, {1, .5f}, {-2, 1}, {0, 3}, {4, 2}, {-30, 0}, {0, 30}, {-30, 0}, {1, 1}};
            const auto delta    = deltas[*index];
            const auto before   = room->background()->getPosition();
            send(EventTouch::EventCode::BEGAN, p);
            send(EventTouch::EventCode::MOVED, p + delta);
            if (*index == 7)
                send(EventTouch::EventCode::MOVED, p);
            const auto moved = room->background()->getPosition();
            send(*index == 8 ? EventTouch::EventCode::CANCELLED : EventTouch::EventCode::ENDED,
                 *index == 7 ? p : p + delta);
            const bool tapped = session->touchObject() == 866, expected = *index < 5;
            if (tapped != expected)
                throw std::runtime_error("Room tap/drag classification failed: " + std::to_string(*index));
            if (expected && moved != before)
                throw std::runtime_error("Finger jitter moved the room");
            if (*index == 5 && moved.x >= before.x - 10)
                throw std::runtime_error("Horizontal drag did not pan the room");
            session->removeFromParent();
            ++*index;
        }, .5f, "touch-jitter");
        return true;
    }
    if (const char* kind = std::getenv("RB_PROBE_MEDIA"))
    {
        const bool hammer = std::string_view(kind) == "hammer";
        if (!hammer && std::string_view(kind) != "knife")
            throw std::runtime_error("Unknown media scene");
        auto* session = GameSession::create(data, 0, 1, language, 1, false);
        scene->addChild(session);
        auto* action = hammer ? static_cast<ActionMode*>(ActionHit::create(6, 0))
                              : static_cast<ActionMode*>(ActionCut::create(6, 0));
        scene->addChild(action, 20);
        auto frames = std::make_shared<int>(0), pending = std::make_shared<int>(0);
        auto audio    = std::make_shared<std::ofstream>(std::string(directory) + "/audio.tsv");
        action->sound = [frames, audio](std::string_view name) {
            *audio << *frames << '\t' << name << '\n';
            audio->flush();
        };
        scene->schedule([directory, frames, pending](float) {
            char name[64];
            std::snprintf(name, sizeof(name), "/frame-%06d.png", (*frames)++);
            ++*pending;
            utils::captureScreen([pending](bool ok, std::string_view) {
                if (!ok)
                    throw std::runtime_error("Media frame capture failed");
                --*pending;
            }, std::string(directory) + name);
        }, .05f, "media-frames");
        action->complete = [scene, frames, pending](bool pass) {
            if (!pass)
                throw std::runtime_error("Media play failed");
            scene->unschedule("media-input");
            scene->scheduleOnce([scene, frames, pending](float) {
                scene->unschedule("media-frames");
                scene->schedule([pending, frames](float) {
                    if (*pending)
                        return;
                    AXLOGI("RB_PROBE_MEDIA PASS: {} frames", *frames);
                    Director::getInstance()->end();
                }, .1f, "media-finish");
            }, .5f, "media-stop");
        };
        action->open();
        auto age = std::make_shared<float>(0);
        scene->schedule([action, age](float dt) {
            *age += dt;
            if (action->busy())
                return;
            if (action->menuVisible())
                throw std::runtime_error("Media play missed a gesture");
            if (auto* hit = dynamic_cast<ActionHit*>(action))
            {
                if (int n = hit->playableTarget())
                {
                    auto p = hit->targetCenter(n);
                    hit->beginTouch(p);
                    hit->endTouch(p);
                }
                return;
            }
            if (*age < .65f)
                return;
            *age                    = 0;
            auto* cut               = static_cast<ActionCut*>(action);
            const Vec2 directions[] = {{-1, 0}, {-1, 1}, {0, 1}, {1, 1}, {1, 0}, {1, -1}, {0, -1}, {-1, -1}};
            const auto delta        = directions[cut->direction()].getNormalized() * 160;
            const auto p            = Vec2(240, 160) - delta / 2;
            cut->beginTouch(p);
            cut->moveTouch(p + delta);
            cut->endTouch(p + delta);
        }, .01f, "media-input");
        scene->scheduleOnce([](float) { throw std::runtime_error("Media timeout"); }, 60, "media-timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_COLLECTED_GOLD_REENTRY"))
    {
        rb::Progress saved;
        saved.initialize(data);
        for (auto& [id, stage] : saved.stages)
            stage.goldKey = true;
        rb::Progress restored;
        restored.initialize(data);
        restored.load(saved.encode());
        int stages = 0, rooms = 0, keys = 0;
        for (const auto& row : data.table("StageTable").GetArray())
        {
            const int episode = rb::integer(row, "episodeNumber"), stage = rb::integer(row, "stageNumber");
            rb::StageState state;
            state.reset(data, episode, stage, restored.stages.at(rb::integer(row, "stageID")).goldKey);
            AXLOGI("Gold reentry: episode {} stage {}", episode, stage);
            auto* view = RoomView::create(data, state);
            scene->addChild(view);
            for (auto* room : data.stage("RoomData", episode, stage))
            {
                const int roomID = rb::integer(*room, "roomID");
                view->loadRoom(roomID);
                view->refreshShining();
                for (auto* object : data.where("ObjectData", "roomID", roomID))
                {
                    const int id = rb::integer(*object, "objectID");
                    if (rb::integer(*object, "objectType") == 2)
                    {
                        if (state.objects.contains(id))
                            throw std::runtime_error("Collected gold key respawned");
                        ++keys;
                    }
                }
                ++rooms;
            }
            view->removeFromParent();
            ++stages;
        }
        if (stages != 31 || rooms != 52 || keys == 0)
            throw std::runtime_error("Incomplete gold reentry coverage");
        AXLOGI("RB_PROBE_COLLECTED_GOLD_REENTRY PASS: {} stages, {} rooms, {} collected keys", stages, rooms, keys);
        scene->scheduleOnce([](float) { Director::getInstance()->end(); }, .1f, "gold-reentry-finish");
        return true;
    }
    if (std::getenv("RB_PROBE_OFFLINE_SHOP"))
    {
        auto progress = std::make_shared<rb::Progress>();
        progress->initialize(data);
        const int eyes = progress->eyes, insight = progress->insight, hands = progress->hands, feet = progress->feet;
        auto* title = TitleShop::create(*progress, language);
        scene->addChild(title);
        title->open();
        auto* menu = SystemMenu::create(*progress, language, {});
        scene->addChild(menu);
        menu->setVisible(false);
        auto granted = std::make_shared<int>(0), unlocked = std::make_shared<int>(0), phase = std::make_shared<int>(0);
        title->purchase = menu->purchase = [progress, granted](int product) {
            progress->claimShopProduct(product);
            ++*granted;
        };
        title->unlockAll = menu->unlockAll = [progress, unlocked, &data] {
            progress->unlockAllStages(data);
            ++*unlocked;
        };
        scene->schedule([=, &data](float) {
            auto clickTitle = [title](int i) {
                auto p = title->controlCenter(i);
                title->beginTouch(p);
                title->endTouch(p);
            };
            switch ((*phase)++)
            {
            case 0:
                for (int i = 0; i < 6; ++i)
                {
                    clickTitle(i);
                    clickTitle(i);
                    clickTitle(i);
                }
                clickTitle(7);
                break;
            case 1:
                if (!title->messageBox() || !title->messageBox()->ready())
                    throw std::runtime_error("Title confirmation missing");
                title->messageBox()->answer(false);
                break;
            case 2:
                if (*unlocked || progress->episodeOpened(6))
                    throw std::runtime_error("Cancel unlocked stages");
                clickTitle(7);
                break;
            case 3:
                title->messageBox()->answer(true);
                break;
            case 4:
                if (*unlocked != 1)
                    throw std::runtime_error("Title unlock failed");
                title->setVisible(false);
                menu->setVisible(true);
                menu->open(3);
                break;
            case 5:
                for (int i = 0; i < 6; ++i)
                {
                    menu->click(i);
                    menu->click(i);
                    menu->click(i);
                }
                menu->click(6);
                break;
            case 6:
                if (!menu->messageBox() || !menu->messageBox()->ready())
                    throw std::runtime_error("Menu confirmation missing");
                menu->messageBox()->answer(false);
                break;
            case 7:
                if (*unlocked != 1)
                    throw std::runtime_error("Menu cancel unlocked");
                menu->click(6);
                break;
            case 8:
                menu->messageBox()->answer(true);
                break;
            case 9:
            {
                if (*granted != 24 || *unlocked != 2 || progress->eyes != eyes + 32 ||
                    progress->insight != insight + 32 || progress->hands != hands + 32 || progress->feet != feet + 16)
                    throw std::runtime_error("Offline shop quantities incorrect");
                rb::Progress reopened;
                reopened.initialize(data);
                reopened.load(progress->encode());
                if (reopened.encode() != progress->encode())
                    throw std::runtime_error("Shop persistence failed");
                for (int e = 1; e <= 6; ++e)
                    for (int st = 1; st <= 5; ++st)
                        if (!reopened.stageOpened(data, rb::databaseEpisode(e), st))
                            throw std::runtime_error("Locked stage remains");
                utils::captureScreen([](bool ok, std::string_view) {
                    if (!ok)
                        throw std::runtime_error("Shop capture failed");
                    AXLOGI("RB_PROBE_OFFLINE_SHOP PASS repeated grants, cancel/confirm, 30 stages, saved quantities");
                    Director::getInstance()->end();
                }, std::string(directory) + "/offline-shop.png");
                break;
            }
            default:
                break;
            }
        }, .85f, "offline-shop-check");
        scene->scheduleOnce([](float) { throw std::runtime_error("Offline shop timeout"); }, 15,
                            "offline-shop-timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_STAGE_MENU"))
    {
        auto progress = std::make_shared<rb::Progress>();
        progress->initialize(data);
        auto* menu = StageSelect::create(data, *progress, language);
        scene->addChild(menu);
        auto phase = std::make_shared<int>(0);
        auto age   = std::make_shared<float>(0);
        scene->schedule([scene, menu, progress, phase, age, directory](float dt) {
            *age += dt;
            if (*phase == 0 && *age >= 1.8f)
            {
                if (menu->arrows_[0]->getOpacity() != 0 || menu->arrows_[4]->getOpacity() == 0)
                    throw std::runtime_error("Initial arrow pulse/side is incorrect");
                utils::captureScreen([](bool ok, std::string_view) {
                    if (!ok)
                        throw std::runtime_error("Menu arrow capture failed");
                }, std::string(directory) + "/arrows-first.png");
                menu->sortSymbols(1);
                *phase = 1;
                *age   = 0;
            }
            else if (*phase == 1 && *age >= .6f)
            {
                menu->selectEpisode();
                if (!menu->opened_)
                    throw std::runtime_error("First chapter did not open");
                *phase = 2;
                *age   = 0;
            }
            else if (*phase == 2 && *age >= 1.2f)
            {
                menu->closeEpisode();
                *phase = 3;
                *age   = 0;
            }
            else if (*phase == 3 && *age >= .25f)
            {
                if (!menu->moving_ || menu->stageGroup_->getChildrenCount() == 0 || menu->symbols_[1]->getScale() <= 1)
                    throw std::runtime_error("Chapter close animation skipped");
                utils::captureScreen([](bool ok, std::string_view) {
                    if (!ok)
                        throw std::runtime_error("Menu close capture failed");
                }, std::string(directory) + "/doors-closing.png");
                *phase = 4;
            }
            else if (*phase == 4 && *age >= 1.3f)
            {
                if (menu->moving_ || menu->opened_ || menu->stageGroup_->getChildrenCount() != 0)
                    throw std::runtime_error("Chapter close did not complete");
                for (int i = 0; i < 2; ++i)
                    if (menu->doors_[1][i]->getPosition().distance(menu->doorOrigins_[1][i]) > .01f ||
                        std::abs(menu->doors_[1][i]->getScale() - 1) > .01f || menu->doors_[1][i]->getOpacity() != 255)
                        throw std::runtime_error("Chapter doors failed to settle");
                menu->sortSymbols(6);
                *phase = 5;
                *age   = 0;
            }
            else if (*phase == 5 && *age >= 1.8f)
            {
                if (menu->arrows_[4]->getOpacity() != 0 || menu->arrows_[0]->getOpacity() == 0)
                    throw std::runtime_error("Last chapter arrow pulse/side is incorrect");
                utils::captureScreen([](bool ok, std::string_view) {
                    if (!ok)
                        throw std::runtime_error("Last menu capture failed");
                    AXLOGI("RB_PROBE_STAGE_MENU PASS: arrow pulses, boundaries, closing animation");
                    Director::getInstance()->end();
                }, std::string(directory) + "/arrows-last.png");
                scene->unschedule("stage-menu-check");
            }
        }, .05f, "stage-menu-check");
        scene->scheduleOnce([](float) { throw std::runtime_error("Stage menu timeout"); }, 15, "stage-menu-timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_LOADING"))
    {
        auto index  = std::make_shared<int>(0);
        auto active = std::make_shared<bool>(false);
        scene->schedule([scene, index, active, directory](float) {
            if (*active)
                return;
            if (*index == 5)
            {
                AXLOGI("RB_PROBE_LOADING PASS: title, four languages, two-second wait");
                Director::getInstance()->end();
                return;
            }
            *active            = true;
            const int sample   = (*index)++;
            const auto started = std::chrono::steady_clock::now();
            auto* loading      = LoadingScreen::create(sample ? sample - 1 : 0, sample != 0, [active, started] {
                if (std::chrono::duration<float>(std::chrono::steady_clock::now() - started).count() < 1.9f)
                    throw std::runtime_error("Loading finished too early");
                *active = false;
            });
            scene->addChild(loading, 100);
            loading->scheduleOnce([directory, sample](float) {
                utils::captureScreen([](bool ok, std::string_view) {
                    if (!ok)
                        throw std::runtime_error("Loading capture failed");
                }, std::string(directory) + "/loading-" + std::to_string(sample) + ".png");
            }, .75f, "loading-capture");
        }, .1f, "loading-samples");
        scene->scheduleOnce([](float) { throw std::runtime_error("Loading probe timeout"); }, 20, "loading-timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_TITLE_SHOP"))
    {
        auto progress = std::make_shared<rb::Progress>();
        progress->initialize(data);
        progress->eyes    = 3;
        progress->insight = 2;
        progress->hands   = 1;
        progress->feet    = 4;
        auto* shop        = TitleShop::create(*progress, language);
        scene->addChild(shop);
        shop->open();
        auto purchases = std::make_shared<int>(0);
        shop->purchase = [purchases, progress](int index) {
            if (index != 2)
                throw std::runtime_error("Wrong shop purchase callback");
            ++*purchases;
        };
        shop->closed = [purchases] {
            if (*purchases != 1)
                throw std::runtime_error("Shop callback count");
            AXLOGI("RB_PROBE_TITLE_SHOP PASS normal/outside selection, descriptions, callback, close");
            Director::getInstance()->end();
        };
        scene->scheduleOnce([scene, shop, directory](float) {
            auto p = shop->controlCenter(0);
            shop->beginTouch(p);
            shop->endTouch({-100, -100});
            if (shop->selectedIndex() != -1)
                throw std::runtime_error("Shop accepted outside release");
            for (int i = 0; i < 6; ++i)
            {
                p = shop->controlCenter(i);
                shop->beginTouch(p);
                shop->endTouch(p);
                if (shop->selectedIndex() != i || shop->description(0).empty())
                    throw std::runtime_error("Shop selection missing");
            }
            p = shop->controlCenter(2);
            shop->beginTouch(p);
            shop->endTouch(p);
            shop->beginTouch(p);
            shop->endTouch(p);
            scene->scheduleOnce([scene, shop, directory](float) {
                utils::captureScreen([scene, shop](bool ok, std::string_view) {
                    if (!ok)
                        throw std::runtime_error("Shop capture failed");
                    auto p = shop->controlCenter(6);
                    shop->beginTouch(p);
                    shop->endTouch(p);
                }, std::string(directory) + "/title-shop.png");
            }, .5f, "capture-shop");
        }, .5f, "test-shop");
        scene->scheduleOnce([](float) { throw std::runtime_error("Shop timeout"); }, 10, "shop-timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_ACTION_FLOW"))
    {
        auto progress = std::make_shared<rb::Progress>();
        progress->initialize(data);
        auto* session = GameSession::create(data, 0, 1, language, 1, false);
        scene->addChild(session);
        session->bindProgress(*progress);
        session->state().objects.at(1232).state  = 2643;
        session->state().objects.at(1232).active = true;
        session->moveRoom(61, 2, [session] { session->runEvent(12241); });
        auto phase       = std::make_shared<int>(0);
        auto cuts        = std::make_shared<int>(0);
        auto hits        = std::make_shared<int>(0);
        auto capturedHit = std::make_shared<bool>(false);
        scene->schedule([scene, session, progress, phase, cuts, hits, capturedHit, directory](float) {
            if (session->story())
            {
                session->story()->advance();
                return;
            }
            if (auto* d = dynamic_cast<ActionCut*>(session->actionMode()))
            {
                if (d->busy())
                    return;
                if (d->menuVisible())
                    throw std::runtime_error("Integrated cut failed");
                const Vec2 vectors[] = {{-1, 0}, {-1, 1}, {0, 1}, {1, 1}, {1, 0}, {1, -1}, {0, -1}, {-1, -1}};
                auto delta           = vectors[d->direction()].getNormalized() * 160;
                auto p               = Vec2(240, 160) - delta / 2;
                d->beginTouch(p);
                d->moveTouch(p + delta);
                d->endTouch(p + delta);
                ++*cuts;
                if (*cuts == 1)
                    utils::captureScreen([](bool ok, std::string_view) {
                        if (!ok)
                            throw std::runtime_error("Cut trail capture failed");
                    }, std::string(directory) + "/cut-trail.png");
                return;
            }
            if (auto* d = dynamic_cast<ActionHit*>(session->actionMode()))
            {
                if (d->busy())
                    return;
                if (d->menuVisible())
                    throw std::runtime_error("Integrated hit failed");
                if (d->firstTarget() && !*capturedHit)
                {
                    *capturedHit = true;
                    scene->scheduleOnce([directory](float) {
                        utils::captureScreen([](bool ok, std::string_view) {
                            if (!ok)
                                throw std::runtime_error("Hit circle capture failed");
                        }, std::string(directory) + "/hit-circle.png");
                    }, .1f, "capture-hit-ring");
                    return;
                }
                if (int n = d->playableTarget())
                {
                    auto p = d->targetCenter(n);
                    d->beginTouch(p);
                    d->endTouch(p);
                    ++*hits;
                }
                return;
            }
            if (session->busy() || *cuts == 0)
                return;
            if (*phase == 0)
            {
                if (*cuts != 3 || session->state().room != 60 || session->state().objects.at(1232).state != 2611 ||
                    !session->state().objects.at(1220).active)
                    throw std::runtime_error("Cut linked event did not continue");
                *phase                                   = 1;
                session->state().objects.at(1230).active = true;
                session->moveRoom(61, 2, [session] { session->runEvent(12905); });
                return;
            }
            if (*hits == 0)
                return;
            if (*hits != 3 || session->state().room != 60 || !session->state().objects.at(1219).active ||
                session->state().objects.at(1230).active)
                throw std::runtime_error("Hit linked event did not continue");
            scene->unschedule("action-flow");
            utils::captureScreen([](bool ok, std::string_view) {
                if (!ok)
                    throw std::runtime_error("Action flow capture failed");
                AXLOGI("RB_PROBE_ACTION_FLOW PASS");
                Director::getInstance()->end();
            }, std::string(directory) + "/actions-complete.png");
        }, .02f, "action-flow");
        scene->scheduleOnce([](float) { throw std::runtime_error("Action flow timeout"); }, 90, "action-flow-timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_HIT"))
    {
        auto* session = GameSession::create(data, 0, 1, language, 1, false);
        scene->addChild(session);
        for (int level = 1; level <= 20; ++level)
        {
            auto* d = ActionHit::create(level, 0);
            if (d->sequenceSize() < 4 || d->sequenceSize() > 25)
                throw std::runtime_error("Invalid hit level length");
        }
        auto index   = std::make_shared<int>(0);
        auto current = std::make_shared<ActionHit*>(nullptr);
        scene->schedule([scene, index, current, directory](float) {
            if (*current)
                return;
            if (*index == 3)
            {
                AXLOGI("RB_PROBE_HIT PASS");
                Director::getInstance()->end();
                return;
            }
            const int levels[] = {1, 20, 2};
            const int variant  = (*index)++;
            auto* d            = ActionHit::create(levels[variant], 1);
            *current           = d;
            scene->addChild(d, 20);
            auto effects = std::make_shared<int>(0);
            d->shake     = [d, effects, directory, variant] {
                const int number     = d->firstTarget();
                auto* glow           = number == 9 ? d->successCenter_ : d->successOuter_;
                const auto target    = d->targetCenter(number);
                const int occurrence = ++*effects;
                d->scheduleOnce([d, glow, target, directory, variant, occurrence](float) {
                    AXLOGI(
                        "HIT_EFFECT variant={} occurrence={} glowVisible={} opacity={} distance={} itemVisible={} "
                        "itemOpacity={}",
                        variant, occurrence, glow->isVisible(), glow->getOpacity(),
                        glow->getPosition().distance(target), d->itemAction_->isVisible(),
                        d->itemAction_->getOpacity());
                    if (!glow->isVisible() || glow->getOpacity() == 0 || glow->getPosition().distance(target) > .01f ||
                        !d->itemAction_->isVisible() || d->itemAction_->getOpacity() == 0)
                        throw std::runtime_error("Repeated hit effect missing or displaced");
                    if (occurrence <= 2)
                        utils::captureScreen(
                            [](bool ok, std::string_view) {
                            if (!ok)
                                throw std::runtime_error("Hit effect capture failed");
                        }, std::string(directory) + "/hit-effect-" + std::to_string(variant) + "-" +
                                std::to_string(occurrence) + ".png");
                }, .06f, "hit-effect-check");
            };
            d->complete = [current, variant, effects](bool pass) {
                if (pass != (variant != 2))
                    throw std::runtime_error("Hit result mismatch");
                if (pass && *effects < 2)
                    throw std::runtime_error("Repeated hit effects were not exercised");
                *current = nullptr;
            };
            d->open();
            auto phase    = std::make_shared<int>(0);
            auto attempt  = std::make_shared<int>(0);
            auto age      = std::make_shared<float>(0);
            auto captured = std::make_shared<bool>(false);
            d->beginTouch(d->targetCenter(9));
            d->endTouch(d->targetCenter(9));
            d->schedule([d, phase, attempt, age, captured, variant, directory](float dt) {
                *age += dt;
                if (*phase == 3)
                    return;
                if (*phase == 1)
                {
                    if (d->busy() || !d->menuVisible())
                    {
                        *age = 0;
                        return;
                    }
                    if (*age < .6f)
                        return;
                    auto p = d->menuCenter(variant == 2 ? 2 : 1);
                    d->beginTouch(p);
                    d->endTouch(p);
                    if (variant == 2)
                    {
                        *phase = 3;
                        return;
                    }
                    ++*attempt;
                    *phase = 0;
                    *age   = 0;
                    return;
                }
                if (d->busy())
                    return;
                if (d->firstTarget() && !*captured)
                {
                    *captured = true;
                    utils::captureScreen([](bool ok, std::string_view) {
                        if (!ok)
                            throw std::runtime_error("Hit capture failed");
                    }, std::string(directory) + "/hit-" + std::to_string(variant) + ".png");
                    return;
                }
                if (d->menuVisible() && ((variant == 0 && *attempt >= 2) || variant == 1))
                    throw std::runtime_error("Hit rejected correct timing");
                if (!d->firstTarget())
                    return;
                if ((variant == 0 && *attempt == 0) || variant == 2)
                {
                    auto p = d->targetCenter(d->firstTarget());
                    d->beginTouch(p);
                    d->endTouch(p);
                    *phase = 1;
                    *age   = 0;
                    return;
                }
                if (variant == 0 && *attempt == 1)
                {
                    *phase = 1;
                    *age   = 0;
                    return;
                }
                if (d->menuVisible())
                    throw std::runtime_error("Hit rejected correct timing");
                if (const int number = d->playableTarget())
                {
                    auto p = d->targetCenter(number);
                    d->beginTouch(p);
                    d->endTouch(p);
                }
            }, .02f, "hit-test");
        }, .1f, "hit-levels");
        scene->scheduleOnce([](float) { throw std::runtime_error("Hit test timeout"); }, 90, "hit-timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_CUT"))
    {
        auto* session = GameSession::create(data, 0, 1, language, 1, false);
        scene->addChild(session);
        for (int level = 1; level <= 20; ++level)
        {
            auto* d = ActionCut::create(level, 0);
            if (d->stepCount() < 3 || d->stepCount() > 10)
                throw std::runtime_error("Invalid cut level length");
        }
        auto index   = std::make_shared<int>(0);
        auto current = std::make_shared<ActionCut*>(nullptr);
        scene->schedule([scene, index, current, directory](float) {
            if (*current)
                return;
            if (*index == 4)
            {
                AXLOGI("RB_PROBE_CUT PASS");
                Director::getInstance()->end();
                return;
            }
            const int levels[] = {1, 19, 20, 2};
            const int variant  = (*index)++;
            auto* d            = ActionCut::create(levels[variant], 1);
            *current           = d;
            scene->addChild(d, 20);
            d->complete = [current, variant](bool pass) {
                if (pass != (variant != 3))
                    throw std::runtime_error("Cut result mismatch");
                *current = nullptr;
            };
            d->open();
            auto phase   = std::make_shared<int>(0);
            auto attempt = std::make_shared<int>(0);
            auto age     = std::make_shared<float>(0);
            d->beginTouch({240, 160});
            d->moveTouch({440, 160});
            d->endTouch({440, 160});
            if (d->step())
                throw std::runtime_error("Cut accepted reveal input");
            d->schedule([d, phase, attempt, age, variant, directory](float dt) {
                *age += dt;
                if (*phase == 3)
                    return;
                if (*phase == 1)
                {
                    if (d->busy() || !d->menuVisible())
                    {
                        *age = 0;
                        return;
                    }
                    if (*age < .6f)
                        return;
                    auto p = d->menuCenter(variant == 3 ? 2 : 1);
                    d->beginTouch(p);
                    d->endTouch(p);
                    if (variant == 3)
                    {
                        *phase = 3;
                        return;
                    }
                    ++*attempt;
                    *phase = 0;
                    *age   = 0;
                    return;
                }
                if (d->busy())
                    return;
                if (d->step() == 0)
                {
                    utils::captureScreen(
                        [](bool ok, std::string_view) {
                        if (!ok)
                            throw std::runtime_error("Cut capture failed");
                    }, std::string(directory) + "/cut-" + std::to_string(variant) + "-" + std::to_string(*attempt) +
                            ".png");
                }
                const Vec2 center(240, 160);
                if ((variant == 0 && *attempt == 0) || variant == 3)
                {
                    d->beginTouch(center);
                    d->moveTouch(center + Vec2(49, 0));
                    d->endTouch(center + Vec2(49, 0));
                    if (!d->busy() || d->step())
                        throw std::runtime_error("Short cut accepted");
                    *phase = 1;
                    *age   = 0;
                    return;
                }
                if (variant == 0 && *attempt == 1)
                {
                    d->beginTouch(center);
                    d->moveTouch(center + Vec2(-150, 0));
                    d->endTouch(center + Vec2(-150, 0));
                    if (!d->busy())
                        throw std::runtime_error("Wrong direction accepted");
                    *phase = 1;
                    *age   = 0;
                    return;
                }
                if (variant == 0 && *attempt == 2)
                {
                    d->beginTouch(center);
                    *phase = 1;
                    *age   = 0;
                    return;
                }
                const Vec2 vectors[] = {{-1, 0}, {-1, 1}, {0, 1}, {1, 1}, {1, 0}, {1, -1}, {0, -1}, {-1, -1}};
                const auto delta     = vectors[d->direction()].getNormalized() * 150.f;
                const auto p         = center - delta / 2;
                const int before     = d->step();
                d->beginTouch(p);
                d->moveTouch(p + delta);
                if (variant == 0 && before == 0)
                    d->moveTouch(p + delta + Vec2(0, 50));
                d->endTouch(p + delta);
                if (d->step() != before + 1)
                    throw std::runtime_error("Correct cut rejected");
                if (d->step() == d->stepCount())
                    *phase = 3;
            }, .2f, "cut-test");
        }, .1f, "cut-levels");
        scene->scheduleOnce([](float) { throw std::runtime_error("Cut test timeout"); }, 150, "cut-timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_EPISODE2_STAGE2"))
    {
        auto* owner    = std::getenv("RB_ROUTE_DEMO") ? dynamic_cast<MainScene*>(scene) : nullptr;
        routeDemoOwner = owner;
        auto progress  = owner ? owner->routeDemoProgress() : std::make_shared<rb::Progress>();
        if (!owner)
            progress->initialize(data);
        auto* session = owner ? owner->routeDemoSession() : GameSession::create(data, 2, 2, language, 1, false);
        if (!owner)
        {
            scene->addChild(session);
            session->bindProgress(*progress);
            session->begin();
        }
        auto step   = std::make_shared<int>(0);
        auto age    = std::make_shared<float>(0);
        auto puzzle = std::make_shared<int>(0);
        scene->schedule([scene, session, progress, step, age, puzzle, directory, &data](float dt) {
            *age += dt;
            if (session->story())
            {
                advanceRouteStory(session);
                return;
            }
            if (session->itemDialog())
            {
                if (session->itemDialog()->ready())
                    session->itemDialog()->close();
                return;
            }
            if (auto* d = dynamic_cast<ActionHit*>(session->actionMode()))
            {
                if (d->busy())
                    return;
                if (d->menuVisible())
                    throw std::runtime_error("E2Stage2 glass action failed");
                if (int target = d->playableTarget())
                {
                    auto p = d->targetCenter(target);
                    d->beginTouch(p);
                    d->endTouch(p);
                }
                return;
            }
            if (auto* d = session->detail())
            {
                if (d->busy() || *age < .4f)
                    return;
                *age = 0;
                if (d->id() == 3)
                {
                    const int route[] = {32, 2, 4, 6, 8, 10, 40, 38, 36, 34, 64, 66, 68, 98, 100, 70, 72, 42, 44, 14};
                    if (*puzzle >= 20)
                        throw std::runtime_error("E2Stage2 circuit failed");
                    auto* circuit = dynamic_cast<DetailCircuit*>(d);
                    if (circuit->ways().empty())
                        return;
                    auto p = d->controlCenter(route[*puzzle]);
                    d->beginTouch(p);
                    d->endTouch(p);
                    if (circuit->currentPoint() != route[*puzzle])
                        throw std::runtime_error("E2Stage2 circuit input rejected at " + std::to_string(*puzzle));
                    ++*puzzle;
                }
                else if (d->id() == 4)
                {
                    for (int i : {4, 5, 4, 5})
                    {
                        auto p = d->controlCenter(i);
                        d->beginTouch(p);
                        d->endTouch(p);
                    }
                    auto p = d->controlCenter(11);
                    d->beginTouch(p);
                    d->moveTouch(p + Vec2(0, -40));
                    d->endTouch(p + Vec2(0, -40));
                }
                else
                    throw std::runtime_error("E2Stage2 unexpected detail");
                return;
            }
            if (session->clearOverlay())
            {
                if (*step != 22)
                    throw std::runtime_error("E2Stage2 premature clear");
                scene->unschedule("episode1-e2stage2");
                scene->scheduleOnce([directory](float) {
                    utils::captureScreen([](bool ok, std::string_view) {
                        if (!ok)
                            throw std::runtime_error("E2Stage2 capture failed");
                        AXLOGI("RB_PROBE_EPISODE2_STAGE2 PASS");
                        finishRoute();
                    }, std::string(directory) + "/episode1-e2stage2-clear.png");
                }, 2, "e2stage2-clear");
                return;
            }
            if (session->busy() || *age < .8f)
                return;
            auto click = [session, age, &data](int id, int cursor = 0, int item = 0) {
                auto* room = session->room();
                auto p     = room->background()->convertToWorldSpace(room->objectPosition(id));
                if (room->hitObject(p) != id || p.y < 0 || p.y > 320)
                    for (auto* area : data.where("AreaData", "objectID", id))
                    {
                        const int w = rb::integer(*area, "AreaWidth") / 2, h = rb::integer(*area, "AreaHeight") / 2;
                        if (w < 1 || h < 1)
                            continue;
                        auto q = room->background()->convertToWorldSpace(
                            {rb::integer(*area, "AreaX") / 2 + w / 2.f,
                             room->background()->getContentSize().height - rb::integer(*area, "AreaY") / 2 - h / 2.f});
                        if (q.y >= 0 && q.y <= 320 && room->hitObject(q) == id)
                        {
                            p = q;
                            break;
                        }
                    }
                if (p.x < 1 || p.x > 479)
                {
                    room->beginDrag();
                    room->drag((240 - p.x) / .8f);
                    room->drag(0);
                    room->settle();
                    *age = 0;
                    return false;
                }
                if (room->hitObject(p) != id)
                    throw std::runtime_error("E2Stage2 hit mismatch: " + std::to_string(id));
                session->selectCursor(cursor, item);
                session->tap(p);
                *age = 0;
                if (session->touchObject() != id)
                    throw std::runtime_error("E2Stage2 tap rejected");
                return true;
            };
            auto itemPoint = [session](int id) {
                auto ids = session->state().inventory();
                auto it  = std::find(ids.begin(), ids.end(), id);
                if (it == ids.end())
                    throw std::runtime_error("E2Stage2 item absent: " + std::to_string(id));
                int i = static_cast<int>(it - ids.begin());
                return Vec2(48 + (i % 5) * 65, 208 - (i / 5) * 55);
            };
            auto combine = [session, scene, age, itemPoint](int a, int b) {
                auto* inv = session->inventory();
                auto p = itemPoint(a), q = itemPoint(b);
                inv->beginTouch(p);
                scene->scheduleOnce([inv, q](float) {
                    inv->moveTouch(q);
                    inv->endTouch(q);
                }, .8f, "e2stage2-combine");
                *age = -2.5f;
            };
            bool next = false;
            switch (*step)
            {
            case 0:
                next = click(46);
                break;
            case 1:
                next = click(58, 3, 15);
                break;
            case 2:
                next = click(43);
                break;
            case 3:
                next = click(44);
                break;
            case 4:
                session->inventory()->open();
                session->inventory()->selectMode(3);
                *age = 0;
                next = true;
                break;
            case 5:
                combine(10, 12);
                next = true;
                break;
            case 6:
                session->inventory()->close();
                *age = 0;
                next = true;
                break;
            case 7:
                next = click(57, 3, 13);
                break;
            case 8:
                if (session->room()->directionAnimationCount() != 1)
                    throw std::runtime_error("Beam missing");
                next = click(57, 3, 13);
                break;
            case 9:
                if (session->room()->directionAnimationCount() != 0)
                    throw std::runtime_error("Beam removal failed");
                next = click(57, 3, 13);
                break;
            case 10:
                next = click(37);
                break;
            case 11:
                next = click(45);
                break;
            case 12:
                next = click(62, 3, 14);
                break;
            case 13:
                next = click(56);
                break;
            case 14:
                next = click(39);
                break;
            case 15:
                if (session->state().objects.at(339).state != 418)
                    throw std::runtime_error("Screen up failed");
                next = click(39);
                break;
            case 16:
                if (session->state().objects.at(339).state != 417)
                    throw std::runtime_error("Screen down failed");
                next = click(39);
                break;
            case 17:
                next = click(50);
                break;
            case 18:
                next = click(52);
                break;
            case 19:
                next = click(42, 3, 16);
                break;
            case 20:
                next = click(42);
                break;
            case 21:
                next = click(42, 2);
                break;
            default:
                throw std::runtime_error("E2Stage2 exit failed");
            }
            if (next)
            {
                AXLOGI("E2Stage2 step {}", *step);
                ++*step;
            }
        }, .02f, "episode1-e2stage2");
        scene->scheduleOnce([](float) { throw std::runtime_error("E2Stage2 timeout"); },
                            std::getenv("RB_ROUTE_DEMO") ? 3600 : 300, "e2stage2-timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_EPISODE2_STAGE3"))
    {
        auto* owner    = std::getenv("RB_ROUTE_DEMO") ? dynamic_cast<MainScene*>(scene) : nullptr;
        routeDemoOwner = owner;
        auto progress  = owner ? owner->routeDemoProgress() : std::make_shared<rb::Progress>();
        if (!owner)
            progress->initialize(data);
        auto* session = owner ? owner->routeDemoSession() : GameSession::create(data, 2, 3, language, 1, false);
        if (!owner)
        {
            scene->addChild(session);
            session->bindProgress(*progress);
            session->begin();
        }
        auto step = std::make_shared<int>(0);
        auto age  = std::make_shared<float>(0);
        scene->schedule([scene, session, progress, step, age, directory, &data](float dt) {
            *age += dt;
            if (session->story())
            {
                advanceRouteStory(session);
                return;
            }
            if (session->itemDialog())
            {
                if (session->itemDialog()->ready())
                    session->itemDialog()->close();
                return;
            }
            if (auto* d = session->detail())
            {
                if (d->busy())
                    return;
                if (d->id() != 5)
                    throw std::runtime_error("E2Stage3 unexpected detail");
                for (int i : {4, 7, 1, 0})
                {
                    auto p = d->controlCenter(i);
                    d->beginTouch(p);
                    d->endTouch(p);
                }
                return;
            }
            if (session->clearOverlay())
            {
                if (*step != 23)
                    throw std::runtime_error("E2Stage3 premature clear");
                scene->unschedule("episode1-e2stage3");
                scene->scheduleOnce([directory](float) {
                    utils::captureScreen([](bool ok, std::string_view) {
                        if (!ok)
                            throw std::runtime_error("E2Stage3 capture failed");
                        AXLOGI("RB_PROBE_EPISODE2_STAGE3 PASS");
                        finishRoute();
                    }, std::string(directory) + "/episode1-e2stage3-clear.png");
                }, 2, "e2stage3-clear");
                return;
            }
            if (session->busy() || *age < .8f)
                return;
            auto click = [session, age, &data](int id, int cursor = 0, int item = 0) {
                auto* room = session->room();
                auto p     = room->background()->convertToWorldSpace(room->objectPosition(id));
                if (room->hitObject(p) != id || p.y < 0 || p.y > 320)
                    for (auto* area : data.where("AreaData", "objectID", id))
                    {
                        const int w = rb::integer(*area, "AreaWidth") / 2, h = rb::integer(*area, "AreaHeight") / 2;
                        if (w < 1 || h < 1)
                            continue;
                        auto q = room->background()->convertToWorldSpace(
                            {rb::integer(*area, "AreaX") / 2 + w / 2.f,
                             room->background()->getContentSize().height - rb::integer(*area, "AreaY") / 2 - h / 2.f});
                        if (q.y >= 0 && q.y <= 320 && room->hitObject(q) == id)
                        {
                            p = q;
                            break;
                        }
                    }
                if (p.x < 1 || p.x > 479)
                {
                    room->beginDrag();
                    room->drag((240 - p.x) / .8f);
                    room->drag(0);
                    room->settle();
                    *age = 0;
                    return false;
                }
                if (room->hitObject(p) != id)
                    throw std::runtime_error("E2Stage3 hit mismatch: " + std::to_string(id));
                session->selectCursor(cursor, item);
                session->tap(p);
                *age = 0;
                if (session->touchObject() != id)
                    throw std::runtime_error("E2Stage3 tap rejected");
                return true;
            };
            auto itemPoint = [session](int id) {
                auto ids = session->state().inventory();
                auto it  = std::find(ids.begin(), ids.end(), id);
                if (it == ids.end())
                    throw std::runtime_error("E2Stage3 item absent: " + std::to_string(id));
                int i = static_cast<int>(it - ids.begin());
                return Vec2(48 + (i % 5) * 65, 208 - (i / 5) * 55);
            };
            auto combine = [session, scene, age, itemPoint](int a, int b) {
                auto* inv = session->inventory();
                auto p = itemPoint(a), q = itemPoint(b);
                inv->beginTouch(p);
                scene->scheduleOnce([inv, q](float) {
                    inv->moveTouch(q);
                    inv->endTouch(q);
                }, .8f, "e2stage3-combine");
                *age = -2.5f;
            };
            bool next = false;
            switch (*step)
            {
            case 0:
                next = click(76);
                break;
            case 1:
                next = click(63);
                break;
            case 2:
                next = click(64);
                break;
            case 3:
                next = click(91);
                break;
            case 4:
                next = click(65);
                break;
            case 5:
                next = click(82);
                break;
            case 6:
                next = click(66);
                break;
            case 7:
                next = click(84);
                break;
            case 8:
                next = click(67);
                break;
            case 9:
                next = click(68);
                break;
            case 10:
                next = click(90, 3, 22);
                break;
            case 11:
                next = click(89);
                break;
            case 12:
                next = click(90, 3, 23);
                break;
            case 13:
                session->inventory()->open();
                session->inventory()->selectMode(3);
                *age = 0;
                next = true;
                break;
            case 14:
                combine(11, 17);
                next = true;
                break;
            case 15:
                combine(18, 19);
                next = true;
                break;
            case 16:
                session->inventory()->close();
                *age = 0;
                next = true;
                break;
            case 17:
                next = click(94, 3, 20);
                break;
            case 18:
                next = click(93, 3, 21);
                break;
            case 19:
                next = click(94, 2);
                break;
            case 20:
                next = click(103, 3, 24);
                break;
            case 21:
                next = click(103);
                break;
            case 22:
                next = click(104, 2);
                break;
            default:
                throw std::runtime_error("E2Stage3 exit failed");
            }
            if (next)
            {
                AXLOGI("E2Stage3 step {}", *step);
                ++*step;
            }
        }, .02f, "episode1-e2stage3");
        scene->scheduleOnce([](float) { throw std::runtime_error("E2Stage3 timeout"); },
                            std::getenv("RB_ROUTE_DEMO") ? 3600 : 300, "e2stage3-timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_EPISODE2_STAGE4"))
    {
        auto* owner    = std::getenv("RB_ROUTE_DEMO") ? dynamic_cast<MainScene*>(scene) : nullptr;
        routeDemoOwner = owner;
        auto progress  = owner ? owner->routeDemoProgress() : std::make_shared<rb::Progress>();
        if (!owner)
            progress->initialize(data);
        auto* session = owner ? owner->routeDemoSession() : GameSession::create(data, 2, 4, language, 1, false);
        if (!owner)
        {
            scene->addChild(session);
            session->bindProgress(*progress);
            session->begin();
        }
        auto step = std::make_shared<int>(0);
        auto age  = std::make_shared<float>(0);
        scene->schedule([scene, session, progress, step, age, directory, &data](float dt) {
            *age += dt;
            if (session->story())
            {
                advanceRouteStory(session);
                return;
            }
            if (session->itemDialog())
            {
                if (session->itemDialog()->ready())
                    session->itemDialog()->close();
                return;
            }
            if (auto* cut = dynamic_cast<ActionCut*>(session->actionMode()))
            {
                if (cut->busy())
                    return;
                if (cut->menuVisible())
                    throw std::runtime_error("E2Stage4 cut failed");
                const Vec2 vectors[] = {{-1, 0}, {-1, 1}, {0, 1}, {1, 1}, {1, 0}, {1, -1}, {0, -1}, {-1, -1}};
                auto delta           = vectors[cut->direction()].getNormalized() * 160;
                auto p               = Vec2(240, 160) - delta / 2;
                cut->beginTouch(p);
                cut->moveTouch(p + delta);
                cut->endTouch(p + delta);
                return;
            }
            if (auto* d = session->detail())
            {
                if (d->busy() || *age < .6f)
                    return;
                *age = 0;
                if (d->id() == 9)
                {
                    for (int i : {0, 2, 4, 8})
                    {
                        auto p = d->controlCenter(i);
                        d->beginTouch(p);
                        d->endTouch(p);
                    }
                    return;
                }
                if (auto* watch = dynamic_cast<DetailWatchBox*>(d))
                {
                    const int target[] = {1, 2, 3, 0};
                    for (int i = 0; i < 4; ++i)
                        if (watch->entered()[i] != target[i])
                        {
                            auto p = d->controlCenter(i);
                            d->beginTouch(p);
                            d->endTouch(p);
                            return;
                        }
                    return;
                }
                if (auto* safe = dynamic_cast<DetailSafe*>(d))
                {
                    const int target[] = {21, 0, 18, 4};
                    for (int col = 0; col < 4; ++col)
                        if (safe->entered()[col] != target[col])
                        {
                            auto p = d->controlCenter(col);
                            d->beginTouch(p);
                            d->moveTouch(p + Vec2(0, 50));
                            d->endTouch(p + Vec2(0, 50));
                            return;
                        }
                    return;
                }
                throw std::runtime_error("E2Stage4 unexpected detail");
            }
            if (session->clearOverlay())
            {
                if (*step != 20)
                    throw std::runtime_error("E2Stage4 premature clear");
                scene->unschedule("episode1-e2stage4");
                scene->scheduleOnce([directory](float) {
                    utils::captureScreen([](bool ok, std::string_view) {
                        if (!ok)
                            throw std::runtime_error("E2Stage4 capture failed");
                        AXLOGI("RB_PROBE_EPISODE2_STAGE4 PASS");
                        finishRoute();
                    }, std::string(directory) + "/episode1-e2stage4-clear.png");
                }, 2, "e2stage4-clear");
                return;
            }
            if (session->busy() || *age < .8f)
                return;
            auto click = [session, age, &data](int id, int cursor = 0, int item = 0) {
                auto* room = session->room();
                auto p     = room->background()->convertToWorldSpace(room->objectPosition(id));
                if (room->hitObject(p) != id || p.y < 0 || p.y > 320)
                    for (auto* area : data.where("AreaData", "objectID", id))
                    {
                        const int w = rb::integer(*area, "AreaWidth") / 2, h = rb::integer(*area, "AreaHeight") / 2;
                        if (w < 1 || h < 1)
                            continue;
                        auto q = room->background()->convertToWorldSpace(
                            {rb::integer(*area, "AreaX") / 2 + w / 2.f,
                             room->background()->getContentSize().height - rb::integer(*area, "AreaY") / 2 - h / 2.f});
                        if (q.y >= 0 && q.y <= 320 && room->hitObject(q) == id)
                        {
                            p = q;
                            break;
                        }
                    }
                if (p.x < 1 || p.x > 479)
                {
                    room->beginDrag();
                    room->drag((240 - p.x) / .8f);
                    room->drag(0);
                    room->settle();
                    *age = 0;
                    return false;
                }
                if (room->hitObject(p) != id)
                    throw std::runtime_error("E2Stage4 hit mismatch: " + std::to_string(id));
                session->selectCursor(cursor, item);
                session->tap(p);
                *age = 0;
                if (session->touchObject() != id)
                    throw std::runtime_error("E2Stage4 tap rejected");
                return true;
            };
            auto itemPoint = [session](int id) {
                auto ids = session->state().inventory();
                auto it  = std::find(ids.begin(), ids.end(), id);
                if (it == ids.end())
                    throw std::runtime_error("E2Stage4 item absent: " + std::to_string(id));
                int i = static_cast<int>(it - ids.begin());
                return Vec2(48 + (i % 5) * 65, 208 - (i / 5) * 55);
            };
            auto combine = [session, scene, age, itemPoint](int a, int b) {
                auto* inv = session->inventory();
                auto p = itemPoint(a), q = itemPoint(b);
                inv->beginTouch(p);
                scene->scheduleOnce([inv, q](float) {
                    inv->moveTouch(q);
                    inv->endTouch(q);
                }, .8f, "e2stage4-combine");
                *age = -2.5f;
            };
            bool next = false;
            switch (*step)
            {
            case 0:
                next = click(124);
                break;
            case 1:
                next = click(118, 2);
                break;
            case 2:
                next = click(128);
                break;
            case 3:
                next = click(130, 2);
                break;
            case 4:
                next = click(114, 3, 66);
                break;
            case 5:
                next = click(106);
                break;
            case 6:
                next = click(112, 3, 25);
                break;
            case 7:
                next = click(112);
                break;
            case 8:
                next = click(123);
                break;
            case 9:
                next = click(118, 2);
                break;
            case 10:
                next = click(352);
                break;
            case 11:
                next = click(353);
                break;
            case 12:
                session->inventory()->open();
                session->inventory()->selectMode(2);
                *age = 0;
                next = true;
                break;
            case 13:
            {
                auto* inv = session->inventory();
                auto p    = itemPoint(26);
                inv->beginTouch(p);
                scene->scheduleOnce([inv, p](float) {
                    inv->moveTouch(p + Vec2(20, 0));
                    inv->moveTouch(p + Vec2(40, 0));
                    inv->endTouch(p + Vec2(40, 0));
                }, .8f, "e2stage4-split");
                *age = -1;
                next = true;
            }
            break;
            case 14:
                session->inventory()->close();
                *age = 0;
                next = true;
                break;
            case 15:
                next = click(130, 2);
                break;
            case 16:
                next = click(109, 2);
                break;
            case 17:
                next = click(145, 3, 27);
                break;
            case 18:
                next = click(145);
                break;
            case 19:
                next = click(143, 2);
                break;
            default:
                throw std::runtime_error("E2Stage4 exit failed");
            }
            if (next)
            {
                AXLOGI("E2Stage4 step {}", *step);
                ++*step;
            }
        }, .02f, "episode1-e2stage4");
        scene->scheduleOnce([](float) { throw std::runtime_error("E2Stage4 timeout"); },
                            std::getenv("RB_ROUTE_DEMO") ? 3600 : 300, "e2stage4-timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_EPISODE2_STAGE5"))
    {
        auto* owner    = std::getenv("RB_ROUTE_DEMO") ? dynamic_cast<MainScene*>(scene) : nullptr;
        routeDemoOwner = owner;
        auto progress  = owner ? owner->routeDemoProgress() : std::make_shared<rb::Progress>();
        if (!owner)
            progress->initialize(data);
        auto* session = owner ? owner->routeDemoSession() : GameSession::create(data, 2, 5, language, 1, false);
        if (!owner)
        {
            scene->addChild(session);
            session->bindProgress(*progress);
            session->begin();
        }
        auto step = std::make_shared<int>(0);
        auto age  = std::make_shared<float>(0);
        scene->schedule([session, step, directory](float) {
            AXLOGI("E2Stage5 status step={} busy={} story={} item={} detail={} action={} actionBusy={}", *step,
                   session->busy(), session->story() != nullptr, session->itemDialog() != nullptr,
                   session->detail() != nullptr, session->actionMode() != nullptr,
                   session->actionMode() ? session->actionMode()->busy() : false);
            utils::captureScreen([](bool, std::string_view) {}, std::string(directory) + "/status.png");
        }, 10, "e2stage5-status");
        scene->schedule([scene, session, progress, step, age, directory, &data](float dt) {
            *age += dt;
            if (session->story())
            {
                advanceRouteStory(session);
                return;
            }
            if (session->itemDialog())
            {
                if (session->itemDialog()->ready())
                    session->itemDialog()->close();
                return;
            }
            if (auto* cut = dynamic_cast<ActionCut*>(session->actionMode()))
            {
                if (cut->busy())
                    return;
                if (cut->menuVisible())
                    throw std::runtime_error("E2Stage5 cut failed");
                const Vec2 vectors[] = {{-1, 0}, {-1, 1}, {0, 1}, {1, 1}, {1, 0}, {1, -1}, {0, -1}, {-1, -1}};
                auto delta           = vectors[cut->direction()].getNormalized() * 160;
                auto p               = Vec2(240, 160) - delta / 2;
                cut->beginTouch(p);
                cut->moveTouch(p + delta);
                cut->endTouch(p + delta);
                return;
            }
            if (auto* d = dynamic_cast<ActionHit*>(session->actionMode()))
            {
                if (d->busy())
                    return;
                if (d->menuVisible())
                    throw std::runtime_error("E2Stage5 hit failed");
                if (int target = d->playableTarget())
                {
                    auto p = d->targetCenter(target);
                    d->beginTouch(p);
                    d->endTouch(p);
                }
                return;
            }
            if (auto* d = session->detail())
            {
                if (d->busy())
                    return;
                if (d->id() == 12)
                {
                    for (int i : {1, 1, 1, 1})
                    {
                        auto p = d->controlCenter(i);
                        d->beginTouch(p);
                        d->endTouch(p);
                    }
                    return;
                }
                if (auto* wine = dynamic_cast<DetailWineBox*>(d))
                {
                    for (int i = 0; i < 14; ++i)
                    {
                        int target = i % 2 == 0 || i == 13 ? 2 : 1;
                        while (wine->entered()[i] != target)
                        {
                            auto p = d->controlCenter(i);
                            d->beginTouch(p);
                            d->endTouch(p);
                        }
                    }
                    return;
                }
                throw std::runtime_error("E2Stage5 unexpected detail");
            }
            if (session->clearOverlay())
            {
                if (*step != 41)
                    throw std::runtime_error("E2Stage5 premature clear");
                scene->unschedule("episode1-e2stage5");
                scene->scheduleOnce([directory](float) {
                    utils::captureScreen([](bool ok, std::string_view) {
                        if (!ok)
                            throw std::runtime_error("E2Stage5 capture failed");
                        AXLOGI("RB_PROBE_EPISODE2_STAGE5 PASS");
                        finishRoute();
                    }, std::string(directory) + "/episode1-e2stage5-clear.png");
                }, 2, "e2stage5-clear");
                return;
            }
            if (session->busy() || *age < .8f)
                return;
            auto click = [session, age, &data](int id, int cursor = 0, int item = 0) {
                auto* room = session->room();
                auto p     = room->background()->convertToWorldSpace(room->objectPosition(id));
                if (room->hitObject(p) != id || p.y < 0 || p.y > 320 || p.x < 1 || p.x > 479)
                    for (auto* area : data.where("AreaData", "objectID", id))
                    {
                        const int w = rb::integer(*area, "AreaWidth") / 2, h = rb::integer(*area, "AreaHeight") / 2;
                        if (w < 1 || h < 1)
                            continue;
                        auto q = room->background()->convertToWorldSpace(
                            {std::clamp(rb::integer(*area, "AreaX") / 2 + w / 2.f, 1.f,
                                        room->background()->getContentSize().width - 1),
                             room->background()->getContentSize().height - rb::integer(*area, "AreaY") / 2 - h / 2.f});
                        if (q.y >= 0 && q.y <= 320 && room->hitObject(q) == id)
                        {
                            p = q;
                            break;
                        }
                    }
                if (p.x < 1 || p.x > 479)
                {
                    room->beginDrag();
                    room->drag((240 - p.x) / .8f);
                    room->drag(0);
                    room->settle();
                    *age = 0;
                    return false;
                }
                if (room->hitObject(p) != id)
                    throw std::runtime_error("E2Stage5 hit mismatch: " + std::to_string(id));
                session->selectCursor(cursor, item);
                session->tap(p);
                *age = 0;
                if (session->touchObject() != id)
                    throw std::runtime_error("E2Stage5 tap rejected");
                return true;
            };
            auto itemPoint = [session](int id) {
                auto ids = session->state().inventory();
                auto it  = std::find(ids.begin(), ids.end(), id);
                if (it == ids.end())
                    throw std::runtime_error("E2Stage5 item absent: " + std::to_string(id));
                int i = static_cast<int>(it - ids.begin());
                return Vec2(48 + (i % 5) * 65, 208 - (i / 5) * 55);
            };
            auto combine = [session, scene, age, itemPoint](int a, int b) {
                auto* inv = session->inventory();
                auto p = itemPoint(a), q = itemPoint(b);
                inv->beginTouch(p);
                scene->scheduleOnce([inv, q](float) {
                    inv->moveTouch(q);
                    inv->endTouch(q);
                }, .8f, "e2stage5-combine");
                *age = -2.5f;
            };
            bool next = false;
            switch (*step)
            {
            case 0:
                next = click(216);
                break;
            case 1:
                next = click(242, 2);
                break;
            case 2:
                next = click(187, 2);
                break;
            case 3:
                next = click(188);
                break;
            case 4:
                next = click(189);
                break;
            case 5:
                next = click(1288, 3, 38);
                break;
            case 6:
                next = click(205, 2);
                break;
            case 7:
                next = click(186, 2);
                break;
            case 8:
                next = click(177);
                break;
            case 9:
                next = click(164);
                break;
            case 10:
                next = click(165);
                break;
            case 11:
                next = click(168);
                break;
            case 12:
                next = click(1289, 3, 38);
                break;
            case 13:
                next = click(181, 2);
                break;
            case 14:
                next = click(1290, 3, 38);
                break;
            case 15:
                next = click(151, 2);
                break;
            case 16:
                next = click(182, 2);
                break;
            case 17:
                next = click(185, 2);
                break;
            case 18:
                session->inventory()->open();
                session->inventory()->selectMode(3);
                *age = 0;
                next = true;
                break;
            case 19:
                combine(264, 265);
                next = true;
                break;
            case 20:
                combine(267, 266);
                next = true;
                break;
            case 21:
                combine(31, 32);
                next = true;
                break;
            case 22:
                session->inventory()->close();
                *age = 0;
                next = true;
                break;
            case 23:
                if (session->state().objects.at(225).state != 284)
                    throw std::runtime_error("E2Stage5 fuse box was not unlocked by the opening scene");
                next = click(225);
                break;
            case 24:
                next = click(235, 3, 33);
                break;
            case 25:
                next = click(231);
                break;
            case 26:
                next = click(230);
                break;
            case 27:
                if (!session->state().objects.at(233).active || !session->state().objects.at(234).active ||
                    session->state().objects.at(230).state != 293)
                    throw std::runtime_error("E2Stage5 colored light clue is not visible");
                utils::captureScreen([](bool ok, std::string_view) {
                    if (!ok)
                        throw std::runtime_error("E2Stage5 light clue capture failed");
                }, std::string(directory) + "/colored-light-clue.png");
                next = true;
                break;
            case 28:
                next = click(230);
                break;
            case 29:
                next = click(222);
                break;
            case 30:
                next = click(219);
                break;
            case 31:
                session->inventory()->open();
                session->inventory()->selectMode(3);
                *age = 0;
                next = true;
                break;
            case 32:
                combine(34, 35);
                next = true;
                break;
            case 33:
                session->inventory()->close();
                *age = 0;
                next = true;
                break;
            case 34:
                (void)itemPoint(36);
                next = click(229);
                break;
            case 35:
                next = click(227, 3, 37);
                break;
            case 36:
                next = click(227, 3, 39);
                break;
            case 37:
                next = click(223, 3, 39);
                break;
            case 38:
                next = click(1344);
                break;
            case 39:
                next = click(227, 3, 268);
                break;
            case 40:
                next = click(227, 2);
                break;
            default:
                throw std::runtime_error("E2Stage5 exit failed");
            }
            if (next)
            {
                AXLOGI("E2Stage5 step {}", *step);
                ++*step;
            }
        }, .02f, "episode1-e2stage5");
        scene->scheduleOnce([](float) { throw std::runtime_error("E2Stage5 timeout"); },
                            std::getenv("RB_ROUTE_DEMO") ? 3600 : 480, "e2stage5-timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_EPISODE3_STAGE5"))
    {
        auto* owner    = std::getenv("RB_ROUTE_DEMO") ? dynamic_cast<MainScene*>(scene) : nullptr;
        routeDemoOwner = owner;
        auto progress  = owner ? owner->routeDemoProgress() : std::make_shared<rb::Progress>();
        if (!owner)
            progress->initialize(data);
        auto* session = owner ? owner->routeDemoSession() : GameSession::create(data, 3, 5, language, 1, false);
        if (!owner)
        {
            scene->addChild(session);
            session->bindProgress(*progress);
            session->begin();
        }
        auto step = std::make_shared<int>(0);
        auto age  = std::make_shared<float>(0);
        scene->schedule([scene, session, progress, step, age, directory, &data](float dt) {
            *age += dt;
            if (session->story())
            {
                advanceRouteStory(session);
                return;
            }
            if (session->itemDialog())
            {
                if (session->itemDialog()->ready())
                    session->itemDialog()->close();
                return;
            }
            if (auto* cut = dynamic_cast<ActionCut*>(session->actionMode()))
            {
                if (cut->busy())
                    return;
                if (cut->menuVisible())
                    throw std::runtime_error("E3Stage5 cut failed");
                const Vec2 vectors[] = {{-1, 0}, {-1, 1}, {0, 1}, {1, 1}, {1, 0}, {1, -1}, {0, -1}, {-1, -1}};
                auto delta           = vectors[cut->direction()].getNormalized() * 160;
                auto p               = Vec2(240, 160) - delta / 2;
                cut->beginTouch(p);
                cut->moveTouch(p + delta);
                cut->endTouch(p + delta);
                return;
            }
            if (auto* d = dynamic_cast<ActionHit*>(session->actionMode()))
            {
                if (d->busy())
                    return;
                if (d->menuVisible())
                    throw std::runtime_error("E3Stage5 hit failed");
                if (int target = d->playableTarget())
                {
                    auto p = d->targetCenter(target);
                    d->beginTouch(p);
                    d->endTouch(p);
                }
                return;
            }
            if (auto* d = session->detail())
            {
                if (d->busy())
                    return;
                if (d->id() == 79)
                {
                    d->close();
                    return;
                }
                throw std::runtime_error("E3Stage5 unexpected detail");
            }
            if (session->clearOverlay())
            {
                if (*step != 33)
                    throw std::runtime_error("E3Stage5 premature clear");
                scene->unschedule("episode1-e3stage5");
                scene->scheduleOnce([directory](float) {
                    utils::captureScreen([](bool ok, std::string_view) {
                        if (!ok)
                            throw std::runtime_error("E3Stage5 capture failed");
                        AXLOGI("RB_PROBE_EPISODE3_STAGE5 PASS");
                        finishRoute();
                    }, std::string(directory) + "/episode1-e3stage5-clear.png");
                }, 2, "e3stage5-clear");
                return;
            }
            if (session->busy() || *age < .8f)
                return;
            auto click = [session, age, &data](int id, int cursor = 0, int item = 0) {
                auto* room = session->room();
                auto p     = room->background()->convertToWorldSpace(room->objectPosition(id));
                if (room->hitObject(p) != id || p.y < 0 || p.y > 320)
                    for (auto* area : data.where("AreaData", "objectID", id))
                    {
                        const int w = rb::integer(*area, "AreaWidth") / 2, h = rb::integer(*area, "AreaHeight") / 2;
                        if (w < 1 || h < 1)
                            continue;
                        auto q = room->background()->convertToWorldSpace(
                            {rb::integer(*area, "AreaX") / 2 + w / 2.f,
                             room->background()->getContentSize().height - rb::integer(*area, "AreaY") / 2 - h / 2.f});
                        if (q.y >= 0 && q.y <= 320 && room->hitObject(q) == id)
                        {
                            p = q;
                            break;
                        }
                    }
                if (p.x < 1 || p.x > 479)
                {
                    room->beginDrag();
                    room->drag((240 - p.x) / .8f);
                    room->drag(0);
                    room->settle();
                    *age = 0;
                    return false;
                }
                if (room->hitObject(p) != id)
                    throw std::runtime_error("E3Stage5 hit mismatch: " + std::to_string(id));
                session->selectCursor(cursor, item);
                session->tap(p);
                *age = 0;
                if (session->touchObject() != id)
                    throw std::runtime_error("E3Stage5 tap rejected");
                return true;
            };
            auto itemPoint = [session](int id) {
                auto ids = session->state().inventory();
                auto it  = std::find(ids.begin(), ids.end(), id);
                if (it == ids.end())
                    throw std::runtime_error("E3Stage5 item absent: " + std::to_string(id));
                int i = static_cast<int>(it - ids.begin());
                return Vec2(48 + (i % 5) * 65, 208 - (i / 5) * 55);
            };
            auto combine = [session, scene, age, itemPoint](int a, int b) {
                auto* inv = session->inventory();
                auto p = itemPoint(a), q = itemPoint(b);
                inv->beginTouch(p);
                scene->scheduleOnce([inv, q](float) {
                    inv->moveTouch(q);
                    inv->endTouch(q);
                }, .8f, "e3stage5-combine");
                *age = -2.5f;
            };
            bool next = false;
            switch (*step)
            {
            case 0:
                next = click(1106);
                break;
            case 1:
                next = click(1107);
                break;
            case 2:
                next = click(1122, 3, 244);
                break;
            case 3:
                next = click(1108);
                break;
            case 4:
                next = click(1120, 3, 249);
                break;
            case 5:
                next = click(1121, 3, 245);
                break;
            case 6:
                next = click(1111);
                break;
            case 7:
                next = click(1120, 3, 250);
                break;
            case 8:
                next = click(1132, 3, 245);
                break;
            case 9:
                next = click(1109);
                break;
            case 10:
                next = click(1120, 3, 251);
                break;
            case 11:
                next = click(1140, 3, 245);
                break;
            case 12:
                next = click(1176);
                break;
            case 13:
                next = click(1120, 3, 253);
                break;
            case 14:
                next = click(1138);
                break;
            case 15:
                next = click(1110);
                break;
            case 16:
                next = click(1112);
                break;
            case 17:
                session->inventory()->open();
                session->inventory()->selectMode(2);
                *age = 0;
                next = true;
                break;
            case 18:
            {
                auto* inv = session->inventory();
                auto p    = itemPoint(246);
                inv->beginTouch(p);
                scene->scheduleOnce([inv, p](float) {
                    inv->moveTouch(p + Vec2(20, 0));
                    inv->moveTouch(p + Vec2(40, 0));
                    inv->endTouch(p + Vec2(40, 0));
                }, .8f, "e3stage5-split");
                *age = -1;
                next = true;
            };
            break;
            case 19:
                session->inventory()->selectMode(3);
                *age = 0;
                next = true;
                break;
            case 20:
                combine(254, 244);
                next = true;
                break;
            case 21:
                session->inventory()->close();
                *age = 0;
                next = true;
                break;
            case 22:
                next = click(1120, 3, 248);
                break;
            case 23:
                next = click(1120, 3, 252);
                break;
            case 24:
                next = click(1137);
                break;
            case 25:
                next = click(1113);
                break;
            case 26:
                next = click(1124, 3, 257);
                break;
            case 27:
                next = click(1124, 3, 245);
                break;
            case 28:
                next = click(1124, 3, 245);
                break;
            case 29:
                next = click(1124);
                break;
            case 30:
                next = click(1124, 2);
                break;
            case 31:
                next = click(1205);
                break;
            case 32:
                next = click(1205, 2);
                break;
            default:
                throw std::runtime_error("E3Stage5 exit failed");
            }
            if (next)
            {
                AXLOGI("E3Stage5 step {}", *step);
                ++*step;
            }
        }, .02f, "episode1-e3stage5");
        scene->scheduleOnce([](float) { throw std::runtime_error("E3Stage5 timeout"); },
                            std::getenv("RB_ROUTE_DEMO") ? 3600 : 300, "e3stage5-timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_EPISODE4_STAGE1"))
    {
        auto* owner    = std::getenv("RB_ROUTE_DEMO") ? dynamic_cast<MainScene*>(scene) : nullptr;
        routeDemoOwner = owner;
        auto progress  = owner ? owner->routeDemoProgress() : std::make_shared<rb::Progress>();
        if (!owner)
            progress->initialize(data);
        auto* session = owner ? owner->routeDemoSession() : GameSession::create(data, 4, 1, language, 1, false);
        if (!owner)
        {
            scene->addChild(session);
            session->bindProgress(*progress);
            session->begin();
        }
        auto step = std::make_shared<int>(0);
        auto age  = std::make_shared<float>(0);
        scene->schedule([scene, session, progress, step, age, directory, &data](float dt) {
            *age += dt;
            if (session->story())
            {
                advanceRouteStory(session);
                return;
            }
            if (session->itemDialog())
            {
                if (session->itemDialog()->ready())
                    session->itemDialog()->close();
                return;
            }
            if (auto* d = dynamic_cast<ActionHit*>(session->actionMode()))
            {
                if (d->busy())
                    return;
                if (d->menuVisible())
                    throw std::runtime_error("E4Stage1 hit failed");
                if (int target = d->playableTarget())
                {
                    auto p = d->targetCenter(target);
                    d->beginTouch(p);
                    d->endTouch(p);
                }
                return;
            }
            if (auto* d = session->detail())
            {
                if (d->busy())
                    return;
                if (d->id() == 16)
                {
                    d->close();
                    return;
                }
                throw std::runtime_error("E4Stage1 unexpected detail");
            }
            if (session->clearOverlay())
            {
                if (*step != 37)
                    throw std::runtime_error("E4Stage1 premature clear");
                scene->unschedule("episode1-e4stage1");
                scene->scheduleOnce([directory](float) {
                    utils::captureScreen([](bool ok, std::string_view) {
                        if (!ok)
                            throw std::runtime_error("E4Stage1 capture failed");
                        AXLOGI("RB_PROBE_EPISODE4_STAGE1 PASS");
                        finishRoute();
                    }, std::string(directory) + "/episode1-e4stage1-clear.png");
                }, 2, "e4stage1-clear");
                return;
            }
            if (session->busy() || *age < .8f)
                return;
            auto click = [session, age, &data](int id, int cursor = 0, int item = 0) {
                auto* room = session->room();
                auto p     = room->background()->convertToWorldSpace(room->objectPosition(id));
                if (room->hitObject(p) != id || p.y < 0 || p.y > 320)
                    for (auto* area : data.where("AreaData", "objectID", id))
                    {
                        const int w = rb::integer(*area, "AreaWidth") / 2, h = rb::integer(*area, "AreaHeight") / 2;
                        if (w < 1 || h < 1)
                            continue;
                        auto q = room->background()->convertToWorldSpace(
                            {rb::integer(*area, "AreaX") / 2 + w / 2.f,
                             room->background()->getContentSize().height - rb::integer(*area, "AreaY") / 2 - h / 2.f});
                        if (q.y >= 0 && q.y <= 320 && room->hitObject(q) == id)
                        {
                            p = q;
                            break;
                        }
                    }
                if (p.x < 1 || p.x > 479)
                {
                    room->beginDrag();
                    room->drag((240 - p.x) / .8f);
                    room->drag(0);
                    room->settle();
                    *age = 0;
                    return false;
                }
                if (room->hitObject(p) != id)
                    throw std::runtime_error("E4Stage1 hit mismatch: " + std::to_string(id));
                session->selectCursor(cursor, item);
                session->tap(p);
                *age = 0;
                if (session->touchObject() != id)
                    throw std::runtime_error("E4Stage1 tap rejected");
                return true;
            };
            auto itemPoint = [session](int id) {
                auto ids = session->state().inventory();
                auto it  = std::find(ids.begin(), ids.end(), id);
                if (it == ids.end())
                    throw std::runtime_error("E4Stage1 item absent: " + std::to_string(id));
                int i = static_cast<int>(it - ids.begin());
                return Vec2(48 + (i % 5) * 65, 208 - (i / 5) * 55);
            };
            auto combine = [session, scene, age, itemPoint](int a, int b) {
                auto* inv = session->inventory();
                auto p = itemPoint(a), q = itemPoint(b);
                inv->beginTouch(p);
                scene->scheduleOnce([inv, q](float) {
                    inv->moveTouch(q);
                    inv->endTouch(q);
                }, .8f, "e4stage1-combine");
                *age = -2.5f;
            };
            auto split = [session, scene, age, itemPoint](int id) {
                auto* inv = session->inventory();
                auto p    = itemPoint(id);
                inv->beginTouch(p);
                scene->scheduleOnce([inv, p](float) {
                    inv->moveTouch(p + Vec2(20, 0));
                    inv->moveTouch(p + Vec2(40, 0));
                    inv->endTouch(p + Vec2(40, 0));
                }, .8f, "e4stage1-split");
                *age = -1;
            };
            bool next = false;
            switch (*step)
            {
            case 0:
                next = click(355);
                break;
            case 1:
                next = click(354, 3, 67);
                break;
            case 2:
                next = click(400);
                break;
            case 3:
                next = click(368);
                break;
            case 4:
                next = click(369);
                break;
            case 5:
                next = click(370);
                break;
            case 6:
                next = click(371);
                break;
            case 7:
                next = click(362);
                break;
            case 8:
                session->inventory()->open();
                session->inventory()->selectMode(2);
                *age = 0;
                next = true;
                break;
            case 9:
                split(69);
                next = true;
                break;
            case 10:
                split(70);
                next = true;
                break;
            case 11:
                split(71);
                next = true;
                break;
            case 12:
                split(72);
                next = true;
                break;
            case 13:
                split(73);
                next = true;
                break;
            case 14:
                split(74);
                next = true;
                break;
            case 15:
                session->inventory()->close();
                *age = 0;
                next = true;
                break;
            case 16:
                next = click(513, 3, 258);
                break;
            case 17:
                next = click(514, 3, 259);
                break;
            case 18:
                next = click(515, 3, 260);
                break;
            case 19:
                next = click(516, 3, 261);
                break;
            case 20:
                next = click(517, 3, 262);
                break;
            case 21:
                next = click(518, 3, 263);
                break;
            case 22:
                next = click(368, 3, 67);
                break;
            case 23:
                next = click(372);
                break;
            case 24:
                next = click(369, 3, 67);
                break;
            case 25:
                next = click(373);
                break;
            case 26:
                next = click(370, 3, 67);
                break;
            case 27:
                next = click(374);
                break;
            case 28:
                next = click(371, 3, 67);
                break;
            case 29:
                next = click(375);
                break;
            case 30:
                next = click(391, 3, 82);
                break;
            case 31:
                next = click(392, 3, 83);
                break;
            case 32:
                next = click(393, 3, 84);
                break;
            case 33:
                next = click(394, 3, 85);
                break;
            case 34:
                next = click(364);
                break;
            case 35:
                next = click(397, 3, 68);
                break;
            case 36:
                next = click(356, 2);
                break;
            default:
                throw std::runtime_error("E4Stage1 exit failed");
            }
            if (next)
            {
                AXLOGI("E4Stage1 step {}", *step);
                ++*step;
            }
        }, .02f, "episode1-e4stage1");
        scene->scheduleOnce([](float) { throw std::runtime_error("E4Stage1 timeout"); },
                            std::getenv("RB_ROUTE_DEMO") ? 3600 : 600, "e4stage1-timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_EPISODE4_STAGE3"))
    {
        auto* owner    = std::getenv("RB_ROUTE_DEMO") ? dynamic_cast<MainScene*>(scene) : nullptr;
        routeDemoOwner = owner;
        auto progress  = owner ? owner->routeDemoProgress() : std::make_shared<rb::Progress>();
        if (!owner)
            progress->initialize(data);
        auto* session = owner ? owner->routeDemoSession() : GameSession::create(data, 4, 3, language, 1, false);
        if (!owner)
        {
            scene->addChild(session);
            session->bindProgress(*progress);
            session->begin();
        }
        auto step = std::make_shared<int>(0);
        auto age  = std::make_shared<float>(0);
        scene->schedule([scene, session, progress, step, age, directory, &data](float dt) {
            *age += dt;
            if (session->story())
            {
                advanceRouteStory(session);
                return;
            }
            if (session->itemDialog())
            {
                if (session->itemDialog()->ready())
                    session->itemDialog()->close();
                return;
            }
            if (auto* d = dynamic_cast<ActionHit*>(session->actionMode()))
            {
                if (d->busy())
                    return;
                if (d->menuVisible())
                    throw std::runtime_error("E4Stage3 hit failed");
                if (int target = d->playableTarget())
                {
                    auto p = d->targetCenter(target);
                    d->beginTouch(p);
                    d->endTouch(p);
                }
                return;
            }
            if (auto* d = session->detail())
            {
                if (d->busy() || *age < .7f)
                    return;
                auto* h = dynamic_cast<DetailHistoricSafe*>(d);
                if (!h)
                    throw std::runtime_error("E4Stage3 unexpected detail");
                int target = h->entered().size() < 2 ? 1 : 0;
                auto p = d->controlCenter(target), center = d->controlCenter(10);
                if (std::abs(p.x - center.x) > 1)
                {
                    d->beginTouch(center);
                    d->moveTouch(center + Vec2(center.x - p.x, 0));
                    d->endTouch(center + Vec2(center.x - p.x, 0));
                }
                else
                {
                    d->beginTouch(p);
                    d->endTouch(p);
                }
                *age = 0;
                return;
            }
            if (session->clearOverlay())
            {
                if (*step != 34)
                    throw std::runtime_error("E4Stage3 premature clear");
                scene->unschedule("episode1-e4stage3");
                scene->scheduleOnce([directory](float) {
                    utils::captureScreen([](bool ok, std::string_view) {
                        if (!ok)
                            throw std::runtime_error("E4Stage3 capture failed");
                        AXLOGI("RB_PROBE_EPISODE4_STAGE3 PASS");
                        finishRoute();
                    }, std::string(directory) + "/episode1-e4stage3-clear.png");
                }, 2, "e4stage3-clear");
                return;
            }
            if (session->busy() || *age < .8f)
                return;
            auto click = [session, age, &data](int id, int cursor = 0, int item = 0) {
                auto* room = session->room();
                auto p     = room->background()->convertToWorldSpace(room->objectPosition(id));
                if (room->hitObject(p) != id || p.y < 0 || p.y > 320)
                    for (auto* area : data.where("AreaData", "objectID", id))
                    {
                        const int w = rb::integer(*area, "AreaWidth") / 2, h = rb::integer(*area, "AreaHeight") / 2;
                        if (w < 1 || h < 1)
                            continue;
                        auto q = room->background()->convertToWorldSpace(
                            {rb::integer(*area, "AreaX") / 2 + w / 2.f,
                             room->background()->getContentSize().height - rb::integer(*area, "AreaY") / 2 - h / 2.f});
                        if (q.y >= 0 && q.y <= 320 && room->hitObject(q) == id)
                        {
                            p = q;
                            break;
                        }
                    }
                if (p.x < 1 || p.x > 479)
                {
                    room->beginDrag();
                    room->drag((240 - p.x) / .8f);
                    room->drag(0);
                    room->settle();
                    *age = 0;
                    return false;
                }
                if (room->hitObject(p) != id)
                    throw std::runtime_error("E4Stage3 hit mismatch: " + std::to_string(id));
                session->selectCursor(cursor, item);
                session->tap(p);
                *age = 0;
                if (session->touchObject() != id)
                    throw std::runtime_error("E4Stage3 tap rejected");
                return true;
            };
            auto itemPoint = [session](int id) {
                auto ids = session->state().inventory();
                auto it  = std::find(ids.begin(), ids.end(), id);
                if (it == ids.end())
                    throw std::runtime_error("E4Stage3 item absent: " + std::to_string(id));
                int i = static_cast<int>(it - ids.begin());
                return Vec2(48 + (i % 5) * 65, 208 - (i / 5) * 55);
            };
            auto combine = [session, scene, age, itemPoint](int a, int b) {
                auto* inv = session->inventory();
                auto p = itemPoint(a), q = itemPoint(b);
                inv->beginTouch(p);
                scene->scheduleOnce([inv, q](float) {
                    inv->moveTouch(q);
                    inv->endTouch(q);
                }, .8f, "e4stage3-combine");
                *age = -2.5f;
            };
            bool next = false;
            switch (*step)
            {
            case 0:
                next = click(284);
                break;
            case 1:
                next = click(295, 3, 58);
                break;
            case 2:
                next = click(306);
                break;
            case 3:
                next = click(285);
                break;
            case 4:
                next = click(336, 3, 59);
                break;
            case 5:
                next = click(338, 3, 58);
                break;
            case 6:
                next = click(335);
                break;
            case 7:
                next = click(304, 3, 60);
                break;
            case 8:
                next = click(286);
                break;
            case 9:
                next = click(334, 3, 58);
                break;
            case 10:
                next = click(301, 3, 61);
                break;
            case 11:
                next = click(308, 3, 62);
                break;
            case 12:
                next = click(342);
                break;
            case 13:
                next = click(301, 3, 61);
                break;
            case 14:
                next = click(308, 3, 62);
                break;
            case 15:
                next = click(342);
                break;
            case 16:
                next = click(301, 3, 61);
                break;
            case 17:
                next = click(308, 3, 62);
                break;
            case 18:
                next = click(342);
                break;
            case 19:
                next = click(301, 3, 61);
                break;
            case 20:
                next = click(308, 3, 62);
                break;
            case 21:
                next = click(342);
                break;
            case 22:
                next = click(343);
                break;
            case 23:
                next = click(325, 3, 64);
                break;
            case 24:
                next = click(301, 3, 61);
                break;
            case 25:
                next = click(308, 3, 62);
                break;
            case 26:
                next = click(301, 3, 61);
                break;
            case 27:
                next = click(308, 3, 62);
                break;
            case 28:
                next = click(345);
                break;
            case 29:
                next = click(324, 3, 63);
                break;
            case 30:
                next = click(320);
                break;
            case 31:
                next = click(289);
                break;
            case 32:
                next = click(323, 3, 65);
                break;
            case 33:
                next = click(316, 2);
                break;
            default:
                throw std::runtime_error("E4Stage3 exit failed");
            }
            if (next)
            {
                AXLOGI("E4Stage3 step {}", *step);
                ++*step;
            }
        }, .02f, "episode1-e4stage3");
        scene->scheduleOnce([](float) { throw std::runtime_error("E4Stage3 timeout"); },
                            std::getenv("RB_ROUTE_DEMO") ? 3600 : 600, "e4stage3-timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_EPISODE4_STAGE5"))
    {
        auto* owner    = std::getenv("RB_ROUTE_DEMO") ? dynamic_cast<MainScene*>(scene) : nullptr;
        routeDemoOwner = owner;
        auto progress  = owner ? owner->routeDemoProgress() : std::make_shared<rb::Progress>();
        if (!owner)
            progress->initialize(data);
        auto* session = owner ? owner->routeDemoSession() : GameSession::create(data, 4, 5, language, 1, false);
        if (!owner)
        {
            scene->addChild(session);
            session->bindProgress(*progress);
            session->begin();
        }
        auto step   = std::make_shared<int>(0);
        auto age    = std::make_shared<float>(0);
        auto timing = std::make_shared<bool>(false);
        scene->schedule([scene, session, progress, step, age, timing, directory, &data](float dt) {
            *age += dt;
            if (session->story())
            {
                advanceRouteStory(session);
                return;
            }
            if (session->itemDialog())
            {
                if (session->itemDialog()->ready())
                    session->itemDialog()->close();
                return;
            }
            if (auto* d = dynamic_cast<ActionHit*>(session->actionMode()))
            {
                if (d->busy())
                    return;
                if (d->menuVisible())
                    throw std::runtime_error("E4Stage5 hit failed");
                if (int target = d->playableTarget())
                {
                    auto p = d->targetCenter(target);
                    d->beginTouch(p);
                    d->endTouch(p);
                }
                return;
            }
            if (auto* d = session->detail())
            {
                if (d->busy() || *age < .65f)
                    return;
                if (d->id() == 21)
                {
                    d->close();
                    return;
                }
                if (auto* pad = dynamic_cast<DetailAlienPad*>(d))
                {
                    const std::map<int, std::string> words = {{22, "pendant"}, {27, "earth"},   {28, "alien"},
                                                              {29, "crystal"}, {30, "english"}, {31, "peace"}};
                    const auto word                        = words.at(d->id());
                    if (pad->entered().size() >= word.size())
                        return;
                    auto p    = pad->controlCenter(26);
                    int delta = (word.at(pad->entered().size()) - 'a' - pad->selected() + 26) % 26;
                    if (delta > 13)
                        delta -= 26;
                    pad->beginTouch(p);
                    if (delta)
                    {
                        delta = std::clamp(delta, -3, 3);
                        p += Vec2(-45.5f * delta, 0);
                        pad->moveTouch(p);
                    }
                    pad->endTouch(p);
                    *age = 0;
                    return;
                }
                throw std::runtime_error("E4Stage5 unexpected detail");
            }
            if (session->clearOverlay())
            {
                if (*step != 71)
                    throw std::runtime_error("E4Stage5 premature clear");
                scene->unschedule("episode1-e4stage5");
                scene->scheduleOnce([directory](float) {
                    utils::captureScreen([](bool ok, std::string_view) {
                        if (!ok)
                            throw std::runtime_error("E4Stage5 capture failed");
                        AXLOGI("RB_PROBE_EPISODE4_STAGE5 PASS");
                        finishRoute();
                    }, std::string(directory) + "/episode1-e4stage5-clear.png");
                }, 2, "e4stage5-clear");
                return;
            }
            if (session->busy() || *age < .8f)
                return;
            auto click = [session, age, &data](int id, int cursor = 0, int item = 0) {
                auto* room = session->room();
                auto p     = room->background()->convertToWorldSpace(room->objectPosition(id));
                if (room->hitObject(p) != id || p.y < 0 || p.y > 320)
                    for (auto* area : data.where("AreaData", "objectID", id))
                    {
                        const int w = rb::integer(*area, "AreaWidth") / 2, h = rb::integer(*area, "AreaHeight") / 2;
                        if (w < 1 || h < 1)
                            continue;
                        auto q = room->background()->convertToWorldSpace(
                            {rb::integer(*area, "AreaX") / 2 + w / 2.f,
                             room->background()->getContentSize().height - rb::integer(*area, "AreaY") / 2 - h / 2.f});
                        if (q.y >= 0 && q.y <= 320 && room->hitObject(q) == id)
                        {
                            p = q;
                            break;
                        }
                    }
                if (p.x < 1 || p.x > 479)
                {
                    room->beginDrag();
                    room->drag((240 - p.x) / .8f);
                    room->drag(0);
                    room->settle();
                    *age = 0;
                    return false;
                }
                if (room->hitObject(p) != id)
                    throw std::runtime_error("E4Stage5 hit mismatch: " + std::to_string(id));
                session->selectCursor(cursor, item);
                session->tap(p);
                *age = 0;
                if (session->touchObject() != id)
                    throw std::runtime_error("E4Stage5 tap rejected");
                return true;
            };
            auto itemPoint = [session](int id) {
                auto ids = session->state().inventory();
                auto it  = std::find(ids.begin(), ids.end(), id);
                if (it == ids.end())
                    throw std::runtime_error("E4Stage5 item absent: " + std::to_string(id));
                int i = static_cast<int>(it - ids.begin());
                return Vec2(48 + (i % 5) * 65, 208 - (i / 5) * 55);
            };
            auto combine = [session, scene, age, itemPoint](int a, int b) {
                auto* inv = session->inventory();
                auto p = itemPoint(a), q = itemPoint(b);
                inv->beginTouch(p);
                scene->scheduleOnce([inv, q](float) {
                    inv->moveTouch(q);
                    inv->endTouch(q);
                }, .8f, "e4stage5-combine");
                *age = -2.5f;
            };
            bool next = false;
            switch (*step)
            {
            case 0:
                next = click(684, 1);
                break;
            case 1:
                next = click(683, 2);
                break;
            case 2:
                next = click(673);
                break;
            case 3:
                next = click(673);
                break;
            case 4:
                next = click(673);
                break;
            case 5:
                next = click(674);
                break;
            case 6:
                next = click(674);
                break;
            case 7:
                next = click(674);
                break;
            case 8:
                next = click(675);
                break;
            case 9:
                next = click(675);
                break;
            case 10:
                next = click(675);
                break;
            case 11:
                next = click(676);
                break;
            case 12:
                next = click(676);
                break;
            case 13:
                next = click(676);
                break;
            case 14:
                next = click(668);
                break;
            case 15:
                next = click(669);
                break;
            case 16:
                next = click(670);
                break;
            case 17:
                next = click(671);
                break;
            case 18:
                next = click(679, 2);
                break;
            case 19:
                next = click(686, 3, 154);
                break;
            case 20:
                next = click(687, 3, 155);
                break;
            case 21:
                next = click(688, 3, 156);
                break;
            case 22:
                next = click(689, 3, 157);
                break;
            case 23:
                next = click(710);
                break;
            case 24:
                next = click(694);
                break;
            case 25:
                next = click(695);
                break;
            case 26:
                next = click(696);
                break;
            case 27:
                next = click(694);
                break;
            case 28:
                next = click(696);
                break;
            case 29:
                next = click(694);
                break;
            case 30:
                next = click(696);
                break;
            case 31:
                next = click(695);
                break;
            case 32:
                next = click(696);
                break;
            case 33:
                next = click(697);
                break;
            case 34:
                next = click(697);
                break;
            case 35:
                next = click(696);
                break;
            case 36:
                next = click(695);
                break;
            case 37:
                next = click(697);
                break;
            case 38:
                next = click(710);
                break;
            case 39:
                next = click(702);
                break;
            case 40:
                next = click(703);
                break;
            case 41:
                next = click(704);
                break;
            case 42:
                next = click(705);
                break;
            case 43:
                next = click(706);
                break;
            case 44:
                next = click(707);
                break;
            case 45:
                next = click(708);
                break;
            case 46:
                next = click(709);
                break;
            case 47:
                session->inventory()->open();
                session->inventory()->selectMode(3);
                *age = 0;
                next = true;
                break;
            case 48:
                combine(158, 159);
                next = true;
                break;
            case 49:
                combine(160, 161);
                next = true;
                break;
            case 50:
                combine(162, 163);
                next = true;
                break;
            case 51:
                combine(165, 166);
                next = true;
                break;
            case 52:
                combine(167, 168);
                next = true;
                break;
            case 53:
                combine(169, 170);
                next = true;
                break;
            case 54:
                session->inventory()->close();
                *age = 0;
                next = true;
                break;
            case 55:
                next = click(685, 2);
                break;
            case 56:
                next = click(770, 3, 171);
                break;
            case 57:
                next = click(771, 3, 164);
                break;
            case 58:
                next = click(779);
                break;
            case 59:
                next = click(794);
                break;
            case 60:
                next = click(779);
                break;
            case 61:
                next = click(794);
                break;
            case 62:
                next = click(779);
                break;
            case 63:
                next = click(794);
                break;
            case 64:
                next = click(779);
                break;
            case 65:
                next = click(794);
                break;
            case 66:
                next = click(779);
                break;
            case 67:
                next = click(794);
                break;
            case 68:
                next = click(779);
                break;
            case 69:
                next = click(794);
                break;
            case 70:
                next = click(773, 2);
                break;
            default:
                throw std::runtime_error("E4Stage5 exit failed");
            }
            if (next)
            {
                AXLOGI("E4Stage5 step {}", *step);
                ++*step;
            }
        }, .02f, "episode1-e4stage5");
        scene->scheduleOnce([](float) { throw std::runtime_error("E4Stage5 timeout"); },
                            std::getenv("RB_ROUTE_DEMO") ? 3600 : 900, "e4stage5-timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_EPISODE5_STAGE2"))
    {
        auto* owner    = std::getenv("RB_ROUTE_DEMO") ? dynamic_cast<MainScene*>(scene) : nullptr;
        routeDemoOwner = owner;
        auto progress  = owner ? owner->routeDemoProgress() : std::make_shared<rb::Progress>();
        if (!owner)
            progress->initialize(data);
        auto* session = owner ? owner->routeDemoSession() : GameSession::create(data, 5, 2, language, 1, false);
        if (!owner)
        {
            scene->addChild(session);
            session->bindProgress(*progress);
            session->begin();
        }
        auto step   = std::make_shared<int>(0);
        auto age    = std::make_shared<float>(0);
        auto timing = std::make_shared<bool>(false);
        scene->schedule([scene, session, progress, step, age, timing, directory, &data](float dt) {
            *age += dt;
            if (session->story())
            {
                advanceRouteStory(session);
                return;
            }
            if (session->itemDialog())
            {
                if (session->itemDialog()->ready())
                    session->itemDialog()->close();
                return;
            }
            if (auto* d = dynamic_cast<ActionHit*>(session->actionMode()))
            {
                if (d->busy())
                    return;
                if (d->menuVisible())
                    throw std::runtime_error("E5Stage2 hit failed");
                if (int target = d->playableTarget())
                {
                    auto p = d->targetCenter(target);
                    d->beginTouch(p);
                    d->endTouch(p);
                }
                return;
            }
            if (auto* d = session->detail())
            {
                if (d->busy() || *age < .7f)
                    return;
                if (d->id() == 41 || d->id() == 63 || d->id() == 64)
                {
                    d->close();
                    return;
                }
                if (d->id() == 42)
                {
                    for (int i : {1, 8, 1, 4, 1, 8, 1, 4})
                    {
                        auto p = d->controlCenter(i);
                        d->beginTouch(p);
                        d->endTouch(p);
                    }
                    return;
                }
                if (auto* safe = dynamic_cast<DetailSafe*>(d))
                {
                    const std::vector<int> password = {4, 11, 4, 11};
                    for (int col = 0; col < 4; ++col)
                        if (safe->entered()[col] != password[col])
                        {
                            auto p = safe->controlCenter(col);
                            safe->beginTouch(p);
                            safe->moveTouch(p + Vec2(0, 50));
                            safe->endTouch(p + Vec2(0, 50));
                            *age = 0;
                            return;
                        }
                    return;
                }
                throw std::runtime_error("E5Stage2 unexpected detail");
            }
            if (session->clearOverlay())
            {
                if (*step != 41)
                    throw std::runtime_error("E5Stage2 premature clear");
                scene->unschedule("episode1-e5stage2");
                scene->scheduleOnce([directory](float) {
                    utils::captureScreen([](bool ok, std::string_view) {
                        if (!ok)
                            throw std::runtime_error("E5Stage2 capture failed");
                        AXLOGI("RB_PROBE_EPISODE5_STAGE2 PASS");
                        finishRoute();
                    }, std::string(directory) + "/episode1-e5stage2-clear.png");
                }, 2, "e5stage2-clear");
                return;
            }
            if (session->busy() || *age < .8f)
                return;
            auto click = [session, age, &data](int id, int cursor = 0, int item = 0) {
                auto* room = session->room();
                auto p     = room->background()->convertToWorldSpace(room->objectPosition(id));
                if (room->hitObject(p) != id || p.y < 0 || p.y > 320)
                    for (auto* area : data.where("AreaData", "objectID", id))
                    {
                        const int w = rb::integer(*area, "AreaWidth") / 2, h = rb::integer(*area, "AreaHeight") / 2;
                        if (w < 1 || h < 1)
                            continue;
                        auto q = room->background()->convertToWorldSpace(
                            {rb::integer(*area, "AreaX") / 2 + w / 2.f,
                             room->background()->getContentSize().height - rb::integer(*area, "AreaY") / 2 - h / 2.f});
                        if (q.y >= 0 && q.y <= 320 && room->hitObject(q) == id)
                        {
                            p = q;
                            break;
                        }
                    }
                if (p.x < 1 || p.x > 479)
                {
                    room->beginDrag();
                    room->drag((240 - p.x) / .8f);
                    room->drag(0);
                    room->settle();
                    *age = 0;
                    return false;
                }
                if (room->hitObject(p) != id)
                {
                    for (auto* area : data.where("AreaData", "objectID", id))
                        for (int y = 1; y < rb::integer(*area, "AreaHeight") / 2; y += 2)
                            for (int x = 1; x < rb::integer(*area, "AreaWidth") / 2; x += 2)
                            {
                                auto q = room->background()->convertToWorldSpace(
                                    {rb::integer(*area, "AreaX") / 2.f + x,
                                     room->background()->getContentSize().height - rb::integer(*area, "AreaY") / 2.f -
                                         y});
                                if (q.x > 0 && q.x < 480 && q.y > 0 && q.y < 320 && room->hitObject(q) == id)
                                    p = q;
                            }
                }
                if (room->hitObject(p) != id)
                    throw std::runtime_error("E5Stage2 hit mismatch: " + std::to_string(id) + " got " +
                                             std::to_string(room->hitObject(p)));
                session->selectCursor(cursor, item);
                session->tap(p);
                *age = 0;
                if (session->touchObject() != id)
                    throw std::runtime_error("E5Stage2 tap rejected");
                return true;
            };
            auto itemPoint = [session](int id) {
                auto ids = session->state().inventory();
                auto it  = std::find(ids.begin(), ids.end(), id);
                if (it == ids.end())
                    throw std::runtime_error("E5Stage2 item absent: " + std::to_string(id));
                int i = static_cast<int>(it - ids.begin());
                return Vec2(48 + (i % 5) * 65, 208 - (i / 5) * 55);
            };
            auto combine = [session, scene, age, itemPoint](int a, int b) {
                auto* inv = session->inventory();
                auto p = itemPoint(a), q = itemPoint(b);
                inv->beginTouch(p);
                scene->scheduleOnce([inv, q](float) {
                    inv->moveTouch(q);
                    inv->endTouch(q);
                }, .8f, "e5stage2-combine");
                *age = -2.5f;
            };
            auto split = [session, scene, age, itemPoint](int id) {
                auto* inv = session->inventory();
                auto p    = itemPoint(id);
                inv->beginTouch(p);
                scene->scheduleOnce([inv, p](float) {
                    inv->moveTouch(p + Vec2(20, 0));
                    inv->moveTouch(p + Vec2(40, 0));
                    inv->endTouch(p + Vec2(40, 0));
                }, .8f, "e5stage2-split");
                *age = -1;
            };
            bool next = false;
            switch (*step)
            {
            case 0:
                next = click(594);
                break;
            case 1:
                next = click(589);
                break;
            case 2:
                next = click(594, 3, 131);
                break;
            case 3:
                next = click(594, 1);
                break;
            case 4:
                next = click(591);
                break;
            case 5:
                next = click(626, 3, 132);
                break;
            case 6:
                next = click(605, 1);
                break;
            case 7:
                next = click(590);
                break;
            case 8:
                next = click(592);
                break;
            case 9:
                next = click(597);
                break;
            case 10:
                next = click(623);
                break;
            case 11:
                next = click(593);
                break;
            case 12:
                next = click(624);
                break;
            case 13:
                next = click(600);
                break;
            case 14:
                next = click(607, 1);
                break;
            case 15:
                next = click(607, 3, 133);
                break;
            case 16:
                next = click(607, 1);
                break;
            case 17:
                session->inventory()->open();
                session->inventory()->selectMode(2);
                *age = 0;
                next = true;
                break;
            case 18:
                split(134);
                next = true;
                break;
            case 19:
                split(135);
                next = true;
                break;
            case 20:
                split(136);
                next = true;
                break;
            case 21:
                split(137);
                next = true;
                break;
            case 22:
                session->inventory()->close();
                *age = 0;
                next = true;
                break;
            case 23:
                next = click(609, 3, 143);
                break;
            case 24:
                next = click(609, 3, 138);
                break;
            case 25:
                next = click(610, 3, 143);
                break;
            case 26:
                next = click(610, 3, 139);
                break;
            case 27:
                next = click(611, 3, 143);
                break;
            case 28:
                next = click(611, 3, 141);
                break;
            case 29:
                next = click(612, 3, 143);
                break;
            case 30:
                next = click(612, 3, 140);
                break;
            case 31:
                next = click(614);
                break;
            case 32:
                next = click(615);
                break;
            case 33:
                next = click(613);
                break;
            case 34:
                next = click(616);
                break;
            case 35:
                next = click(607, 3, 142);
                break;
            case 36:
                next = click(607);
                break;
            case 37:
                next = click(621);
                break;
            case 38:
                next = click(603, 3, 145);
                break;
            case 39:
                next = click(604, 3, 144);
                break;
            case 40:
                next = click(602, 2);
                break;
            default:
                throw std::runtime_error("E5Stage2 exit failed");
            }
            if (next)
            {
                AXLOGI("E5Stage2 step {}", *step);
                ++*step;
            }
        }, .02f, "episode1-e5stage2");
        scene->scheduleOnce([](float) { throw std::runtime_error("E5Stage2 timeout"); },
                            std::getenv("RB_ROUTE_DEMO") ? 3600 : 600, "e5stage2-timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_EPISODE5_STAGE3"))
    {
        auto* owner    = std::getenv("RB_ROUTE_DEMO") ? dynamic_cast<MainScene*>(scene) : nullptr;
        routeDemoOwner = owner;
        auto progress  = owner ? owner->routeDemoProgress() : std::make_shared<rb::Progress>();
        if (!owner)
            progress->initialize(data);
        auto* session = owner ? owner->routeDemoSession() : GameSession::create(data, 5, 3, language, 1, false);
        if (!owner)
        {
            scene->addChild(session);
            session->bindProgress(*progress);
            session->begin();
        }
        auto step   = std::make_shared<int>(0);
        auto age    = std::make_shared<float>(0);
        auto timing = std::make_shared<bool>(false);
        scene->schedule([scene, session, progress, step, age, timing, directory, &data](float dt) {
            *age += dt;
            if (session->story())
            {
                advanceRouteStory(session);
                return;
            }
            if (session->itemDialog())
            {
                if (session->itemDialog()->ready())
                    session->itemDialog()->close();
                return;
            }
            if (auto* d = dynamic_cast<ActionHit*>(session->actionMode()))
            {
                if (d->busy())
                    return;
                if (d->menuVisible())
                    throw std::runtime_error("E5Stage3 hit failed");
                if (int target = d->playableTarget())
                {
                    auto p = d->targetCenter(target);
                    d->beginTouch(p);
                    d->endTouch(p);
                }
                return;
            }
            if (auto* cut = dynamic_cast<ActionCut*>(session->actionMode()))
            {
                if (cut->busy())
                    return;
                if (cut->menuVisible())
                    throw std::runtime_error("E5Stage3 cut failed");
                const Vec2 vectors[] = {{-1, 0}, {-1, 1}, {0, 1}, {1, 1}, {1, 0}, {1, -1}, {0, -1}, {-1, -1}};
                auto delta           = vectors[cut->direction()].getNormalized() * 160;
                auto p               = Vec2(240, 160) - delta / 2;
                cut->beginTouch(p);
                cut->moveTouch(p + delta);
                cut->endTouch(p + delta);
                return;
            }
            if (auto* d = session->detail())
            {
                if (d->busy() || *age < .7f)
                    return;
                if (d->id() == 44 || d->id() == 45 || d->id() == 46)
                {
                    d->close();
                    return;
                }
                if (d->id() == 47)
                {
                    for (int i : {7, 9, 7, 9})
                    {
                        auto p = d->controlCenter(i);
                        d->beginTouch(p);
                        d->endTouch(p);
                    }
                    return;
                }
                if (d->id() == 49)
                {
                    for (int i : {4, 3, 2, 0, 1, 3, 0, 2, 0})
                    {
                        auto p = d->controlCenter(i);
                        d->beginTouch(p);
                        d->endTouch(p);
                    }
                    return;
                }
                if (auto* center = dynamic_cast<DetailCenterSafe*>(d))
                {
                    const int target[] = {1, 2, 2, 3, 2, 1, 2};
                    for (int c = 0; c < 7; ++c)
                        if (center->entered()[c] != target[c])
                        {
                            auto p = d->controlCenter(c * 3 + center->entered()[c]);
                            d->beginTouch(p);
                            d->moveTouch(p + Vec2(0, 30));
                            d->endTouch(p + Vec2(0, 30));
                            *age = 0;
                            return;
                        }
                    return;
                }
                throw std::runtime_error("E5Stage3 unexpected detail");
            }
            if (session->clearOverlay())
            {
                if (*step != 22)
                    throw std::runtime_error("E5Stage3 premature clear");
                scene->unschedule("episode1-e5stage3");
                scene->scheduleOnce([directory](float) {
                    utils::captureScreen([](bool ok, std::string_view) {
                        if (!ok)
                            throw std::runtime_error("E5Stage3 capture failed");
                        AXLOGI("RB_PROBE_EPISODE5_STAGE3 PASS");
                        finishRoute();
                    }, std::string(directory) + "/episode1-e5stage3-clear.png");
                }, 2, "e5stage3-clear");
                return;
            }
            if (session->busy() || *age < .8f)
                return;
            auto click = [session, age, &data](int id, int cursor = 0, int item = 0) {
                auto* room = session->room();
                auto p     = room->background()->convertToWorldSpace(room->objectPosition(id));
                if (room->hitObject(p) != id || p.y < 0 || p.y > 320)
                    for (auto* area : data.where("AreaData", "objectID", id))
                    {
                        const int w = rb::integer(*area, "AreaWidth") / 2, h = rb::integer(*area, "AreaHeight") / 2;
                        if (w < 1 || h < 1)
                            continue;
                        auto q = room->background()->convertToWorldSpace(
                            {rb::integer(*area, "AreaX") / 2 + w / 2.f,
                             room->background()->getContentSize().height - rb::integer(*area, "AreaY") / 2 - h / 2.f});
                        if (q.y >= 0 && q.y <= 320 && room->hitObject(q) == id)
                        {
                            p = q;
                            break;
                        }
                    }
                if (p.x < 1 || p.x > 479)
                {
                    room->beginDrag();
                    room->drag((240 - p.x) / .8f);
                    room->drag(0);
                    room->settle();
                    *age = 0;
                    return false;
                }
                if (room->hitObject(p) != id)
                    throw std::runtime_error("E5Stage3 hit mismatch: " + std::to_string(id));
                session->selectCursor(cursor, item);
                session->tap(p);
                *age = 0;
                if (session->touchObject() != id)
                    throw std::runtime_error("E5Stage3 tap rejected");
                return true;
            };
            auto itemPoint = [session](int id) {
                auto ids = session->state().inventory();
                auto it  = std::find(ids.begin(), ids.end(), id);
                if (it == ids.end())
                    throw std::runtime_error("E5Stage3 item absent: " + std::to_string(id));
                int i = static_cast<int>(it - ids.begin());
                return Vec2(48 + (i % 5) * 65, 208 - (i / 5) * 55);
            };
            auto combine = [session, scene, age, itemPoint](int a, int b) {
                auto* inv = session->inventory();
                auto p = itemPoint(a), q = itemPoint(b);
                inv->beginTouch(p);
                scene->scheduleOnce([inv, q](float) {
                    inv->moveTouch(q);
                    inv->endTouch(q);
                }, .8f, "e5stage3-combine");
                *age = -2.5f;
            };
            auto split = [session, scene, age, itemPoint](int id) {
                auto* inv = session->inventory();
                auto p    = itemPoint(id);
                inv->beginTouch(p);
                scene->scheduleOnce([inv, p](float) {
                    inv->moveTouch(p + Vec2(20, 0));
                    inv->moveTouch(p + Vec2(40, 0));
                    inv->endTouch(p + Vec2(40, 0));
                }, .8f, "e5stage3-split");
                *age = -1;
            };
            bool next = false;
            switch (*step)
            {
            case 0:
                next = click(627, 1);
                break;
            case 1:
                next = click(633);
                break;
            case 2:
                next = click(637);
                break;
            case 3:
                next = click(637, 3, 153);
                break;
            case 4:
                next = click(635);
                break;
            case 5:
                next = click(630);
                break;
            case 6:
                next = click(653);
                break;
            case 7:
                next = click(654);
                break;
            case 8:
                next = click(654);
                break;
            case 9:
                next = click(651);
                break;
            case 10:
                next = click(651, 1);
                break;
            case 11:
                next = click(636);
                break;
            case 12:
                next = click(655);
                break;
            case 13:
                session->inventory()->open();
                session->inventory()->selectMode(2);
                *age = 0;
                next = true;
                break;
            case 14:
                split(147);
                next = true;
                break;
            case 15:
                session->inventory()->close();
                *age = 0;
                next = true;
                break;
            case 16:
                next = click(640, 3, 149);
                break;
            case 17:
                next = click(634);
                break;
            case 18:
                next = click(627, 3, 151);
                break;
            case 19:
                next = click(627, 1);
                break;
            case 20:
                next = click(628);
                break;
            case 21:
                next = click(629, 2);
                break;
            default:
                throw std::runtime_error("E5Stage3 exit failed");
            }
            if (next)
            {
                AXLOGI("E5Stage3 step {}", *step);
                ++*step;
            }
        }, .02f, "episode1-e5stage3");
        scene->scheduleOnce([](float) { throw std::runtime_error("E5Stage3 timeout"); },
                            std::getenv("RB_ROUTE_DEMO") ? 3600 : 600, "e5stage3-timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_EPISODE5_STAGE5"))
    {
        auto* owner    = std::getenv("RB_ROUTE_DEMO") ? dynamic_cast<MainScene*>(scene) : nullptr;
        routeDemoOwner = owner;
        auto progress  = owner ? owner->routeDemoProgress() : std::make_shared<rb::Progress>();
        if (!owner)
            progress->initialize(data);
        auto* session = owner ? owner->routeDemoSession() : GameSession::create(data, 5, 5, language, 1, false);
        if (!owner)
        {
            scene->addChild(session);
            session->bindProgress(*progress);
            session->begin();
        }
        auto step   = std::make_shared<int>(0);
        auto age    = std::make_shared<float>(0);
        auto timing = std::make_shared<bool>(false);
        scene->schedule([scene, session, progress, step, age, timing, directory, &data](float dt) {
            *age += dt;
            if (session->story())
            {
                advanceRouteStory(session);
                return;
            }
            if (session->itemDialog())
            {
                if (session->itemDialog()->ready())
                    session->itemDialog()->close();
                return;
            }
            if (auto* d = dynamic_cast<ActionHit*>(session->actionMode()))
            {
                if (d->busy())
                    return;
                if (d->menuVisible())
                    throw std::runtime_error("E5Stage5 hit failed");
                if (int target = d->playableTarget())
                {
                    auto p = d->targetCenter(target);
                    d->beginTouch(p);
                    d->endTouch(p);
                }
                return;
            }
            if (auto* d = session->detail())
            {
                if (d->busy() || *age < .65f)
                    return;
                if (d->id() == 72)
                {
                    for (int i : {2, 8})
                    {
                        auto p = d->controlCenter(i);
                        d->beginTouch(p);
                        d->endTouch(p);
                    }
                    return;
                }
                if (auto* pad = dynamic_cast<DetailAlienPad*>(d))
                {
                    const std::string word = "push";
                    if (pad->entered().size() >= word.size())
                        return;
                    auto p    = pad->controlCenter(26);
                    int delta = (word.at(pad->entered().size()) - 'a' - pad->selected() + 26) % 26;
                    if (delta > 13)
                        delta -= 26;
                    pad->beginTouch(p);
                    if (delta)
                    {
                        delta = std::clamp(delta, -3, 3);
                        p += Vec2(-45.5f * delta, 0);
                        pad->moveTouch(p);
                    }
                    pad->endTouch(p);
                    *age = 0;
                    return;
                }
                throw std::runtime_error("E5Stage5 unexpected detail");
            }
            if (session->clearOverlay())
            {
                if (*step != 16)
                    throw std::runtime_error("E5Stage5 premature clear");
                scene->unschedule("episode1-e5stage5");
                scene->scheduleOnce([directory](float) {
                    utils::captureScreen([](bool ok, std::string_view) {
                        if (!ok)
                            throw std::runtime_error("E5Stage5 capture failed");
                        AXLOGI("RB_PROBE_EPISODE5_STAGE5 PASS");
                        finishRoute();
                    }, std::string(directory) + "/episode1-e5stage5-clear.png");
                }, 2, "e5stage5-clear");
                return;
            }
            if (session->busy() || *age < .8f)
                return;
            auto click = [session, age, &data](int id, int cursor = 0, int item = 0) {
                auto* room = session->room();
                auto p     = room->background()->convertToWorldSpace(room->objectPosition(id));
                if (room->hitObject(p) != id || p.y < 0 || p.y > 320)
                    for (auto* area : data.where("AreaData", "objectID", id))
                    {
                        const int w = rb::integer(*area, "AreaWidth") / 2, h = rb::integer(*area, "AreaHeight") / 2;
                        if (w < 1 || h < 1)
                            continue;
                        auto q = room->background()->convertToWorldSpace(
                            {rb::integer(*area, "AreaX") / 2 + w / 2.f,
                             room->background()->getContentSize().height - rb::integer(*area, "AreaY") / 2 - h / 2.f});
                        if (q.y >= 0 && q.y <= 320 && room->hitObject(q) == id)
                        {
                            p = q;
                            break;
                        }
                    }
                if (p.x < 1 || p.x > 479)
                {
                    room->beginDrag();
                    room->drag((240 - p.x) / .8f);
                    room->drag(0);
                    room->settle();
                    *age = 0;
                    return false;
                }
                if (room->hitObject(p) != id)
                    throw std::runtime_error("E5Stage5 hit mismatch: " + std::to_string(id));
                session->selectCursor(cursor, item);
                session->tap(p);
                *age = 0;
                if (session->touchObject() != id)
                    throw std::runtime_error("E5Stage5 tap rejected");
                return true;
            };
            auto itemPoint = [session](int id) {
                auto ids = session->state().inventory();
                auto it  = std::find(ids.begin(), ids.end(), id);
                if (it == ids.end())
                    throw std::runtime_error("E5Stage5 item absent: " + std::to_string(id));
                int i = static_cast<int>(it - ids.begin());
                return Vec2(48 + (i % 5) * 65, 208 - (i / 5) * 55);
            };
            auto combine = [session, scene, age, itemPoint](int a, int b) {
                auto* inv = session->inventory();
                auto p = itemPoint(a), q = itemPoint(b);
                inv->beginTouch(p);
                scene->scheduleOnce([inv, q](float) {
                    inv->moveTouch(q);
                    inv->endTouch(q);
                }, .8f, "e5stage5-combine");
                *age = -2.5f;
            };
            auto split = [session, scene, age, itemPoint](int id) {
                auto* inv = session->inventory();
                auto p    = itemPoint(id);
                inv->beginTouch(p);
                scene->scheduleOnce([inv, p](float) {
                    inv->moveTouch(p + Vec2(20, 0));
                    inv->moveTouch(p + Vec2(40, 0));
                    inv->endTouch(p + Vec2(40, 0));
                }, .8f, "e5stage5-split");
                *age = -1;
            };
            bool next = false;
            switch (*step)
            {
            case 0:
                next = click(1020);
                break;
            case 1:
                next = click(1025);
                break;
            case 2:
                next = click(1028, 2);
                break;
            case 3:
                next = click(1029, 2);
                break;
            case 4:
                next = click(1030, 2);
                break;
            case 5:
                next = click(1031, 2);
                break;
            case 6:
                next = click(1032, 2);
                break;
            case 7:
                next = click(1035, 2);
                break;
            case 8:
                next = click(1040, 2);
                break;
            case 9:
                next = click(1043, 2);
                break;
            case 10:
                next = click(1044, 2);
                break;
            case 11:
                next = click(1045, 2);
                break;
            case 12:
                next = click(1046, 2);
                break;
            case 13:
                next = click(1047, 2);
                break;
            case 14:
                next = click(1341);
                break;
            case 15:
                next = click(1048, 2);
                break;
            default:
                throw std::runtime_error("E5Stage5 exit failed");
            }
            if (next)
            {
                AXLOGI("E5Stage5 step {}", *step);
                ++*step;
            }
        }, .02f, "episode1-e5stage5");
        scene->scheduleOnce([](float) { throw std::runtime_error("E5Stage5 timeout"); },
                            std::getenv("RB_ROUTE_DEMO") ? 3600 : 600, "e5stage5-timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_EPISODE6_STAGE2"))
    {
        auto* owner    = std::getenv("RB_ROUTE_DEMO") ? dynamic_cast<MainScene*>(scene) : nullptr;
        routeDemoOwner = owner;
        auto progress  = owner ? owner->routeDemoProgress() : std::make_shared<rb::Progress>();
        if (!owner)
            progress->initialize(data);
        auto* session = owner ? owner->routeDemoSession() : GameSession::create(data, 6, 2, language, 1, false);
        if (!owner)
        {
            scene->addChild(session);
            session->bindProgress(*progress);
            session->begin();
        }
        auto step   = std::make_shared<int>(0);
        auto age    = std::make_shared<float>(0);
        auto timing = std::make_shared<bool>(false);
        scene->schedule([scene, session, progress, step, age, timing, directory, &data](float dt) {
            *age += dt;
            if (session->story())
            {
                advanceRouteStory(session);
                return;
            }
            if (session->itemDialog())
            {
                if (session->itemDialog()->ready())
                    session->itemDialog()->close();
                return;
            }
            if (auto* d = dynamic_cast<ActionHit*>(session->actionMode()))
            {
                if (d->busy())
                    return;
                if (d->menuVisible())
                    throw std::runtime_error("E6Stage2 hit failed");
                if (int target = d->playableTarget())
                {
                    auto p = d->targetCenter(target);
                    d->beginTouch(p);
                    d->endTouch(p);
                }
                return;
            }
            if (auto* d = dynamic_cast<DetailFittedPieces*>(session->detail()))
            {
                if (d->busy() || *age < .65f)
                    return;
                for (int i = 0; i < 9; ++i)
                    if (!d->fitted(i))
                    {
                        auto p = d->controlCenter(i), q = d->targetCenter(i);
                        d->beginTouch(p);
                        d->moveTouch(q);
                        d->endTouch(q);
                        *age = 0;
                        return;
                    }
                return;
            }
            if (session->clearOverlay())
            {
                if (*step != 29)
                    throw std::runtime_error("E6Stage2 premature clear");
                scene->unschedule("episode1-e6stage2");
                scene->scheduleOnce([directory](float) {
                    utils::captureScreen([](bool ok, std::string_view) {
                        if (!ok)
                            throw std::runtime_error("E6Stage2 capture failed");
                        AXLOGI("RB_PROBE_EPISODE6_STAGE2 PASS");
                        finishRoute();
                    }, std::string(directory) + "/episode1-e6stage2-clear.png");
                }, 2, "e6stage2-clear");
                return;
            }
            if (session->busy() || *age < .8f)
                return;
            auto click = [session, age, &data](int id, int cursor = 0, int item = 0) {
                auto* room = session->room();
                auto p     = room->background()->convertToWorldSpace(room->objectPosition(id));
                if (room->hitObject(p) != id || p.y < 0 || p.y > 320)
                    for (auto* area : data.where("AreaData", "objectID", id))
                    {
                        const int w = rb::integer(*area, "AreaWidth") / 2, h = rb::integer(*area, "AreaHeight") / 2;
                        if (w < 1 || h < 1)
                            continue;
                        auto q = room->background()->convertToWorldSpace(
                            {rb::integer(*area, "AreaX") / 2 + w / 2.f,
                             room->background()->getContentSize().height - rb::integer(*area, "AreaY") / 2 - h / 2.f});
                        if (q.y >= 0 && q.y <= 320 && room->hitObject(q) == id)
                        {
                            p = q;
                            break;
                        }
                    }
                if (p.x < 1 || p.x > 479)
                {
                    room->beginDrag();
                    room->drag((240 - p.x) / .8f);
                    room->drag(0);
                    room->settle();
                    *age = 0;
                    return false;
                }
                if (room->hitObject(p) != id)
                    throw std::runtime_error("E6Stage2 hit mismatch: " + std::to_string(id));
                session->selectCursor(cursor, item);
                session->tap(p);
                *age = 0;
                if (session->touchObject() != id)
                    throw std::runtime_error("E6Stage2 tap rejected");
                return true;
            };
            auto itemPoint = [session](int id) {
                auto ids = session->state().inventory();
                auto it  = std::find(ids.begin(), ids.end(), id);
                if (it == ids.end())
                    throw std::runtime_error("E6Stage2 item absent: " + std::to_string(id));
                int i = static_cast<int>(it - ids.begin());
                return Vec2(48 + (i % 5) * 65, 208 - (i / 5) * 55);
            };
            auto combine = [session, scene, age, itemPoint](int a, int b) {
                auto* inv = session->inventory();
                auto p = itemPoint(a), q = itemPoint(b);
                inv->beginTouch(p);
                scene->scheduleOnce([inv, q](float) {
                    inv->moveTouch(q);
                    inv->endTouch(q);
                }, .8f, "e6stage2-combine");
                *age = -2.5f;
            };
            auto split = [session, scene, age, itemPoint](int id) {
                auto* inv = session->inventory();
                auto p    = itemPoint(id);
                inv->beginTouch(p);
                scene->scheduleOnce([inv, p](float) {
                    inv->moveTouch(p + Vec2(20, 0));
                    inv->moveTouch(p + Vec2(40, 0));
                    inv->endTouch(p + Vec2(40, 0));
                }, .8f, "e6stage2-split");
                *age = -1;
            };
            bool next = false;
            switch (*step)
            {
            case 0:
                next = click(1388);
                break;
            case 1:
                next = click(1532);
                break;
            case 2:
                next = click(1399);
                break;
            case 3:
                next = click(1389);
                break;
            case 4:
                next = click(1390);
                break;
            case 5:
                next = click(1392);
                break;
            case 6:
                next = click(1393);
                break;
            case 7:
                next = click(1396);
                break;
            case 8:
                next = click(1400);
                break;
            case 9:
                next = click(1397);
                break;
            case 10:
                next = click(1401);
                break;
            case 11:
                next = click(1394);
                break;
            case 12:
                next = click(1489);
                break;
            case 13:
                next = click(1395);
                break;
            case 14:
                next = click(1495);
                break;
            case 15:
                next = click(1497, 3, 329);
                break;
            case 16:
                next = click(1391);
                break;
            case 17:
                next = click(1488);
                break;
            case 18:
                next = click(1488, 3, 320);
                break;
            case 19:
                next = click(1488, 3, 321);
                break;
            case 20:
                next = click(1488, 3, 322);
                break;
            case 21:
                next = click(1488, 3, 323);
                break;
            case 22:
                next = click(1488, 3, 324);
                break;
            case 23:
                next = click(1488, 3, 325);
                break;
            case 24:
                next = click(1488, 3, 326);
                break;
            case 25:
                next = click(1488, 3, 327);
                break;
            case 26:
                next = click(1488, 3, 328);
                break;
            case 27:
                next = click(1398);
                break;
            case 28:
                next = click(1404, 2);
                break;
            default:
                throw std::runtime_error("E6Stage2 exit failed");
            }
            if (next)
            {
                AXLOGI("E6Stage2 step {}", *step);
                ++*step;
            }
        }, .02f, "episode1-e6stage2");
        scene->scheduleOnce([](float) { throw std::runtime_error("E6Stage2 timeout"); },
                            std::getenv("RB_ROUTE_DEMO") ? 3600 : 600, "e6stage2-timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_EPISODE6_STAGE3"))
    {
        auto* owner    = std::getenv("RB_ROUTE_DEMO") ? dynamic_cast<MainScene*>(scene) : nullptr;
        routeDemoOwner = owner;
        auto progress  = owner ? owner->routeDemoProgress() : std::make_shared<rb::Progress>();
        if (!owner)
            progress->initialize(data);
        auto* session = owner ? owner->routeDemoSession() : GameSession::create(data, 6, 3, language, 1, false);
        if (!owner)
        {
            scene->addChild(session);
            session->bindProgress(*progress);
            session->begin();
        }
        auto step   = std::make_shared<int>(0);
        auto age    = std::make_shared<float>(0);
        auto timing = std::make_shared<bool>(false);
        scene->schedule([scene, session, progress, step, age, timing, directory, &data](float dt) {
            *age += dt;
            if (session->story())
            {
                advanceRouteStory(session);
                return;
            }
            if (session->itemDialog())
            {
                if (session->itemDialog()->ready())
                    session->itemDialog()->close();
                return;
            }
            if (auto* d = dynamic_cast<ActionHit*>(session->actionMode()))
            {
                if (d->busy())
                    return;
                if (d->menuVisible())
                    throw std::runtime_error("E6Stage3 hit failed");
                if (int target = d->playableTarget())
                {
                    auto p = d->targetCenter(target);
                    d->beginTouch(p);
                    d->endTouch(p);
                }
                return;
            }
            if (auto* d = session->detail())
            {
                if (d->busy() || *age < .7f)
                    return;
                if (d->id() == 96)
                {
                    if (*age < 3)
                        return;
                    d->close();
                    return;
                }
                if (d->id() == 95)
                {
                    for (int i : {9, 7, 6, 4})
                    {
                        auto p = d->controlCenter(i);
                        d->beginTouch(p);
                        d->endTouch(p);
                    }
                    auto p = d->controlCenter(11);
                    d->beginTouch(p);
                    d->moveTouch(p + Vec2(0, -40));
                    d->endTouch(p + Vec2(0, -40));
                    return;
                }
                throw std::runtime_error("E6Stage3 unexpected detail");
            }
            if (session->clearOverlay())
            {
                if (*step != 23)
                    throw std::runtime_error("E6Stage3 premature clear");
                scene->unschedule("episode1-e6stage3");
                scene->scheduleOnce([directory](float) {
                    utils::captureScreen([](bool ok, std::string_view) {
                        if (!ok)
                            throw std::runtime_error("E6Stage3 capture failed");
                        AXLOGI("RB_PROBE_EPISODE6_STAGE3 PASS");
                        finishRoute();
                    }, std::string(directory) + "/episode1-e6stage3-clear.png");
                }, 2, "e6stage3-clear");
                return;
            }
            if (session->busy() || *age < .8f)
                return;
            auto click = [session, age, &data](int id, int cursor = 0, int item = 0) {
                auto* room = session->room();
                auto p     = room->background()->convertToWorldSpace(room->objectPosition(id));
                if (room->hitObject(p) != id || p.y < 0 || p.y > 320)
                    for (auto* area : data.where("AreaData", "objectID", id))
                    {
                        const int w = rb::integer(*area, "AreaWidth") / 2, h = rb::integer(*area, "AreaHeight") / 2;
                        if (w < 1 || h < 1)
                            continue;
                        auto q = room->background()->convertToWorldSpace(
                            {rb::integer(*area, "AreaX") / 2 + w / 2.f,
                             room->background()->getContentSize().height - rb::integer(*area, "AreaY") / 2 - h / 2.f});
                        if (q.y >= 0 && q.y <= 320 && room->hitObject(q) == id)
                        {
                            p = q;
                            break;
                        }
                    }
                if (p.x < 1 || p.x > 479)
                {
                    room->beginDrag();
                    room->drag((240 - p.x) / .8f);
                    room->drag(0);
                    room->settle();
                    *age = 0;
                    return false;
                }
                if (room->hitObject(p) != id)
                    throw std::runtime_error("E6Stage3 hit mismatch: " + std::to_string(id));
                session->selectCursor(cursor, item);
                session->tap(p);
                *age = 0;
                if (session->touchObject() != id)
                    throw std::runtime_error("E6Stage3 tap rejected");
                return true;
            };
            auto itemPoint = [session](int id) {
                auto ids = session->state().inventory();
                auto it  = std::find(ids.begin(), ids.end(), id);
                if (it == ids.end())
                    throw std::runtime_error("E6Stage3 item absent: " + std::to_string(id));
                int i = static_cast<int>(it - ids.begin());
                return Vec2(48 + (i % 5) * 65, 208 - (i / 5) * 55);
            };
            auto combine = [session, scene, age, itemPoint](int a, int b) {
                auto* inv = session->inventory();
                auto p = itemPoint(a), q = itemPoint(b);
                inv->beginTouch(p);
                scene->scheduleOnce([inv, q](float) {
                    inv->moveTouch(q);
                    inv->endTouch(q);
                }, .8f, "e6stage3-combine");
                *age = -2.5f;
            };
            auto split = [session, scene, age, itemPoint](int id) {
                auto* inv = session->inventory();
                auto p    = itemPoint(id);
                inv->beginTouch(p);
                scene->scheduleOnce([inv, p](float) {
                    inv->moveTouch(p + Vec2(20, 0));
                    inv->moveTouch(p + Vec2(40, 0));
                    inv->endTouch(p + Vec2(40, 0));
                }, .8f, "e6stage3-split");
                *age = -1;
            };
            bool next = false;
            switch (*step)
            {
            case 0:
                next = click(1416);
                break;
            case 1:
                next = click(1417);
                break;
            case 2:
                next = click(1418);
                break;
            case 3:
                next = click(1419);
                break;
            case 4:
                next = click(1421);
                break;
            case 5:
                next = click(1499);
                break;
            case 6:
                next = click(1420);
                break;
            case 7:
                next = click(1432);
                break;
            case 8:
                next = click(1422);
                break;
            case 9:
                next = click(1500, 3, 300);
                break;
            case 10:
                next = click(1423);
                break;
            case 11:
                next = click(1431);
                break;
            case 12:
                next = click(1424, 3, 301);
                break;
            case 13:
                next = click(1425, 3, 303);
                break;
            case 14:
                next = click(1426, 3, 304);
                break;
            case 15:
                next = click(1429);
                break;
            case 16:
                next = click(1437);
                break;
            case 17:
                next = click(1438);
                break;
            case 18:
                next = click(1434, 2);
                break;
            case 19:
                next = click(1505, 3, 297);
                break;
            case 20:
                next = click(1436);
                break;
            case 21:
                next = click(1510, 2);
                break;
            case 22:
                next = click(1435, 2);
                break;
            default:
                throw std::runtime_error("E6Stage3 exit failed");
            }
            if (next)
            {
                AXLOGI("E6Stage3 step {}", *step);
                ++*step;
            }
        }, .02f, "episode1-e6stage3");
        scene->scheduleOnce([](float) { throw std::runtime_error("E6Stage3 timeout"); },
                            std::getenv("RB_ROUTE_DEMO") ? 3600 : 600, "e6stage3-timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_EPISODE6_STAGE4"))
    {
        auto* owner    = std::getenv("RB_ROUTE_DEMO") ? dynamic_cast<MainScene*>(scene) : nullptr;
        routeDemoOwner = owner;
        auto progress  = owner ? owner->routeDemoProgress() : std::make_shared<rb::Progress>();
        if (!owner)
            progress->initialize(data);
        auto* session = owner ? owner->routeDemoSession() : GameSession::create(data, 6, 4, language, 1, false);
        if (!owner)
        {
            scene->addChild(session);
            session->bindProgress(*progress);
            session->begin();
        }
        auto step   = std::make_shared<int>(0);
        auto age    = std::make_shared<float>(0);
        auto timing = std::make_shared<bool>(false);
        scene->schedule([scene, session, progress, step, age, timing, directory, &data](float dt) {
            *age += dt;
            if (session->story())
            {
                advanceRouteStory(session);
                return;
            }
            if (session->itemDialog())
            {
                if (session->itemDialog()->ready())
                    session->itemDialog()->close();
                return;
            }
            if (auto* d = dynamic_cast<ActionHit*>(session->actionMode()))
            {
                if (d->busy())
                    return;
                if (d->menuVisible())
                    throw std::runtime_error("E6Stage4 hit failed");
                if (int target = d->playableTarget())
                {
                    auto p = d->targetCenter(target);
                    d->beginTouch(p);
                    d->endTouch(p);
                }
                return;
            }
            if (session->clearOverlay())
            {
                if (*step != 18)
                    throw std::runtime_error("E6Stage4 premature clear");
                scene->unschedule("episode1-e6stage4");
                scene->scheduleOnce([directory](float) {
                    utils::captureScreen([](bool ok, std::string_view) {
                        if (!ok)
                            throw std::runtime_error("E6Stage4 capture failed");
                        AXLOGI("RB_PROBE_EPISODE6_STAGE4 PASS");
                        finishRoute();
                    }, std::string(directory) + "/episode1-e6stage4-clear.png");
                }, 2, "e6stage4-clear");
                return;
            }
            if (session->busy() || *age < .8f)
                return;
            auto click = [session, age, &data](int id, int cursor = 0, int item = 0) {
                auto* room = session->room();
                auto p     = room->background()->convertToWorldSpace(room->objectPosition(id));
                if (room->hitObject(p) != id || p.y < 0 || p.y > 320)
                    for (auto* area : data.where("AreaData", "objectID", id))
                    {
                        const int w = rb::integer(*area, "AreaWidth") / 2, h = rb::integer(*area, "AreaHeight") / 2;
                        if (w < 1 || h < 1)
                            continue;
                        auto q = room->background()->convertToWorldSpace(
                            {rb::integer(*area, "AreaX") / 2 + w / 2.f,
                             room->background()->getContentSize().height - rb::integer(*area, "AreaY") / 2 - h / 2.f});
                        if (q.y >= 0 && q.y <= 320 && room->hitObject(q) == id)
                        {
                            p = q;
                            break;
                        }
                    }
                if (p.x < 1 || p.x > 479)
                {
                    room->beginDrag();
                    room->drag((240 - p.x) / .8f);
                    room->drag(0);
                    room->settle();
                    *age = 0;
                    return false;
                }
                if (room->hitObject(p) != id)
                    throw std::runtime_error("E6Stage4 hit mismatch: " + std::to_string(id));
                session->selectCursor(cursor, item);
                session->tap(p);
                *age = 0;
                if (session->touchObject() != id)
                    throw std::runtime_error("E6Stage4 tap rejected");
                return true;
            };
            auto itemPoint = [session](int id) {
                auto ids = session->state().inventory();
                auto it  = std::find(ids.begin(), ids.end(), id);
                if (it == ids.end())
                    throw std::runtime_error("E6Stage4 item absent: " + std::to_string(id));
                int i = static_cast<int>(it - ids.begin());
                return Vec2(48 + (i % 5) * 65, 208 - (i / 5) * 55);
            };
            auto combine = [session, scene, age, itemPoint](int a, int b) {
                auto* inv = session->inventory();
                auto p = itemPoint(a), q = itemPoint(b);
                inv->beginTouch(p);
                scene->scheduleOnce([inv, q](float) {
                    inv->moveTouch(q);
                    inv->endTouch(q);
                }, .8f, "e6stage4-combine");
                *age = -2.5f;
            };
            auto split = [session, scene, age, itemPoint](int id) {
                auto* inv = session->inventory();
                auto p    = itemPoint(id);
                inv->beginTouch(p);
                scene->scheduleOnce([inv, p](float) {
                    inv->moveTouch(p + Vec2(20, 0));
                    inv->moveTouch(p + Vec2(40, 0));
                    inv->endTouch(p + Vec2(40, 0));
                }, .8f, "e6stage4-split");
                *age = -1;
            };
            bool next = false;
            switch (*step)
            {
            case 0:
                next = click(1439);
                break;
            case 1:
                next = click(1440);
                break;
            case 2:
                next = click(1443);
                break;
            case 3:
                next = click(1520);
                break;
            case 4:
                next = click(1514);
                break;
            case 5:
                next = click(1442);
                break;
            case 6:
                next = click(1516, 3, 330);
                break;
            case 7:
                next = click(1512, 3, 333);
                break;
            case 8:
                next = click(1512);
                break;
            case 9:
                next = click(1513, 3, 337);
                break;
            case 10:
                next = click(1511, 3, 336);
                break;
            case 11:
                next = click(1445);
                break;
            case 12:
                next = click(1445);
                break;
            case 13:
                next = click(1446);
                break;
            case 14:
                next = click(1447);
                break;
            case 15:
                next = click(1516, 3, 331);
                break;
            case 16:
                next = click(1515);
                break;
            case 17:
                next = click(1515, 2);
                break;
            default:
                throw std::runtime_error("E6Stage4 exit failed");
            }
            if (next)
            {
                AXLOGI("E6Stage4 step {}", *step);
                ++*step;
            }
        }, .02f, "episode1-e6stage4");
        scene->scheduleOnce([](float) { throw std::runtime_error("E6Stage4 timeout"); },
                            std::getenv("RB_ROUTE_DEMO") ? 3600 : 600, "e6stage4-timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_EPISODE6_STAGE5"))
    {
        auto* owner    = std::getenv("RB_ROUTE_DEMO") ? dynamic_cast<MainScene*>(scene) : nullptr;
        routeDemoOwner = owner;
        auto progress  = owner ? owner->routeDemoProgress() : std::make_shared<rb::Progress>();
        if (!owner)
            progress->initialize(data);
        auto* session = owner ? owner->routeDemoSession() : GameSession::create(data, 6, 5, language, 1, false);
        if (!owner)
        {
            scene->addChild(session);
            session->bindProgress(*progress);
            session->begin();
        }
        auto step   = std::make_shared<int>(0);
        auto age    = std::make_shared<float>(0);
        auto timing = std::make_shared<bool>(false);
        scene->schedule([scene, session, progress, step, age, timing, directory, &data](float dt) {
            *age += dt;
            if (session->story())
            {
                advanceRouteStory(session);
                return;
            }
            if (session->itemDialog())
            {
                if (session->itemDialog()->ready())
                    session->itemDialog()->close();
                return;
            }
            if (auto* d = dynamic_cast<ActionHit*>(session->actionMode()))
            {
                if (d->busy())
                    return;
                if (d->menuVisible())
                    throw std::runtime_error("E6Stage5 hit failed");
                if (int target = d->playableTarget())
                {
                    auto p = d->targetCenter(target);
                    d->beginTouch(p);
                    d->endTouch(p);
                }
                return;
            }
            if (session->clearOverlay())
            {
                if (*step != 30)
                    throw std::runtime_error("E6Stage5 premature clear");
                scene->unschedule("episode1-e6stage5");
                scene->scheduleOnce([directory](float) {
                    utils::captureScreen([](bool ok, std::string_view) {
                        if (!ok)
                            throw std::runtime_error("E6Stage5 capture failed");
                        AXLOGI("RB_PROBE_EPISODE6_STAGE5 PASS");
                        finishRoute();
                    }, std::string(directory) + "/episode1-e6stage5-clear.png");
                }, 2, "e6stage5-clear");
                return;
            }
            if (session->busy() || *age < .8f)
                return;
            auto click = [session, age, &data](int id, int cursor = 0, int item = 0) {
                auto* room = session->room();
                auto p     = room->background()->convertToWorldSpace(room->objectPosition(id));
                if (room->hitObject(p) != id || p.y < 0 || p.y > 320)
                    for (auto* area : data.where("AreaData", "objectID", id))
                    {
                        const int w = rb::integer(*area, "AreaWidth") / 2, h = rb::integer(*area, "AreaHeight") / 2;
                        if (w < 1 || h < 1)
                            continue;
                        auto q = room->background()->convertToWorldSpace(
                            {rb::integer(*area, "AreaX") / 2 + w / 2.f,
                             room->background()->getContentSize().height - rb::integer(*area, "AreaY") / 2 - h / 2.f});
                        if (q.y >= 0 && q.y <= 320 && room->hitObject(q) == id)
                        {
                            p = q;
                            break;
                        }
                    }
                if (p.x < 1 || p.x > 479)
                {
                    room->beginDrag();
                    room->drag((240 - p.x) / .8f);
                    room->drag(0);
                    room->settle();
                    *age = 0;
                    return false;
                }
                if (room->hitObject(p) != id)
                    throw std::runtime_error("E6Stage5 hit mismatch: " + std::to_string(id));
                session->selectCursor(cursor, item);
                session->tap(p);
                *age = 0;
                if (session->touchObject() != id)
                    throw std::runtime_error("E6Stage5 tap rejected");
                return true;
            };
            auto itemPoint = [session](int id) {
                auto ids = session->state().inventory();
                auto it  = std::find(ids.begin(), ids.end(), id);
                if (it == ids.end())
                    throw std::runtime_error("E6Stage5 item absent: " + std::to_string(id));
                int i = static_cast<int>(it - ids.begin());
                return Vec2(48 + (i % 5) * 65, 208 - (i / 5) * 55);
            };
            auto combine = [session, scene, age, itemPoint](int a, int b) {
                auto* inv = session->inventory();
                auto p = itemPoint(a), q = itemPoint(b);
                inv->beginTouch(p);
                scene->scheduleOnce([inv, q](float) {
                    inv->moveTouch(q);
                    inv->endTouch(q);
                }, .8f, "e6stage5-combine");
                *age = -2.5f;
            };
            auto split = [session, scene, age, itemPoint](int id) {
                auto* inv = session->inventory();
                auto p    = itemPoint(id);
                inv->beginTouch(p);
                scene->scheduleOnce([inv, p](float) {
                    inv->moveTouch(p + Vec2(20, 0));
                    inv->moveTouch(p + Vec2(40, 0));
                    inv->endTouch(p + Vec2(40, 0));
                }, .8f, "e6stage5-split");
                *age = -1;
            };
            bool next = false;
            switch (*step)
            {
            case 0:
                next = click(1449);
                break;
            case 1:
                next = click(1450);
                break;
            case 2:
                next = click(1451);
                break;
            case 3:
                next = click(1452);
                break;
            case 4:
                next = click(1455);
                break;
            case 5:
                next = click(1461);
                break;
            case 6:
                next = click(1454);
                break;
            case 7:
                next = click(1462);
                break;
            case 8:
                next = click(1453);
                break;
            case 9:
                session->inventory()->open();
                session->inventory()->selectMode(3);
                *age = 0;
                next = true;
                break;
            case 10:
                combine(308, 306);
                next = true;
                break;
            case 11:
                combine(309, 307);
                next = true;
                break;
            case 12:
                combine(311, 312);
                next = true;
                break;
            case 13:
                combine(311, 313);
                next = true;
                break;
            case 14:
                session->inventory()->close();
                *age = 0;
                next = true;
                break;
            case 15:
                next = click(1457, 3, 338);
                break;
            case 16:
                next = click(1457);
                break;
            case 17:
                next = click(1460, 3, 314);
                break;
            case 18:
                next = click(1459, 3, 315);
                break;
            case 19:
                next = click(1464, 2);
                break;
            case 20:
                next = click(1471);
                break;
            case 21:
                next = click(1472);
                break;
            case 22:
                session->inventory()->open();
                session->inventory()->selectMode(3);
                *age = 0;
                next = true;
                break;
            case 23:
                combine(316, 317);
                next = true;
                break;
            case 24:
                session->inventory()->close();
                *age = 0;
                next = true;
                break;
            case 25:
                next = click(1480, 3, 318);
                break;
            case 26:
                next = click(1480, 2);
                break;
            case 27:
                next = click(1477, 3, 319);
                break;
            case 28:
                next = click(1478);
                break;
            case 29:
                next = click(1478, 2);
                break;
            default:
                throw std::runtime_error("E6Stage5 exit failed");
            }
            if (next)
            {
                AXLOGI("E6Stage5 step {}", *step);
                ++*step;
            }
        }, .02f, "episode1-e6stage5");
        scene->scheduleOnce([](float) { throw std::runtime_error("E6Stage5 timeout"); },
                            std::getenv("RB_ROUTE_DEMO") ? 3600 : 600, "e6stage5-timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_EPISODE6_STAGE1"))
    {
        auto* owner    = std::getenv("RB_ROUTE_DEMO") ? dynamic_cast<MainScene*>(scene) : nullptr;
        routeDemoOwner = owner;
        auto progress  = owner ? owner->routeDemoProgress() : std::make_shared<rb::Progress>();
        if (!owner)
            progress->initialize(data);
        auto* session = owner ? owner->routeDemoSession() : GameSession::create(data, 6, 1, language, 1, false);
        if (!owner)
        {
            scene->addChild(session);
            session->bindProgress(*progress);
            session->begin();
        }
        auto step   = std::make_shared<int>(0);
        auto age    = std::make_shared<float>(0);
        auto timing = std::make_shared<bool>(false);
        scene->schedule([scene, session, progress, step, age, timing, directory, &data](float dt) {
            *age += dt;
            if (session->story())
            {
                advanceRouteStory(session);
                return;
            }
            if (session->itemDialog())
            {
                if (session->itemDialog()->ready())
                    session->itemDialog()->close();
                return;
            }
            if (auto* d = dynamic_cast<ActionHit*>(session->actionMode()))
            {
                if (d->busy())
                    return;
                if (d->menuVisible())
                    throw std::runtime_error("E6Stage1 hit failed");
                if (int target = d->playableTarget())
                {
                    auto p = d->targetCenter(target);
                    d->beginTouch(p);
                    d->endTouch(p);
                }
                return;
            }
            if (session->clearOverlay())
            {
                if (*step != 22)
                    throw std::runtime_error("E6Stage1 premature clear");
                scene->unschedule("episode1-e6stage1");
                scene->scheduleOnce([directory](float) {
                    utils::captureScreen([](bool ok, std::string_view) {
                        if (!ok)
                            throw std::runtime_error("E6Stage1 capture failed");
                        AXLOGI("RB_PROBE_EPISODE6_STAGE1 PASS");
                        finishRoute();
                    }, std::string(directory) + "/episode1-e6stage1-clear.png");
                }, 2, "e6stage1-clear");
                return;
            }
            if (session->busy() || *age < .8f)
                return;
            auto click = [session, age, &data](int id, int cursor = 0, int item = 0) {
                auto* room = session->room();
                auto p     = room->background()->convertToWorldSpace(room->objectPosition(id));
                if (room->hitObject(p) != id || p.y < 0 || p.y > 320)
                    for (auto* area : data.where("AreaData", "objectID", id))
                    {
                        const int w = rb::integer(*area, "AreaWidth") / 2, h = rb::integer(*area, "AreaHeight") / 2;
                        if (w < 1 || h < 1)
                            continue;
                        auto q = room->background()->convertToWorldSpace(
                            {rb::integer(*area, "AreaX") / 2 + w / 2.f,
                             room->background()->getContentSize().height - rb::integer(*area, "AreaY") / 2 - h / 2.f});
                        if (q.y >= 0 && q.y <= 320 && room->hitObject(q) == id)
                        {
                            p = q;
                            break;
                        }
                    }
                if (p.x < 1 || p.x > 479)
                {
                    room->beginDrag();
                    room->drag((240 - p.x) / .8f);
                    room->drag(0);
                    room->settle();
                    *age = 0;
                    return false;
                }
                if (room->hitObject(p) != id)
                    throw std::runtime_error("E6Stage1 hit mismatch: " + std::to_string(id));
                session->selectCursor(cursor, item);
                session->tap(p);
                *age = 0;
                if (session->touchObject() != id)
                    throw std::runtime_error("E6Stage1 tap rejected");
                return true;
            };
            auto itemPoint = [session](int id) {
                auto ids = session->state().inventory();
                auto it  = std::find(ids.begin(), ids.end(), id);
                if (it == ids.end())
                    throw std::runtime_error("E6Stage1 item absent: " + std::to_string(id));
                int i = static_cast<int>(it - ids.begin());
                return Vec2(48 + (i % 5) * 65, 208 - (i / 5) * 55);
            };
            auto combine = [session, scene, age, itemPoint](int a, int b) {
                auto* inv = session->inventory();
                auto p = itemPoint(a), q = itemPoint(b);
                inv->beginTouch(p);
                scene->scheduleOnce([inv, q](float) {
                    inv->moveTouch(q);
                    inv->endTouch(q);
                }, .8f, "e6stage1-combine");
                *age = -2.5f;
            };
            auto split = [session, scene, age, itemPoint](int id) {
                auto* inv = session->inventory();
                auto p    = itemPoint(id);
                inv->beginTouch(p);
                scene->scheduleOnce([inv, p](float) {
                    inv->moveTouch(p + Vec2(20, 0));
                    inv->moveTouch(p + Vec2(40, 0));
                    inv->endTouch(p + Vec2(40, 0));
                }, .8f, "e6stage1-split");
                *age = -1;
            };
            bool next = false;
            switch (*step)
            {
            case 0:
                next = click(1359);
                break;
            case 1:
                session->inventory()->open();
                session->inventory()->selectMode(2);
                *age = 0;
                next = true;
                break;
            case 2:
                split(284);
                next = true;
                break;
            case 3:
                session->inventory()->close();
                *age = 0;
                next = true;
                break;
            case 4:
                next = click(1357);
                break;
            case 5:
                next = click(1357, 3, 286);
                break;
            case 6:
                next = click(1356);
                break;
            case 7:
                next = click(1356, 2);
                break;
            case 8:
                next = click(1360);
                break;
            case 9:
                next = click(1362);
                break;
            case 10:
                next = click(1363);
                break;
            case 11:
                session->inventory()->open();
                session->inventory()->selectMode(2);
                *age = 0;
                next = true;
                break;
            case 12:
                split(287);
                next = true;
                break;
            case 13:
                split(294);
                next = true;
                break;
            case 14:
                split(295);
                next = true;
                break;
            case 15:
                session->inventory()->close();
                *age = 0;
                next = true;
                break;
            case 16:
                next = click(1371, 3, 289);
                break;
            case 17:
                next = click(1372, 3, 290);
                break;
            case 18:
                next = click(1371);
                break;
            case 19:
                next = click(1377, 3, 292);
                break;
            case 20:
                next = click(1487, 3, 291);
                break;
            case 21:
                next = click(1487, 2);
                break;
            default:
                throw std::runtime_error("E6Stage1 exit failed");
            }
            if (next)
            {
                AXLOGI("E6Stage1 step {}", *step);
                ++*step;
            }
        }, .02f, "episode1-e6stage1");
        scene->scheduleOnce([](float) { throw std::runtime_error("E6Stage1 timeout"); },
                            std::getenv("RB_ROUTE_DEMO") ? 3600 : 600, "e6stage1-timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_EPISODE5_STAGE4"))
    {
        auto* owner    = std::getenv("RB_ROUTE_DEMO") ? dynamic_cast<MainScene*>(scene) : nullptr;
        routeDemoOwner = owner;
        auto progress  = owner ? owner->routeDemoProgress() : std::make_shared<rb::Progress>();
        if (!owner)
            progress->initialize(data);
        auto* session = owner ? owner->routeDemoSession() : GameSession::create(data, 5, 4, language, 1, false);
        if (!owner)
        {
            scene->addChild(session);
            session->bindProgress(*progress);
            session->begin();
        }
        auto step   = std::make_shared<int>(0);
        auto age    = std::make_shared<float>(0);
        auto timing = std::make_shared<bool>(false);
        scene->schedule([scene, session, progress, step, age, timing, directory, &data](float dt) {
            *age += dt;
            if (session->story())
            {
                advanceRouteStory(session);
                return;
            }
            if (session->itemDialog())
            {
                if (session->itemDialog()->ready())
                    session->itemDialog()->close();
                return;
            }
            if (auto* d = dynamic_cast<ActionHit*>(session->actionMode()))
            {
                if (d->busy())
                    return;
                if (d->menuVisible())
                    throw std::runtime_error("E5Stage4 hit failed");
                if (int target = d->playableTarget())
                {
                    auto p = d->targetCenter(target);
                    d->beginTouch(p);
                    d->endTouch(p);
                }
                return;
            }
            if (auto* d = session->detail())
            {
                if (d->busy() || *age < .7f)
                    return;
                if (d->id() == 69)
                {
                    d->close();
                    return;
                }
                if (d->id() == 71)
                {
                    for (int i : {3, 8, 4, 6})
                    {
                        auto p = d->controlCenter(i);
                        d->beginTouch(p);
                        d->endTouch(p);
                    }
                    return;
                }
                if (auto* woman = dynamic_cast<DetailWomanStatue*>(d))
                {
                    const int target[] = {2, 6, 1, 3};
                    for (int c = 0; c < 4; ++c)
                        if (woman->entered()[c] != target[c])
                        {
                            auto p = d->controlCenter(c);
                            d->beginTouch(p);
                            d->moveTouch(p + Vec2(woman->digitWidth(), 0));
                            d->endTouch(p + Vec2(woman->digitWidth(), 0));
                            *age = 0;
                            return;
                        }
                    return;
                }
                throw std::runtime_error("E5Stage4 unexpected detail");
            }
            if (session->clearOverlay())
            {
                if (*step != 36)
                    throw std::runtime_error("E5Stage4 premature clear");
                scene->unschedule("episode1-e5stage4");
                scene->scheduleOnce([directory](float) {
                    utils::captureScreen([](bool ok, std::string_view) {
                        if (!ok)
                            throw std::runtime_error("E5Stage4 capture failed");
                        AXLOGI("RB_PROBE_EPISODE5_STAGE4 PASS");
                        finishRoute();
                    }, std::string(directory) + "/episode1-e5stage4-clear.png");
                }, 2, "e5stage4-clear");
                return;
            }
            if (session->busy() || *age < .8f)
                return;
            auto click = [session, age, &data](int id, int cursor = 0, int item = 0) {
                auto* room = session->room();
                auto p     = room->background()->convertToWorldSpace(room->objectPosition(id));
                if (room->hitObject(p) != id || p.y < 0 || p.y > 320)
                    for (auto* area : data.where("AreaData", "objectID", id))
                    {
                        const int w = rb::integer(*area, "AreaWidth") / 2, h = rb::integer(*area, "AreaHeight") / 2;
                        if (w < 1 || h < 1)
                            continue;
                        auto q = room->background()->convertToWorldSpace(
                            {rb::integer(*area, "AreaX") / 2 + w / 2.f,
                             room->background()->getContentSize().height - rb::integer(*area, "AreaY") / 2 - h / 2.f});
                        if (q.y >= 0 && q.y <= 320 && room->hitObject(q) == id)
                        {
                            p = q;
                            break;
                        }
                    }
                if (p.x < 1 || p.x > 479)
                {
                    room->beginDrag();
                    room->drag((240 - p.x) / .8f);
                    room->drag(0);
                    room->settle();
                    *age = 0;
                    return false;
                }
                if (room->hitObject(p) != id)
                    throw std::runtime_error("E5Stage4 hit mismatch: " + std::to_string(id));
                session->selectCursor(cursor, item);
                session->tap(p);
                *age = 0;
                if (session->touchObject() != id)
                    throw std::runtime_error("E5Stage4 tap rejected");
                return true;
            };
            auto itemPoint = [session](int id) {
                auto ids = session->state().inventory();
                auto it  = std::find(ids.begin(), ids.end(), id);
                if (it == ids.end())
                    throw std::runtime_error("E5Stage4 item absent: " + std::to_string(id));
                int i = static_cast<int>(it - ids.begin());
                return Vec2(48 + (i % 5) * 65, 208 - (i / 5) * 55);
            };
            auto combine = [session, scene, age, itemPoint](int a, int b) {
                auto* inv = session->inventory();
                auto p = itemPoint(a), q = itemPoint(b);
                inv->beginTouch(p);
                scene->scheduleOnce([inv, q](float) {
                    inv->moveTouch(q);
                    inv->endTouch(q);
                }, .8f, "e5stage4-combine");
                *age = -2.5f;
            };
            auto split = [session, scene, age, itemPoint](int id) {
                auto* inv = session->inventory();
                auto p    = itemPoint(id);
                inv->beginTouch(p);
                scene->scheduleOnce([inv, p](float) {
                    inv->moveTouch(p + Vec2(20, 0));
                    inv->moveTouch(p + Vec2(40, 0));
                    inv->endTouch(p + Vec2(40, 0));
                }, .8f, "e5stage4-split");
                *age = -1;
            };
            bool next = false;
            switch (*step)
            {
            case 0:
                next = click(972);
                break;
            case 1:
                next = click(980, 1);
                break;
            case 2:
                next = click(979, 1);
                break;
            case 3:
                next = click(997);
                break;
            case 4:
                next = click(986);
                break;
            case 5:
                next = click(991);
                break;
            case 6:
                next = click(975, 3, 216);
                break;
            case 7:
                next = click(975);
                break;
            case 8:
                next = click(975, 2);
                break;
            case 9:
                next = click(1004);
                break;
            case 10:
                next = click(1005);
                break;
            case 11:
                next = click(1006);
                break;
            case 12:
                next = click(1007);
                break;
            case 13:
                next = click(1003);
                break;
            case 14:
                next = click(1009, 3, 215);
                break;
            case 15:
                next = click(1002);
                break;
            case 16:
                next = click(1001, 3, 224);
                break;
            case 17:
                next = click(1008, 2);
                break;
            case 18:
                next = click(995, 3, 218);
                break;
            case 19:
                next = click(993);
                break;
            case 20:
                next = click(993, 3, 223);
                break;
            case 21:
                next = click(996, 3, 225);
                break;
            case 22:
                session->inventory()->open();
                session->inventory()->selectMode(2);
                *age = 0;
                next = true;
                break;
            case 23:
                split(218);
                next = true;
                break;
            case 24:
                session->inventory()->selectMode(3);
                *age = 0;
                next = true;
                break;
            case 25:
                combine(220, 217);
                next = true;
                break;
            case 26:
                session->inventory()->close();
                *age = 0;
                next = true;
                break;
            case 27:
                next = click(1355, 3, 221);
                break;
            case 28:
                next = click(1355, 3, 221);
                break;
            case 29:
                next = click(1355, 3, 221);
                break;
            case 30:
                next = click(1355, 3, 221);
                break;
            case 31:
                next = click(981, 3, 226);
                break;
            case 32:
                next = click(982, 3, 227);
                break;
            case 33:
                next = click(983, 3, 228);
                break;
            case 34:
                next = click(984, 3, 229);
                break;
            case 35:
                next = click(973, 2);
                break;
            default:
                throw std::runtime_error("E5Stage4 exit failed");
            }
            if (next)
            {
                AXLOGI("E5Stage4 step {}", *step);
                ++*step;
            }
        }, .02f, "episode1-e5stage4");
        scene->scheduleOnce([](float) { throw std::runtime_error("E5Stage4 timeout"); },
                            std::getenv("RB_ROUTE_DEMO") ? 3600 : 600, "e5stage4-timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_EPISODE5_STAGE1"))
    {
        auto* owner    = std::getenv("RB_ROUTE_DEMO") ? dynamic_cast<MainScene*>(scene) : nullptr;
        routeDemoOwner = owner;
        auto progress  = owner ? owner->routeDemoProgress() : std::make_shared<rb::Progress>();
        if (!owner)
            progress->initialize(data);
        auto* session = owner ? owner->routeDemoSession() : GameSession::create(data, 5, 1, language, 1, false);
        if (!owner)
        {
            scene->addChild(session);
            session->bindProgress(*progress);
            session->begin();
        }
        auto step   = std::make_shared<int>(0);
        auto age    = std::make_shared<float>(0);
        auto timing = std::make_shared<bool>(false);
        scene->schedule([scene, session, progress, step, age, timing, directory, &data](float dt) {
            *age += dt;
            if (session->story())
            {
                advanceRouteStory(session);
                return;
            }
            if (session->itemDialog())
            {
                if (session->itemDialog()->ready())
                    session->itemDialog()->close();
                return;
            }
            if (auto* d = dynamic_cast<ActionHit*>(session->actionMode()))
            {
                if (d->busy())
                    return;
                if (d->menuVisible())
                    throw std::runtime_error("E5Stage1 hit failed");
                if (int target = d->playableTarget())
                {
                    auto p = d->targetCenter(target);
                    d->beginTouch(p);
                    d->endTouch(p);
                }
                return;
            }
            if (auto* d = session->detail())
            {
                if (d->busy())
                    return;
                if (d->id() >= 32 && d->id() <= 37)
                {
                    d->close();
                    return;
                }
                if (d->id() == 38)
                {
                    for (int i : {6, 4, 5, 9, 3, 3})
                    {
                        auto p = d->controlCenter(i);
                        d->beginTouch(p);
                        d->endTouch(p);
                    }
                    return;
                }
                if (d->id() == 40)
                {
                    for (int i : {2, 1, 4, 6})
                    {
                        auto p = d->controlCenter(i);
                        d->beginTouch(p);
                        d->endTouch(p);
                    }
                    auto p = d->controlCenter(11);
                    d->beginTouch(p);
                    d->moveTouch(p + Vec2(0, -40));
                    d->endTouch(p + Vec2(0, -40));
                    return;
                }
                throw std::runtime_error("E5Stage1 unexpected detail");
            }
            if (session->clearOverlay())
            {
                if (*step != 35)
                    throw std::runtime_error("E5Stage1 premature clear");
                scene->unschedule("episode1-e5stage1");
                scene->scheduleOnce([directory](float) {
                    utils::captureScreen([](bool ok, std::string_view) {
                        if (!ok)
                            throw std::runtime_error("E5Stage1 capture failed");
                        AXLOGI("RB_PROBE_EPISODE5_STAGE1 PASS");
                        finishRoute();
                    }, std::string(directory) + "/episode1-e5stage1-clear.png");
                }, 2, "e5stage1-clear");
                return;
            }
            if (session->busy() || *age < .8f)
                return;
            auto click = [session, age, &data](int id, int cursor = 0, int item = 0) {
                auto* room = session->room();
                auto p     = room->background()->convertToWorldSpace(room->objectPosition(id));
                if (room->hitObject(p) != id || p.y < 0 || p.y > 320)
                    for (auto* area : data.where("AreaData", "objectID", id))
                    {
                        const int w = rb::integer(*area, "AreaWidth") / 2, h = rb::integer(*area, "AreaHeight") / 2;
                        if (w < 1 || h < 1)
                            continue;
                        auto q = room->background()->convertToWorldSpace(
                            {rb::integer(*area, "AreaX") / 2 + w / 2.f,
                             room->background()->getContentSize().height - rb::integer(*area, "AreaY") / 2 - h / 2.f});
                        if (q.y >= 0 && q.y <= 320 && room->hitObject(q) == id)
                        {
                            p = q;
                            break;
                        }
                    }
                if (p.x < 1 || p.x > 479)
                {
                    room->beginDrag();
                    room->drag((240 - p.x) / .8f);
                    room->drag(0);
                    room->settle();
                    *age = 0;
                    return false;
                }
                if (room->hitObject(p) != id)
                    throw std::runtime_error("E5Stage1 hit mismatch: " + std::to_string(id));
                session->selectCursor(cursor, item);
                session->tap(p);
                *age = 0;
                if (session->touchObject() != id)
                    throw std::runtime_error("E5Stage1 tap rejected");
                return true;
            };
            auto itemPoint = [session](int id) {
                auto ids = session->state().inventory();
                auto it  = std::find(ids.begin(), ids.end(), id);
                if (it == ids.end())
                    throw std::runtime_error("E5Stage1 item absent: " + std::to_string(id));
                int i = static_cast<int>(it - ids.begin());
                return Vec2(48 + (i % 5) * 65, 208 - (i / 5) * 55);
            };
            auto combine = [session, scene, age, itemPoint](int a, int b) {
                auto* inv = session->inventory();
                auto p = itemPoint(a), q = itemPoint(b);
                inv->beginTouch(p);
                scene->scheduleOnce([inv, q](float) {
                    inv->moveTouch(q);
                    inv->endTouch(q);
                }, .8f, "e5stage1-combine");
                *age = -2.5f;
            };
            auto split = [session, scene, age, itemPoint](int id) {
                auto* inv = session->inventory();
                auto p    = itemPoint(id);
                inv->beginTouch(p);
                scene->scheduleOnce([inv, p](float) {
                    inv->moveTouch(p + Vec2(20, 0));
                    inv->moveTouch(p + Vec2(40, 0));
                    inv->endTouch(p + Vec2(40, 0));
                }, .8f, "e5stage1-split");
                *age = -1;
            };
            bool next = false;
            switch (*step)
            {
            case 0:
                next = click(558, 1);
                break;
            case 1:
                next = click(559, 1);
                break;
            case 2:
                next = click(561, 1);
                break;
            case 3:
                next = click(562, 1);
                break;
            case 4:
                next = click(564, 1);
                break;
            case 5:
                next = click(575);
                break;
            case 6:
                next = click(576);
                break;
            case 7:
                next = click(563, 3, 124);
                break;
            case 8:
                next = click(563, 1);
                break;
            case 9:
                session->inventory()->open();
                session->inventory()->selectMode(2);
                *age = 0;
                next = true;
                break;
            case 10:
                split(114);
                next = true;
                break;
            case 11:
                split(115);
                next = true;
                break;
            case 12:
                split(116);
                next = true;
                break;
            case 13:
                session->inventory()->selectMode(3);
                *age = 0;
                next = true;
                break;
            case 14:
                combine(117, 119);
                next = true;
                break;
            case 15:
                session->inventory()->close();
                *age = 0;
                next = true;
                break;
            case 16:
                next = click(566, 3, 122);
                break;
            case 17:
                next = click(566);
                break;
            case 18:
                next = click(566);
                break;
            case 19:
                next = click(566);
                break;
            case 20:
                next = click(566);
                break;
            case 21:
                next = click(566);
                break;
            case 22:
                next = click(566);
                break;
            case 23:
                next = click(566);
                break;
            case 24:
                next = click(574);
                break;
            case 25:
                next = click(568);
                break;
            case 26:
                next = click(569);
                break;
            case 27:
                next = click(570);
                break;
            case 28:
                next = click(573);
                break;
            case 29:
                next = click(572);
                break;
            case 30:
                next = click(579, 3, 129);
                break;
            case 31:
                next = click(571);
                break;
            case 32:
                next = click(570);
                break;
            case 33:
                next = click(570, 3, 130);
                break;
            case 34:
                next = click(581, 2);
                break;
            default:
                throw std::runtime_error("E5Stage1 exit failed");
            }
            if (next)
            {
                AXLOGI("E5Stage1 step {}", *step);
                ++*step;
            }
        }, .02f, "episode1-e5stage1");
        scene->scheduleOnce([](float) { throw std::runtime_error("E5Stage1 timeout"); },
                            std::getenv("RB_ROUTE_DEMO") ? 3600 : 600, "e5stage1-timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_EPISODE4_STAGE4"))
    {
        auto* owner    = std::getenv("RB_ROUTE_DEMO") ? dynamic_cast<MainScene*>(scene) : nullptr;
        routeDemoOwner = owner;
        auto progress  = owner ? owner->routeDemoProgress() : std::make_shared<rb::Progress>();
        if (!owner)
            progress->initialize(data);
        auto* session = owner ? owner->routeDemoSession() : GameSession::create(data, 4, 4, language, 1, false);
        if (!owner)
        {
            scene->addChild(session);
            session->bindProgress(*progress);
            session->begin();
        }
        auto step   = std::make_shared<int>(0);
        auto age    = std::make_shared<float>(0);
        auto timing = std::make_shared<bool>(false);
        scene->schedule([scene, session, progress, step, age, timing, directory, &data](float dt) {
            *age += dt;
            if (session->story())
            {
                advanceRouteStory(session);
                return;
            }
            if (session->itemDialog())
            {
                if (session->itemDialog()->ready())
                    session->itemDialog()->close();
                return;
            }
            if (auto* d = dynamic_cast<ActionHit*>(session->actionMode()))
            {
                if (d->busy())
                    return;
                if (d->menuVisible())
                    throw std::runtime_error("E4Stage4 hit failed");
                if (int target = d->playableTarget())
                {
                    auto p = d->targetCenter(target);
                    d->beginTouch(p);
                    d->endTouch(p);
                }
                return;
            }
            if (auto* d = session->detail())
            {
                if (d->busy())
                    return;
                if (d->id() == 18)
                {
                    if (!*timing)
                    {
                        *timing = true;
                        auto p  = d->controlCenter(0);
                        d->beginTouch(p);
                        scene->scheduleOnce([d, p](float) { d->endTouch(p); }, 12.5f, "e4stage4-timing");
                    }
                    return;
                }
                if (d->id() == 19 || d->id() == 20)
                {
                    d->close();
                    return;
                }
                throw std::runtime_error("E4Stage4 unexpected detail");
            }
            if (session->clearOverlay())
            {
                if (*step != 47)
                    throw std::runtime_error("E4Stage4 premature clear");
                scene->unschedule("episode1-e4stage4");
                scene->scheduleOnce([directory](float) {
                    utils::captureScreen([](bool ok, std::string_view) {
                        if (!ok)
                            throw std::runtime_error("E4Stage4 capture failed");
                        AXLOGI("RB_PROBE_EPISODE4_STAGE4 PASS");
                        finishRoute();
                    }, std::string(directory) + "/episode1-e4stage4-clear.png");
                }, 2, "e4stage4-clear");
                return;
            }
            if (session->busy() || *age < .8f)
                return;
            auto click = [session, age, &data](int id, int cursor = 0, int item = 0) {
                auto* room = session->room();
                auto p     = room->background()->convertToWorldSpace(room->objectPosition(id));
                if (room->hitObject(p) != id || p.y < 0 || p.y > 320)
                    for (auto* area : data.where("AreaData", "objectID", id))
                    {
                        const int w = rb::integer(*area, "AreaWidth") / 2, h = rb::integer(*area, "AreaHeight") / 2;
                        if (w < 1 || h < 1)
                            continue;
                        auto q = room->background()->convertToWorldSpace(
                            {rb::integer(*area, "AreaX") / 2 + w / 2.f,
                             room->background()->getContentSize().height - rb::integer(*area, "AreaY") / 2 - h / 2.f});
                        if (q.y >= 0 && q.y <= 320 && room->hitObject(q) == id)
                        {
                            p = q;
                            break;
                        }
                    }
                if (p.x < 1 || p.x > 479)
                {
                    room->beginDrag();
                    room->drag((240 - p.x) / .8f);
                    room->drag(0);
                    room->settle();
                    *age = 0;
                    return false;
                }
                if (room->hitObject(p) != id)
                    throw std::runtime_error("E4Stage4 hit mismatch: " + std::to_string(id));
                session->selectCursor(cursor, item);
                session->tap(p);
                *age = 0;
                if (session->touchObject() != id)
                    throw std::runtime_error("E4Stage4 tap rejected");
                return true;
            };
            auto itemPoint = [session](int id) {
                auto ids = session->state().inventory();
                auto it  = std::find(ids.begin(), ids.end(), id);
                if (it == ids.end())
                    throw std::runtime_error("E4Stage4 item absent: " + std::to_string(id));
                int i = static_cast<int>(it - ids.begin());
                return Vec2(48 + (i % 5) * 65, 208 - (i / 5) * 55);
            };
            auto combine = [session, scene, age, itemPoint](int a, int b) {
                auto* inv = session->inventory();
                auto p = itemPoint(a), q = itemPoint(b);
                inv->beginTouch(p);
                scene->scheduleOnce([inv, q](float) {
                    inv->moveTouch(q);
                    inv->endTouch(q);
                }, .8f, "e4stage4-combine");
                *age = -2.5f;
            };
            bool next = false;
            switch (*step)
            {
            case 0:
                next = click(435);
                break;
            case 1:
                next = click(436);
                break;
            case 2:
                next = click(459, 1);
                break;
            case 3:
                next = click(459, 3, 96);
                break;
            case 4:
                next = click(459, 2);
                break;
            case 5:
                next = click(461);
                break;
            case 6:
                next = click(462, 1);
                break;
            case 7:
                next = click(462, 1);
                break;
            case 8:
                next = click(462, 1);
                break;
            case 9:
                next = click(463, 3, 96);
                break;
            case 10:
                next = click(469);
                break;
            case 11:
                next = click(464, 3, 96);
                break;
            case 12:
                next = click(470);
                break;
            case 13:
                next = click(465, 3, 96);
                break;
            case 14:
                next = click(471);
                break;
            case 15:
                next = click(475, 2);
                break;
            case 16:
                next = click(441, 3, 98);
                break;
            case 17:
                next = click(457);
                break;
            case 18:
                next = click(458);
                break;
            case 19:
                next = click(459, 2);
                break;
            case 20:
                next = click(477, 3, 97);
                break;
            case 21:
                next = click(475, 2);
                break;
            case 22:
                next = click(442, 3, 99);
                break;
            case 23:
                next = click(443, 3, 101);
                break;
            case 24:
                next = click(444, 3, 100);
                break;
            case 25:
                next = click(445, 3, 102);
                break;
            case 26:
                next = click(446);
                break;
            case 27:
                next = click(446);
                break;
            case 28:
                next = click(446);
                break;
            case 29:
                next = click(446);
                break;
            case 30:
                next = click(447);
                break;
            case 31:
                next = click(447);
                break;
            case 32:
                next = click(447);
                break;
            case 33:
                next = click(447);
                break;
            case 34:
                next = click(447);
                break;
            case 35:
                next = click(448);
                break;
            case 36:
                next = click(448);
                break;
            case 37:
                next = click(449);
                break;
            case 38:
                next = click(449);
                break;
            case 39:
                next = click(449);
                break;
            case 40:
                next = click(449);
                break;
            case 41:
                next = click(449);
                break;
            case 42:
                next = click(449);
                break;
            case 43:
                next = click(449);
                break;
            case 44:
                next = click(434);
                break;
            case 45:
                next = click(425, 3, 103);
                break;
            case 46:
                next = click(424, 2);
                break;
            default:
                throw std::runtime_error("E4Stage4 exit failed");
            }
            if (next)
            {
                AXLOGI("E4Stage4 step {}", *step);
                ++*step;
            }
        }, .02f, "episode1-e4stage4");
        scene->scheduleOnce([](float) { throw std::runtime_error("E4Stage4 timeout"); },
                            std::getenv("RB_ROUTE_DEMO") ? 3600 : 600, "e4stage4-timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_EPISODE4_STAGE2"))
    {
        auto* owner    = std::getenv("RB_ROUTE_DEMO") ? dynamic_cast<MainScene*>(scene) : nullptr;
        routeDemoOwner = owner;
        auto progress  = owner ? owner->routeDemoProgress() : std::make_shared<rb::Progress>();
        if (!owner)
            progress->initialize(data);
        auto* session = owner ? owner->routeDemoSession() : GameSession::create(data, 4, 2, language, 1, false);
        if (!owner)
        {
            scene->addChild(session);
            session->bindProgress(*progress);
            session->begin();
        }
        auto step = std::make_shared<int>(0);
        auto age  = std::make_shared<float>(0);
        scene->schedule([scene, session, progress, step, age, directory, &data](float dt) {
            *age += dt;
            if (session->story())
            {
                advanceRouteStory(session);
                return;
            }
            if (session->itemDialog())
            {
                if (session->itemDialog()->ready())
                    session->itemDialog()->close();
                return;
            }
            if (auto* d = dynamic_cast<ActionHit*>(session->actionMode()))
            {
                if (d->busy())
                    return;
                if (d->menuVisible())
                    throw std::runtime_error("E4Stage2 hit failed");
                if (int target = d->playableTarget())
                {
                    auto p = d->targetCenter(target);
                    d->beginTouch(p);
                    d->endTouch(p);
                }
                return;
            }
            if (auto* d = session->detail())
            {
                if (d->busy())
                    return;
                if (d->id() == 13)
                {
                    for (int i : {0, 5, 8, 15})
                    {
                        auto p = d->controlCenter(i);
                        d->beginTouch(p);
                        d->endTouch(p);
                    }
                    return;
                }
                throw std::runtime_error("E4Stage2 unexpected detail");
            }
            if (session->clearOverlay())
            {
                if (*step != 20)
                    throw std::runtime_error("E4Stage2 premature clear");
                scene->unschedule("episode1-e4stage2");
                scene->scheduleOnce([directory](float) {
                    utils::captureScreen([](bool ok, std::string_view) {
                        if (!ok)
                            throw std::runtime_error("E4Stage2 capture failed");
                        AXLOGI("RB_PROBE_EPISODE4_STAGE2 PASS");
                        finishRoute();
                    }, std::string(directory) + "/episode1-e4stage2-clear.png");
                }, 2, "e4stage2-clear");
                return;
            }
            if (session->busy() || *age < .8f)
                return;
            auto click = [session, age, &data](int id, int cursor = 0, int item = 0) {
                auto* room = session->room();
                auto p     = room->background()->convertToWorldSpace(room->objectPosition(id));
                if (room->hitObject(p) != id || p.y < 0 || p.y > 320)
                    for (auto* area : data.where("AreaData", "objectID", id))
                    {
                        const int w = rb::integer(*area, "AreaWidth") / 2, h = rb::integer(*area, "AreaHeight") / 2;
                        if (w < 1 || h < 1)
                            continue;
                        auto q = room->background()->convertToWorldSpace(
                            {rb::integer(*area, "AreaX") / 2 + w / 2.f,
                             room->background()->getContentSize().height - rb::integer(*area, "AreaY") / 2 - h / 2.f});
                        if (q.y >= 0 && q.y <= 320 && room->hitObject(q) == id)
                        {
                            p = q;
                            break;
                        }
                    }
                if (p.x < 1 || p.x > 479)
                {
                    room->beginDrag();
                    room->drag((240 - p.x) / .8f);
                    room->drag(0);
                    room->settle();
                    *age = 0;
                    return false;
                }
                if (room->hitObject(p) != id)
                    throw std::runtime_error("E4Stage2 hit mismatch: " + std::to_string(id));
                session->selectCursor(cursor, item);
                session->tap(p);
                *age = 0;
                if (session->touchObject() != id)
                    throw std::runtime_error("E4Stage2 tap rejected");
                return true;
            };
            auto itemPoint = [session](int id) {
                auto ids = session->state().inventory();
                auto it  = std::find(ids.begin(), ids.end(), id);
                if (it == ids.end())
                    throw std::runtime_error("E4Stage2 item absent: " + std::to_string(id));
                int i = static_cast<int>(it - ids.begin());
                return Vec2(48 + (i % 5) * 65, 208 - (i / 5) * 55);
            };
            auto combine = [session, scene, age, itemPoint](int a, int b) {
                auto* inv = session->inventory();
                auto p = itemPoint(a), q = itemPoint(b);
                inv->beginTouch(p);
                scene->scheduleOnce([inv, q](float) {
                    inv->moveTouch(q);
                    inv->endTouch(q);
                }, .8f, "e4stage2-combine");
                *age = -2.5f;
            };
            bool next = false;
            switch (*step)
            {
            case 0:
                next = click(243);
                break;
            case 1:
                next = click(244);
                break;
            case 2:
                next = click(252, 3, 52);
                break;
            case 3:
                next = click(245);
                break;
            case 4:
                next = click(271, 3, 51);
                break;
            case 5:
                next = click(255, 3, 53);
                break;
            case 6:
                next = click(262);
                break;
            case 7:
                next = click(270, 3, 51);
                break;
            case 8:
                next = click(246);
                break;
            case 9:
                session->inventory()->open();
                session->inventory()->selectMode(3);
                *age = 0;
                next = true;
                break;
            case 10:
                combine(54, 55);
                next = true;
                break;
            case 11:
                session->inventory()->close();
                *age = 0;
                next = true;
                break;
            case 12:
                next = click(262, 3, 56);
                break;
            case 13:
            {
                SheetSprite* clue = nullptr;
                for (auto* child : session->room()->background()->getChildren())
                    if (auto* sheet = dynamic_cast<SheetSprite*>(child))
                        if (sheet->getTexture()->getPath().find("e4_r2_dir5") != std::string::npos)
                            clue = sheet;
                if (!clue)
                    throw std::runtime_error("Pyramid route skipped 1324 clue");
                const auto width = clue->getTexture()->getContentSizeInPixels().width / 6;
                if (std::abs(clue->getTextureRect().origin.x - width * 5) > .01f ||
                    std::abs(clue->getTextureRect().size.width - width) > .01f)
                    throw std::runtime_error("Pyramid route cannot read complete 1324 clue");
                next = click(274);
                break;
            }
            case 14:
                next = click(251);
                break;
            case 15:
                next = click(257, 3, 51);
                break;
            case 16:
                next = click(257, 3, 51);
                break;
            case 17:
                next = click(248);
                break;
            case 18:
                next = click(282, 3, 57);
                break;
            case 19:
                next = click(265, 2);
                break;
            default:
                throw std::runtime_error("E4Stage2 exit failed");
            }
            if (next)
            {
                AXLOGI("E4Stage2 step {}", *step);
                ++*step;
            }
        }, .02f, "episode1-e4stage2");
        scene->scheduleOnce([](float) { throw std::runtime_error("E4Stage2 timeout"); },
                            std::getenv("RB_ROUTE_DEMO") ? 3600 : 600, "e4stage2-timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_EPISODE3_STAGE3"))
    {
        auto* owner    = std::getenv("RB_ROUTE_DEMO") ? dynamic_cast<MainScene*>(scene) : nullptr;
        routeDemoOwner = owner;
        auto progress  = owner ? owner->routeDemoProgress() : std::make_shared<rb::Progress>();
        if (!owner)
            progress->initialize(data);
        auto* session = owner ? owner->routeDemoSession() : GameSession::create(data, 3, 3, language, 1, false);
        if (!owner)
        {
            scene->addChild(session);
            session->bindProgress(*progress);
            session->begin();
        }
        auto step = std::make_shared<int>(0);
        auto age  = std::make_shared<float>(0);
        scene->schedule([scene, session, progress, step, age, directory, &data](float dt) {
            *age += dt;
            if (session->story())
            {
                advanceRouteStory(session);
                return;
            }
            if (session->itemDialog())
            {
                if (session->itemDialog()->ready())
                    session->itemDialog()->close();
                return;
            }
            if (auto* d = dynamic_cast<ActionHit*>(session->actionMode()))
            {
                if (d->busy())
                    return;
                if (d->menuVisible())
                    throw std::runtime_error("E3Stage3 hit failed");
                if (int target = d->playableTarget())
                {
                    auto p = d->targetCenter(target);
                    d->beginTouch(p);
                    d->endTouch(p);
                }
                return;
            }
            if (auto* d = session->detail())
            {
                if (d->busy())
                    return;
                if (d->id() == 74)
                {
                    d->close();
                    return;
                }
                if (d->id() == 76)
                {
                    for (int i : {0, 3, 2, 7})
                    {
                        auto p = d->controlCenter(i);
                        d->beginTouch(p);
                        d->endTouch(p);
                    }
                    auto p = d->controlCenter(11);
                    d->beginTouch(p);
                    d->moveTouch(p + Vec2(0, -40));
                    d->endTouch(p + Vec2(0, -40));
                    return;
                }
                throw std::runtime_error("E3Stage3 unexpected detail");
            }
            if (session->clearOverlay())
            {
                if (*step != 16)
                    throw std::runtime_error("E3Stage3 premature clear");
                scene->unschedule("episode1-e3stage3");
                scene->scheduleOnce([directory](float) {
                    utils::captureScreen([](bool ok, std::string_view) {
                        if (!ok)
                            throw std::runtime_error("E3Stage3 capture failed");
                        AXLOGI("RB_PROBE_EPISODE3_STAGE3 PASS");
                        finishRoute();
                    }, std::string(directory) + "/episode1-e3stage3-clear.png");
                }, 2, "e3stage3-clear");
                return;
            }
            if (session->busy() || *age < .8f)
                return;
            auto click = [session, age, &data](int id, int cursor = 0, int item = 0) {
                auto* room = session->room();
                auto p     = room->background()->convertToWorldSpace(room->objectPosition(id));
                if (room->hitObject(p) != id || p.y < 0 || p.y > 320)
                    for (auto* area : data.where("AreaData", "objectID", id))
                    {
                        const int w = rb::integer(*area, "AreaWidth") / 2, h = rb::integer(*area, "AreaHeight") / 2;
                        if (w < 1 || h < 1)
                            continue;
                        auto q = room->background()->convertToWorldSpace(
                            {rb::integer(*area, "AreaX") / 2 + w / 2.f,
                             room->background()->getContentSize().height - rb::integer(*area, "AreaY") / 2 - h / 2.f});
                        if (q.y >= 0 && q.y <= 320 && room->hitObject(q) == id)
                        {
                            p = q;
                            break;
                        }
                    }
                if (p.x < 1 || p.x > 479)
                {
                    room->beginDrag();
                    room->drag((240 - p.x) / .8f);
                    room->drag(0);
                    room->settle();
                    *age = 0;
                    return false;
                }
                if (room->hitObject(p) != id)
                    throw std::runtime_error("E3Stage3 hit mismatch: " + std::to_string(id));
                session->selectCursor(cursor, item);
                session->tap(p);
                *age = 0;
                if (session->touchObject() != id)
                    throw std::runtime_error("E3Stage3 tap rejected");
                return true;
            };
            auto itemPoint = [session](int id) {
                auto ids = session->state().inventory();
                auto it  = std::find(ids.begin(), ids.end(), id);
                if (it == ids.end())
                    throw std::runtime_error("E3Stage3 item absent: " + std::to_string(id));
                int i = static_cast<int>(it - ids.begin());
                return Vec2(48 + (i % 5) * 65, 208 - (i / 5) * 55);
            };
            auto combine = [session, scene, age, itemPoint](int a, int b) {
                auto* inv = session->inventory();
                auto p = itemPoint(a), q = itemPoint(b);
                inv->beginTouch(p);
                scene->scheduleOnce([inv, q](float) {
                    inv->moveTouch(q);
                    inv->endTouch(q);
                }, .8f, "e3stage3-combine");
                *age = -2.5f;
            };
            bool next = false;
            switch (*step)
            {
            case 0:
                next = click(1094);
                break;
            case 1:
                next = click(1094);
                break;
            case 2:
                next = click(1088);
                break;
            case 3:
                next = click(1064, 3, 231);
                break;
            case 4:
                next = click(1063, 1);
                break;
            case 5:
                next = click(1077);
                break;
            case 6:
                next = click(1087);
                break;
            case 7:
                next = click(1074);
                break;
            case 8:
                next = click(1091);
                break;
            case 9:
                next = click(1075);
                break;
            case 10:
                next = click(1075, 3, 232);
                break;
            case 11:
                next = click(1075, 3, 230);
                break;
            case 12:
                next = click(1090);
                break;
            case 13:
                next = click(1092, 3, 236);
                break;
            case 14:
                next = click(1092);
                break;
            case 15:
                next = click(1092, 2);
                break;
            default:
                throw std::runtime_error("E3Stage3 exit failed");
            }
            if (next)
            {
                AXLOGI("E3Stage3 step {}", *step);
                ++*step;
            }
        }, .02f, "episode1-e3stage3");
        scene->scheduleOnce([](float) { throw std::runtime_error("E3Stage3 timeout"); },
                            std::getenv("RB_ROUTE_DEMO") ? 3600 : 300, "e3stage3-timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_EPISODE3_STAGE4"))
    {
        auto* owner    = std::getenv("RB_ROUTE_DEMO") ? dynamic_cast<MainScene*>(scene) : nullptr;
        routeDemoOwner = owner;
        auto progress  = owner ? owner->routeDemoProgress() : std::make_shared<rb::Progress>();
        if (!owner)
            progress->initialize(data);
        auto* session = owner ? owner->routeDemoSession() : GameSession::create(data, 3, 4, language, 1, false);
        if (!owner)
        {
            scene->addChild(session);
            session->bindProgress(*progress);
            session->begin();
        }
        auto step = std::make_shared<int>(0);
        auto age  = std::make_shared<float>(0);
        scene->schedule([scene, session, progress, step, age, directory, &data](float dt) {
            *age += dt;
            if (session->story())
            {
                advanceRouteStory(session);
                return;
            }
            if (session->itemDialog())
            {
                if (session->itemDialog()->ready())
                    session->itemDialog()->close();
                return;
            }
            if (auto* d = session->detail())
            {
                if (d->busy())
                    return;
                if (auto* g = dynamic_cast<DetailGearHandle*>(d))
                {
                    if (g->moving())
                        return;
                    for (int i = 0; i < g->count(); ++i)
                    {
                        auto a = g->angle(i);
                        if (a == 0)
                            continue;
                        auto p = g->controlCenter(i);
                        g->beginTouch(p);
                        g->moveTouch(p + Vec2(a > 180 || a < 0 ? 11 : -11, 0));
                        g->endTouch(p);
                        *age = 0;
                        return;
                    }
                    return;
                }
                if (d->id() == 77)
                {
                    for (int i : {1, 9, 6, 3})
                    {
                        auto p = d->controlCenter(i);
                        d->beginTouch(p);
                        d->endTouch(p);
                    }
                    auto p = d->controlCenter(11);
                    d->beginTouch(p);
                    d->moveTouch(p + Vec2(0, -40));
                    d->endTouch(p + Vec2(0, -40));
                    return;
                }
                throw std::runtime_error("E3Stage4 unexpected detail");
            }
            if (session->clearOverlay())
            {
                if (*step != 22)
                    throw std::runtime_error("E3Stage4 premature clear");
                scene->unschedule("episode1-e3stage4");
                scene->scheduleOnce([directory](float) {
                    utils::captureScreen([](bool ok, std::string_view) {
                        if (!ok)
                            throw std::runtime_error("E3Stage4 capture failed");
                        AXLOGI("RB_PROBE_EPISODE3_STAGE4 PASS");
                        finishRoute();
                    }, std::string(directory) + "/episode1-e3stage4-clear.png");
                }, 2, "e3stage4-clear");
                return;
            }
            if (session->busy() || *age < .8f)
                return;
            auto click = [session, age, &data](int id, int cursor = 0, int item = 0) {
                auto* room = session->room();
                auto p     = room->background()->convertToWorldSpace(room->objectPosition(id));
                if (room->hitObject(p) != id || p.y < 0 || p.y > 320)
                    for (auto* area : data.where("AreaData", "objectID", id))
                    {
                        const int w = rb::integer(*area, "AreaWidth") / 2, h = rb::integer(*area, "AreaHeight") / 2;
                        if (w < 1 || h < 1)
                            continue;
                        auto q = room->background()->convertToWorldSpace(
                            {rb::integer(*area, "AreaX") / 2 + w / 2.f,
                             room->background()->getContentSize().height - rb::integer(*area, "AreaY") / 2 - h / 2.f});
                        if (q.y >= 0 && q.y <= 320 && room->hitObject(q) == id)
                        {
                            p = q;
                            break;
                        }
                    }
                if (p.x < 1 || p.x > 479)
                {
                    room->beginDrag();
                    room->drag((240 - p.x) / .8f);
                    room->drag(0);
                    room->settle();
                    *age = 0;
                    return false;
                }
                if (room->hitObject(p) != id)
                    throw std::runtime_error("E3Stage4 hit mismatch: " + std::to_string(id));
                session->selectCursor(cursor, item);
                session->tap(p);
                *age = 0;
                if (session->touchObject() != id)
                    throw std::runtime_error("E3Stage4 tap rejected");
                return true;
            };
            auto itemPoint = [session](int id) {
                auto ids = session->state().inventory();
                auto it  = std::find(ids.begin(), ids.end(), id);
                if (it == ids.end())
                    throw std::runtime_error("E3Stage4 item absent: " + std::to_string(id));
                int i = static_cast<int>(it - ids.begin());
                return Vec2(48 + (i % 5) * 65, 208 - (i / 5) * 55);
            };
            auto combine = [session, scene, age, itemPoint](int a, int b) {
                auto* inv = session->inventory();
                auto p = itemPoint(a), q = itemPoint(b);
                inv->beginTouch(p);
                scene->scheduleOnce([inv, q](float) {
                    inv->moveTouch(q);
                    inv->endTouch(q);
                }, .8f, "e3stage4-combine");
                *age = -2.5f;
            };
            bool next = false;
            switch (*step)
            {
            case 0:
                next = click(1105);
                break;
            case 1:
                next = click(1105);
                break;
            case 2:
                next = click(1105);
                break;
            case 3:
                next = click(1105);
                break;
            case 4:
                next = click(1158);
                break;
            case 5:
                next = click(1099);
                break;
            case 6:
                next = click(1160, 3, 237);
                break;
            case 7:
                next = click(1160);
                break;
            case 8:
                next = click(1098);
                break;
            case 9:
                next = click(1100);
                break;
            case 10:
                next = click(1101);
                break;
            case 11:
                session->inventory()->open();
                session->inventory()->selectMode(3);
                *age = 0;
                next = true;
                break;
            case 12:
                combine(238, 239);
                next = true;
                break;
            case 13:
                combine(240, 242);
                next = true;
                break;
            case 14:
                session->inventory()->close();
                *age = 0;
                next = true;
                break;
            case 15:
                next = click(1143);
                break;
            case 16:
                next = click(1143, 3, 241);
                break;
            case 17:
                next = click(1155);
                break;
            case 18:
                next = click(1168);
                break;
            case 19:
                next = click(1161, 3, 243);
                break;
            case 20:
                next = click(1161);
                break;
            case 21:
                next = click(1161, 2);
                break;
            default:
                throw std::runtime_error("E3Stage4 exit failed");
            }
            if (next)
            {
                AXLOGI("E3Stage4 step {}", *step);
                ++*step;
            }
        }, .02f, "episode1-e3stage4");
        scene->scheduleOnce([](float) { throw std::runtime_error("E3Stage4 timeout"); },
                            std::getenv("RB_ROUTE_DEMO") ? 3600 : 300, "e3stage4-timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_EPISODE3_STAGE2"))
    {
        auto* owner    = std::getenv("RB_ROUTE_DEMO") ? dynamic_cast<MainScene*>(scene) : nullptr;
        routeDemoOwner = owner;
        auto progress  = owner ? owner->routeDemoProgress() : std::make_shared<rb::Progress>();
        if (!owner)
            progress->initialize(data);
        auto* session = owner ? owner->routeDemoSession() : GameSession::create(data, 3, 2, language, 1, false);
        if (!owner)
        {
            scene->addChild(session);
            session->bindProgress(*progress);
            session->begin();
        }
        auto step = std::make_shared<int>(0);
        auto age  = std::make_shared<float>(0);
        scene->schedule([scene, session, progress, step, age, directory, &data](float dt) {
            *age += dt;
            if (session->story())
            {
                advanceRouteStory(session);
                return;
            }
            if (session->itemDialog())
            {
                if (session->itemDialog()->ready())
                    session->itemDialog()->close();
                return;
            }
            if (auto* d = session->detail())
            {
                if (d->busy())
                    return;
                if (d->id() == 68)
                {
                    for (int i : {6, 6, 1, 4})
                    {
                        auto p = d->controlCenter(i);
                        d->beginTouch(p);
                        d->endTouch(p);
                    }
                    auto p = d->controlCenter(11);
                    d->beginTouch(p);
                    d->moveTouch(p + Vec2(0, -40));
                    d->endTouch(p + Vec2(0, -40));
                    return;
                }
                throw std::runtime_error("E3Stage2 unexpected detail");
            }
            if (session->clearOverlay())
            {
                if (*step != 16)
                    throw std::runtime_error("E3Stage2 premature clear");
                scene->unschedule("episode1-e3stage2");
                scene->scheduleOnce([directory](float) {
                    utils::captureScreen([](bool ok, std::string_view) {
                        if (!ok)
                            throw std::runtime_error("E3Stage2 capture failed");
                        AXLOGI("RB_PROBE_EPISODE3_STAGE2 PASS");
                        finishRoute();
                    }, std::string(directory) + "/episode1-e3stage2-clear.png");
                }, 2, "e3stage2-clear");
                return;
            }
            if (session->busy() || *age < .8f)
                return;
            auto click = [session, age, &data](int id, int cursor = 0, int item = 0) {
                auto* room = session->room();
                auto p     = room->background()->convertToWorldSpace(room->objectPosition(id));
                if (room->hitObject(p) != id || p.y < 0 || p.y > 320)
                    for (auto* area : data.where("AreaData", "objectID", id))
                    {
                        const int w = rb::integer(*area, "AreaWidth") / 2, h = rb::integer(*area, "AreaHeight") / 2;
                        if (w < 1 || h < 1)
                            continue;
                        auto q = room->background()->convertToWorldSpace(
                            {rb::integer(*area, "AreaX") / 2 + w / 2.f,
                             room->background()->getContentSize().height - rb::integer(*area, "AreaY") / 2 - h / 2.f});
                        if (q.y >= 0 && q.y <= 320 && room->hitObject(q) == id)
                        {
                            p = q;
                            break;
                        }
                    }
                if (p.x < 1 || p.x > 479)
                {
                    room->beginDrag();
                    room->drag((240 - p.x) / .8f);
                    room->drag(0);
                    room->settle();
                    *age = 0;
                    return false;
                }
                if (room->hitObject(p) != id)
                    throw std::runtime_error("E3Stage2 hit mismatch: " + std::to_string(id));
                session->selectCursor(cursor, item);
                session->tap(p);
                *age = 0;
                if (session->touchObject() != id)
                    throw std::runtime_error("E3Stage2 tap rejected");
                return true;
            };
            auto itemPoint = [session](int id) {
                auto ids = session->state().inventory();
                auto it  = std::find(ids.begin(), ids.end(), id);
                if (it == ids.end())
                    throw std::runtime_error("E3Stage2 item absent: " + std::to_string(id));
                int i = static_cast<int>(it - ids.begin());
                return Vec2(48 + (i % 5) * 65, 208 - (i / 5) * 55);
            };
            auto combine = [session, scene, age, itemPoint](int a, int b) {
                auto* inv = session->inventory();
                auto p = itemPoint(a), q = itemPoint(b);
                inv->beginTouch(p);
                scene->scheduleOnce([inv, q](float) {
                    inv->moveTouch(q);
                    inv->endTouch(q);
                }, .8f, "e3stage2-combine");
                *age = -2.5f;
            };
            bool next = false;
            switch (*step)
            {
            case 0:
                next = click(825);
                break;
            case 1:
                next = click(825);
                break;
            case 2:
                next = click(845);
                break;
            case 3:
                next = click(847);
                break;
            case 4:
                session->inventory()->open();
                session->inventory()->selectMode(3);
                *age = 0;
                next = true;
                break;
            case 5:
                combine(186, 185);
                next = true;
                break;
            case 6:
                session->inventory()->close();
                *age = 0;
                next = true;
                break;
            case 7:
                next = click(843, 3, 189);
                break;
            case 8:
                next = click(839, 3, 184);
                break;
            case 9:
                if (session->state().objects.at(829).state != 1996)
                    throw std::runtime_error("Fan state not advanced");
                next = true;
                break;
            case 10:
                next = click(846);
                break;
            case 11:
                next = click(848);
                break;
            case 12:
                next = click(844);
                break;
            case 13:
                next = click(851, 3, 187);
                break;
            case 14:
                next = click(851);
                break;
            case 15:
                next = click(851, 2);
                break;
            default:
                throw std::runtime_error("E3Stage2 exit failed");
            }
            if (next)
            {
                AXLOGI("E3Stage2 step {}", *step);
                ++*step;
            }
        }, .02f, "episode1-e3stage2");
        scene->scheduleOnce([](float) { throw std::runtime_error("E3Stage2 timeout"); },
                            std::getenv("RB_ROUTE_DEMO") ? 3600 : 300, "e3stage2-timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_EPISODE3_STAGE1"))
    {
        auto* owner    = std::getenv("RB_ROUTE_DEMO") ? dynamic_cast<MainScene*>(scene) : nullptr;
        routeDemoOwner = owner;
        auto progress  = owner ? owner->routeDemoProgress() : std::make_shared<rb::Progress>();
        if (!owner)
            progress->initialize(data);
        auto* session = owner ? owner->routeDemoSession() : GameSession::create(data, 3, 1, language, 1, false);
        if (!owner)
        {
            scene->addChild(session);
            session->bindProgress(*progress);
            session->begin();
        }
        auto step = std::make_shared<int>(0);
        auto age  = std::make_shared<float>(0);
        scene->schedule([scene, session, progress, step, age, directory, &data](float dt) {
            *age += dt;
            if (session->story())
            {
                advanceRouteStory(session);
                return;
            }
            if (session->itemDialog())
            {
                if (session->itemDialog()->ready())
                    session->itemDialog()->close();
                return;
            }
            if (auto* d = session->detail())
            {
                if (d->busy())
                    return;
                if (d->id() == 67)
                {
                    auto p = d->controlCenter(0);
                    d->beginTouch(p);
                    d->moveTouch(p + Vec2(45, 0));
                    d->moveTouch(p);
                    d->endTouch(p);
                    return;
                }
                if (d->id() == 65)
                {
                    for (int i : {0, 8, 1, 7})
                    {
                        auto p = d->controlCenter(i);
                        d->beginTouch(p);
                        d->endTouch(p);
                    }
                    auto p = d->controlCenter(11);
                    d->beginTouch(p);
                    d->moveTouch(p + Vec2(0, -40));
                    d->endTouch(p + Vec2(0, -40));
                    return;
                }
                throw std::runtime_error("E3Stage1 unexpected detail");
            }
            if (session->clearOverlay())
            {
                if (*step != 10)
                    throw std::runtime_error("E3Stage1 premature clear");
                scene->unschedule("episode1-e3stage1");
                scene->scheduleOnce([directory](float) {
                    utils::captureScreen([](bool ok, std::string_view) {
                        if (!ok)
                            throw std::runtime_error("E3Stage1 capture failed");
                        AXLOGI("RB_PROBE_EPISODE3_STAGE1 PASS");
                        finishRoute();
                    }, std::string(directory) + "/episode1-e3stage1-clear.png");
                }, 2, "e3stage1-clear");
                return;
            }
            if (session->busy() || *age < .8f)
                return;
            auto click = [session, age, &data](int id, int cursor = 0, int item = 0) {
                auto* room = session->room();
                auto p     = room->background()->convertToWorldSpace(room->objectPosition(id));
                if (room->hitObject(p) != id || p.y < 0 || p.y > 320)
                    for (auto* area : data.where("AreaData", "objectID", id))
                    {
                        const int w = rb::integer(*area, "AreaWidth") / 2, h = rb::integer(*area, "AreaHeight") / 2;
                        if (w < 1 || h < 1)
                            continue;
                        auto q = room->background()->convertToWorldSpace(
                            {rb::integer(*area, "AreaX") / 2 + w / 2.f,
                             room->background()->getContentSize().height - rb::integer(*area, "AreaY") / 2 - h / 2.f});
                        if (q.y >= 0 && q.y <= 320 && room->hitObject(q) == id)
                        {
                            p = q;
                            break;
                        }
                    }
                if (p.x < 1 || p.x > 479)
                {
                    room->beginDrag();
                    room->drag((240 - p.x) / .8f);
                    room->drag(0);
                    room->settle();
                    *age = 0;
                    return false;
                }
                if (room->hitObject(p) != id)
                    throw std::runtime_error("E3Stage1 hit mismatch: " + std::to_string(id));
                session->selectCursor(cursor, item);
                session->tap(p);
                *age = 0;
                if (session->touchObject() != id)
                    throw std::runtime_error("E3Stage1 tap rejected");
                return true;
            };
            auto itemPoint = [session](int id) {
                auto ids = session->state().inventory();
                auto it  = std::find(ids.begin(), ids.end(), id);
                if (it == ids.end())
                    throw std::runtime_error("E3Stage1 item absent: " + std::to_string(id));
                int i = static_cast<int>(it - ids.begin());
                return Vec2(48 + (i % 5) * 65, 208 - (i / 5) * 55);
            };
            auto combine = [session, scene, age, itemPoint](int a, int b) {
                auto* inv = session->inventory();
                auto p = itemPoint(a), q = itemPoint(b);
                inv->beginTouch(p);
                scene->scheduleOnce([inv, q](float) {
                    inv->moveTouch(q);
                    inv->endTouch(q);
                }, .8f, "e3stage1-combine");
                *age = -2.5f;
            };
            bool next = false;
            switch (*step)
            {
            case 0:
                next = click(795);
                break;
            case 1:
                next = click(795);
                break;
            case 2:
                next = click(795);
                break;
            case 3:
                next = click(806);
                break;
            case 4:
                next = click(805, 3, 182);
                break;
            case 5:
                next = click(800);
                break;
            case 6:
                next = click(804);
                break;
            case 7:
                next = click(798, 3, 183);
                break;
            case 8:
                next = click(798);
                break;
            case 9:
                next = click(798, 2);
                break;
            default:
                throw std::runtime_error("E3Stage1 exit failed");
            }
            if (next)
            {
                AXLOGI("E3Stage1 step {}", *step);
                ++*step;
            }
        }, .02f, "episode1-e3stage1");
        scene->scheduleOnce([](float) { throw std::runtime_error("E3Stage1 timeout"); },
                            std::getenv("RB_ROUTE_DEMO") ? 3600 : 300, "e3stage1-timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_EPISODE2_STAGE1"))
    {
        auto* owner    = std::getenv("RB_ROUTE_DEMO") ? dynamic_cast<MainScene*>(scene) : nullptr;
        routeDemoOwner = owner;
        auto progress  = owner ? owner->routeDemoProgress() : std::make_shared<rb::Progress>();
        if (!owner)
            progress->initialize(data);
        auto* session = owner ? owner->routeDemoSession() : GameSession::create(data, 2, 1, language, 1, false);
        if (!owner)
        {
            scene->addChild(session);
            session->bindProgress(*progress);
            session->begin();
        }
        auto step = std::make_shared<int>(0);
        auto age  = std::make_shared<float>(0);
        scene->schedule([scene, session, progress, step, age, directory, &data](float dt) {
            *age += dt;
            if (session->story())
            {
                advanceRouteStory(session);
                return;
            }
            if (session->itemDialog())
            {
                if (session->itemDialog()->ready())
                    session->itemDialog()->close();
                return;
            }
            if (auto* d = session->detail())
            {
                if (d->busy())
                    return;
                if (d->id() != 2)
                    throw std::runtime_error("E2Stage1 unexpected detail");
                for (int i : {0, 1, 6, 7})
                {
                    auto p = d->controlCenter(i);
                    d->beginTouch(p);
                    d->endTouch(p);
                }
                return;
            }
            if (session->clearOverlay())
            {
                if (*step != 17)
                    throw std::runtime_error("E2Stage1 premature clear");
                scene->unschedule("episode1-e2stage1");
                scene->scheduleOnce([directory](float) {
                    utils::captureScreen([](bool ok, std::string_view) {
                        if (!ok)
                            throw std::runtime_error("E2Stage1 capture failed");
                        AXLOGI("RB_PROBE_EPISODE2_STAGE1 PASS");
                        finishRoute();
                    }, std::string(directory) + "/episode1-e2stage1-clear.png");
                }, 2, "e2stage1-clear");
                return;
            }
            if (session->busy() || *age < .8f)
                return;
            auto click = [session, age, &data](int id, int cursor = 0, int item = 0) {
                auto* room = session->room();
                auto p     = room->background()->convertToWorldSpace(room->objectPosition(id));
                if (room->hitObject(p) != id || p.y < 0 || p.y > 320)
                    for (auto* area : data.where("AreaData", "objectID", id))
                    {
                        const int w = rb::integer(*area, "AreaWidth") / 2, h = rb::integer(*area, "AreaHeight") / 2;
                        if (w < 1 || h < 1)
                            continue;
                        auto q = room->background()->convertToWorldSpace(
                            {rb::integer(*area, "AreaX") / 2 + w / 2.f,
                             room->background()->getContentSize().height - rb::integer(*area, "AreaY") / 2 - h / 2.f});
                        if (q.y >= 0 && q.y <= 320 && room->hitObject(q) == id)
                        {
                            p = q;
                            break;
                        }
                    }
                if (p.x < 1 || p.x > 479)
                {
                    room->beginDrag();
                    room->drag((240 - p.x) / .8f);
                    room->drag(0);
                    room->settle();
                    *age = 0;
                    return false;
                }
                if (room->hitObject(p) != id)
                    throw std::runtime_error("E2Stage1 hit mismatch: " + std::to_string(id));
                session->selectCursor(cursor, item);
                session->tap(p);
                *age = 0;
                if (session->touchObject() != id)
                    throw std::runtime_error("E2Stage1 tap rejected");
                return true;
            };
            auto itemPoint = [session](int id) {
                auto ids = session->state().inventory();
                auto it  = std::find(ids.begin(), ids.end(), id);
                if (it == ids.end())
                    throw std::runtime_error("E2Stage1 item absent: " + std::to_string(id));
                int i = static_cast<int>(it - ids.begin());
                return Vec2(48 + (i % 5) * 65, 208 - (i / 5) * 55);
            };
            auto combine = [session, scene, age, itemPoint](int a, int b) {
                auto* inv = session->inventory();
                auto p = itemPoint(a), q = itemPoint(b);
                inv->beginTouch(p);
                scene->scheduleOnce([inv, q](float) {
                    inv->moveTouch(q);
                    inv->endTouch(q);
                }, .8f, "e2stage1-combine");
                *age = -2.5f;
            };
            bool next = false;
            switch (*step)
            {
            case 0:
                next = click(18);
                break;
            case 1:
                next = click(19);
                break;
            case 2:
                next = click(10);
                break;
            case 3:
                next = click(20);
                break;
            case 4:
                next = click(21);
                break;
            case 5:
                session->inventory()->open();
                session->inventory()->selectMode(3);
                *age = 0;
                next = true;
                break;
            case 6:
                combine(3, 4);
                next = true;
                break;
            case 7:
                session->inventory()->selectMode(2);
                *age = 0;
                next = true;
                break;
            case 8:
            {
                auto* inv = session->inventory();
                auto p    = itemPoint(6);
                inv->beginTouch(p);
                scene->scheduleOnce([inv, p](float) {
                    inv->moveTouch(p + Vec2(20, 0));
                    inv->moveTouch(p + Vec2(40, 0));
                    inv->endTouch(p + Vec2(40, 0));
                }, .8f, "e2stage1-split");
                *age = -1;
                next = true;
                break;
            }
            case 9:
                session->inventory()->selectMode(3);
                *age = 0;
                next = true;
                break;
            case 10:
                combine(7, 8);
                next = true;
                break;
            case 11:
                session->inventory()->close();
                *age = 0;
                next = true;
                break;
            case 12:
                next = click(16, 3, 5);
                break;
            case 13:
                next = click(16);
                break;
            case 14:
                next = click(8, 3, 9);
                break;
            case 15:
                next = click(16, 3, 1);
                break;
            case 16:
                next = click(17, 2);
                break;
            default:
                throw std::runtime_error("E2Stage1 exit failed");
            }
            if (next)
            {
                AXLOGI("E2Stage1 step {}", *step);
                ++*step;
            }
        }, .02f, "episode1-e2stage1");
        scene->scheduleOnce([](float) { throw std::runtime_error("E2Stage1 timeout"); },
                            std::getenv("RB_ROUTE_DEMO") ? 3600 : 300, "e2stage1-timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_EPISODE1_STAGE5"))
    {
        auto* owner    = std::getenv("RB_ROUTE_DEMO") ? dynamic_cast<MainScene*>(scene) : nullptr;
        routeDemoOwner = owner;
        auto progress  = owner ? owner->routeDemoProgress() : std::make_shared<rb::Progress>();
        if (!owner)
            progress->initialize(data);
        auto* session = owner ? owner->routeDemoSession() : GameSession::create(data, 1, 5, language, 1, false);
        if (!owner)
        {
            scene->addChild(session);
            session->bindProgress(*progress);
            session->begin();
        }
        auto step = std::make_shared<int>(0);
        auto age  = std::make_shared<float>(0);
        scene->schedule([scene, session, progress, step, age, directory, &data](float dt) {
            *age += dt;
            if (session->story())
            {
                advanceRouteStory(session);
                return;
            }
            if (session->itemDialog())
            {
                if (session->itemDialog()->ready())
                    session->itemDialog()->close();
                return;
            }
            if (auto* d = session->detail())
            {
                if (d->busy())
                    return;
                if (d->id() != 62)
                    throw std::runtime_error("Stage5 unexpected detail");
                for (int i : {1, 4, 6, 8, 9})
                {
                    auto p = d->controlCenter(i);
                    d->beginTouch(p);
                    d->endTouch(p);
                }
                return;
            }
            if (session->clearOverlay())
            {
                if (*step != 25)
                    throw std::runtime_error("Stage5 premature clear");
                scene->unschedule("episode1-stage5");
                scene->scheduleOnce([directory](float) {
                    utils::captureScreen([](bool ok, std::string_view) {
                        if (!ok)
                            throw std::runtime_error("Stage5 capture failed");
                        AXLOGI("RB_PROBE_EPISODE1_STAGE5 PASS");
                        finishRoute();
                    }, std::string(directory) + "/episode1-stage5-clear.png");
                }, 2, "stage5-clear");
                return;
            }
            if (session->busy() || *age < .8f)
                return;
            auto click = [session, age, &data](int id, int cursor = 0, int item = 0) {
                auto* room = session->room();
                auto p     = room->background()->convertToWorldSpace(room->objectPosition(id));
                if (room->hitObject(p) != id || p.y < 0 || p.y > 320)
                    for (auto* area : data.where("AreaData", "objectID", id))
                    {
                        const int w = rb::integer(*area, "AreaWidth") / 2, h = rb::integer(*area, "AreaHeight") / 2;
                        if (w < 1 || h < 1)
                            continue;
                        auto q = room->background()->convertToWorldSpace(
                            {rb::integer(*area, "AreaX") / 2 + w / 2.f,
                             room->background()->getContentSize().height - rb::integer(*area, "AreaY") / 2 - h / 2.f});
                        if (q.y >= 0 && q.y <= 320 && room->hitObject(q) == id)
                        {
                            p = q;
                            break;
                        }
                    }
                if (p.x < 1 || p.x > 479)
                {
                    room->beginDrag();
                    room->drag((240 - p.x) / .8f);
                    room->drag(0);
                    room->settle();
                    *age = 0;
                    return false;
                }
                if (room->hitObject(p) != id)
                    throw std::runtime_error("Stage5 hit mismatch: " + std::to_string(id));
                session->selectCursor(cursor, item);
                session->tap(p);
                *age = 0;
                if (session->touchObject() != id)
                    throw std::runtime_error("Stage5 tap rejected");
                return true;
            };
            auto itemPoint = [session](int id) {
                auto ids = session->state().inventory();
                auto it  = std::find(ids.begin(), ids.end(), id);
                if (it == ids.end())
                    throw std::runtime_error("Stage5 item absent: " + std::to_string(id));
                int i = static_cast<int>(it - ids.begin());
                return Vec2(48 + (i % 5) * 65, 208 - (i / 5) * 55);
            };
            auto combine = [session, scene, age, itemPoint](int a, int b) {
                auto* inv = session->inventory();
                auto p = itemPoint(a), q = itemPoint(b);
                inv->beginTouch(p);
                scene->scheduleOnce([inv, q](float) {
                    inv->moveTouch(q);
                    inv->endTouch(q);
                }, .8f, "stage5-combine");
                *age = -2.5f;
            };
            bool next = false;
            switch (*step)
            {
            case 0:
                next = click(933);
                break;
            case 1:
                next = click(942, 0, 0);
                break;
            case 2:
                next = click(951, 0, 0);
                break;
            case 3:
                next = click(934, 3, 200);
                break;
            case 4:
                next = click(934, 0, 0);
                break;
            case 5:
                next = click(946, 0, 0);
                break;
            case 6:
                next = click(936, 0, 0);
                break;
            case 7:
                next = click(1202, 0, 0);
                break;
            case 8:
                next = click(938, 0, 0);
                break;
            case 9:
                next = click(948, 0, 0);
                break;
            case 10:
                session->inventory()->open();
                session->inventory()->selectMode(2);
                *age = 0;
                next = true;
                break;
            case 11:
            {
                auto* inv = session->inventory();
                auto p    = itemPoint(214);
                inv->beginTouch(p);
                scene->scheduleOnce([inv, p](float) {
                    inv->moveTouch(p + Vec2(20, 0));
                    inv->moveTouch(p + Vec2(40, 0));
                    inv->endTouch(p + Vec2(40, 0));
                }, .8f, "stage5-split");
                *age = -1;
                next = true;
                break;
            }
            case 12:
                session->inventory()->selectMode(3);
                *age = 0;
                next = true;
                break;
            case 13:
                combine(199, 201);
                next = true;
                break;
            case 14:
                session->inventory()->close();
                *age = 0;
                next = true;
                break;
            case 15:
                next = click(943, 3, 202);
                break;
            case 16:
                next = click(943, 3, 203);
                break;
            case 17:
                next = click(943, 2);
                break;
            case 18:
                next = click(963);
                break;
            case 19:
                session->inventory()->open();
                session->inventory()->selectMode(3);
                *age = 0;
                next = true;
                break;
            case 20:
                combine(210, 212);
                next = true;
                break;
            case 21:
                session->inventory()->close();
                *age = 0;
                next = true;
                break;
            case 22:
                next = click(960, 3, 211);
                break;
            case 23:
                next = click(961);
                break;
            case 24:
                next = click(959, 2);
                break;
            default:
                throw std::runtime_error("Stage5 exit failed");
            }
            if (next)
            {
                AXLOGI("Stage5 step {}", *step);
                ++*step;
            }
        }, .02f, "episode1-stage5");
        scene->scheduleOnce([](float) { throw std::runtime_error("Stage5 timeout"); },
                            std::getenv("RB_ROUTE_DEMO") ? 3600 : 300, "stage5-timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_EPISODE1_STAGE4"))
    {
        auto* owner    = std::getenv("RB_ROUTE_DEMO") ? dynamic_cast<MainScene*>(scene) : nullptr;
        routeDemoOwner = owner;
        auto progress  = owner ? owner->routeDemoProgress() : std::make_shared<rb::Progress>();
        if (!owner)
            progress->initialize(data);
        auto* session = owner ? owner->routeDemoSession() : GameSession::create(data, 1, 4, language, 1, false);
        if (!owner)
        {
            scene->addChild(session);
            session->bindProgress(*progress);
            session->begin();
        }
        auto step = std::make_shared<int>(0);
        auto age  = std::make_shared<float>(0);
        scene->schedule([scene, session, progress, step, age, directory, &data](float dt) {
            *age += dt;
            if (session->story())
            {
                advanceRouteStory(session);
                return;
            }
            if (session->itemDialog())
            {
                if (session->itemDialog()->ready())
                    session->itemDialog()->close();
                return;
            }
            if (auto* d = session->detail())
            {
                if (d->busy() || *age < .4f)
                    return;
                if (d->id() == 60)
                {
                    auto* m = dynamic_cast<DetailMonitorControl*>(d);
                    if (*age < 2)
                        return;
                    auto p = m->controlCenter(1);
                    m->beginTouch(p);
                    m->endTouch(p);
                    if (m->selectedPage() == 0)
                        m->close();
                    *age = 0;
                    return;
                }
                if (d->id() == 56)
                {
                    for (int i : {3, 5, 2, 0})
                    {
                        auto p = d->controlCenter(i);
                        d->beginTouch(p);
                        d->endTouch(p);
                    }
                    return;
                }
                if (auto* g = dynamic_cast<DetailGearHandle*>(d))
                {
                    if (g->moving())
                        return;
                    for (int i = 0; i < g->count(); ++i)
                    {
                        auto a = g->angle(i);
                        if (a == 0)
                            continue;
                        auto p = g->controlCenter(i);
                        g->beginTouch(p);
                        g->moveTouch(p + Vec2(a > 180 || a < 0 ? 11 : -11, 0));
                        g->endTouch(p);
                        *age = 0;
                        return;
                    }
                    return;
                }
                if (auto* safe = dynamic_cast<DetailSafe*>(d))
                {
                    const std::vector<int> password =
                        d->id() == 57 ? std::vector<int>{1, 8, 5, 6}
                                      : (d->id() == 58 ? std::vector<int>{7, 5, 5, 3} : std::vector<int>{6, 4, 9, 4});
                    for (int col = 0; col < 4; ++col)
                        if (safe->entered()[col] != password[col])
                        {
                            auto p = safe->controlCenter(col);
                            safe->beginTouch(p);
                            safe->moveTouch(p + Vec2(0, 50));
                            safe->endTouch(p + Vec2(0, 50));
                            *age = 0;
                            return;
                        }
                    return;
                }
                throw std::runtime_error("Stage4 unexpected detail");
            }
            if (session->clearOverlay())
            {
                if (*step != 23)
                    throw std::runtime_error("Stage4 premature clear");
                scene->unschedule("episode1-stage4");
                scene->scheduleOnce([directory](float) {
                    utils::captureScreen([](bool ok, std::string_view) {
                        if (!ok)
                            throw std::runtime_error("Stage4 capture failed");
                        AXLOGI("RB_PROBE_EPISODE1_STAGE4 PASS");
                        finishRoute();
                    }, std::string(directory) + "/episode1-stage4-clear.png");
                }, 2, "stage4-clear");
                return;
            }
            if (session->busy() || *age < .8f)
                return;
            auto click = [session, age, &data](int id, int cursor = 0, int item = 0) {
                auto* room = session->room();
                auto p     = room->background()->convertToWorldSpace(room->objectPosition(id));
                if (room->hitObject(p) != id || p.y < 0 || p.y > 320)
                    for (auto* area : data.where("AreaData", "objectID", id))
                    {
                        const int w = rb::integer(*area, "AreaWidth") / 2, h = rb::integer(*area, "AreaHeight") / 2;
                        if (w < 1 || h < 1)
                            continue;
                        auto q = room->background()->convertToWorldSpace(
                            {rb::integer(*area, "AreaX") / 2 + w / 2.f,
                             room->background()->getContentSize().height - rb::integer(*area, "AreaY") / 2 - h / 2.f});
                        if (q.y >= 0 && q.y <= 320 && room->hitObject(q) == id)
                        {
                            p = q;
                            break;
                        }
                    }
                if (p.x < 1 || p.x > 479)
                {
                    room->beginDrag();
                    room->drag((240 - p.x) / .8f);
                    room->drag(0);
                    room->settle();
                    *age = 0;
                    return false;
                }
                if (room->hitObject(p) != id)
                    throw std::runtime_error("Stage4 hit mismatch: " + std::to_string(id));
                session->selectCursor(cursor, item);
                session->tap(p);
                *age = 0;
                if (session->touchObject() != id)
                    throw std::runtime_error("Stage4 tap rejected");
                return true;
            };
            bool next = false;
            switch (*step)
            {
            case 0:
                next = click(888, 0, 0);
                break;
            case 1:
                next = click(883, 0, 0);
                break;
            case 2:
                next = click(884, 3, 190);
                break;
            case 3:
                next = click(885, 3, 190);
                break;
            case 4:
                next = click(886, 0, 0);
                break;
            case 5:
                next = click(890, 3, 192);
                break;
            case 6:
                next = click(890, 0, 0);
                break;
            case 7:
                next = click(891, 2, 0);
                break;
            case 8:
                next = click(901, 0, 0);
                break;
            case 9:
                next = click(914, 0, 0);
                break;
            case 10:
                next = click(917, 3, 194);
                break;
            case 11:
                next = click(903, 0, 0);
                break;
            case 12:
                next = click(898, 0, 0);
                break;
            case 13:
                next = click(913, 0, 0);
                break;
            case 14:
                next = click(899, 0, 0);
                break;
            case 15:
                next = click(912, 0, 0);
                break;
            case 16:
                next = click(900, 0, 0);
                break;
            case 17:
                next = click(911, 0, 0);
                break;
            case 18:
                next = click(1198, 3, 195);
                break;
            case 19:
                next = click(1198, 3, 196);
                break;
            case 20:
                next = click(1198, 3, 197);
                break;
            case 21:
                next = click(908, 0, 0);
                break;
            case 22:
                next = click(910, 2, 0);
                break;
            default:
                throw std::runtime_error("Stage4 exit failed");
            }
            if (next)
            {
                AXLOGI("Stage4 step {}", *step);
                ++*step;
            }
        }, .02f, "episode1-stage4");
        scene->scheduleOnce([](float) { throw std::runtime_error("Stage4 timeout"); },
                            std::getenv("RB_ROUTE_DEMO") ? 3600 : 300, "stage4-timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_EPISODE1_STAGE3"))
    {
        auto* owner    = std::getenv("RB_ROUTE_DEMO") ? dynamic_cast<MainScene*>(scene) : nullptr;
        routeDemoOwner = owner;
        auto progress  = owner ? owner->routeDemoProgress() : std::make_shared<rb::Progress>();
        if (!owner)
            progress->initialize(data);
        auto* session = owner ? owner->routeDemoSession() : GameSession::create(data, 1, 3, language, 1, false);
        if (!owner)
        {
            scene->addChild(session);
            session->bindProgress(*progress);
            session->begin();
        }
        auto step = std::make_shared<int>(0);
        auto age  = std::make_shared<float>(0);
        scene->schedule([scene, session, progress, step, age, directory, &data](float dt) {
            *age += dt;
            if (session->story())
            {
                advanceRouteStory(session);
                return;
            }
            if (session->itemDialog())
            {
                if (session->itemDialog()->ready())
                    session->itemDialog()->close();
                return;
            }
            if (auto* d = session->detail())
            {
                if (d->busy())
                    return;
                if (d->id() == 52)
                {
                    d->close();
                    return;
                }
                if (d->id() == 50)
                {
                    if (*age > 16)
                        d->close();
                    return;
                }
                auto* safe = dynamic_cast<DetailArrowSafe*>(d);
                if (!safe)
                    throw std::runtime_error("Stage3 unexpected detail");
                for (int i : {0, 1, 2, 3, 0, 3})
                {
                    auto p = safe->controlCenter(i);
                    safe->beginTouch(p);
                    safe->endTouch(p);
                }
                return;
            }
            if (auto* cut = dynamic_cast<ActionCut*>(session->actionMode()))
            {
                if (cut->menuVisible())
                    throw std::runtime_error("Stage3 cut failed");
                if (cut->busy())
                    return;
                const Vec2 vectors[] = {{-1, 0}, {-1, 1}, {0, 1}, {1, 1}, {1, 0}, {1, -1}, {0, -1}, {-1, -1}};
                const auto delta     = vectors[cut->direction()].getNormalized() * 160;
                auto p               = Vec2(240, 160) - delta / 2;
                cut->beginTouch(p);
                cut->moveTouch(p + delta);
                cut->endTouch(p + delta);
                return;
            }
            if (session->clearOverlay())
            {
                if (*step != 23)
                    throw std::runtime_error("Stage3 premature clear");
                scene->unschedule("episode1-stage3");
                scene->scheduleOnce([directory](float) {
                    utils::captureScreen([](bool ok, std::string_view) {
                        if (!ok)
                            throw std::runtime_error("Stage3 capture failed");
                        AXLOGI("RB_PROBE_EPISODE1_STAGE3 PASS");
                        finishRoute();
                    }, std::string(directory) + "/episode1-stage3-clear.png");
                }, 2, "stage3-clear");
                return;
            }
            if (session->busy() || *age < .8f)
                return;
            auto click = [session, age, &data](int id, int cursor = 0, int item = 0) {
                auto* room = session->room();
                auto p     = room->background()->convertToWorldSpace(room->objectPosition(id));
                if (room->hitObject(p) != id || p.y < 0 || p.y > 320)
                    for (auto* area : data.where("AreaData", "objectID", id))
                    {
                        const int w = rb::integer(*area, "AreaWidth") / 2, h = rb::integer(*area, "AreaHeight") / 2;
                        if (w < 1 || h < 1)
                            continue;
                        auto q = room->background()->convertToWorldSpace(
                            {rb::integer(*area, "AreaX") / 2 + w / 2.f,
                             room->background()->getContentSize().height - rb::integer(*area, "AreaY") / 2 - h / 2.f});
                        if (q.y >= 0 && q.y <= 320 && room->hitObject(q) == id)
                        {
                            p = q;
                            break;
                        }
                    }
                if (p.x < 1 || p.x > 479)
                {
                    room->beginDrag();
                    room->drag((240 - p.x) / .8f);
                    room->drag(0);
                    room->settle();
                    *age = 0;
                    return false;
                }
                if (room->hitObject(p) != id)
                    throw std::runtime_error("Stage3 hit mismatch: " + std::to_string(id));
                session->selectCursor(cursor, item);
                session->tap(p);
                *age = 0;
                if (session->touchObject() != id)
                    throw std::runtime_error("Stage3 tap rejected");
                return true;
            };
            bool next = false;
            switch (*step)
            {
            case 0:
                next = click(545);
                break;
            case 1:
                next = click(529, 3, 175);
                break;
            case 2:
                next = click(529, 1);
                break;
            case 3:
                next = click(549);
                break;
            case 4:
                next = click(552);
                break;
            case 5:
                next = click(535, 3, 181);
                break;
            case 6:
                next = click(535);
                break;
            case 7:
                next = click(547);
                break;
            case 8:
                next = click(536, 3, 181);
                break;
            case 9:
                next = click(536);
                break;
            case 10:
                next = click(548);
                break;
            case 11:
                session->inventory()->open();
                session->inventory()->selectMode(3);
                *age = 0;
                next = true;
                break;
            case 12:
            {
                auto* inv  = session->inventory();
                auto ids   = session->state().inventory();
                auto point = [ids](int id) {
                    auto it = std::find(ids.begin(), ids.end(), id);
                    if (it == ids.end())
                        throw std::runtime_error("Stage3 combine source absent");
                    int i = static_cast<int>(it - ids.begin());
                    return Vec2(48 + (i % 5) * 65, 208 - (i / 5) * 55);
                };
                auto p = point(177), q = point(178);
                inv->beginTouch(p);
                scene->scheduleOnce([inv, q](float) {
                    inv->moveTouch(q);
                    inv->endTouch(q);
                }, .8f, "stage3-combine");
                *age = -2.5f;
                next = true;
                break;
            }
            case 13:
                session->inventory()->close();
                *age = 0;
                next = true;
                break;
            case 14:
                next = click(712, 3, 179);
                break;
            case 15:
                next = click(541);
                break;
            case 16:
                next = click(551, 3, 172);
                break;
            case 17:
                next = click(526, 3, 180);
                break;
            case 18:
                next = click(543);
                break;
            case 19:
                next = click(526, 1);
                break;
            case 20:
                next = click(526, 3, 174);
                break;
            case 21:
                next = click(544, 1);
                break;
            case 22:
                next = click(544, 2);
                break;
            default:
                throw std::runtime_error("Stage3 exit failed");
            }
            if (next)
            {
                AXLOGI("Stage3 step {}", *step);
                ++*step;
            }
        }, .02f, "episode1-stage3");
        scene->scheduleOnce([](float) { throw std::runtime_error("Stage3 timeout"); },
                            std::getenv("RB_ROUTE_DEMO") ? 3600 : 240, "stage3-timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_BLIND"))
    {
        const int id = std::atoi(std::getenv("RB_PROBE_BLIND")), stage = id == 52 ? 4 : (id == 14 ? 2 : 3),
                  expected = id == 52 ? 1 : 2;
        auto* session      = GameSession::create(data, 1, stage, language, 1, false);
        scene->addChild(session);
        auto* room     = session->room();
        const int home = id == 52 ? 47 : session->state().room;
        room->loadRoom(home, id == 50 ? 4 : 2);
        if (!room->direction(id, [scene, room, directory, id, home, expected] {
            if (room->directionAnimationCount() != expected)
                throw std::runtime_error("Blind clue missing");
            if (id == 52)
                room->loadRoom(46);
            room->loadRoom(home, id == 50 ? 4 : 2);
            scene->scheduleOnce([room, directory, id, expected](float) {
                if (room->directionAnimationCount() != expected)
                    throw std::runtime_error("Blind clue lost on room change");
                utils::captureScreen([id](bool ok, std::string_view) {
                    if (!ok)
                        throw std::runtime_error("Blind clue capture failed");
                    AXLOGI("RB_PROBE_BLIND {} PASS", id);
                    Director::getInstance()->end();
                }, std::string(directory) + "/blind" + std::to_string(id) + ".png");
            }, 1, "blind-capture");
        }))
            throw std::runtime_error("Blind direction missing");
        return true;
    }
    if (std::getenv("RB_PROBE_EP3_LOOPS"))
    {
        int stage     = std::atoi(std::getenv("RB_PROBE_EP3_LOOPS"));
        auto* session = GameSession::create(data, 3, stage, language, 1, false);
        scene->addChild(session);
        auto* room                              = session->room();
        int home                                = session->state().room;
        const std::vector<std::vector<int>> all = {{55}, {59, 60}, {63, 64, 65}, {67, 68}, {70}};
        auto ids                                = all.at(stage - 1);
        auto index                              = std::make_shared<int>(0);
        auto waiting                            = std::make_shared<bool>(false);
        scene->schedule([scene, room, home, stage, ids, index, waiting, directory](float) {
            if (*waiting)
                return;
            if (*index < int(ids.size()))
            {
                *waiting = true;
                room->direction(ids[(*index)++], [waiting] { *waiting = false; });
                return;
            }
            scene->unschedule("ep3loops-check");
            if (room->directionAnimationCount() != ids.size())
                throw std::runtime_error("EP3 loop registration failed");
            room->loadRoom(home, stage == 3 ? 4 : 0);
            scene->scheduleOnce([scene, room, stage, ids, directory](float) {
                if (room->directionAnimationCount() != ids.size())
                    throw std::runtime_error("EP3 loops lost on reload");
                utils::captureScreen([room, stage, ids](bool ok, std::string_view) {
                    if (!ok)
                        throw std::runtime_error("EP3 loop capture failed");
                    auto finish = [room, stage, ids] {
                        if (room->directionAnimationCount() != ids.size() - (stage < 5 ? 1 : 0))
                            throw std::runtime_error("EP3 TV removal failed");
                        AXLOGI("RB_PROBE_EP3_LOOPS {} PASS", stage);
                        Director::getInstance()->end();
                    };
                    if (stage == 5)
                        finish();
                    else
                    {
                        int remove[] = {56, 61, 66, 69};
                        room->direction(remove[stage - 1], finish);
                    }
                }, std::string(directory) + "/ep3loops-" + std::to_string(stage) + ".png");
            }, 1, "ep3loops-capture");
        }, .5f, "ep3loops-check");
        scene->scheduleOnce([](float) { throw std::runtime_error("EP3 loops timeout"); }, 12, "ep3loops-timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_ROOM_LOOPS"))
    {
        auto* session = GameSession::create(data, 1, 3, language, 1, false);
        scene->addChild(session);
        auto* room     = session->room();
        const int home = session->state().room;
        room->direction(75, [scene, room, home, directory] {
            if (room->directionAnimationCount() != 1)
                throw std::runtime_error("Fire animation absent");
            room->direction(74, [scene, room, home, directory] {
                if (room->directionAnimationCount() != 2)
                    throw std::runtime_error("Water animation absent");
                scene->scheduleOnce([scene, room, home, directory](float) {
                    utils::captureScreen([](bool ok, std::string_view) {
                        if (!ok)
                            throw std::runtime_error("Room loops capture failed");
                    }, std::string(directory) + "/room-loops.png");
                    room->loadRoom(home);
                    if (room->directionAnimationCount() != 2)
                        throw std::runtime_error("Room animations lost on revisit");
                    room->direction(76, [room] {
                        if (room->directionAnimationCount() != 1)
                            throw std::runtime_error("Fire removal failed");
                        AXLOGI("RB_PROBE_ROOM_LOOPS PASS");
                        Director::getInstance()->end();
                    });
                }, 1, "room-loops-capture");
            });
        });
        scene->scheduleOnce([](float) { throw std::runtime_error("Room loops timeout"); }, 10, "room-loops-timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_SYMBOL_LOCK"))
    {
        auto* d = dynamic_cast<DetailElectronicDoorLock*>(DetailView::create(data, 56, {120, 120}, language, 1));
        if (!d)
            throw std::runtime_error("Symbol lock class missing");
        scene->addChild(d);
        d->open();
        auto phase  = std::make_shared<int>(0);
        d->complete = [](bool pass) {
            if (!pass)
                throw std::runtime_error("Symbol lock rejected code");
            AXLOGI("RB_PROBE_SYMBOL_LOCK PASS");
            Director::getInstance()->end();
        };
        scene->schedule([scene, d, phase, directory](float) {
            if (d->busy())
                return;
            auto click = [d](int i) {
                auto p = d->controlCenter(i);
                d->beginTouch(p);
                d->endTouch(p);
            };
            if (*phase == 0)
            {
                auto p = d->controlCenter(0);
                d->beginTouch(p);
                d->endTouch({0, 0});
                if (!d->entered().empty())
                    throw std::runtime_error("Symbol release outside accepted");
                click(1);
                d->retry();
                if (!d->entered().empty())
                    throw std::runtime_error("Symbol retry failed");
                for (int i = 0; i < 4; ++i)
                    click(1);
                if (!d->busy())
                    throw std::runtime_error("Symbol wrong input not checked");
                ++*phase;
                return;
            }
            if (*phase == 1)
            {
                if (!d->entered().empty())
                    throw std::runtime_error("Symbol wrong input not cleared");
                for (int i : {3, 5, 2, 0})
                    click(i);
                ++*phase;
                scene->unschedule("symbol-lock-check");
                scene->scheduleOnce([directory](float) {
                    utils::captureScreen([](bool ok, std::string_view) {
                        if (!ok)
                            throw std::runtime_error("Symbol capture failed");
                    }, std::string(directory) + "/symbol-lock.png");
                }, .35f, "symbol-capture");
            }
        }, .2f, "symbol-lock-check");
        scene->scheduleOnce([](float) { throw std::runtime_error("Symbol lock timeout"); }, 20, "symbol-lock-timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_ARROW_SAFE"))
    {
        auto* d = dynamic_cast<DetailArrowSafe*>(DetailView::create(data, 51, {120, 120}, language, 1));
        if (!d)
            throw std::runtime_error("Arrow class missing");
        scene->addChild(d);
        d->open();
        auto phase  = std::make_shared<int>(0);
        d->complete = [](bool pass) {
            if (!pass)
                throw std::runtime_error("Arrow safe rejected code");
            AXLOGI("RB_PROBE_ARROW_SAFE PASS");
            Director::getInstance()->end();
        };
        scene->schedule([scene, d, phase, directory](float) {
            if (d->busy())
                return;
            auto click = [d](int i) {
                auto p = d->controlCenter(i);
                d->beginTouch(p);
                d->endTouch(p);
            };
            if (*phase == 0)
            {
                auto p = d->controlCenter(0);
                d->beginTouch(p);
                d->endTouch({0, 0});
                if (!d->entered().empty())
                    throw std::runtime_error("Arrow release outside accepted");
                click(1);
                d->retry();
                if (!d->entered().empty())
                    throw std::runtime_error("Arrow retry failed");
                for (int i = 0; i < 6; ++i)
                    click(1);
                if (!d->busy())
                    throw std::runtime_error("Arrow wrong input not checked");
                ++*phase;
                return;
            }
            if (*phase == 1)
            {
                if (!d->entered().empty())
                    throw std::runtime_error("Arrow wrong input not cleared");
                utils::captureScreen([](bool ok, std::string_view) {
                    if (!ok)
                        throw std::runtime_error("Arrow capture failed");
                }, std::string(directory) + "/arrow-safe.png");
                for (int i : {0, 1, 2, 3, 0, 3})
                    click(i);
                ++*phase;
                scene->unschedule("arrow-safe-check");
            }
        }, .2f, "arrow-safe-check");
        scene->scheduleOnce([](float) { throw std::runtime_error("Arrow safe timeout"); }, 20, "arrow-safe-timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_DUST"))
    {
        auto* d = dynamic_cast<DetailDustingFrame*>(DetailView::create(data, 67, {120, 120}, language, 1));
        if (!d)
            throw std::runtime_error("Dust class missing");
        scene->addChild(d);
        d->open();
        auto phase  = std::make_shared<int>(0);
        d->complete = [](bool pass) {
            if (!pass)
                throw std::runtime_error("Dust result failed");
            AXLOGI("RB_PROBE_DUST PASS");
            Director::getInstance()->end();
        };
        scene->schedule([scene, d, phase, directory](float) {
            if (d->busy())
                return;
            auto p = d->controlCenter(0);
            if (*phase == 0)
            {
                d->beginTouch(p);
                d->moveTouch(p + Vec2(80, 0));
                d->endTouch(p + Vec2(80, 0));
                if (d->remaining() != 6 || d->busy())
                    throw std::runtime_error("Dust threshold mismatch");
                ++*phase;
            }
            if (*phase == 1)
            {
                d->beginTouch(p);
                d->moveTouch(p + Vec2(81, 0));
                d->endTouch(p + Vec2(81, 0));
                ++*phase;
                return;
            }
            if (*phase == 2)
            {
                if (d->remaining() != 5)
                    throw std::runtime_error("Dust single wipe failed");
                d->retry();
                if (d->remaining() != 6)
                    throw std::runtime_error("Dust reset failed");
                ++*phase;
                return;
            }
            d->beginTouch(p);
            d->moveTouch(p + Vec2(45, 0));
            d->moveTouch(p);
            d->endTouch(p);
            if (d->remaining() == 1)
            {
                scene->unschedule("dust-check");
                scene->scheduleOnce([directory](float) {
                    utils::captureScreen([](bool ok, std::string_view) {
                        if (!ok)
                            throw std::runtime_error("Dust capture failed");
                    }, std::string(directory) + "/dust.png");
                }, 1.1f, "dust-capture");
            }
        }, .6f, "dust-check");
        scene->scheduleOnce([](float) { throw std::runtime_error("Dust timeout"); }, 25, "dust-timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_WINE"))
    {
        auto* d = dynamic_cast<DetailWineBox*>(DetailView::create(data, 11, {120, 120}, language, 1));
        if (!d)
            throw std::runtime_error("Wine class missing");
        scene->addChild(d);
        d->open();
        d->complete = [](bool pass) {
            if (!pass)
                throw std::runtime_error("Wine result failed");
            AXLOGI("RB_PROBE_WINE PASS");
            Director::getInstance()->end();
        };
        scene->schedule([scene, d, directory](float) {
            if (d->busy())
                return;
            auto click = [d](int i) {
                auto p = d->controlCenter(i);
                d->beginTouch(p);
                d->endTouch(p);
            };
            auto p = d->controlCenter(0);
            d->beginTouch(p);
            d->endTouch({0, 0});
            if (d->entered()[0])
                throw std::runtime_error("Wine outside release accepted");
            for (int j = 0; j < 3; ++j)
                click(1);
            if (d->entered()[1])
                throw std::runtime_error("Wine cycle failed");
            click(3);
            click(14);
            for (int v : d->entered())
                if (v)
                    throw std::runtime_error("Wine center clear failed");
            click(12);
            d->retry();
            for (int v : d->entered())
                if (v)
                    throw std::runtime_error("Wine retry failed");
            for (int i = 0; i < 14; ++i)
            {
                click(i);
                if (i % 2 == 0 || i == 13)
                    click(i);
            }
            scene->unschedule("wine-check");
            scene->scheduleOnce([directory](float) {
                utils::captureScreen([](bool ok, std::string_view) {
                    if (!ok)
                        throw std::runtime_error("Wine capture failed");
                }, std::string(directory) + "/wine.png");
            }, .4f, "wine-capture");
        }, .5f, "wine-check");
        scene->scheduleOnce([](float) { throw std::runtime_error("Wine timeout"); }, 15, "wine-timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_TELEVISION"))
    {
        auto* d = DetailView::create(data, 96, {120, 120}, language, 1);
        scene->addChild(d);
        d->open();
        d->complete = [](bool pass) {
            if (pass)
                throw std::runtime_error("TV unexpected success");
            AXLOGI("RB_PROBE_TELEVISION PASS");
            Director::getInstance()->end();
        };
        scene->scheduleOnce([d, directory](float) {
            utils::captureScreen([](bool ok, std::string_view) {
                if (!ok)
                    throw std::runtime_error("TV capture");
            }, std::string(directory) + "/television.png");
        }, 2.35f, "television-capture");
        scene->scheduleOnce([d](float) { d->close(); }, 4.5f, "television-close");
        return true;
    }
    if (std::getenv("RB_PROBE_ELEVATOR"))
    {
        auto* d = dynamic_cast<DetailElevator*>(DetailView::create(data, 72, {120, 120}, language, 1));
        scene->addChild(d);
        d->open();
        auto phase  = std::make_shared<int>(0);
        d->complete = [](bool pass) {
            if (!pass)
                throw std::runtime_error("Elevator failed");
            AXLOGI("RB_PROBE_ELEVATOR PASS");
            Director::getInstance()->end();
        };
        scene->schedule([scene, d, phase, directory](float) {
            if (d->busy())
                return;
            auto tap = [d](int i) {
                auto p = d->controlCenter(i);
                d->beginTouch(p);
                d->endTouch(p);
            };
            if (*phase == 0)
            {
                d->beginTouch(d->controlCenter(2));
                d->endTouch({0, 0});
                if (!d->entered().empty())
                    throw std::runtime_error("Elevator outside release");
                tap(4);
                d->retry();
                if (!d->entered().empty())
                    throw std::runtime_error("Elevator retry");
                tap(1);
                tap(1);
                ++*phase;
                return;
            }
            if (!d->entered().empty())
                throw std::runtime_error("Elevator wrong reset");
            tap(2);
            tap(8);
            scene->unschedule("elevator-check");
            scene->scheduleOnce([directory](float) {
                utils::captureScreen([](bool ok, std::string_view) {
                    if (!ok)
                        throw std::runtime_error("Elevator capture");
                }, std::string(directory) + "/elevator.png");
            }, .5f, "elevator-capture");
        }, .6f, "elevator-check");
        scene->scheduleOnce([](float) { throw std::runtime_error("Elevator timeout"); }, 15, "elevator-timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_WOMAN"))
    {
        auto* d = dynamic_cast<DetailWomanStatue*>(DetailView::create(data, 70, {120, 120}, language, 1));
        scene->addChild(d);
        d->open();
        auto phase  = std::make_shared<int>(0);
        d->complete = [](bool pass) {
            if (!pass)
                throw std::runtime_error("Woman failed");
            AXLOGI("RB_PROBE_WOMAN PASS");
            Director::getInstance()->end();
        };
        scene->schedule([scene, d, phase, directory](float) {
            if (d->busy())
                return;
            auto slide = [d](int i, float dx) {
                auto p = d->controlCenter(i);
                d->beginTouch(p);
                d->moveTouch(p + Vec2(dx, 0));
                d->endTouch(p + Vec2(dx, 0));
            };
            if (*phase == 0)
            {
                d->beginTouch({0, 0});
                d->moveTouch({20, 0});
                d->endTouch({20, 0});
                slide(0, d->digitWidth());
                if (d->entered()[0] != 9)
                    throw std::runtime_error("Woman wrap failed");
                ++*phase;
                return;
            }
            if (*phase == 1)
            {
                d->retry();
                ++*phase;
                return;
            }
            if (*phase == 2)
            {
                if (d->entered()[0] != 9)
                    throw std::runtime_error("Woman retry input changed");
                auto p = d->controlCenter(0);
                d->beginTouch(p);
                d->endTouch(p);
                if (d->entered()[0] != 0)
                    throw std::runtime_error("Woman retry position failed");
                ++*phase;
                return;
            }
            const int target[] = {2, 6, 1, 3};
            bool done          = true;
            for (int c = 0; c < 4; ++c)
                if (d->entered()[c] != target[c])
                {
                    slide(c, d->digitWidth());
                    done = c == 3 && d->entered()[3] == 3;
                    break;
                }
            if (!done)
                return;
            scene->unschedule("woman-check");
            scene->scheduleOnce([directory](float) {
                utils::captureScreen([](bool ok, std::string_view) {
                    if (!ok)
                        throw std::runtime_error("Woman capture");
                }, std::string(directory) + "/woman.png");
            }, .5f, "woman-capture");
        }, .6f, "woman-check");
        scene->scheduleOnce([](float) { throw std::runtime_error("Woman timeout"); }, 40, "woman-timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_CENTER"))
    {
        auto* d = dynamic_cast<DetailCenterSafe*>(DetailView::create(data, 48, {120, 120}, language, 1));
        scene->addChild(d);
        d->open();
        auto phase  = std::make_shared<int>(0);
        d->complete = [](bool pass) {
            if (!pass)
                throw std::runtime_error("Center failed");
            AXLOGI("RB_PROBE_CENTER PASS");
            Director::getInstance()->end();
        };
        scene->schedule([scene, d, phase, directory](float) {
            if (d->busy())
                return;
            auto slide = [d](int i, float dy) {
                auto p = d->controlCenter(i);
                d->beginTouch(p);
                d->moveTouch(p + Vec2(0, dy));
                d->endTouch(p + Vec2(0, dy));
            };
            if (*phase == 0)
            {
                slide(2, 30);
                if (d->entered()[0])
                    throw std::runtime_error("Center distant block moved");
                slide(0, -30);
                if (d->entered()[0])
                    throw std::runtime_error("Center wrong direction moved");
                slide(0, 2);
                if (d->entered()[0])
                    throw std::runtime_error("Center short drag moved");
                ++*phase;
                return;
            }
            if (*phase == 1)
            {
                slide(0, 30);
                if (d->entered()[0] != 1)
                    throw std::runtime_error("Center upward failed");
                ++*phase;
                return;
            }
            if (*phase == 2)
            {
                slide(0, -30);
                if (d->entered()[0])
                    throw std::runtime_error("Center downward failed");
                ++*phase;
                return;
            }
            if (*phase == 3)
            {
                slide(0, 30);
                ++*phase;
                return;
            }
            if (*phase == 4)
            {
                d->retry();
                ++*phase;
                return;
            }
            const int target[] = {1, 2, 2, 3, 2, 1, 2};
            bool done          = true;
            for (int c = 0; c < 7; ++c)
                if (d->entered()[c] != target[c])
                {
                    slide(c * 3 + d->entered()[c], 30);
                    done = c == 6 && d->entered()[6] == 2;
                    break;
                }
            if (!done)
                return;
            scene->unschedule("center-check");
            scene->scheduleOnce([directory](float) {
                utils::captureScreen([](bool ok, std::string_view) {
                    if (!ok)
                        throw std::runtime_error("Center capture");
                }, std::string(directory) + "/center.png");
            }, 1.f, "center-capture");
        }, .5f, "center-check");
        scene->scheduleOnce([](float) { throw std::runtime_error("Center timeout"); }, 30, "center-timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_DROPBOOK"))
    {
        auto* d = dynamic_cast<DetailDropBook*>(DetailView::create(data, 49, {120, 120}, language, 1));
        scene->addChild(d);
        d->open();
        auto phase  = std::make_shared<int>(0);
        d->complete = [](bool pass) {
            if (!pass)
                throw std::runtime_error("DropBook failed");
            AXLOGI("RB_PROBE_DROPBOOK PASS");
            Director::getInstance()->end();
        };
        scene->schedule([scene, d, phase, directory](float) {
            if (d->busy())
                return;
            auto click = [d](int i) {
                auto p = d->controlCenter(i);
                d->beginTouch(p);
                d->endTouch(p);
            };
            if (*phase == 0)
            {
                auto p = d->controlCenter(0);
                d->beginTouch(p);
                d->endTouch({0, 0});
                if (d->count())
                    throw std::runtime_error("DropBook outside accepted");
                click(0);
                d->retry();
                if (d->count())
                    throw std::runtime_error("DropBook retry");
                for (int i = 0; i < 9; ++i)
                    click(0);
                ++*phase;
                return;
            }
            if (d->count())
                throw std::runtime_error("DropBook wrong reset");
            for (int i : {4, 3, 2, 0, 1, 3, 0, 2, 0})
                click(i);
            scene->unschedule("dropbook-check");
            scene->scheduleOnce([directory](float) {
                utils::captureScreen([](bool ok, std::string_view) {
                    if (!ok)
                        throw std::runtime_error("DropBook capture");
                }, std::string(directory) + "/dropbook.png");
            }, .7f, "dropbook-capture");
        }, .8f, "dropbook-check");
        scene->scheduleOnce([](float) { throw std::runtime_error("DropBook timeout"); }, 20, "dropbook-timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_FURNITURE"))
    {
        auto* d = dynamic_cast<DetailFurnitureLeft*>(DetailView::create(data, 42, {120, 120}, language, 1));
        scene->addChild(d);
        d->open();
        auto phase  = std::make_shared<int>(0);
        d->complete = [](bool pass) {
            if (!pass)
                throw std::runtime_error("Furniture failed");
            AXLOGI("RB_PROBE_FURNITURE PASS");
            Director::getInstance()->end();
        };
        scene->schedule([scene, d, phase, directory](float) {
            if (d->busy())
                return;
            auto click = [d](int i) {
                auto p = d->controlCenter(i);
                d->beginTouch(p);
                d->endTouch(p);
            };
            if (*phase == 0)
            {
                auto p = d->controlCenter(0);
                d->beginTouch(p);
                d->endTouch({0, 0});
                click(2);
                if (!d->entered().empty())
                    throw std::runtime_error("Furniture inactive/outside accepted");
                click(1);
                d->retry();
                if (d->firstActive() || !d->entered().empty())
                    throw std::runtime_error("Furniture retry state");
                click(8);
                d->retry();
                if (!d->firstActive())
                    throw std::runtime_error("Furniture second group");
                for (int i : {0, 2, 0, 2, 0, 2, 0, 2})
                    click(i);
                ++*phase;
                return;
            }
            if (!d->entered().empty() || !d->firstActive())
                throw std::runtime_error("Furniture wrong reset");
            for (int i : {1, 8, 1, 4, 1, 8, 1, 4})
                click(i);
            scene->unschedule("furniture-check");
            scene->scheduleOnce([directory](float) {
                utils::captureScreen([](bool ok, std::string_view) {
                    if (!ok)
                        throw std::runtime_error("Furniture capture");
                }, std::string(directory) + "/furniture.png");
            }, .4f, "furniture-capture");
        }, .8f, "furniture-check");
        scene->scheduleOnce([](float) { throw std::runtime_error("Furniture timeout"); }, 20, "furniture-timeout");
        return true;
    }
    if (const char* value = std::getenv("RB_PROBE_ALIEN"))
    {
        const int id                           = std::atoi(value);
        const std::map<int, std::string> words = {{22, "pendant"}, {27, "earth"}, {28, "alien"}, {29, "crystal"},
                                                  {30, "english"}, {31, "peace"}, {73, "push"}};
        const auto word                        = words.at(id);
        auto* d = dynamic_cast<DetailAlienPad*>(DetailView::create(data, id, {120, 120}, language, 1));
        if (!d)
            throw std::runtime_error("Alien class missing");
        scene->addChild(d);
        d->open();
        auto phase  = std::make_shared<int>(0);
        d->complete = [](bool pass) {
            if (!pass)
                throw std::runtime_error("Alien result failed");
            AXLOGI("RB_PROBE_ALIEN PASS");
            Director::getInstance()->end();
        };
        scene->schedule([scene, d, phase, directory, word, id](float) {
            if (d->busy())
                return;
            auto p     = d->controlCenter(26);
            auto click = [d, p] {
                d->beginTouch(p);
                d->endTouch(p);
            };
            if (*phase == 0)
            {
                d->beginTouch(p);
                d->endTouch({0, 0});
                if (!d->entered().empty())
                    throw std::runtime_error("Alien outside accepted");
                click();
                d->retry();
                if (!d->entered().empty())
                    throw std::runtime_error("Alien retry failed");
                for (size_t i = 0; i < word.size(); ++i)
                    click();
                ++*phase;
                return;
            }
            if (*phase == 1)
            {
                if (!d->entered().empty())
                    throw std::runtime_error("Alien wrong word not cleared");
                ++*phase;
            }
            int target = word.at(d->entered().size()) - 'a', delta = (target - d->selected() + 26) % 26;
            if (delta > 13)
                delta -= 26;
            if (delta)
            {
                delta = std::clamp(delta, -3, 3);
                d->beginTouch(p);
                d->moveTouch(p + Vec2(-45.5f * delta, 0));
                d->endTouch(p + Vec2(-45.5f * delta, 0));
                return;
            }
            click();
            if (d->entered().size() == word.size())
            {
                scene->unschedule("alien-check");
                scene->scheduleOnce([directory, id](float) {
                    utils::captureScreen([](bool ok, std::string_view) {
                        if (!ok)
                            throw std::runtime_error("Alien capture failed");
                    }, std::string(directory) + "/alien-" + std::to_string(id) + ".png");
                }, .4f, "alien-capture");
            }
        }, .6f, "alien-check");
        scene->scheduleOnce([](float) { throw std::runtime_error("Alien timeout"); }, 120, "alien-timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_TIME"))
    {
        auto* d = dynamic_cast<DetailTimeControl*>(DetailView::create(data, 18, {120, 120}, language, 1));
        if (!d)
            throw std::runtime_error("Time class missing");
        scene->addChild(d);
        d->open();
        auto phase  = std::make_shared<int>(0);
        d->complete = [](bool pass) {
            if (!pass)
                throw std::runtime_error("Time result failed");
            AXLOGI("RB_PROBE_TIME PASS");
            Director::getInstance()->end();
        };
        scene->schedule([scene, d, phase, directory](float) {
            if (d->busy())
                return;
            auto p = d->controlCenter(0);
            if (*phase == 0)
            {
                d->beginTouch(p);
                d->endTouch({0, 0});
                if (d->targetIndex() != 0)
                    throw std::runtime_error("Time outside accepted");
                d->beginTouch(p);
                d->endTouch(p);
                ++*phase;
                return;
            }
            if (*phase == 1)
            {
                if (d->targetIndex() != 1)
                    throw std::runtime_error("Time failure target not advanced");
                for (int i = 0; i < 10; ++i)
                    d->retry();
                if (d->targetIndex() != 0)
                    throw std::runtime_error("Time target wrap failed");
                for (int i = 0; i < 5; ++i)
                    d->retry();
                d->beginTouch(p);
                scene->scheduleOnce([d, p, directory](float) {
                    d->endTouch(p);
                    utils::captureScreen([](bool ok, std::string_view) {
                        if (!ok)
                            throw std::runtime_error("Time capture failed");
                    }, std::string(directory) + "/time.png");
                }, 4.5f, "time-release");
                scene->unschedule("time-check");
                ++*phase;
            }
        }, .5f, "time-check");
        scene->scheduleOnce([](float) { throw std::runtime_error("Time timeout"); }, 20, "time-timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_BOOK"))
    {
        auto* d = dynamic_cast<DetailHistoricBook*>(DetailView::create(data, 20, {120, 120}, language, 1));
        if (!d)
            throw std::runtime_error("Book class missing");
        scene->addChild(d);
        d->open();
        auto phase  = std::make_shared<int>(0);
        d->complete = [](bool pass) {
            if (pass)
                throw std::runtime_error("Book unexpectedly solved");
            AXLOGI("RB_PROBE_BOOK PASS");
            Director::getInstance()->end();
        };
        scene->schedule([scene, d, phase, directory](float) {
            if (d->busy())
                return;
            auto p = d->controlCenter(0);
            int n  = (*phase)++;
            if (n == 8)
            {
                scene->unschedule("book-check");
                d->close();
                return;
            }
            float dx = n == 0 ? -40 : n < 5 ? -60 : 60;
            d->beginTouch(p);
            d->moveTouch(p + Vec2(dx, 0));
            d->endTouch(p + Vec2(dx, 0));
            int expected = n == 0 ? 0 : n < 4 ? n : n == 4 ? 3 : 7 - n;
            if (d->page() != expected)
                throw std::runtime_error("Book wrong page: phase=" + std::to_string(n) +
                                         " got=" + std::to_string(d->page()) + " expected=" + std::to_string(expected));
            if (n == 3)
                scene->scheduleOnce([directory](float) {
                    utils::captureScreen([](bool ok, std::string_view) {
                        if (!ok)
                            throw std::runtime_error("Book capture failed");
                    }, std::string(directory) + "/book.png");
                }, .6f, "book-capture");
        }, .8f, "book-check");
        scene->scheduleOnce([](float) { throw std::runtime_error("Book timeout"); }, 20, "book-timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_HISTORIC"))
    {
        auto* d = dynamic_cast<DetailHistoricSafe*>(DetailView::create(data, 17, {120, 120}, language, 1));
        if (!d)
            throw std::runtime_error("Historic class missing");
        scene->addChild(d);
        d->open();
        auto phase  = std::make_shared<int>(0);
        d->complete = [](bool pass) {
            if (!pass)
                throw std::runtime_error("Historic result failed");
            AXLOGI("RB_PROBE_HISTORIC PASS");
            Director::getInstance()->end();
        };
        scene->schedule([scene, d, phase, directory](float) {
            if (d->busy())
                return;
            auto click = [d](int i) {
                auto p = d->controlCenter(i);
                d->beginTouch(p);
                d->endTouch(p);
            };
            auto slide = [d](float dx) {
                auto p = d->controlCenter(10);
                d->beginTouch(p);
                d->moveTouch(p + Vec2(dx, 0));
                d->endTouch(p + Vec2(dx, 0));
            };
            switch ((*phase)++)
            {
            case 0:
            {
                auto p = d->controlCenter(0);
                d->beginTouch(p);
                d->endTouch({0, 0});
                if (!d->entered().empty())
                    throw std::runtime_error("Historic outside accepted");
                click(0);
                d->retry();
                if (!d->entered().empty())
                    throw std::runtime_error("Historic retry failed");
                for (int i = 0; i < 4; ++i)
                    click(0);
                break;
            }
            case 1:
                if (!d->entered().empty())
                    throw std::runtime_error("Historic wrong password not cleared");
                slide(-70);
                break;
            case 2:
                if (!d->entered().empty())
                    throw std::runtime_error("Historic slide entered a number");
                click(1);
                break;
            case 3:
                click(1);
                break;
            case 4:
                slide(70);
                break;
            case 5:
                click(0);
                break;
            case 6:
                click(0);
                scene->unschedule("historic-check");
                scene->scheduleOnce([directory](float) {
                    utils::captureScreen([](bool ok, std::string_view) {
                        if (!ok)
                            throw std::runtime_error("Historic capture failed");
                    }, std::string(directory) + "/historic.png");
                }, .4f, "historic-capture");
                break;
            }
        }, 1.f, "historic-check");
        scene->scheduleOnce([](float) { throw std::runtime_error("Historic timeout"); }, 20, "historic-timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_PYRAMID"))
    {
        auto* d = dynamic_cast<DetailPyramidBlock*>(DetailView::create(data, 13, {120, 120}, language, 1));
        if (!d)
            throw std::runtime_error("Pyramid class missing");
        scene->addChild(d);
        d->open();
        d->complete = [](bool pass) {
            if (!pass)
                throw std::runtime_error("Pyramid result failed");
            AXLOGI("RB_PROBE_PYRAMID PASS");
            Director::getInstance()->end();
        };
        scene->schedule([scene, d, directory](float) {
            if (d->busy())
                return;
            auto click = [d](int i) {
                auto p = d->controlCenter(i);
                d->beginTouch(p);
                d->endTouch(p);
            };
            auto p = d->controlCenter(0);
            d->beginTouch(p);
            d->endTouch({0, 0});
            if (d->entered()[0])
                throw std::runtime_error("Pyramid outside release accepted");
            click(1);
            click(1);
            if (d->entered()[1])
                throw std::runtime_error("Pyramid toggle failed");
            click(3);
            d->retry();
            for (bool v : d->entered())
                if (v)
                    throw std::runtime_error("Pyramid retry failed");
            for (int i : {0, 5, 8, 15})
                click(i);
            scene->unschedule("pyramid-check");
            scene->scheduleOnce([directory](float) {
                utils::captureScreen([](bool ok, std::string_view) {
                    if (!ok)
                        throw std::runtime_error("Pyramid capture failed");
                }, std::string(directory) + "/pyramid.png");
            }, .4f, "pyramid-capture");
        }, .5f, "pyramid-check");
        scene->scheduleOnce([](float) { throw std::runtime_error("Pyramid timeout"); }, 15, "pyramid-timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_SECRET"))
    {
        auto* d = dynamic_cast<DetailSecretPattern*>(DetailView::create(data, 9, {120, 120}, language, 1));
        if (!d)
            throw std::runtime_error("Secret class missing");
        scene->addChild(d);
        d->open();
        d->complete = [](bool pass) {
            if (!pass)
                throw std::runtime_error("Secret result failed");
            AXLOGI("RB_PROBE_SECRET PASS");
            Director::getInstance()->end();
        };
        scene->schedule([scene, d, directory](float) {
            if (d->busy())
                return;
            auto click = [d](int i) {
                auto p = d->controlCenter(i);
                d->beginTouch(p);
                d->endTouch(p);
            };
            auto p = d->controlCenter(0);
            d->beginTouch(p);
            d->endTouch({0, 0});
            if (d->entered()[0])
                throw std::runtime_error("Secret outside release accepted");
            click(1);
            click(1);
            if (d->entered()[1])
                throw std::runtime_error("Secret toggle failed");
            click(3);
            d->retry();
            for (bool v : d->entered())
                if (v)
                    throw std::runtime_error("Secret retry failed");
            for (int i : {0, 2, 4, 8})
                click(i);
            scene->unschedule("secret-check");
            scene->scheduleOnce([directory](float) {
                utils::captureScreen([](bool ok, std::string_view) {
                    if (!ok)
                        throw std::runtime_error("Secret capture failed");
                }, std::string(directory) + "/secret.png");
            }, .4f, "secret-capture");
        }, .5f, "secret-check");
        scene->scheduleOnce([](float) { throw std::runtime_error("Secret timeout"); }, 15, "secret-timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_WATCH"))
    {
        auto* d = dynamic_cast<DetailWatchBox*>(DetailView::create(data, 7, {120, 120}, language, 1));
        if (!d)
            throw std::runtime_error("Watch class missing");
        scene->addChild(d);
        d->open();
        auto phase  = std::make_shared<int>(0);
        d->complete = [](bool pass) {
            if (!pass)
                throw std::runtime_error("Watch result failed");
            AXLOGI("RB_PROBE_WATCH PASS");
            Director::getInstance()->end();
        };
        scene->schedule([scene, d, phase, directory](float) {
            if (d->busy())
                return;
            auto click = [d](int i) {
                auto p = d->controlCenter(i);
                d->beginTouch(p);
                d->endTouch(p);
            };
            if (*phase == 0)
            {
                auto p = d->controlCenter(0);
                d->beginTouch(p);
                d->endTouch({0, 0});
                if (d->entered()[0] != 0)
                    throw std::runtime_error("Watch outside release accepted");
            }
            if (*phase < 4)
            {
                click(3);
                ++*phase;
                return;
            }
            if (*phase == 4)
            {
                if (d->entered()[3] != 0)
                    throw std::runtime_error("Watch wrap failed");
                click(1);
                ++*phase;
                return;
            }
            if (*phase == 5)
            {
                d->retry();
                ++*phase;
                return;
            }
            if (*phase == 6)
            {
                for (int v : d->entered())
                    if (v)
                        throw std::runtime_error("Watch retry failed");
            }
            const int route[] = {0, 1, 1, 2, 2, 2};
            click(route[*phase - 6]);
            if (++*phase == 12)
            {
                scene->unschedule("watch-check");
                scene->scheduleOnce([directory](float) {
                    utils::captureScreen([](bool ok, std::string_view) {
                        if (!ok)
                            throw std::runtime_error("Watch capture failed");
                    }, std::string(directory) + "/watch.png");
                }, 1.1f, "watch-capture");
            }
        }, .65f, "watch-check");
        scene->scheduleOnce([](float) { throw std::runtime_error("Watch timeout"); }, 25, "watch-timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_CIRCUIT"))
    {
        auto* d = dynamic_cast<DetailCircuit*>(DetailView::create(data, 3, {120, 120}, language, 1));
        if (!d)
            throw std::runtime_error("Circuit class missing");
        scene->addChild(d);
        d->open();
        auto phase = std::make_shared<int>(0), index = std::make_shared<int>(0);
        auto age    = std::make_shared<float>(0);
        d->complete = [](bool pass) {
            if (!pass)
                throw std::runtime_error("Circuit rejected route");
            AXLOGI("RB_PROBE_CIRCUIT PASS");
            Director::getInstance()->end();
        };
        scene->schedule([scene, d, phase, index, age, directory](float dt) {
            *age += dt;
            if (*age < 1.5f || d->busy())
                return;
            auto click = [d](int i) {
                auto p = d->controlCenter(i);
                d->beginTouch(p);
                d->endTouch(p);
            };
            if (*phase == 0)
            {
                click(40);
                if (d->currentPoint() != 30)
                    throw std::runtime_error("Circuit accepted disconnected point");
                auto p = d->controlCenter(32);
                d->beginTouch(p);
                d->endTouch({0, 0});
                if (d->currentPoint() != 30)
                    throw std::runtime_error("Circuit accepted outside release");
                click(32);
                d->retry();
                ++*phase;
                return;
            }
            if (*phase == 1)
            {
                if (d->currentPoint() != 30)
                    throw std::runtime_error("Circuit retry failed");
                ++*phase;
                return;
            }
            if (*phase == 2)
            {
                const int route[] = {32, 34, 36, 38, 40, 42, 44, 14};
                click(route[*index]);
                if (d->currentPoint() != route[*index])
                    throw std::runtime_error("Circuit wrong-route input rejected");
                if (++*index == 8)
                {
                    ++*phase;
                    *index = 0;
                }
                return;
            }
            if (*phase == 3)
            {
                if (d->currentPoint() != 30)
                    return;
                ++*phase;
                return;
            }
            const int route[] = {32, 2, 4, 6, 8, 10, 40, 38, 36, 34, 64, 66, 68, 98, 100, 70, 72, 42, 44, 14};
            click(route[*index]);
            if (d->currentPoint() != route[*index])
                throw std::runtime_error("Circuit route rejected");
            if (*index == 16)
                scene->scheduleOnce([directory](float) {
                    utils::captureScreen([](bool ok, std::string_view) {
                        if (!ok)
                            throw std::runtime_error("Circuit capture failed");
                    }, std::string(directory) + "/circuit.png");
                }, .35f, "circuit-capture");
            if (++*index == 20)
                scene->unschedule("circuit-check");
        }, .5f, "circuit-check");
        scene->scheduleOnce([](float) { throw std::runtime_error("Circuit timeout"); }, 35, "circuit-timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_FINGER"))
    {
        int id  = std::atoi(std::getenv("RB_PROBE_FINGER"));
        auto* d = dynamic_cast<DetailFingerScan*>(DetailView::create(data, id, {120, 120}, language, 1));
        if (!d)
            throw std::runtime_error("Finger class missing");
        scene->addChild(d);
        d->open();
        auto phase  = std::make_shared<int>(0);
        d->complete = [id](bool pass) {
            if (!pass)
                throw std::runtime_error("Finger rejected correct input");
            AXLOGI("RB_PROBE_FINGER {} PASS", id);
            Director::getInstance()->end();
        };
        scene->schedule([scene, d, phase, directory, id](float) {
            if (d->busy())
                return;
            auto click = [d](int i) {
                auto p = d->controlCenter(i);
                d->beginTouch(p);
                d->endTouch(p);
            };
            if (*phase == 0)
            {
                auto p = d->controlCenter(0);
                d->beginTouch(p);
                d->endTouch({0, 0});
                if (d->entered()[0])
                    throw std::runtime_error("Finger outside release accepted");
                click(1);
                click(1);
                if (d->entered()[1])
                    throw std::runtime_error("Finger toggle failed");
                click(2);
                d->retry();
                for (bool b : d->entered())
                    if (b)
                        throw std::runtime_error("Finger retry failed");
                for (int i : {2, 3, 4, 5})
                    click(i);
                ++*phase;
                return;
            }
            for (bool b : d->entered())
                if (b)
                    throw std::runtime_error("Finger wrong input not cleared");
            scene->unschedule("finger-check");
            for (int i : {0, 1, 6, 7})
                click(i);
            scene->scheduleOnce([directory, id](float) {
                utils::captureScreen([](bool ok, std::string_view) {
                    if (!ok)
                        throw std::runtime_error("Finger capture failed");
                }, std::string(directory) + "/finger-" + std::to_string(id) + ".png");
            }, 2.2f, "finger-capture");
        }, .4f, "finger-check");
        scene->scheduleOnce([](float) { throw std::runtime_error("Finger timeout"); }, 20, "finger-timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_GEAR"))
    {
        const int id = std::atoi(std::getenv("RB_PROBE_GEAR"));
        auto* d      = dynamic_cast<DetailGearHandle*>(DetailView::create(data, id, {120, 120}, language, 1));
        if (!d)
            throw std::runtime_error("Gear class missing");
        scene->addChild(d);
        d->open();
        auto phase  = std::make_shared<int>(0);
        d->complete = [id](bool pass) {
            if (!pass)
                throw std::runtime_error("Gear rejected alignment");
            AXLOGI("RB_PROBE_GEAR {} PASS", id);
            Director::getInstance()->end();
        };
        scene->schedule([scene, d, phase, directory, id](float) {
            if (d->busy() || d->moving())
                return;
            if (*phase == 0)
            {
                d->retry();
                ++*phase;
                return;
            }
            if (*phase == 1)
            {
                utils::captureScreen([](bool ok, std::string_view) {
                    if (!ok)
                        throw std::runtime_error("Gear capture failed");
                }, std::string(directory) + "/gear-" + std::to_string(id) + ".png");
                ++*phase;
            }
            for (int i = 0; i < d->count(); ++i)
            {
                float a = d->angle(i);
                if (a == 0)
                    continue;
                auto p = d->controlCenter(i);
                d->beginTouch(p);
                d->moveTouch(p + Vec2(a > 180 || a < 0 ? 11 : -11, 0));
                d->endTouch(p);
                return;
            }
        }, .3f, "gear-check");
        scene->scheduleOnce([](float) { throw std::runtime_error("Gear timeout"); }, 60, "gear-timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_SWITCH"))
    {
        auto* d = dynamic_cast<DetailSwitch*>(DetailView::create(data, 62, {120, 120}, language, 1));
        if (!d)
            throw std::runtime_error("Switch class missing");
        scene->addChild(d);
        d->open();
        auto phase  = std::make_shared<int>(0);
        d->complete = [](bool pass) {
            if (!pass)
                throw std::runtime_error("Switch rejected password");
            AXLOGI("RB_PROBE_SWITCH PASS");
            Director::getInstance()->end();
        };
        scene->schedule([scene, d, phase, directory](float) {
            if (d->busy())
                return;
            auto click = [d](int i) {
                auto p = d->controlCenter(i);
                d->beginTouch(p);
                d->endTouch(p);
            };
            if (*phase == 0)
            {
                auto p = d->controlCenter(0);
                d->beginTouch(p);
                d->endTouch({0, 0});
                if (d->entered()[0])
                    throw std::runtime_error("Switch accepted outside release");
                click(0);
                click(0);
                if (d->entered()[0])
                    throw std::runtime_error("Switch toggle failed");
                click(2);
                click(3);
                d->retry();
                for (bool b : d->entered())
                    if (b)
                        throw std::runtime_error("Switch retry failed");
                click(1);
                click(4);
                ++*phase;
                return;
            }
            scene->unschedule("switch-check");
            utils::captureScreen([d, click](bool ok, std::string_view) {
                if (!ok)
                    throw std::runtime_error("Switch capture failed");
                click(6);
                click(8);
                click(9);
            }, std::string(directory) + "/switch.png");
        }, .6f, "switch-check");
        scene->scheduleOnce([](float) { throw std::runtime_error("Switch timeout"); }, 15, "switch-timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_MONITOR"))
    {
        auto* d = dynamic_cast<DetailMonitorControl*>(DetailView::create(data, 60, {120, 120}, language, 1));
        if (!d)
            throw std::runtime_error("Monitor class missing");
        scene->addChild(d);
        d->open();
        auto step   = std::make_shared<int>(0);
        d->complete = [](bool pass) {
            if (pass)
                throw std::runtime_error("Monitor clue unexpectedly solved");
            AXLOGI("RB_PROBE_MONITOR PASS");
            Director::getInstance()->end();
        };
        scene->schedule([scene, d, step, directory](float) {
            if (d->busy())
                return;
            if (*step == 0)
            {
                auto p = d->controlCenter(0);
                d->beginTouch(p);
                d->endTouch({0, 0});
                if (d->selectedPage() != 0)
                    throw std::runtime_error("Monitor accepted outside release");
            }
            const int buttons[]  = {0, 1, 1, 1, 1, 0};
            const int expected[] = {2, 0, 1, 2, 0, 2};
            if (*step < 6)
            {
                auto p = d->controlCenter(buttons[*step]);
                d->beginTouch(p);
                d->endTouch(p);
                if (d->selectedPage() != expected[*step])
                    throw std::runtime_error("Monitor wrap mismatch");
                ++*step;
                return;
            }
            scene->unschedule("monitor-check");
            utils::captureScreen([d](bool ok, std::string_view) {
                if (!ok)
                    throw std::runtime_error("Monitor capture failed");
                d->close();
            }, std::string(directory) + "/monitor.png");
        }, .6f, "monitor-check");
        scene->scheduleOnce([](float) { throw std::runtime_error("Monitor timeout"); }, 15, "monitor-timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_PICTURE_FRAME"))
    {
        auto* d = dynamic_cast<DetailPictureFrame*>(DetailView::create(data, 50, {120, 120}, language, 1));
        if (!d)
            throw std::runtime_error("Picture frame class missing");
        scene->addChild(d);
        d->open();
        auto index = std::make_shared<int>(0), previous = std::make_shared<int>(-1);
        d->complete = [](bool pass) {
            if (pass)
                throw std::runtime_error("Picture clue unexpectedly solved");
            AXLOGI("RB_PROBE_PICTURE_FRAME PASS");
            Director::getInstance()->end();
        };
        scene->schedule([scene, d, index, previous, directory](float) {
            const int direction = d->eyeDirection();
            if (direction < 0 || direction == *previous)
                return;
            const int expected[] = {0, 1, 2, 3, 0, 3, 4, 0};
            if (direction != expected[*index])
                throw std::runtime_error("Picture frame direction mismatch");
            *previous = direction;
            if (*index == 0)
                scene->scheduleOnce([directory](float) {
                    utils::captureScreen([](bool ok, std::string_view) {
                        if (!ok)
                            throw std::runtime_error("Picture capture failed");
                    }, std::string(directory) + "/picture-frame.png");
                }, .75f, "picture-capture");
            if (++*index == 8)
            {
                scene->unschedule("picture-check");
                d->close();
            }
        }, .02f, "picture-check");
        scene->scheduleOnce([](float) { throw std::runtime_error("Picture frame timeout"); }, 25, "picture-timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_KEY_NAME"))
    {
        auto* dialog = GetItemDialog::create(data, 181, language);
        scene->addChild(dialog);
        dialog->open();
        std::string rendered;
        std::function<void(Node*)> visit = [&](Node* node) {
            if (auto* label = dynamic_cast<Label*>(node))
                rendered += label->getString();
            for (auto* child : node->getChildren())
                visit(child);
        };
        visit(dialog);
        for (unsigned char c : rendered)
            if (c < 0x20 || c == 0x7f)
                throw std::runtime_error("Item name renders control character");
        if (language == 0 && rendered != "小钥匙")
            throw std::runtime_error("Small key label mismatch");
        scene->scheduleOnce([directory](float) {
            utils::captureScreen([](bool ok, std::string_view) {
                if (!ok)
                    throw std::runtime_error("Key name capture failed");
                AXLOGI("RB_PROBE_KEY_NAME PASS");
                Director::getInstance()->end();
            }, std::string(directory) + "/small-key.png");
        }, 1.5f, "key-capture");
        return true;
    }
    if (std::getenv("RB_PROBE_CURTAIN_CRASH"))
    {
        auto* session = GameSession::create(data, 1, 1, language, 1, false);
        scene->addChild(session);
        session->state().objects.at(860).state = 1966;
        session->room()->loadRoom(session->state().room);
        auto* room = session->room();
        auto p     = room->background()->convertToWorldSpace(room->objectPosition(860));
        session->selectCursor(0);
        session->tap(p);
        scene->scheduleOnce([scene, session, room, directory](float) {
            if (room->directionAnimationCount() != 4)
                throw std::runtime_error("Four curtain clues missing");
            if (session->state().objects.at(866).state != 1985 || session->state().objects.at(860).state != 1986)
                throw std::runtime_error("Curtain states incomplete");
            room->loadRoom(session->state().room);
            scene->scheduleOnce([room, directory](float) {
                if (room->directionAnimationCount() != 4)
                    throw std::runtime_error("Curtain clues lost on reload");
                utils::captureScreen([](bool ok, std::string_view) {
                    if (!ok)
                        throw std::runtime_error("Curtain capture failed");
                    AXLOGI("RB_PROBE_CURTAIN_CRASH PASS");
                    Director::getInstance()->end();
                }, std::string(directory) + "/curtain.png");
            }, 2, "curtain-capture");
        }, 6, "curtain-check");
        return true;
    }
    if (std::getenv("RB_PROBE_EPISODE1_STAGE1") || std::getenv("RB_PROBE_EPISODE1_STAGE2"))
    {
        const int stage = std::getenv("RB_PROBE_EPISODE1_STAGE2") ? 2 : 1;
        auto* owner     = std::getenv("RB_ROUTE_DEMO") ? dynamic_cast<MainScene*>(scene) : nullptr;
        routeDemoOwner  = owner;
        auto progress   = owner ? owner->routeDemoProgress() : std::make_shared<rb::Progress>();
        if (!owner)
            progress->initialize(data);
        auto* session = owner ? owner->routeDemoSession() : GameSession::create(data, 1, stage, language, 1, false);
        if (!owner)
        {
            scene->addChild(session);
            session->bindProgress(*progress);
            session->begin();
        }
        auto step = std::make_shared<int>(0);
        auto age  = std::make_shared<float>(0);
        scene->schedule([scene, session, progress, step, age, directory, stage, &data](float dt) {
            *age += dt;

            if (auto* hit = dynamic_cast<ActionHit*>(session->actionMode()))
            {
                if (hit->busy())
                    return;
                if (hit->menuVisible())
                    throw std::runtime_error("Curtain hammer failed");
                if (int target = hit->playableTarget())
                {
                    auto p = hit->targetCenter(target);
                    hit->beginTouch(p);
                    hit->endTouch(p);
                }
                return;
            }
            if (session->story())
            {
                advanceRouteStory(session);
                return;
            }
            if (session->itemDialog())
            {
                if (session->itemDialog()->ready())
                    session->itemDialog()->close();
                return;
            }
            if (auto* d = session->detail())
            {
                if (stage == 2)
                {
                    auto* lock = dynamic_cast<DetailDoorLock*>(d);
                    if (!lock || d->id() != 25)
                        throw std::runtime_error("Stage2 unexpected detail");
                    if (d->busy())
                        return;
                    for (int digit : {7, 2, 3, 0})
                    {
                        auto p = lock->controlCenter(digit);
                        lock->beginTouch(p);
                        lock->endTouch(p);
                    }
                    return;
                }
                auto* safe = dynamic_cast<DetailSafe*>(d);
                if (!safe || d->id() != 24)
                    throw std::runtime_error("Stage1 unexpected detail");
                if (d->busy() || *age < .65f)
                    return;
                const int password[] = {0, 2, 1, 3};
                for (int col = 0; col < 4; ++col)
                    if (safe->entered()[col] != password[col])
                    {
                        auto p = safe->controlCenter(col);
                        safe->beginTouch(p);
                        safe->moveTouch(p + Vec2(0, 50));
                        safe->endTouch(p + Vec2(0, 50));
                        *age = 0;
                        return;
                    }
                return;
            }
            if (session->clearOverlay())
            {
                if (*step != (stage == 1 ? 19 : 3))
                    throw std::runtime_error("Stage1 premature clear");
                scene->unschedule("episode1-stage1");
                scene->scheduleOnce([directory, stage](float) {
                    utils::captureScreen([stage](bool ok, std::string_view) {
                        if (!ok)
                            throw std::runtime_error("Stage capture failed");
                        AXLOGI("RB_PROBE_EPISODE1_STAGE{} PASS", stage);
                        finishRoute();
                    }, std::string(directory) + "/episode1-stage" + std::to_string(stage) + "-clear.png");
                }, 2, "stage1-capture");
                return;
            }
            if (session->busy() || *age < .8f)
                return;
            if (*step >= (stage == 1 ? 19 : 3))
                throw std::runtime_error("Stage1 exit failed");
            if (stage == 1 && *step >= 6 && *step <= 9)
            {
                auto* inv = session->inventory();
                if (*step == 6)
                {
                    inv->open();
                    inv->selectMode(2);
                }
                else if (*step == 9)
                    inv->close();
                else
                {
                    const int item = *step == 7 ? 88 : 89;
                    auto ids       = session->state().inventory();
                    auto it        = std::find(ids.begin(), ids.end(), item);
                    if (it == ids.end())
                        throw std::runtime_error("Curtain split item missing");
                    int i = int(it - ids.begin());
                    Vec2 p(48 + (i % 5) * 65, 208 - (i / 5) * 55);
                    inv->beginTouch(p);
                    scene->scheduleOnce([inv, p](float) {
                        inv->moveTouch(p + Vec2(20, 0));
                        inv->moveTouch(p + Vec2(40, 0));
                        inv->endTouch(p + Vec2(40, 0));
                    }, .8f, "curtain-split");
                }
                ++*step;
                *age = -2;
                return;
            }
            if (stage == 1 && *step == 13)
            {
                if (session->room()->directionAnimationCount() != 4)
                    throw std::runtime_error("Curtain route skipped clues");
                session->room()->loadRoom(session->state().room);
                ++*step;
                *age = 0;
                return;
            }
            const std::vector<int> objects = stage == 1
                                                 ? std::vector<int>{870, 868, 872, 859, 859, 869, 0,   0,   0,  0,
                                                                    860, 860, 860, 0,   863, 871, 864, 864, 864}
                                                 : std::vector<int>{874, 493, 493};
            const std::vector<int> cursors =
                stage == 1 ? std::vector<int>{0, 3, 0, 3, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 3, 0, 2}
                           : std::vector<int>{0, 0, 2};
            auto* room   = session->room();
            const int id = objects[*step];
            auto p       = room->background()->convertToWorldSpace(room->objectPosition(id));
            if (room->hitObject(p) != id || p.y < 0 || p.y > 320)
                for (auto* area : data.where("AreaData", "objectID", id))
                {
                    const int w = rb::integer(*area, "AreaWidth") / 2, h = rb::integer(*area, "AreaHeight") / 2;
                    if (w < 1 || h < 1)
                        continue;
                    auto q = room->background()->convertToWorldSpace(
                        {rb::integer(*area, "AreaX") / 2 + w / 2.f,
                         room->background()->getContentSize().height - rb::integer(*area, "AreaY") / 2 - h / 2.f});
                    if (q.y >= 0 && q.y <= 320 && room->hitObject(q) == id)
                    {
                        p = q;
                        break;
                    }
                }
            if (p.x > 479 || p.x < 1)
            {
                room->beginDrag();
                room->drag((240 - p.x) / .8f);
                room->drag(0);
                room->settle();
                *age = 0;
                return;
            }
            if (room->hitObject(p) != id)
                throw std::runtime_error("Stage1 hit mismatch: " + std::to_string(id));
            if (stage == 1 && *step == 16)
            {
                const auto& items = session->state().inventory();
                if (std::find(items.begin(), items.end(), 93) == items.end())
                    throw std::runtime_error("Stage1 key absent");
            }
            session->selectCursor(cursors[*step], stage == 1 ? (*step == 1    ? 95
                                                                : *step == 3  ? 87
                                                                : *step == 10 ? 92
                                                                : *step == 16 ? 93
                                                                              : 0)
                                                             : 0);
            session->tap(p);
            if (session->touchObject() != id)
                throw std::runtime_error("Stage1 tap rejected");
            ++*step;
            *age = 0;
        }, .02f, "episode1-stage1");
        scene->scheduleOnce([](float) { throw std::runtime_error("Stage1 timeout"); },
                            std::getenv("RB_ROUTE_DEMO") ? 3600 : 120, "stage1-timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_TUTORIAL_TRAINING"))
    {
        auto progress = std::make_shared<rb::Progress>();
        progress->initialize(data);
        auto* session = GameSession::create(data, 0, 1, language, 1, false);
        scene->addChild(session);
        session->bindProgress(*progress);
        session->begin();
        auto panel = std::make_shared<int>(0), phase = std::make_shared<int>(0), step = std::make_shared<int>(0);
        auto age = std::make_shared<float>(0);
        scene->schedule([scene, session, progress, panel, phase, step, age, directory, &data](float dt) {
            *age += dt;
            if (session->story())
            {
                session->story()->advance();
                return;
            }
            if (session->itemDialog())
            {
                if (session->itemDialog()->ready())
                    session->itemDialog()->close();
                return;
            }
            if (auto* d = session->detail())
            {
                if (*phase == 3)
                {
                    if (d->busy())
                        return;
                    if (d->id() == 92)
                    {
                        d->close();
                        return;
                    }
                    if (d->id() == 93)
                    {
                        auto* safe = dynamic_cast<DetailScreenSafe*>(d);
                        if (!safe)
                            throw std::runtime_error("Wrong safe class");
                        if (safe->entered().empty())
                        {
                            for (int digit : {1, 2, 3, 4})
                            {
                                auto p = safe->controlCenter(digit);
                                safe->beginTouch(p);
                                safe->endTouch(p);
                            }
                            auto p = safe->controlCenter(11);
                            safe->beginTouch(p);
                            safe->moveTouch(p + Vec2(0, -31));
                            safe->endTouch(p + Vec2(0, -31));
                        }
                        return;
                    }
                    throw std::runtime_error("Unexpected training puzzle");
                }
                if (*phase != 1 && *phase != 2)
                    throw std::runtime_error("Unexpected training detail");
                if (*phase == 1 && *age > 2)
                {
                    if (d->id() != 80 + *panel)
                        throw std::runtime_error("Wrong tutorial panel");
                    d->close();
                    *phase = 2;
                    *age   = 0;
                }
                return;
            }
            if (auto* d = dynamic_cast<ActionCut*>(session->actionMode()))
            {
                if (d->menuVisible())
                    throw std::runtime_error("Training cut failed");
                if (d->busy())
                    return;
                const Vec2 v[] = {{-1, 0}, {-1, 1}, {0, 1}, {1, 1}, {1, 0}, {1, -1}, {0, -1}, {-1, -1}};
                auto delta     = v[d->direction()].getNormalized() * 160;
                auto p         = Vec2(240, 160) - delta / 2;
                d->beginTouch(p);
                d->moveTouch(p + delta);
                d->endTouch(p + delta);
                return;
            }
            if (auto* d = dynamic_cast<ActionHit*>(session->actionMode()))
            {
                if (d->menuVisible())
                    throw std::runtime_error("Training hit failed");
                if (d->busy())
                    return;
                if (int n = d->playableTarget())
                {
                    auto p = d->targetCenter(n);
                    d->beginTouch(p);
                    d->endTouch(p);
                }
                return;
            }
            if (session->clearOverlay())
            {
                if (*panel != 11)
                    throw std::runtime_error("Training cleared too early");
                scene->unschedule("tutorial-training");
                scene->scheduleOnce([directory](float) {
                    utils::captureScreen([](bool ok, std::string_view) {
                        if (!ok)
                            throw std::runtime_error("Training capture failed");
                        AXLOGI("RB_PROBE_TUTORIAL_TRAINING complete PASS");
                        Director::getInstance()->end();
                    }, std::string(directory) + "/training-clear.png");
                }, 2, "training-clear-capture");
                return;
            }
            if (session->busy() || *age < .8f)
                return;
            auto click = [session, age, &data](int id, int cursor = 0, int item = 0) {
                auto* room = session->room();
                auto p     = room->background()->convertToWorldSpace(room->objectPosition(id));
                // Some original objects include remote 1x1 placeholder areas.
                // Pick an actual hit rectangle when their combined center misses.
                if (room->hitObject(p) != id)
                    for (auto* area : data.where("AreaData", "objectID", id))
                    {
                        const int w = rb::integer(*area, "AreaWidth") / 2, h = rb::integer(*area, "AreaHeight") / 2;
                        if (w < 1 || h < 1)
                            continue;
                        auto candidate = room->background()->convertToWorldSpace(
                            {rb::integer(*area, "AreaX") / 2 + w / 2.f,
                             room->background()->getContentSize().height - rb::integer(*area, "AreaY") / 2 - h / 2.f});
                        if (room->hitObject(candidate) == id)
                        {
                            p = candidate;
                            break;
                        }
                    }
                if (room->hitObject(p) != id)
                    throw std::runtime_error("Training hit mismatch: " + std::to_string(id));
                session->selectCursor(cursor, item);
                session->tap(p);
                *age = 0;
                if (session->touchObject() != id)
                    throw std::runtime_error("Training tap rejected");
            };
            if (*phase == 0)
            {
                click(1214 + *panel);
                *phase = 1;
                return;
            }
            if (*phase == 2)
            {
                if (*panel == 10)
                {
                    ++*panel;
                    *phase = 0;
                    *age   = 0;
                    return;
                }
                click(1226);
                *phase = 3;
                *step  = 0;
                return;
            }
            if (session->state().room == 60)
            {
                if (!session->state().objects.at(1215 + *panel).active)
                    throw std::runtime_error("Training next monitor absent");
                AXLOGI("RB_TRAINING panel {} PASS", *panel);
                ++*panel;
                *phase = 0;
                *age   = 0;
                return;
            }
            if (session->state().room != 61)
                throw std::runtime_error("Unexpected training room");
            switch (*panel)
            {
            case 0:
                click(1228);
                break;
            case 1:
                click(1228, 1);
                break;
            case 2:
                click(1242);
                break;
            case 3:
                if ((*step)++ == 0)
                    click(1228, 3, 269);
                else
                    click(1228);
                break;
            case 4:
                if ((*step)++ == 0)
                    click(1243);
                else
                    click(1230, 3, 270);
                break;
            case 5:
                if ((*step)++ == 0)
                    click(1244);
                else
                    click(1232, 3, 271);
                break;
            case 6:
                if ((*step)++ == 0)
                    click(1232);
                else
                    click(1232, 2);
                break;
            case 7:
            {
                auto* inv   = session->inventory();
                const int n = (*step)++;
                if (n == 0)
                {
                    click(1245);
                    break;
                }
                if (n == 1)
                {
                    inv->open();
                    inv->selectMode(2);
                    *age = 0;
                    break;
                }
                if (n >= 2 && n <= 4)
                {
                    const int item = 270 + n;
                    const auto ids = session->state().inventory();
                    auto it        = std::find(ids.begin(), ids.end(), item);
                    if (it == ids.end())
                        throw std::runtime_error("Training split source missing: " + std::to_string(item));
                    int index = static_cast<int>(it - ids.begin());
                    Vec2 p(48 + (index % 5) * 65, 208 - (index / 5) * 55);
                    inv->beginTouch(p);
                    scene->scheduleOnce([inv, p](float) {
                        inv->moveTouch(p + Vec2(20, 0));
                        inv->moveTouch(p + Vec2(40, 0));
                        inv->endTouch(p + Vec2(40, 0));
                    }, .8f, "training-split");
                    *age = -1.f;
                    break;
                }
                if (n == 5)
                {
                    inv->close();
                    *age = 0;
                    break;
                }
                if (n >= 6 && n <= 8)
                {
                    click(1233 + n - 6, 3, 275 + n - 6);
                    break;
                }
                throw std::runtime_error("Training gem condition failed");
            }
            case 8:
            {
                auto* inv   = session->inventory();
                const int n = (*step)++;
                if (n == 0)
                {
                    click(1247);
                    break;
                }
                if (n == 1)
                {
                    click(1248);
                    break;
                }
                if (n == 2)
                {
                    inv->open();
                    inv->selectMode(3);
                    *age = 0;
                    break;
                }
                if (n == 3)
                {
                    auto ids   = session->state().inventory();
                    auto point = [ids](int id) {
                        auto it = std::find(ids.begin(), ids.end(), id);
                        if (it == ids.end())
                            throw std::runtime_error("Training combine source missing");
                        int i = static_cast<int>(it - ids.begin());
                        return Vec2(48 + (i % 5) * 65, 208 - (i / 5) * 55);
                    };
                    auto p = point(278), q = point(279);
                    inv->beginTouch(p);
                    scene->scheduleOnce([inv, q](float) {
                        inv->moveTouch(q);
                        inv->endTouch(q);
                    }, .8f, "training-combine");
                    *age = -2.5f;
                    break;
                }
                if (n == 4)
                {
                    if (inv->operating())
                        throw std::runtime_error("Training combination unfinished");
                    inv->close();
                    *age = 0;
                    break;
                }
                if (n == 5)
                {
                    click(1238, 3, 280);
                    break;
                }
                throw std::runtime_error("Training target return failed");
            }
            case 11:
                switch ((*step)++)
                {
                case 0:
                    click(1230, 3, 270);
                    break;
                case 1:
                    click(1254);
                    break;
                case 2:
                    click(1261, 3, 271);
                    break;
                case 3:
                    click(1260);
                    break;
                case 4:
                    click(1253);
                    break;
                case 5:
                    click(1259, 3, 269);
                    break;
                case 6:
                    click(1259);
                    break;
                case 7:
                    click(1255);
                    break;
                case 8:
                    click(1232, 3, 282);
                    break;
                case 9:
                    click(1232);
                    break;
                case 10:
                    click(1232, 2);
                    break;
                default:
                    throw std::runtime_error("Training final exit failed");
                }
                break;
            case 9:
                switch ((*step)++)
                {
                case 0:
                    click(1239, 1);
                    break;
                case 1:
                    click(1240);
                    break;
                case 2:
                    click(1249);
                    break;
                case 3:
                    click(1241, 3, 281);
                    break;
                default:
                    throw std::runtime_error("Training safe return failed");
                }
                break;
            }
        }, .02f, "tutorial-training");
        scene->scheduleOnce([](float) { throw std::runtime_error("Training timeout"); }, 540, "training-timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_TUTORIAL_FLOW"))
    {
        auto progress = std::make_shared<rb::Progress>();
        progress->initialize(data);
        auto* session = GameSession::create(data, 0, 1, language, 1, false);
        scene->addChild(session);
        session->bindProgress(*progress);
        session->begin();
        auto panel = std::make_shared<int>(0);
        auto phase = std::make_shared<int>(0);
        auto age   = std::make_shared<float>(0);
        scene->schedule([scene, session, progress, panel, phase, age, directory](float dt) {
            *age += dt;
            if (session->story())
            {
                session->story()->advance();
                return;
            }
            if (auto* d = session->detail())
            {
                if (*phase == 2)
                    return;
                if (*phase != 1 || d->id() != 80 + *panel)
                    throw std::runtime_error("Tutorial event opened wrong panel");
                if (*age > 2)
                {
                    d->close();
                    *phase = 2;
                    *age   = 0;
                }
                return;
            }
            if (session->clearOverlay())
            {
                if (*panel != 12)
                    throw std::runtime_error("Tutorial cleared too early");
                scene->unschedule("tutorial-flow");
                scene->scheduleOnce([directory](float) {
                    utils::captureScreen([](bool ok, std::string_view) {
                        if (!ok)
                            throw std::runtime_error("Tutorial flow capture failed");
                        Director::getInstance()->end();
                    }, std::string(directory) + "/tutorial-clear.png");
                    AXLOGI("RB_PROBE_TUTORIAL_FLOW PASS");
                }, 2, "capture-clear");
                return;
            }
            if (session->busy() || *age < .5f)
                return;
            auto click = [session](int id) {
                auto* room = session->room();
                auto p     = room->background()->convertToWorldSpace(room->objectPosition(id));
                if (room->hitObject(p) != id)
                    throw std::runtime_error("Tutorial object hit failed: " + std::to_string(id));
                session->selectCursor(0);
                session->tap(p);
                if (session->touchObject() != id)
                    throw std::runtime_error("Tutorial tap rejected");
            };
            if (*phase == 0)
            {
                click(1214 + *panel);
                *phase = 1;
                *age   = 0;
                return;
            }
            if (*phase == 2)
            {
                const int old = (*panel)++;
                *phase        = 0;
                *age          = 0;
                if (old != 10)
                    click(1227);
                return;
            }
            throw std::runtime_error("Tutorial panel failed to open");
        }, .1f, "tutorial-flow");
        scene->scheduleOnce([](float) { throw std::runtime_error("Tutorial flow timeout"); }, 150, "flow-timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_PIECES"))
    {
        auto* session = GameSession::create(data, 6, 2, language, 1, false);
        scene->addChild(session);
        auto* d = static_cast<DetailFittedPieces*>(DetailView::create(data, 94, {100, 100}, language, 1));
        scene->addChild(d, 20);
        d->open();
        d->complete = [](bool pass) {
            if (!pass)
                throw std::runtime_error("Pieces did not complete");
            AXLOGI("RB_PROBE_PIECES PASS");
            Director::getInstance()->end();
        };
        scene->scheduleOnce([d, directory](float) {
            utils::captureScreen([](bool ok, std::string_view) {
                if (!ok)
                    throw std::runtime_error("Pieces capture failed");
            }, std::string(directory) + "/pieces-start.png");
            auto step          = std::make_shared<int>(0);
            const auto initial = d->controlCenter(0);
            d->schedule([d, step, initial, directory](float) {
                auto drag = [d](int i, Vec2 target) {
                    auto p = d->controlCenter(i);
                    d->beginTouch(p);
                    d->moveTouch(target);
                    d->endTouch(target);
                };
                const int s = (*step)++;
                if (s == 0)
                {
                    drag(0, d->targetCenter(0) + Vec2(11, 0));
                    if (d->fitted(0))
                        throw std::runtime_error("Pieces accepted outside tolerance");
                    return;
                }
                if (s == 1)
                {
                    if (d->controlCenter(0).distance(initial) > .01f)
                        throw std::runtime_error("Piece failed return");
                    drag(0, d->targetCenter(0) + Vec2(10, 10));
                    if (!d->fitted(0))
                        throw std::runtime_error("Pieces rejected boundary");
                    return;
                }
                if (s == 2)
                {
                    if (d->controlCenter(0).distance(d->targetCenter(0)) > .01f)
                        throw std::runtime_error("Piece failed snap");
                    d->retry();
                    return;
                }
                if (s == 3)
                {
                    if (d->controlCenter(0).distance(initial) > .01f || !d->fitted(0))
                        throw std::runtime_error("Pieces retry differs from original");
                    drag(0, initial + Vec2(0, -20));
                    if (d->fitted(0))
                        throw std::runtime_error("Piece fit flag not cleared");
                    return;
                }
                if (s < 13)
                {
                    const int i = s - 4;
                    drag(i, d->targetCenter(i));
                    if (!d->fitted(i))
                        throw std::runtime_error("Piece drop failed: " + std::to_string(i));
                    return;
                }
                d->unschedule("pieces");
                if (!d->busy())
                    throw std::runtime_error("Pieces did not trigger success");
                utils::captureScreen([](bool ok, std::string_view) {
                    if (!ok)
                        throw std::runtime_error("Pieces result capture failed");
                }, std::string(directory) + "/pieces-result.png");
            }, .65f, "pieces");
        }, 1.3f, "pieces-test");
        scene->scheduleOnce([](float) { throw std::runtime_error("Pieces test timeout"); }, 20, "timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_TUTORIAL"))
    {
        auto* session = GameSession::create(data, 0, 1, language, 1, false);
        scene->addChild(session);
        auto index   = std::make_shared<int>(0);
        auto current = std::make_shared<DetailView*>(nullptr);
        scene->schedule([scene, &data, index, current, language, directory](float) {
            if (*current)
                return;
            if (*index == 11)
            {
                AXLOGI("RB_PROBE_TUTORIAL PASS");
                Director::getInstance()->end();
                return;
            }
            const int ids[] = {80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90};
            const int id    = ids[(*index)++];
            auto* d         = DetailView::create(data, id, {100, 100}, language, 0);
            *current        = d;
            scene->addChild(d, 20);
            d->complete = [current](bool pass) {
                if (!pass)
                    throw std::runtime_error("Tutorial close failed");
                *current = nullptr;
            };
            d->open();
            auto* pages = dynamic_cast<DetailTutorial*>(d);
            if (pages)
                d->scheduleOnce([pages](float) {
                    auto p = pages->controlCenter(0);
                    pages->beginTouch(p);
                    pages->moveTouch(p + Vec2(-80, 0));
                    pages->endTouch(p + Vec2(-80, 0));
                    if (pages->selectedPage() != 0 || !pages->displayBusy())
                        throw std::runtime_error("Tutorial early input accepted");
                }, 1.2f, "early");
            d->scheduleOnce([d, pages, directory, id](float) {
                utils::captureScreen([](bool ok, std::string_view) {
                    if (!ok)
                        throw std::runtime_error("Tutorial capture failed");
                }, std::string(directory) + "/tutorial-" + std::to_string(id) + ".png");
                if (!pages)
                {
                    d->scheduleOnce([d](float) { d->close(); }, .2f, "close-single");
                    return;
                }
                if (pages->displayBusy())
                    throw std::runtime_error("Tutorial did not open");
                auto step     = std::make_shared<int>(0);
                auto expected = std::make_shared<int>(0);
                d->schedule([pages, step, expected](float) {
                    if (pages->selectedPage() != *expected || std::abs(pages->pagePosition(*expected) - 240) > .01f)
                        throw std::runtime_error("Tutorial page failed to settle");
                    auto swipe = [pages](float dx) {
                        auto p = pages->controlCenter(pages->selectedPage());
                        pages->beginTouch(p);
                        pages->moveTouch(p + Vec2(dx, 0));
                        pages->endTouch(p + Vec2(dx, 0));
                    };
                    const int n = pages->pageCount(), s = (*step)++;
                    if (s == 0)
                    {
                        swipe(-50);
                        return;
                    }
                    if (s < n)
                    {
                        swipe(-60);
                        ++*expected;
                        return;
                    }
                    if (s == n)
                    {
                        swipe(-60);
                        return;
                    }
                    if (s < 2 * n)
                    {
                        swipe(60);
                        --*expected;
                        return;
                    }
                    if (s == 2 * n)
                    {
                        swipe(60);
                        return;
                    }
                    pages->unschedule("pages");
                    pages->close();
                    pages->close();
                }, .65f, "pages");
            }, 1.7f, "tutorial-test");
        }, .1f, "tutorials");
        scene->scheduleOnce([](float) { throw std::runtime_error("Tutorial test timeout"); }, 120, "timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_DIRECTION_PARTICLES"))
    {
        auto* session = GameSession::create(data, 4, 5, language, 1, false);
        scene->addChild(session);
        auto* room = session->room();
        auto count = std::make_shared<int>(0);
        for (int id : {28, 42, 43, 44, 45, 46, 77})
            if (!room->direction(id, [count] { ++*count; }))
                throw std::runtime_error("Particle direction not dispatched");
        scene->scheduleOnce([scene, room, count, directory](float) {
            if (*count != 7 || room->directionParticleCount() != 7)
                throw std::runtime_error("Particle direction completion failed");
            utils::captureScreen([](bool ok, std::string_view) {
                if (!ok)
                    throw std::runtime_error("Direction capture failed");
            }, std::string(directory) + "/direction-particles.png");
            scene->scheduleOnce([scene, room, count](float) {
                for (int id : {29, 47, 78})
                    room->direction(id, [count] { ++*count; });
                scene->scheduleOnce([room, count](float) {
                    if (*count != 10 || room->directionParticleCount())
                        throw std::runtime_error("Particle direction removal failed");
                    Director::getInstance()->end();
                }, .7f, "removed");
            }, .2f, "remove");
        }, .7f, "direction-check");
        return true;
    }
    if (std::getenv("RB_PROBE_DOOR_LOCK"))
    {
        auto* d = dynamic_cast<DetailDoorLock*>(DetailView::create(data, 25, {100, 100}, language, 2));
        if (!d)
            throw std::runtime_error("Door lock dispatch failed");
        scene->addChild(d);
        d->open();
        d->complete = [](bool pass) {
            if (!pass)
                throw std::runtime_error("Door lock rejected correct password");
            AXLOGI("RB_PROBE_DOOR_LOCK PASS");
            Director::getInstance()->end();
        };
        auto tap = [d](int n) {
            auto p = d->controlCenter(n);
            d->beginTouch(p);
            d->endTouch(p);
        };
        d->scheduleOnce([d, tap, directory](float) {
            utils::captureScreen([](bool ok, std::string_view) {
                if (!ok)
                    throw std::runtime_error("Door lock capture failed");
            }, std::string(directory) + "/door-lock.png");
            auto p = d->controlCenter(2);
            d->beginTouch(p);
            d->endTouch({0, 0});
            if (!d->entered().empty())
                throw std::runtime_error("Door lock accepted release outside");
            tap(2);
            tap(3);
            tap(10);
            if (d->entered() != "2")
                throw std::runtime_error("Door lock deletion failed");
            d->retry();
            for (int n : {0, 0, 0, 0})
                tap(n);
            if (!d->busy())
                throw std::runtime_error("Door lock missing failure animation");
            d->scheduleOnce([d, tap](float) {
                if (d->busy() || !d->entered().empty())
                    throw std::runtime_error("Door lock failure did not reset");
                for (int n : {7, 2, 3, 0})
                    tap(n);
                if (!d->busy())
                    throw std::runtime_error("Door lock missing success animation");
            }, 1, "door-correct");
        }, 1.5f, "door-test");
        scene->scheduleOnce([](float) { throw std::runtime_error("Door lock timeout"); }, 20, "door-timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_SAFE_SMALL_DRAG"))
    {
        auto* d = static_cast<DetailSafe*>(DetailView::create(data, 15, {100, 100}, language, 2));
        scene->addChild(d);
        d->open();
        auto step     = std::make_shared<int>(0);
        auto age      = std::make_shared<float>(0);
        auto checking = std::make_shared<bool>(false);
        d->schedule([d, step, age, checking](float dt) {
            if (d->busy())
                return;
            const float offsets[] = {.25f, -.25f, .75f, -.75f, 1.5f, -1.5f, 5.f, -5.f, 12.f, -12.f};
            if (*checking)
            {
                *age += dt;
                if (*age < .5f)
                    return;
                const float center = d->board_->getContentSize().height / 2 + 6;
                if (d->input_[0] != 0 || std::abs(d->wheels_[0][0]->getPositionY() - center) > .01f)
                    throw std::runtime_error("Small safe drag did not return exactly to its detent");
                AXLOGI("SAFE_SMALL_DRAG {} PASS", offsets[*step]);
                ++*step;
                *checking = false;
            }
            if (*step == 10)
            {
                AXLOGI("RB_PROBE_SAFE_SMALL_DRAG PASS");
                Director::getInstance()->end();
                return;
            }
            const auto p = d->controlCenter(0);
            d->beginTouch(p);
            d->moveTouch(p + Vec2(0, offsets[*step]));
            d->endTouch(p + Vec2(0, offsets[*step]));
            *age      = 0;
            *checking = true;
        }, .02f, "safe-small-drag");
        return true;
    }
    if (std::getenv("RB_PROBE_SAFE_VISUAL"))
    {
        auto* d = static_cast<DetailSafe*>(DetailView::create(data, 15, {100, 100}, language, 2));
        scene->addChild(d);
        d->open();
        auto frame    = std::make_shared<int>(0);
        auto active   = std::make_shared<bool>(false);
        auto previous = std::make_shared<std::vector<float>>();
        d->schedule([d, frame, active, previous, directory](float) {
            if (d->busy())
                return;
            const int f     = (*frame)++;
            const int phase = f % 150, column = (f / 150) % 4;
            const auto origin = d->controlCenter(column);
            if (phase == 0)
            {
                d->beginTouch(origin);
                *active = true;
            }
            if (phase < 90)
                d->moveTouch(origin + Vec2(0, (column % 2 ? 50.f * std::floor(phase / 10.f) : phase * 3.f)));
            if (phase == 90)
            {
                d->endTouch(origin + Vec2(0, column % 2 ? 400.f : 267.f));
                *active = false;
            }
            int mismatch = 0;
            std::vector<float> current;
            for (auto* node : d->wheels_[column])
            {
                const float y = node->getPositionY();
                current.push_back(y);
                const int opacity = 255 - std::clamp(static_cast<int>(y > 174  ? (y - 174) * 10
                                                                      : y < 78 ? (78 - y) * 10
                                                                               : 0),
                                                     0, 255);
                if (std::abs(opacity - node->getOpacity()) > 30)
                    ++mismatch;
            }
            if (mismatch)
                AXLOGI("SAFE_VISUAL frame={} phase={} mismatches={}", f, phase, mismatch);
            if (f < 150 && (phase == 88 || phase == 90 || phase == 92 || phase == 94 || phase == 98 || phase == 110))
                utils::captureScreen([](bool ok, std::string_view) {
                    if (!ok)
                        throw std::runtime_error("Safe frame capture failed");
                }, std::string(directory) + "/safe-frame-" + std::to_string(f) + ".png");
            if (f == 599)
            {
                AXLOGI("RB_PROBE_SAFE_VISUAL COMPLETE");
                Director::getInstance()->end();
            }
        }, 1.f / 60, "safe-visual");
        return true;
    }
    if (std::getenv("RB_PROBE_ROLLING_SAFE"))
    {
        auto index   = std::make_shared<int>(std::getenv("RB_PROBE_SAFE_LAST") ? 5 : 0);
        auto current = std::make_shared<DetailSafe*>(nullptr);
        scene->schedule([scene, &data, index, current, language, directory](float) {
            if (*current)
                return;
            if (*index == 6)
            {
                AXLOGI("RB_PROBE_ROLLING_SAFE PASS");
                Director::getInstance()->end();
                return;
            }
            const int ids[]                               = {24, 15, 43, 57, 58, 59};
            const std::vector<std::vector<int>> passwords = {{0, 2, 1, 3}, {21, 0, 18, 4}, {4, 11, 4, 11},
                                                             {1, 8, 5, 6}, {7, 5, 5, 3},   {6, 4, 9, 4}};
            const int id                                  = ids[*index];
            auto password                                 = passwords[(*index)++];
            auto* d = dynamic_cast<DetailSafe*>(DetailView::create(data, id, {100, 100}, language, 2));
            if (!d)
                throw std::runtime_error("Wrong rolling safe class");
            *current = d;
            scene->addChild(d);
            d->complete = [current, id](bool pass) {
                if (!pass)
                    throw std::runtime_error("Rolling safe rejected code");
                AXLOGI("RB_ROLLING_SAFE {} PASS", id);
                *current = nullptr;
            };
            d->open();
            d->scheduleOnce([d, directory, id, password](float) {
                utils::captureScreen([](bool ok, std::string_view) {
                    if (!ok)
                        throw std::runtime_error("Rolling safe capture failed");
                }, std::string(directory) + "/rolling-safe-" + std::to_string(id) + ".png");
                auto column = std::make_shared<int>(0);
                d->schedule([d, column, password, id](float) {
                    if (d->busy())
                        return;
                    if (*column == 4)
                    {
                        AXLOGI(
                            "SAFE_INCOMPLETE id={} input={},{},{},{} password={},{},{},{} solved={} busy={} actions={}",
                            id, d->input_[0], d->input_[1], d->input_[2], d->input_[3], d->password_[0],
                            d->password_[1], d->password_[2], d->password_[3], d->solved_, d->busy(),
                            d->wheels_[3].back()->getNumberOfRunningActions());
                        throw std::runtime_error("Rolling safe failed to complete");
                    }
                    const int n = *column, now = d->entered()[n], target = password[n],
                              max = (id == 15 || id == 43) ? 26 : 10;
                    if (now == target)
                    {
                        ++*column;
                        return;
                    }
                    const int forward = (target - now + max) % max;
                    const float dy    = forward <= max / 2 ? 50 : -50;
                    auto p            = d->controlCenter(n);
                    d->beginTouch(p);
                    d->moveTouch(p + Vec2(0, dy));
                    d->endTouch(p + Vec2(0, dy));
                    const int expected = (now + (dy > 0 ? 1 : -1) + max) % max;
                    if (d->entered()[n] != expected)
                        throw std::runtime_error("Rolling safe digit/wrap failed: " + std::to_string(id) + " column " +
                                                 std::to_string(n) + " got " + std::to_string(d->entered()[n]) +
                                                 " expected " + std::to_string(expected));
                }, .65f, "roll-digits");
            }, 1.5f, "roll-open");
        }, .2f, "rolling-safes");
        scene->scheduleOnce([](float) { throw std::runtime_error("Rolling safe timeout"); }, 180, "rolling-timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_SAFE"))
    {
        auto* session = GameSession::create(data, 2, 2, language, 1, false);
        scene->addChild(session);
        auto index   = std::make_shared<int>(0);
        auto current = std::make_shared<DetailScreenSafe*>(nullptr);
        scene->schedule([scene, &data, index, current, language, directory](float) {
            if (*current)
                return;
            if (*index == 5)
            {
                Director::getInstance()->end();
                return;
            }
            const int ids[]            = {4, 40, 65, 93, 95};
            const char* passwords[]    = {"4545", "2146", "0817", "1234", "9764"};
            const int id               = ids[*index];
            const std::string password = passwords[(*index)++];
            auto* d  = static_cast<DetailScreenSafe*>(DetailView::create(data, id, {100, 100}, language, 2));
            *current = d;
            scene->addChild(d, 20);
            d->complete = [current](bool pass) {
                if (!pass)
                    throw std::runtime_error("Safe rejected password");
                *current = nullptr;
            };
            d->open();
            d->scheduleOnce([scene, d, password, directory, id](float) {
                utils::captureScreen([](bool ok, std::string_view) {
                    if (!ok)
                        throw std::runtime_error("Safe capture failed");
                }, std::string(directory) + "/safe-" + std::to_string(id) + ".png");
                d->scheduleOnce([d, password](float) {
                    auto p = d->controlCenter(11);
                    d->beginTouch(p);
                    d->moveTouch(p + Vec2(0, -20));
                    d->endTouch(p + Vec2(0, -20));
                    if (d->busy())
                        throw std::runtime_error("Safe checked before handle threshold");
                    d->scheduleOnce([d, password](float) {
                        if (std::abs(d->handleRotation()) > .01f)
                            throw std::runtime_error("Safe handle did not return");
                        auto p = d->controlCenter(11);
                        d->beginTouch(p);
                        d->moveTouch(p + Vec2(0, -35));
                        d->endTouch(p + Vec2(0, -35));
                        if (!d->busy())
                            throw std::runtime_error("Empty safe not checked");
                        d->scheduleOnce([d, password](float) {
                            if (d->busy())
                                throw std::runtime_error("Empty safe failed delay");
                            for (int i = 0; i < 4; ++i)
                            {
                                auto p = d->controlCenter(0);
                                d->beginTouch(p);
                                d->endTouch(p);
                            }
                            auto p = d->controlCenter(11);
                            d->beginTouch(p);
                            d->moveTouch(p + Vec2(0, -35));
                            d->endTouch(p + Vec2(0, -35));
                            if (!d->busy() || !d->entered().empty())
                                throw std::runtime_error("Wrong safe code did not clear");
                            d->scheduleOnce([d, password](float) {
                                if (d->busy() || std::abs(d->handleRotation()) > .01f)
                                    throw std::runtime_error("Safe failure did not reset");
                                for (int i : {2, 3, 10})
                                {
                                    auto p = d->controlCenter(i);
                                    d->beginTouch(p);
                                    d->endTouch(p);
                                }
                                if (d->entered() != "2")
                                    throw std::runtime_error("Safe backspace failed");
                                d->retry();
                                for (char digit : password)
                                {
                                    auto p = d->controlCenter(digit - '0');
                                    d->beginTouch(p);
                                    d->endTouch(p);
                                }
                                if (d->entered() != password || d->busy())
                                    throw std::runtime_error("Safe digit input failed");
                                auto p = d->controlCenter(11);
                                d->beginTouch(p);
                                d->moveTouch(p + Vec2(0, -35));
                                d->endTouch(p + Vec2(0, -35));
                                if (!d->busy())
                                    throw std::runtime_error("Safe success not started");
                            }, 1.1f, "correct-safe");
                        }, .5f, "wrong-safe");
                    }, .3f, "empty-safe");
                }, .15f, "handle-test");
            }, 1.3f, "safe-test");
        }, .2f, "safes");
        scene->scheduleOnce([](float) { throw std::runtime_error("Safe test timeout"); }, 60, "safe-timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_DISPLAY"))
    {
        auto* session = GameSession::create(data, 1, 4, language, 1, false);
        scene->addChild(session);
        auto* d = static_cast<DetailElectronicDisplay*>(DetailView::create(data, 54, {100, 100}, language, 0));
        scene->addChild(d, 20);
        d->open();
        d->complete = [](bool pass) {
            if (!pass)
                throw std::runtime_error("Display detail did not pass");
            Director::getInstance()->end();
        };
        scene->scheduleOnce([d](float) {
            auto p = d->controlCenter(0);
            d->beginTouch(p);
            d->moveTouch(p + Vec2(-80, 0));
            d->endTouch(p + Vec2(-80, 0));
            if (d->selectedPage() != 0 || !d->displayBusy())
                throw std::runtime_error("Display accepted early input");
        }, 1.2f, "early-display");
        scene->scheduleOnce([scene, d, directory](float) {
            if (d->displayBusy())
                throw std::runtime_error("Display did not open");
            utils::captureScreen([](bool ok, std::string_view) {
                if (!ok)
                    throw std::runtime_error("Display capture failed");
            }, std::string(directory) + "/display-first.png");
            auto phase    = std::make_shared<int>(0);
            auto expected = std::make_shared<int>(0);
            scene->schedule([scene, d, directory, phase, expected](float) {
                if (d->selectedPage() != *expected || std::abs(d->pagePosition(*expected) - 240) > .01f)
                    throw std::runtime_error("Display did not settle: step=" + std::to_string(*phase) +
                                             " expected=" + std::to_string(*expected) +
                                             " actual=" + std::to_string(d->selectedPage()) +
                                             " x=" + std::to_string(d->pagePosition(*expected)));
                auto swipe = [d](float dx) {
                    auto p = d->controlCenter(d->selectedPage());
                    d->beginTouch(p);
                    d->moveTouch(p + Vec2(dx, 0));
                    d->endTouch(p + Vec2(dx, 0));
                };
                const int step = (*phase)++;
                if (step == 0)
                {
                    swipe(-40);
                    return;
                }
                if (step <= 5)
                {
                    swipe(-60);
                    ++*expected;
                    return;
                }
                if (step == 6)
                {
                    utils::captureScreen([](bool ok, std::string_view) {
                        if (!ok)
                            throw std::runtime_error("Display last capture failed");
                    }, std::string(directory) + "/display-last.png");
                    return;
                }
                if (step == 7)
                {
                    swipe(-60);
                    return;
                }
                if (step <= 12)
                {
                    swipe(60);
                    --*expected;
                    return;
                }
                if (step == 13)
                {
                    swipe(60);
                    return;
                }
                scene->unschedule("display-pages");
                d->close();
                d->close();
            }, .7f, "display-pages");
        }, 1.7f, "display-test");
        return true;
    }
    if (std::getenv("RB_PROBE_MUSIC"))
    {
        auto* session = GameSession::create(data, 2, 3, language, 1, false);
        scene->addChild(session);
        auto* d = static_cast<DetailMusic*>(DetailView::create(data, 6, {100, 100}, language, 0));
        scene->addChild(d, 20);
        d->open();
        d->complete = [](bool pass) {
            if (!pass)
                throw std::runtime_error("Music detail did not pass");
            Director::getInstance()->end();
        };
        scene->scheduleOnce([scene, d, directory](float) {
            if (d->busy() || !d->noteVisible(0) || d->noteVisible(19))
                throw std::runtime_error("Initial music visibility incorrect");
            utils::captureScreen([](bool ok, std::string_view) {
                if (!ok)
                    throw std::runtime_error("Music capture failed");
            }, std::string(directory) + "/music-start.png");
            scene->scheduleOnce([scene, d, directory](float) {
                auto p             = d->controlCenter(0);
                const float before = d->notePosition(0);
                d->beginTouch(p);
                d->moveTouch(p + Vec2(-60, 0));
                d->endTouch(p + Vec2(-60, 0));
                if (std::abs(d->notePosition(0) - before + 60) > .01f)
                    throw std::runtime_error("Music drag failed");
                scene->scheduleOnce([scene, d, directory](float) {
                    int index = 0;
                    while (index < 20 && !d->noteVisible(index))
                        ++index;
                    if (index == 20)
                        throw std::runtime_error("Music notes disappeared");
                    auto p = d->controlCenter(index);
                    d->beginTouch(p);
                    d->moveTouch(p + Vec2(-2000, 0));
                    d->endTouch(p + Vec2(-2000, 0));
                    scene->scheduleOnce([scene, d, directory](float) {
                        if (!d->noteVisible(19) || d->noteVisible(0))
                            throw std::runtime_error("Music end rebound failed");
                        utils::captureScreen([](bool ok, std::string_view) {
                            if (!ok)
                                throw std::runtime_error("Music end capture failed");
                        }, std::string(directory) + "/music-end.png");
                        scene->scheduleOnce([scene, d](float) {
                            auto p = d->controlCenter(19);
                            d->beginTouch(p);
                            d->moveTouch(p + Vec2(2000, 0));
                            d->endTouch(p + Vec2(2000, 0));
                            scene->scheduleOnce([d](float) {
                                if (!d->noteVisible(0) || d->noteVisible(19))
                                    throw std::runtime_error("Music start rebound failed");
                                d->close();
                            }, .5f, "start-rebound");
                        }, .15f, "reverse-drag");
                    }, .5f, "end-rebound");
                }, .3f, "scroll-end");
            }, .15f, "initial-drag");
        }, 1.3f, "music-test");
        return true;
    }
    if (std::getenv("RB_PROBE_DETAIL_MISC"))
    {
        auto progress = std::make_shared<rb::Progress>();
        progress->initialize(data);
        progress->hands = 0;
        auto* session   = GameSession::create(data, 2, 3, language, 1, false);
        scene->addChild(session);
        session->bindProgress(*progress);
        auto* standard = DetailView::create(data, 10, {50, 50}, language, 0);
        scene->addChild(standard, 20);
        standard->complete = [scene, session, progress](bool pass) {
            if (!pass)
                throw std::runtime_error("Standard detail must pass next event");
            session->runEvent(7187);
            scene->scheduleOnce([scene, session](float) {
                session->detail()->handsRequested();
                if (!session->messageBox())
                    throw std::runtime_error("Out-of-hands prompt missing");
                scene->scheduleOnce([session](float) { session->messageBox()->answer(true); }, .6f, "answer");
                scene->schedule([session](float) {
                    if (!session->detail() && !session->busy() && session->systemMenu() &&
                        session->systemMenu()->mode() == 3)
                        Director::getInstance()->end();
                }, .1f, "shop-ready");
                scene->scheduleOnce([](float) { throw std::runtime_error("Detail did not close into shop"); }, 5,
                                    "shop-timeout");
            }, 1.3f, "hands");
        };
        standard->open();
        scene->scheduleOnce([standard, directory](float) {
            utils::captureScreen([standard](bool ok, std::string_view) {
                if (!ok)
                    throw std::runtime_error("Standard screenshot failed");
                standard->close();
            }, std::string(directory) + "/standard.png");
        }, 1.3f, "capture");
        return true;
    }
    if (std::getenv("RB_PROBE_DETAIL_SESSION"))
    {
        auto progress = std::make_shared<rb::Progress>();
        progress->initialize(data);
        progress->hands = 1;
        auto* session   = GameSession::create(data, 2, 3, language, 1, false);
        scene->addChild(session);
        session->bindProgress(*progress);
        const int original = session->state().objects.at(76).state;
        session->runEvent(7187);
        if (!session->detail() || !session->busy())
            throw std::runtime_error("Detail event was not held");
        session->tap({200, 200});
        if (session->touchCount())
            throw std::runtime_error("Room input leaked through detail");
        scene->scheduleOnce([session](float) { session->detail()->close(); }, 1.3f, "cancel");
        scene->scheduleOnce([scene, session, progress, original, directory](float) {
            if (session->detail() || session->busy() || session->state().objects.at(76).state != original)
                throw std::runtime_error("Cancel did not stop linked row");
            session->runEvent(7187);
            scene->scheduleOnce([scene, session, progress, directory](float) {
                session->detail()->handsRequested();
                if (!session->messageBox())
                    throw std::runtime_error("Detail ability prompt missing");
                scene->scheduleOnce([session](float) {
                    if (!session->messageBox()->ready())
                        throw std::runtime_error("Ability prompt not ready");
                    session->messageBox()->answer(true);
                }, .6f, "answer");
                scene->scheduleOnce([session, progress](float) {
                    if (!session->abilityEffect() || progress->hands != 0)
                        throw std::runtime_error("Detail ability not consumed");
                }, 1.2f, "ability");
                scene->scheduleOnce([session, directory](float) {
                    utils::captureScreen([](bool ok, std::string_view) {
                        if (!ok)
                            throw std::runtime_error("Success screenshot failed");
                    }, std::string(directory) + "/success.png");
                }, 5.3f, "success-capture");
                scene->scheduleOnce([session, progress](float) {
                    if (session->detail() || session->busy() || progress->hands != 0 ||
                        session->state().objects.at(76).state != 1572 || session->state().objects.at(75).active ||
                        !session->state().objects.at(63).active)
                        throw std::runtime_error("Detail success did not release linked row");
                    Director::getInstance()->end();
                }, 10.2f, "done");
            }, 1.3f, "hands");
        }, 3.1f, "again");
        return true;
    }
    if (std::getenv("RB_PROBE_DETAIL"))
    {
        auto* session = GameSession::create(data, 2, 3, language, 1, false);
        scene->addChild(session);
        int standard = 0;
        for (const auto& row : data.table("DetailData").GetArray())
            if (rb::integer(row, "detailType") == 0)
            {
                auto* d = DetailView::create(data, rb::integer(row, "detailID"), {100, 100}, language, 3);
                scene->addChild(d);
                d->removeFromParent();
                ++standard;
            }
        if (standard != 31)
            throw std::runtime_error("Standard detail catalog incomplete");
        auto index = std::make_shared<int>(0), completions = std::make_shared<int>(0);
        auto current = std::make_shared<DetailView*>(nullptr);
        scene->schedule([scene, &data, index, completions, current, directory, language](float) {
            if (*current)
                return;
            if (*index == 5)
            {
                if (*completions != 5)
                    throw std::runtime_error("Pad result count mismatch");
                scene->unschedule("pads");
                auto* result = ResultFeedback::create(false);
                scene->addChild(result, 30);
                result->complete = [] { Director::getInstance()->end(); };
                result->play();
                scene->scheduleOnce([directory](float) {
                    utils::captureScreen([](bool ok, std::string_view) {
                        if (!ok)
                            throw std::runtime_error("Failure screenshot failed");
                    }, std::string(directory) + "/failed.png");
                }, 1.8f, "failed-capture");
                return;
            }
            const int ids[]            = {5, 12, 38, 47, 71};
            const char* passwords[]    = {"4710", "1111", "645933", "7979", "3846"};
            const int id               = ids[*index];
            const std::string password = passwords[(*index)++];
            auto* d                    = static_cast<DetailPad*>(DetailView::create(data, id, {100, 100}, language, 3));
            *current                   = d;
            scene->addChild(d, 20);
            d->complete = [current, completions](bool pass) {
                if (!pass)
                    throw std::runtime_error("Correct pad password rejected");
                ++*completions;
                *current = nullptr;
            };
            d->open();
            d->beginTouch(d->controlCenter(1));
            d->endTouch(d->controlCenter(1));
            if (!d->entered().empty())
                throw std::runtime_error("Pad accepted early input");
            d->scheduleOnce([scene, d, password, directory, id](float) {
                if (d->busy())
                    throw std::runtime_error("Pad did not open");
                utils::captureScreen([scene, d, password](bool ok, std::string_view) {
                    if (!ok)
                        throw std::runtime_error("Pad screenshot failed");
                    for (std::size_t i = 0; i < password.size(); ++i)
                    {
                        d->beginTouch(d->controlCenter(0));
                        d->endTouch(d->controlCenter(0));
                    }
                    if (!d->busy())
                        throw std::runtime_error("Bad password did not block input");
                    d->retry();
                    if (!d->entered().empty() || !d->busy())
                        throw std::runtime_error("Retry during failure did not preserve animation");
                    d->scheduleOnce([d, password](float) {
                        if (d->busy() || !d->entered().empty())
                            throw std::runtime_error("Bad password did not reset");
                        d->beginTouch(d->controlCenter(2));
                        d->endTouch(d->controlCenter(2));
                        d->retry();
                        if (!d->entered().empty())
                            throw std::runtime_error("Pad retry did not clear digits");
                        for (char digit : password)
                        {
                            d->beginTouch(d->controlCenter(digit - '0'));
                            d->endTouch(d->controlCenter(digit - '0'));
                        }
                        if (!d->busy() || d->entered() != password)
                            throw std::runtime_error("Pad success state failed");
                    }, 1, "correct");
                }, std::string(directory) + "/pad-" + std::to_string(id) + ".png");
            }, 1.3f, "test-pad");
        }, .2f, "pads");
        scene->scheduleOnce([](float) { throw std::runtime_error("Detail probe timeout"); }, 55, "timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_PARTICLES"))
    {
        auto* session = GameSession::create(data, 1, 1, language, 1, false);
        scene->addChild(session);
        auto count    = std::make_shared<int>(0);
        auto captured = std::make_shared<bool>(false);
        for (int i = 1; i <= 10; ++i)
            session->room()->particle(i, {40.f + (i - 1) % 5 * 100, 100.f + (i - 1) / 5 * 120}, [count] { ++*count; });
        scene->scheduleOnce([captured, directory](float) {
            utils::captureScreen([captured](bool ok, std::string_view) {
                if (!ok)
                    throw std::runtime_error("Particle screenshot failed");
                *captured = true;
            }, std::string(directory) + "/particles.png");
        }, .2f, "capture");
        scene->scheduleOnce([count, captured](float) {
            if (*count != 10 || !*captured)
                throw std::runtime_error("Particle completion failed");
            Director::getInstance()->end();
        }, 1.4f, "done");
        return true;
    }
    if (std::getenv("RB_PROBE_EFFECTS"))
    {
        auto* session = GameSession::create(data, 1, 1, language, 1, false);
        scene->addChild(session);
        auto* room     = session->room();
        int object     = 0;
        Sprite* sprite = nullptr;
        for (const auto& [id, state] : session->state().objects)
        {
            auto* candidate = room->image(rb::string(data.byId("ObjectState", "stateID", state.state), "imageName"));
            if (candidate)
            {
                object = id;
                sprite = candidate;
                break;
            }
        }
        if (!sprite)
            throw std::runtime_error("No effect target");
        const auto position = sprite->getPosition();
        auto count = std::make_shared<int>(0), phase = std::make_shared<int>(0);
        scene->schedule([scene, room, object, sprite, position, count, phase](float) {
            if (*phase)
            {
                if (*count != *phase * 2 || sprite->getPosition().distance(position) > .001f ||
                    std::abs(sprite->getRotation()) > .001f || std::abs(sprite->getScale() - 1) > .001f)
                    throw std::runtime_error("Object effect did not restore transform/completion");
            }
            if (*phase == 3)
            {
                Director::getInstance()->end();
                return;
            }
            const int type = ++*phase;
            room->objectEffect(object, type, [count] { ++*count; });
            room->objectEffect(object, type, [count] { ++*count; });
        }, .8f, "effects");
        return true;
    }
    if (std::getenv("RB_PROBE_GOLD"))
    {
        auto progress = std::make_shared<rb::Progress>();
        progress->initialize(data);
        auto* session = GameSession::create(data, 1, 1, language, 1, false);
        scene->addChild(session);
        session->bindProgress(*progress);
        const int stageID                    = rb::integer(*data.stage("StageTable", 1, 1).front(), "stageID");
        auto saves                           = std::make_shared<int>(0);
        auto sounds                          = std::make_shared<int>(0);
        progress->stages.at(stageID).goldKey = false;
        session->progressChanged             = [saves] { ++*saves; };
        session->sound                       = [sounds](std::string_view file, bool) {
            if (file == "snd_f10")
                ++*sounds;
        };
        session->runEvent(12780);
        if (!session->busy() || !progress->stages.at(stageID).goldKey || *saves != 1)
            throw std::runtime_error("Gold key update failed");
        scene->scheduleOnce([session, directory](float) {
            utils::captureScreen([](bool ok, std::string_view) {
                if (!ok)
                    throw std::runtime_error("Gold screenshot failed");
            }, std::string(directory) + "/gold-key.png");
        }, 1.5f, "capture");
        scene->scheduleOnce([session, progress, saves, sounds](float) {
            if (session->busy() || *saves != 1 || *sounds != 1)
                throw std::runtime_error("Gold completion failed");
            session->runEvent(12780);
        }, 3.3f, "again");
        scene->scheduleOnce([session, progress, saves, sounds](float) {
            if (session->busy() || *saves != 1 || *sounds != 2)
                throw std::runtime_error("Gold duplicate update failed");
            Director::getInstance()->end();
        }, 6.6f, "done");
        return true;
    }
    if (std::getenv("RB_PROBE_MOVIE"))
    {
        auto index = std::make_shared<int>(0);
        scene->schedule([scene, index, directory](float) {
            if (*index < 48)
            {
                const int n = (*index)++;
                auto* movie = EpisodeMovie::create(n % 6 + 1, (n / 6) % 2, n / 12);
                scene->addChild(movie);
                if (!movie->panelCount())
                    throw std::runtime_error("Empty movie");
                movie->removeFromParent();
                return;
            }
            scene->unschedule("catalog");
            auto* movie = EpisodeMovie::create(1, false, 0);
            scene->addChild(movie);
            auto sounds     = std::make_shared<std::vector<std::string>>();
            auto saved      = std::make_shared<float>(0);
            auto completed  = std::make_shared<int>(0);
            movie->music    = [sounds](std::string_view file) { sounds->emplace_back(file); };
            movie->complete = [sounds, completed] {
                if (++*completed != 1 || sounds->size() != 2 || sounds->at(0) != "snd_b1" || !sounds->at(1).empty())
                    throw std::runtime_error("Movie audio/completion failed");
                Director::getInstance()->end();
            };
            movie->open();
            movie->skip();
            if (movie->playing())
                throw std::runtime_error("Movie opening guard failed");
            scene->scheduleOnce([movie, saved](float) {
                if (!movie->playing() || movie->offset() <= 0)
                    throw std::runtime_error("Movie did not scroll");
                *saved = movie->offset();
                movie->setFast(true);
            }, 1.5f, "fast");
            scene->scheduleOnce([movie, saved, directory](float) {
                if (movie->offset() - *saved < 70)
                    throw std::runtime_error("Movie fast scroll failed");
                movie->setFast(false);
                utils::captureScreen([movie](bool ok, std::string_view) {
                    if (!ok)
                        throw std::runtime_error("Movie screenshot failed");
                    movie->skip();
                    movie->skip();
                }, std::string(directory) + "/movie.png");
            }, 2, "capture");
        }, .03f, "catalog");
        return true;
    }
    if (std::getenv("RB_PROBE_CLEAR"))
    {
        auto progress = std::make_shared<rb::Progress>();
        progress->initialize(data);
        auto* session = GameSession::create(data, 2, 3, language, 1, false);
        scene->addChild(session);
        session->bindProgress(*progress);
        auto saves = std::make_shared<int>(0), retries = std::make_shared<int>(0), accepts = std::make_shared<int>(0);
        session->progressChanged = [saves] { ++*saves; };
        session->retryRequested  = [retries] { ++*retries; };
        session->showClear();
        auto* clear = session->clearOverlay();
        if (!clear || clear->rank() != "s" || *saves != 1 || !progress->stageOpened(data, 2, 4))
            throw std::runtime_error("Clear progression failed");
        session->showClear();
        clear->choose(false);
        session->tap({-10000, -10000});
        if (*saves != 1 || *retries || session->touchCount())
            throw std::runtime_error("Duplicate or early clear input");
        scene->scheduleOnce([scene, session, clear, progress, saves, retries, accepts, directory](float) {
            if (!clear->ready())
                throw std::runtime_error("Clear buttons not ready");
            utils::captureScreen([scene, session, clear, progress, saves, retries, accepts](bool ok, std::string_view) {
                if (!ok)
                    throw std::runtime_error("Clear screenshot failed");
                clear->choose(false);
                clear->choose(false);
                scene->scheduleOnce([scene, session, progress, saves, retries, accepts](float) {
                    if (*saves != 1 || *retries != 1 || session->clearOverlay())
                        throw std::runtime_error("Clear retry failed");
                    auto* second = ClearOverlay::create("d");
                    scene->addChild(second, 30);
                    second->complete = [accepts](bool accepted) {
                        if (!accepted || ++*accepts != 1)
                            throw std::runtime_error("Clear accept failed");
                        Director::getInstance()->end();
                    };
                    second->open();
                    second->scheduleOnce([second](float) {
                        second->choose(true);
                        second->choose(true);
                    }, 2.6f, "accept");
                }, .7f, "retry-result");
            }, std::string(directory) + "/clear.png");
        }, 2.6f, "capture");
        return true;
    }
    if (std::getenv("RB_PROBE_GAMEOVER"))
    {
        auto* session = GameSession::create(data, 2, 3, language, 1, false);
        scene->addChild(session);
        session->state().health = 0;
        auto retries = std::make_shared<int>(0), exits = std::make_shared<int>(0);
        session->retryRequested = [retries] { ++*retries; };
        session->exitRequested  = [exits] { ++*exits; };
        session->tap({-10000, -10000});
        auto* over = session->gameOverOverlay();
        if (!over)
            throw std::runtime_error("No game over on exhausted tap");
        over->choose(false);
        if (*retries)
            throw std::runtime_error("Game over accepted early choice");
        const int touches = session->touchCount();
        session->tap({-10000, -10000});
        if (session->touchCount() != touches)
            throw std::runtime_error("Game over accepted gameplay input");
        scene->scheduleOnce([scene, session, over, retries, exits, directory](float) {
            if (!over->ready())
                throw std::runtime_error("Game over not ready");
            utils::captureScreen([scene, session, over, retries, exits](bool ok, std::string_view) {
                if (!ok)
                    throw std::runtime_error("Game over screenshot failed");
                over->choose(false);
                if (*retries != 1 || *exits || session->gameOverOverlay())
                    throw std::runtime_error("Retry outcome failed");
                auto* second = GameOverOverlay::create();
                scene->addChild(second, 20);
                second->complete = [retries, exits](bool exit) {
                    if (!exit)
                        throw std::runtime_error("Wrong exit outcome");
                    ++*exits;
                    if (*retries != 1 || *exits != 1)
                        throw std::runtime_error("Duplicate game over outcome");
                    Director::getInstance()->end();
                };
                second->open();
                second->scheduleOnce([second](float) { second->choose(true); }, 1.7f, "exit");
            }, std::string(directory) + "/game-over.png");
        }, 1.7f, "capture");
        return true;
    }
    if (std::getenv("RB_PROBE_INTRO"))
    {
        auto* session = GameSession::create(data, 2, 3, language, 1, false);
        scene->addChild(session);
        auto sounds    = std::make_shared<std::vector<std::string>>();
        auto captured  = std::make_shared<bool>(false);
        session->sound = [sounds](std::string_view name, bool) { sounds->emplace_back(name); };
        session->startWithIntro();
        session->tap({-10000, -10000});
        if (!session->stageIntro() || session->touchCount() != 0 || session->state().health != 100)
            throw std::runtime_error("Intro input protection failed");
        scene->scheduleOnce([session, captured, directory](float) {
            if (!session->stageIntro())
                throw std::runtime_error("Intro ended before title hold");
            utils::captureScreen([captured](bool ok, std::string_view) {
                if (!ok)
                    throw std::runtime_error("Intro screenshot failed");
                *captured = true;
            }, std::string(directory) + "/stage-intro.png");
        }, 3.1f, "capture");
        scene->scheduleOnce([session, sounds, captured](float) {
            if (session->stageIntro() || !*captured || sounds->empty() || sounds->front() != "snd_f05")
                throw std::runtime_error("Intro completion failed");
            int count = 0;
            for (const auto& sound : *sounds)
                if (sound == "snd_f05")
                    ++count;
            if (count != 1)
                throw std::runtime_error("Intro started twice");
            Director::getInstance()->end();
        }, 6, "complete");
        return true;
    }
    if (std::getenv("RB_PROBE_MENU"))
    {
        auto progress = std::make_shared<rb::Progress>();
        progress->initialize(data);
        progress->eyes    = 2;
        progress->insight = 3;
        progress->hands   = 4;
        progress->feet    = 5;
        auto* session     = GameSession::create(data, 2, 3, language, 1, false);
        scene->addChild(session);
        session->bindProgress(*progress);
        session->openSystemMenu();
        auto* menu = session->systemMenu();
        auto phase = std::make_shared<int>(0), retry = std::make_shared<int>(0), exits = std::make_shared<int>(0),
             purchase              = std::make_shared<int>(0);
        auto capturing             = std::make_shared<bool>(false);
        session->retryRequested    = [retry] { ++*retry; };
        session->exitRequested     = [exits] { ++*exits; };
        session->purchaseRequested = [purchase](int i) {
            if (i != 1)
                throw std::runtime_error("Wrong shop product");
            ++*purchase;
        };
        session->menuAudio = [session, retry, exits, purchase](bool paused) {
            if (paused)
                return;
            if (session->systemMenu() || *retry != 1 || *exits != 1 || *purchase != 1)
                throw std::runtime_error("Menu callbacks failed");
            Director::getInstance()->end();
        };
        scene->schedule([scene, menu, phase, retry, exits, purchase, capturing, directory, progress](float) {
            if (*capturing)
                return;
            auto capture = [capturing, directory](const char* name) {
                *capturing = true;
                utils::captureScreen([capturing](bool ok, std::string_view) {
                    if (!ok)
                        throw std::runtime_error("Menu screenshot failed");
                    *capturing = false;
                }, std::string(directory) + "/" + name + ".png");
            };
            switch ((*phase)++)
            {
            case 0:
                if (menu->mode() != 1)
                    throw std::runtime_error("Menu open failed");
                capture("main");
                break;
            case 1:
                menu->click(1);
                break;
            case 2:
                menu->click(0);
                if (menu->settings().music || !menu->settings().effects)
                    throw std::runtime_error("BGM/effects mapping reversed");
                menu->click(1);
                menu->click(2);
                menu->click(3);
                break;
            case 3:
            {
                auto settings = menu->settings();
                if (settings.effects || settings.music || settings.vibration || settings.textSpeed != 2)
                    throw std::runtime_error("Options failed");
                capture("options");
                break;
            }
            case 4:
                menu->click(100);
                menu->click(2);
                menu->click(1);
                break;
            case 5:
                capture("shop");
                menu->click(1);
                break;
            case 6:
                if (*purchase != 1)
                    throw std::runtime_error("Shop selection failed");
                menu->click(100);
                menu->click(0);
                break;
            case 7:
                if (!menu->messageBox()->ready())
                    throw std::runtime_error("Retry message not ready");
                menu->messageBox()->answer(false);
                break;
            case 8:
                if (*retry)
                    throw std::runtime_error("Cancelled retry fired");
                menu->click(0);
                break;
            case 9:
                menu->messageBox()->answer(true);
                break;
            case 10:
                menu->click(3);
                break;
            case 11:
                menu->messageBox()->answer(true);
                break;
            case 12:
                scene->unschedule("menu-probe");
                menu->click(100);
                break;
            }
        }, 1.0f, "menu-probe");
        scene->scheduleOnce([](float) { throw std::runtime_error("Menu probe timed out"); }, 20, "timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_PAUSE"))
    {
        auto* session = GameSession::create(data, 2, 3, language, 1, false);
        scene->addChild(session);
        session->showUI(true);
        auto count              = std::make_shared<int>(0);
        session->pauseRequested = [session, count] {
            ++*count;
            session->setMenuOpen(true);
        };
        scene->scheduleOnce([session, count, directory](float) {
            utils::captureScreen([session, count](bool ok, std::string_view) {
                if (!ok)
                    throw std::runtime_error("Pause screenshot failed");
                if (!session->pauseButton()->beginTouch({460, 305}))
                    throw std::runtime_error("Pause hit failed");
                session->pauseButton()->endTouch({460, 305});
                if (*count != 1 || session->pauseButton()->beginTouch({460, 305}))
                    throw std::runtime_error("Pause repeated or not activated");
                session->tap({-10000, -10000});
                if (session->touchCount() != 0 || session->state().health != 100)
                    throw std::runtime_error("Menu accepted scene input");
                session->setMenuOpen(false);
                session->scheduleOnce([session, count](float) {
                    session->tap({-10000, -10000});
                    if (session->touchCount() != 1 || session->state().health != 99)
                        throw std::runtime_error("Resume did not restore scene input");
                    if (!session->pauseButton()->beginTouch({460, 305}))
                        throw std::runtime_error("Pause not restored");
                    session->pauseButton()->endTouch({0, 0});
                    if (*count != 1)
                        throw std::runtime_error("Cancelled pause fired");
                    Director::getInstance()->end();
                }, .6f, "finish");
            }, std::string(directory) + "/pause-hud.png");
        }, .6f, "capture");
        scene->scheduleOnce([](float) { throw std::runtime_error("Pause probe timed out"); }, 5, "timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_HEALTH"))
    {
        auto* session = GameSession::create(data, 2, 3, language, 1, false);
        scene->addChild(session);
        session->heroHealth()->show(true);
        if (session->state().health != 100)
            throw std::runtime_error("Original HUD health reset missing");
        auto deaths       = std::make_shared<int>(0);
        session->gameOver = [deaths] { ++*deaths; };
        for (int i = 0; i < 50; ++i)
            session->tap({-10000, -10000});
        if (session->state().health != 50 || session->heroHealth()->opacity(0) != 0 ||
            session->heroHealth()->opacity(1) != 0 || std::abs(session->heroHealth()->opacity(2) - 127) > 1 ||
            session->heroHealth()->opacity(3) != 255 || session->heroHealth()->opacity(4) != 255)
            throw std::runtime_error("Health bar segmentation failed");
        scene->scheduleOnce([session, deaths, directory](float) {
            utils::captureScreen([session, deaths](bool ok, std::string_view) {
                if (!ok)
                    throw std::runtime_error("Health screenshot failed");
                for (int i = 0; i < 50; ++i)
                    session->tap({-10000, -10000});
                if (*deaths != 0 || session->state().health != 0)
                    throw std::runtime_error("Health exhausted too early");
                session->tap({-10000, -10000});
                if (*deaths != 1)
                    throw std::runtime_error("Health did not exhaust on next tap");
                session->fillHealth();
                session->scheduleOnce([session](float) {
                    if (session->state().health != 100)
                        throw std::runtime_error("Health refill value failed");
                    for (int i = 0; i < 5; ++i)
                        if (session->heroHealth()->opacity(i) != 255)
                            throw std::runtime_error("Health refill fade failed");
                    Director::getInstance()->end();
                }, .6f, "finish");
            }, std::string(directory) + "/health-half.png");
        }, .6f, "capture");
        scene->scheduleOnce([](float) { throw std::runtime_error("Health probe timed out"); }, 6, "timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_ABILITY_HUD"))
    {
        auto progress = std::make_shared<rb::Progress>();
        progress->initialize(data);
        progress->eyes = 2;
        progress->feet = 3;
        auto* session  = GameSession::create(data, 2, 3, language, 1, false);
        scene->addChild(session);
        session->bindProgress(*progress, [] { throw std::runtime_error("Unexpected HUD shop"); });
        session->abilityInfo()->show(true);
        scene->scheduleOnce([session, progress, directory](float) {
            if (!session->abilityInfo()->beginTouch({65, 295}))
                throw std::runtime_error("Eyes HUD not clickable");
            session->abilityInfo()->endTouch({65, 295});
            if (!session->messageBox())
                throw std::runtime_error("Eyes HUD did not request confirmation");
            session->scheduleOnce([session, progress, directory](float) {
                session->messageBox()->answer(false);
                session->scheduleOnce([session, progress, directory](float) {
                    if (session->messageBox() || progress->eyes != 2)
                        throw std::runtime_error("HUD cancel changed eyes count");
                    session->inventory()->open();
                    if (session->abilityInfo()->beginTouch({65, 295}))
                        throw std::runtime_error("HUD accepted input behind inventory");
                    session->inventory()->close();
                    session->scheduleOnce([session, progress, directory](float) {
                        utils::captureScreen([session, progress](bool ok, std::string_view) {
                            if (!ok)
                                throw std::runtime_error("HUD capture failed");
                            if (!session->abilityInfo()->beginTouch({23, 295}))
                                throw std::runtime_error("Feet HUD not clickable");
                            session->abilityInfo()->endTouch({23, 295});
                            session->scheduleOnce([session, progress](float) {
                                session->messageBox()->answer(false);
                                session->scheduleOnce([session, progress](float) {
                                    if (progress->feet != 3 || session->messageBox())
                                        throw std::runtime_error("Feet HUD cancel failed");
                                    Director::getInstance()->end();
                                }, .5f, "finish");
                            }, .5f, "cancel-foot");
                        }, std::string(directory) + "/hud.png");
                    }, .6f, "capture");
                }, .5f, "after-cancel");
            }, .5f, "cancel");
        }, .6f, "click");
        scene->scheduleOnce([](float) { throw std::runtime_error("HUD probe timed out"); }, 8, "timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_ABILITIES"))
    {
        auto progress = std::make_shared<rb::Progress>();
        progress->initialize(data);
        progress->eyes = progress->hands = progress->feet = 1;
        auto sounds                                       = std::make_shared<std::vector<std::string>>();
        auto clears                                       = std::make_shared<int>(0);
        auto* session                                     = GameSession::create(data, 2, 3, language, 1, false);
        scene->addChild(session);
        session->state().objects.at(63).disappeared = true;
        session->sound   = [sounds](std::string_view file, bool) { sounds->emplace_back(file); };
        session->overlay = [clears](const rb::Command& c, rb::EventRunner::Complete done) {
            if (c.type != rb::Command::Type::Clear)
                throw std::runtime_error("Feet emitted wrong event");
            ++*clears;
            done(true);
        };
        auto* tutorial = GameSession::create(data, 0, 1, language, 1, false);
        scene->addChild(tutorial);
        if (tutorial->useAbility(*progress, 0) || progress->eyes != 1)
            throw std::runtime_error("Tutorial eyes guard failed");
        tutorial->removeFromParent();
        session->useAbility(*progress, 0, [session, progress, sounds, clears, directory] {
            if (progress->eyes != 0 || !progress->achievements.contains(12) || session->room()->isShining(63) ||
                !session->room()->isShining(64) || session->room()->isShining(1315))
                throw std::runtime_error("Eyes highlight selection failed");
            session->scheduleOnce([session, progress, sounds, clears, directory](float) {
                utils::captureScreen([session, progress, sounds, clears](bool ok, std::string_view) {
                    if (!ok)
                        throw std::runtime_error("Eyes screenshot failed");
                    session->moveRoom(17, 0, [session, progress, sounds, clears] {
                        session->moveRoom(16, 0, [session, progress, sounds, clears] {
                            if (!session->room()->isShining(64) || session->room()->isShining(63))
                                throw std::runtime_error("Eyes room restoration failed");
                            session->state().objects.at(64).disappeared = true;
                            session->room()->refreshShining();
                            if (session->room()->isShining(64))
                                throw std::runtime_error("Collected object still shines");
                            if (!session->useAbility(*progress, 2, [session, progress, sounds, clears] {
                                if (progress->hands != 0)
                                    throw std::runtime_error("Hands deduction failed");
                                if (!session->useAbility(*progress, 3, [progress, sounds, clears] {
                                    if (progress->feet != 0 || *clears != 1 ||
                                        *sounds != std::vector<std::string>{"snd_f45", "snd_f47"})
                                        throw std::runtime_error("Ability result or sound mismatch");
                                    Director::getInstance()->end();
                                }))
                                    throw std::runtime_error("Feet refused");
                            }))
                                throw std::runtime_error("Hands refused");
                        });
                    });
                }, std::string(directory) + "/eyes.png");
            }, .8f, "shine-capture");
        });
        if (session->useAbility(*progress, 3) || progress->feet != 1)
            throw std::runtime_error("Concurrent ability accepted");
        session->tap({0, 0});
        if (session->touchCount())
            throw std::runtime_error("Ability accepted room tap");
        scene->scheduleOnce([](float) { throw std::runtime_error("Abilities probe timed out"); }, 20, "timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_MESSAGE"))
    {
        auto responses = std::make_shared<int>(0);
        auto* box =
            GameMessageBox::create("Would you like to use this ability?", 2, true, [scene, responses](bool yes) {
            if (!yes)
                throw std::runtime_error("Message confirmation failed");
            ++*responses;
            auto* cancel =
                GameMessageBox::create("Cancel\nwithout using an ability", 2, true, [scene, responses](bool accepted) {
                if (accepted)
                    throw std::runtime_error("Message cancellation failed");
                ++*responses;
                auto* info = GameMessageBox::create("Information", 2, false, [responses](bool accepted) {
                    if (accepted || *responses != 2)
                        throw std::runtime_error("Message information result failed");
                    Director::getInstance()->end();
                });
                scene->addChild(info, 30);
                info->scheduleOnce([info](float) { info->answer(true); }, .5f, "answer");
            });
            scene->addChild(cancel, 30);
            cancel->scheduleOnce([cancel](float) { cancel->answer(false); }, .5f, "answer");
        });
        scene->addChild(box, 30);
        box->answer(true);
        if (*responses != 0)
            throw std::runtime_error("Message accepted input while opening");
        box->scheduleOnce([box, directory](float) {
            if (!box->ready())
                throw std::runtime_error("Message did not finish opening");
            utils::captureScreen([box](bool ok, std::string_view) {
                if (!ok)
                    throw std::runtime_error("Message screenshot failed");
                box->answer(true);
            }, std::string(directory) + "/message.png");
        }, .6f, "capture");
        scene->scheduleOnce([](float) { throw std::runtime_error("Message probe timed out"); }, 8, "timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_ABILITY_CONFIRM"))
    {
        auto progress = std::make_shared<rb::Progress>();
        progress->initialize(data);
        progress->insight = 1;
        auto shopCalls = std::make_shared<int>(0), phase = std::make_shared<int>(0);
        auto* session = GameSession::create(data, 2, 3, language, 1, false);
        scene->addChild(session);
        session->state().addItem(24);
        session->bindProgress(*progress, [shopCalls] { ++*shopCalls; });
        session->inventory()->show(true);
        session->inventory()->open();
        scene->scheduleOnce([session](float) { session->requestInsight(24); }, .6f, "request");
        scene->schedule([scene, session, progress, shopCalls, phase, directory](float) {
            auto* message = session->messageBox();
            if (*phase == 0 && message && message->ready())
            {
                if (progress->insight != 1)
                    throw std::runtime_error("Ability deducted before confirmation");
                message->answer(false);
                *phase = 1;
            }
            else if (*phase == 1 && !message)
            {
                if (progress->insight != 1 || session->insight())
                    throw std::runtime_error("Cancelled ability was used");
                progress->insight = 0;
                session->requestInsight(24);
                *phase = 2;
            }
            else if (*phase == 2 && message && message->ready())
            {
                message->answer(true);
                *phase = 3;
            }
            else if (*phase == 3 && !message)
            {
                if (*shopCalls != 1 || session->insight() || progress->insight != 0)
                    throw std::runtime_error("Empty ability shop path failed");
                progress->insight = 1;
                session->inventory()->refreshAbility();
                *phase = 4;
                utils::captureScreen([session](bool ok, std::string_view) {
                    if (!ok)
                        throw std::runtime_error("Ability inventory capture failed");
                    // Button occupies the original top-left location (275,25).
                    session->inventory()->beginTouch({300, 275});
                    session->inventory()->endTouch({300, 275});
                }, std::string(directory) + "/inventory-ability.png");
            }
            else if (*phase == 4 && message && message->ready())
            {
                message->answer(true);
                *phase = 5;
            }
            else if (*phase == 5 && session->insight())
            {
                if (progress->insight != 0 || session->inventory()->opened())
                    throw std::runtime_error("Confirmed ability start failed");
                scene->unschedule("confirm-poll");
                Director::getInstance()->end();
            }
        }, .1f, "confirm-poll");
        scene->scheduleOnce([](float) { throw std::runtime_error("Ability confirmation probe timed out"); }, 12,
                            "timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_INSIGHT"))
    {
        auto progress = std::make_shared<rb::Progress>();
        progress->initialize(data);
        progress->insight = 0;
        auto* session     = GameSession::create(data, 2, 3, language, 1, false);
        scene->addChild(session);
        session->state().addItem(24);
        if (session->useInsight(*progress, 24))
            throw std::runtime_error("Zero-count insight accepted");
        progress->insight = 1;
        auto* tutorial    = GameSession::create(data, 0, 1, language, 1, false);
        scene->addChild(tutorial);
        tutorial->state().addItem(269);
        if (tutorial->useInsight(*progress, 269) || progress->insight != 1)
            throw std::runtime_error("Tutorial insight restriction failed");
        tutorial->removeFromParent();
        auto marked = std::make_shared<bool>(false), captured = std::make_shared<bool>(false),
             closing = std::make_shared<bool>(false);
        scene->scheduleOnce([session, progress](float) {
            if (!session->useInsight(*progress, 24) || progress->insight != 0)
                throw std::runtime_error("Insight start/count failed");
            session->tap({-100000, -100000});
            if (session->touchCount() != 0)
                throw std::runtime_error("Insight accepted scene tap");
            if (session->useInsight(*progress, 24) || progress->insight != 0)
                throw std::runtime_error("Insight consumed twice");
        }, .1f, "insight-start");
        const std::string output(directory);
        scene->schedule([scene, session, progress, marked, captured, closing, output](float) {
            if (auto* insight = session->insight())
            {
                if (insight->markedObject() == 103 && session->state().room == 17)
                    *marked = true;
                if (insight->ready() && !*closing)
                {
                    *closing = true;
                    utils::captureScreen([session, captured](bool ok, std::string_view) {
                        if (!ok)
                            throw std::runtime_error("Insight screenshot failed");
                        *captured = true;
                        session->insight()->close();
                    }, output + "/insight-info.png");
                }
                return;
            }
            if (!*closing)
                return;
            if (!*marked || !*captured || session->state().room != 16 || progress->insight != 0 ||
                !progress->achievements.contains(12))
                throw std::runtime_error("Insight target, return, or progress mismatch");
            scene->unschedule("insight-poll");
            AXLOGI(
                "RB_INSIGHT_PROBE PASS: count, tutorial restriction, touch protection, cross-room target, information, "
                "return");
            Director::getInstance()->end();
        }, .1f, "insight-poll");
        scene->scheduleOnce([](float) { throw std::runtime_error("Insight probe timed out"); }, 30, "insight-timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_ROOM_MOVE"))
    {
        auto* session = GameSession::create(data, 2, 3, language, 1, false);
        scene->addChild(session);
        auto changes         = std::make_shared<int>(0);
        session->roomChanged = [changes] { ++*changes; };
        auto completed       = std::make_shared<bool>(false);
        scene->scheduleOnce([session, completed](float) {
            session->moveRoom(17, 2, [completed] { *completed = true; });
            if (session->state().room != 17)
                throw std::runtime_error("Room ID was not updated at transition start");
        }, .1f, "room-move");
        scene->scheduleOnce([session](float) {
            session->tap({-100000, -100000});
            if (session->touchCount() != 0)
                throw std::runtime_error("Room transition accepted tap");
        }, .3f, "room-block");
        scene->scheduleOnce([session, changes, completed](float) {
            const auto* bg = session->room()->background();
            if (!*completed || *changes != 1 ||
                std::abs(bg->getPositionX() + (bg->getContentSize().width - 480) / 2) > .01f)
                throw std::runtime_error("Room transition did not finish at requested position");
            session->runEvent(903);
        }, 1.4f, "room-return");
        scene->scheduleOnce([session, changes](float) {
            if (session->state().room != 16 || session->busy() || *changes != 2 ||
                session->room()->background()->getPositionX() != 0)
                throw std::runtime_error("Move-room event did not return to original room");
            AXLOGI("RB_ROOM_MOVE_PROBE PASS: target ID, touch protection, midpoint position, room event return");
            Director::getInstance()->end();
        }, 2.7f, "room-done");
        return true;
    }
    if (std::getenv("RB_PROBE_INVENTORY_CRAFT"))
    {
        const bool split = std::atoi(std::getenv("RB_PROBE_INVENTORY_CRAFT")) == 2;
        auto* session    = GameSession::create(data, split ? 0 : 2, 1, language, 1, false);
        scene->addChild(session);
        if (split)
        {
            session->state().addItem(269);
            session->state().addItem(272);
            session->state().addItem(270);
        }
        else
        {
            session->state().addItem(3);
            session->state().addItem(4);
        }
        auto* inventory = session->inventory();
        inventory->show(true);
        scene->scheduleOnce([inventory, split](float) {
            inventory->open();
            inventory->selectMode(split ? 2 : 3);
        }, .6f, "craft-open");
        scene->scheduleOnce([inventory, split](float) { inventory->beginTouch({split ? 113.f : 48.f, 208}); }, 1.3f,
                            "craft-hold");
        scene->scheduleOnce([inventory, split](float) {
            if (split)
            {
                inventory->moveTouch({133, 208});
                inventory->moveTouch({153, 208});
            }
            else
                inventory->moveTouch({113, 208});
            inventory->endTouch({split ? 153.f : 113.f, 208});
        }, 2.1f, "craft-drag");
        const std::string output(directory);
        auto captured = std::make_shared<bool>(false);
        scene->scheduleOnce([inventory, session, split, output, captured](float) {
            const auto expected = split ? std::vector<int>{269, 273, 270, 277} : std::vector<int>{5};
            if (session->state().inventory() != expected || inventory->operating())
                throw std::runtime_error("Inventory craft gesture or completion failed");
            utils::captureScreen([captured](bool ok, std::string_view) {
                if (!ok)
                    throw std::runtime_error("Craft screenshot failed");
                *captured = true;
            }, output + "/inventory-craft.png");
        }, 4.9f, "craft-capture");
        scene->schedule([scene, inventory, session, split, captured](float) {
            if (!*captured)
                return;
            scene->unschedule("craft-done");
            inventory->selectMode(1);
            inventory->beginTouch({210, 170});
            inventory->endTouch({210, 170});
            if (session->pocket()->item() != (split ? 277 : 5))
                throw std::runtime_error("Craft result not selectable");
            AXLOGI("RB_INVENTORY_CRAFT_PROBE PASS: long press, drag, recipe, animation completion, result selection");
            Director::getInstance()->end();
        }, .2f, "craft-done");
        scene->scheduleOnce([](float) { throw std::runtime_error("Craft probe timed out"); }, 20, "craft-timeout");
        return true;
    }
    if (std::getenv("RB_PROBE_INVENTORY"))
    {
        auto* session = GameSession::create(data, 0, 1, language, 1, false);
        scene->addChild(session);
        for (int id : {269, 272, 270})
            session->state().addItem(id);
        auto* inventory = session->inventory();
        inventory->show(true);
        scene->scheduleOnce([inventory](float) {
            if (!inventory->beginTouch({12, 240}))
                throw std::runtime_error("Inventory handle not reachable");
            inventory->moveTouch({102, 240});
            inventory->endTouch({102, 240});
            if (!inventory->opened() || inventory->selectedIndex() != 2)
                throw std::runtime_error("Inventory drag opening failed");
        }, .6f, "inventory-open");
        const std::string output(directory);
        scene->scheduleOnce([inventory, output](float) {
            inventory->beginTouch({210, 170});
            inventory->moveTouch({275, 170});
            inventory->endTouch({275, 170});
            if (inventory->selectedIndex() != 1 || !inventory->opened())
                throw std::runtime_error("Inventory swipe selected wrong item");
        }, 1.3f, "inventory-swipe");
        auto captured = std::make_shared<bool>(false);
        scene->scheduleOnce([output, captured](float) {
            utils::captureScreen([captured](bool ok, std::string_view) {
                if (!ok)
                    throw std::runtime_error("Inventory screenshot failed");
                *captured = true;
            }, output + "/inventory-use.png");
        }, 2, "inventory-capture");
        scene->scheduleOnce([inventory, session](float) {
            inventory->beginTouch({210, 170});
            inventory->endTouch({210, 170});
            if (inventory->opened() || session->pocket()->selected() != 3 || session->pocket()->item() != 272)
                throw std::runtime_error("Inventory item selection did not reach cursor");
        }, 2.2f, "inventory-select");
        scene->scheduleOnce([scene, captured](float) {
            scene->schedule([scene, captured](float) {
                if (!*captured)
                    return;
                scene->unschedule("inventory-done");
                AXLOGI("RB_INVENTORY_PROBE PASS: handle drag, initial order, swipe, tap selection, cursor sync");
                Director::getInstance()->end();
            }, .1f, "inventory-done");
        }, 2.9f, "inventory-wait-capture");
        return true;
    }
    if (std::getenv("RB_PROBE_GET_ITEM"))
    {
        auto* session = GameSession::create(data, 2, 1, language, 1, false);
        scene->addChild(session);
        auto sounds    = std::make_shared<int>(0);
        session->sound = [sounds](std::string_view name, bool) {
            if (name == "snd_f16")
                ++*sounds;
        };
        scene->scheduleOnce([session, &data](float) {
            auto* bg    = session->room()->background();
            bool tapped = false;
            for (auto* area : data.where("AreaData", "objectID", 18))
            {
                const Vec2 local((rb::integer(*area, "AreaX") + rb::integer(*area, "AreaWidth") / 2) / 2.f,
                                 bg->getContentSize().height -
                                     (rb::integer(*area, "AreaY") + rb::integer(*area, "AreaHeight") / 2) / 2.f);
                const auto point = bg->convertToWorldSpace(local);
                if (session->room()->hitObject(point) != 18)
                    continue;
                session->tap(point);
                tapped = true;
                break;
            }
            if (!tapped || !session->itemDialog() || !session->state().items.at(3) ||
                !session->state().objects.at(18).disappeared)
                throw std::runtime_error("Get-item event failed to acquire item and hide source object");
            session->itemDialog()->close();
            if (session->itemDialog()->ready())
                throw std::runtime_error("Opening item overlay accepted early close");
        }, .1f, "get-start");
        const std::string output(directory);
        scene->scheduleOnce([session, sounds, output](float) {
            if (!session->itemDialog() || !session->itemDialog()->ready() || *sounds != 1)
                throw std::runtime_error("Get-item opening or sound timing failed");
            utils::captureScreen([](bool ok, std::string_view) {
                if (!ok)
                    throw std::runtime_error("Item screenshot failed");
            }, output + "/get-item.png");
        }, .8f, "get-capture");
        scene->scheduleOnce([session](float) {
            session->itemDialog()->close();
            session->itemDialog()->close();
        }, 1.2f, "get-close");
        scene->scheduleOnce([session](float) {
            if (session->itemDialog() || session->busy() || !session->state().items.at(3))
                throw std::runtime_error("Get-item close did not complete event");
            session->selectCursor(3, 3);
            AXLOGI(
                "RB_GET_ITEM_PROBE PASS: object tap, acquisition, disappearance, sound, open/close barrier, item "
                "selection");
            Director::getInstance()->end();
        }, 1.9f, "get-done");
        return true;
    }
    if (std::getenv("RB_PROBE_CURSOR"))
    {
        auto* session = GameSession::create(data, 0, 1, language, 1, false);
        scene->addChild(session);
        auto* pocket = session->pocket();
        pocket->show(true);
        auto* feedback = CursorFeedback::create(data);
        scene->addChild(feedback, 5);
        scene->scheduleOnce([feedback](float) {
            feedback->play({160, 160}, 0, 4, 0);
            feedback->play({240, 160}, 1, 4, 0);
            feedback->play({320, 160}, 2, 2, 0);
        }, .9f, "cursor-feedback");
        scene->scheduleOnce([pocket](float) { pocket->open(); }, .6f, "cursor-open");
        const std::string output(directory);
        scene->scheduleOnce([output](float) {
            utils::captureScreen([](bool ok, std::string_view) {
                if (!ok)
                    throw std::runtime_error("Cursor screenshot failed");
            }, output + "/cursor-open.png");
        }, 1.1f, "cursor-capture");
        scene->scheduleOnce([pocket](float) {
            pocket->select(2);
            pocket->close();
        }, 1.3f, "cursor-select");
        scene->scheduleOnce([scene, session, pocket, feedback](float) {
            if (pocket->selected() != 2 || pocket->opened())
                throw std::runtime_error("Cursor selection or close failed");
            const auto rows = session->state().items;
            if (rows.empty())
                throw std::runtime_error("No tutorial item");
            const int item = rows.begin()->first;
            session->state().addItem(item);
            session->selectCursor(3, item);
            pocket->open();
            feedback->play({240, 160}, 3, 3, item);
            if (pocket->item() != item || pocket->selected() != 3)
                throw std::runtime_error("Item cursor sync failed");
        }, 1.8f, "cursor-item");
        scene->scheduleOnce([session, pocket](float) {
            pocket->consumeItem();
            if (pocket->item() != 0 || pocket->selected() != 0)
                throw std::runtime_error("Consumed item cursor not reset");
            pocket->close();
        }, 2.3f, "cursor-consume");
        scene->scheduleOnce([](float) {
            AXLOGI("RB_CURSOR_PROBE PASS: open, selection, close, item selection, consumption");
            Director::getInstance()->end();
        }, 2.8f, "cursor-done");
        return true;
    }
    if (std::getenv("RB_PROBE_SESSION"))
    {
        const int episode = std::atoi(std::getenv("RB_PROBE_SESSION"));
        const int stage   = std::getenv("RB_PROBE_STAGE") ? std::atoi(std::getenv("RB_PROBE_STAGE")) : 1;
        auto* session     = GameSession::create(data, episode, stage, language, 1, false);
        scene->addChild(session);
        scene->scheduleOnce([session](float) { session->begin(); }, .1f, "session-start");
        scene->schedule([scene, session, episode, stage](float) {
            if (auto* story = session->story())
            {
                story->advance();
                return;
            }
            if (session->busy())
                return;
            if (episode == 2 && stage == 5 && session->state().objects.at(225).state != 284)
                throw std::runtime_error("Post-story object state change did not execute");
            const int health = session->state().health;
            session->tap({-1000000, -1000000});
            if (session->touchCount() != 1 || session->state().health != health - (episode ? 1 : 0))
                throw std::runtime_error("Stage tap/health accounting mismatch");
            scene->unschedule("session-poll");
            session->removeFromParent();
            AXLOGI("RB_SESSION_PROBE PASS: stage start sequence, story continuation, tap and health");
            scene->scheduleOnce([](float) { Director::getInstance()->end(); }, .1f, "probe-exit");
        }, .2f, "session-poll");
        scene->scheduleOnce([](float) { throw std::runtime_error("Session start sequence did not complete"); }, 90,
                            "session-timeout");
        return true;
    }
    const int sceneId  = std::getenv("RB_PROBE_SCENE") ? std::atoi(std::getenv("RB_PROBE_SCENE")) : 133;
    const auto& master = data.byId("scene_master", "sceneID", sceneId);
    const int episode = rb::integer(master, "episodeNumber"), stage = rb::integer(master, "stageNumber");
    auto state = std::make_shared<rb::StageState>();
    state->reset(data, episode, stage, false);
    auto* room = RoomView::create(data, *state);
    scene->addChild(room);
    auto* story = StoryDialog::create(data, episode, sceneId, language, 1);
    scene->addChild(story, 10);
    auto captures = std::make_shared<int>(0);
    const std::string output(directory);
    scene->scheduleOnce([story, state](float) { story->open(); }, .1f, "probe-open");
    scene->scheduleOnce([story](float) { story->advance(); }, 2, "probe-skip-text");
    scene->scheduleOnce([output, captures](float) {
        utils::captureScreen([captures](bool ok, std::string_view) {
            if (!ok)
                throw std::runtime_error("Probe screenshot failed");
            ++*captures;
        }, output + "/tutorial-story.png");
    }, 3, "probe-capture");
    // An HD sheet must keep its logical dimensions on every frame, including wrap.
    auto* sheet = SheetSprite::create("dia_e5_next1.png", 4, 1);
    scene->addChild(sheet);
    sheet->setVisible(false);
    const auto size = sheet->getContentSize();
    if (size != Vec2(35, 27))
        throw std::runtime_error("HD sprite sheet is not in logical coordinates");
    for (int i = 0; i < 12; ++i)
    {
        sheet->frame(i % 4);
        if (sheet->getContentSize() != size)
            throw std::runtime_error("Sheet dimensions changed between frames");
    }
    story->complete = [scene, story, state, captures] {
        if (*captures != 1)
            throw std::runtime_error("Probe capture did not complete");
        scene->unschedule("probe-advance");
        story->removeFromParent();
        AXLOGI("RB_VISUAL_PROBE PASS: HD frame dimensions, story open/text/close, room draw");
        scene->scheduleOnce([](float) { Director::getInstance()->end(); }, .1f, "probe-exit");
    };
    scene->scheduleOnce([scene, story](float) {
        scene->schedule([story](float) { story->advance(); }, .2f, "probe-advance");
    }, 3.2f, "probe-start-advance");
    scene->scheduleOnce([](float) { throw std::runtime_error("Story did not complete within 30 seconds"); }, 30,
                        "probe-timeout");
    return true;
}
