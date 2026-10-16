#include "MainScene.h"
#include "TitleShop.h"
#include "StageSelect.h"
#include "GameSession.h"
#include "EpisodeMovie.h"
#include "ClearOverlay.h"
#include "StoryDialog.h"
#include "RoomView.h"
#include "audio/AudioEngine.h"
#include <cstdlib>
#include <memory>
#include <stdexcept>
using namespace ax;
bool runMainFlowProbe(MainScene* scene)
{
    const char* directory = std::getenv("RB_VISUAL_PROBE_OUTPUT");
    if (!directory || !std::getenv("RB_PROBE_SAVE"))
        throw std::runtime_error("Flow probe requires isolated save and output paths");
    if (std::getenv("RB_MEDIA_TITLE"))
    {
        scene->schedule([scene, directory](float) {
            if (scene->loading_ || !scene->title_)
                return;
            scene->unschedule("media-title");
            scene->scheduleOnce([directory](float) {
                utils::captureScreen([](bool ok, std::string_view) {
                    if (!ok)
                        throw std::runtime_error("Title capture failed");
                    Director::getInstance()->end();
                }, std::string(directory) + "/title.png");
            }, 1, "media-title-settled");
        }, .1f, "media-title");
        return true;
    }
    auto phase   = std::make_shared<int>(-5);
    auto retries = std::make_shared<int>(0);
    scene->schedule([scene, phase, retries, directory](float) {
        if (scene->loading_ || scene->transitioning_)
            return;
        switch (*phase)
        {
        case -5:
            scene->settings_.music = true;
            scene->audio_.playMusic("EP5_3");
            ++*phase;
            break;
        case -4:
            if (scene->audio_.musicFile() != "EP5-3.mp3" || scene->audio_.musicId() < 0)
                throw std::runtime_error("Fifth chapter music did not resolve/load");
            if (AudioEngine::getState(scene->audio_.musicId()) != AudioEngine::AudioState::PLAYING)
                return;
            AXLOGI("RB_PROBE_MUSIC PASS: EP5_3 resolves to EP5-3 and plays");
            scene->audio_.playMusic("EP0");
            ++*phase;
            break;
        case -3:
            scene->click(29);
            ++*phase;
            break;
        case -2:
            if (!scene->shop_ || !scene->title_->isVisible())
                throw std::runtime_error("Title shop did not open");
            {
                auto p = scene->shop_->controlCenter(6);
                scene->shop_->beginTouch(p);
                scene->shop_->endTouch(p);
            }
            ++*phase;
            break;
        case -1:
            if (scene->shop_)
                return;
            ++*phase;
            break;
        case 0:
            scene->click(5);
            ++*phase;
            break;
        case 1:
            if (!scene->selection_ || scene->title_->isVisible())
                throw std::runtime_error("Title play did not select stages");
            scene->selection_->play(3, 1);
            ++*phase;
            break;
        case 2:
            if (!scene->movie_ || scene->session_ || !scene->progress_.seenIntros.contains(3))
                throw std::runtime_error("First stage did not start/save intro");
            if (!scene->movie_->playing())
                return;
            scene->movie_->skip();
            ++*phase;
            break;
        case 3:
            if (!scene->session_)
                return;
            if (scene->session_->stageIntro())
                return;
            if (scene->session_->story())
            {
                scene->session_->story()->advance();
                return;
            }
            if (scene->session_->busy())
                return;
            if (scene->session_->room()->directionParticleCount() != 1)
                throw std::runtime_error("First chapter dust direction missing");
            if (scene->session_->state().episode != 3 || scene->session_->state().stage != 1)
                throw std::runtime_error("Intro entered wrong stage");
            scene->session_->retryRequested();
            ++*phase;
            break;
        case 4:
            if (!scene->session_ || scene->movie_ || !scene->session_->stageIntro() || scene->session_->touchCount())
                throw std::runtime_error("Retry did not reset stage directly");
            ++*retries;
            ++*phase;
            break;
        case 5:
            if (scene->session_->stageIntro())
                return;
            if (scene->session_->story())
            {
                scene->session_->story()->advance();
                return;
            }
            if (scene->session_->busy())
                return;
            scene->session_->exitRequested();
            ++*phase;
            break;
        case 6:
            if (!scene->selection_ || !scene->selection_->opened() || scene->selection_->episode() != 1)
                throw std::runtime_error("Exit lost selected episode");
            utils::captureScreen([](bool ok, std::string_view) {
                if (!ok)
                    throw std::runtime_error("Flow selection capture failed");
            }, std::string(directory) + "/restored-selection.png");
            ++*phase;
            break;
        case 7:
            scene->selection_->movie(3, false);
            ++*phase;
            break;
        case 8:
            if (!scene->movie_)
                throw std::runtime_error("Movie replay missing");
            if (!scene->movie_->playing())
                return;
            scene->movie_->skip();
            ++*phase;
            break;
        case 9:
            if (!scene->selection_)
                return;
            if (scene->session_ || !scene->selection_->opened())
                throw std::runtime_error("Movie replay did not return to stages");
            scene->progress_.stages.at(rb::integer(*scene->data_.stage("StageTable", 3, 5).front(), "stageID")).opened =
                true;
            scene->selection_->play(3, 5);
            ++*phase;
            break;
        case 10:
            if (!scene->session_)
                return;
            if (scene->session_->stageIntro())
                return;
            if (scene->session_->story())
            {
                scene->session_->story()->advance();
                return;
            }
            if (scene->session_->busy())
                return;
            scene->session_->showClear();
            ++*phase;
            break;
        case 11:
            if (!scene->session_->clearOverlay()->ready())
                return;
            scene->session_->clearOverlay()->choose(true);
            ++*phase;
            break;
        case 12:
            if (!scene->movie_)
                return;
            if (!scene->progress_.seenEndings.contains(3))
                throw std::runtime_error("Ending was not saved");
            if (!scene->movie_->playing())
                return;
            scene->movie_->skip();
            ++*phase;
            break;
        case 13:
            if (!scene->selection_)
                return;
            if (scene->session_ || scene->movie_ || scene->selection_->episode() != 1 || !scene->selection_->opened())
                throw std::runtime_error("Ending return incorrect");
            {
                rb::Progress loaded;
                loaded.initialize(scene->data_);
                loaded.load(FileUtils::getInstance()->getStringFromFile(scene->savePath_));
                const int id = rb::integer(*scene->data_.stage("StageTable", 3, 5).front(), "stageID");
                if (!loaded.seenIntros.contains(3) || !loaded.seenEndings.contains(3) ||
                    loaded.stages.at(id).rank != "s" || !loaded.episodeOpened(2) || *retries != 1)
                    throw std::runtime_error("Owner progress did not persist");
            }
            scene->selection_->back();
            ++*phase;
            break;
        case 14:
            if (!scene->title_->isVisible() || scene->selection_ || scene->session_ || scene->movie_)
                throw std::runtime_error("Return to title failed");
            AXLOGI("RB_PROBE_FLOW PASS: title, first intro, retry, exit, replay, clear, ending, persistent progress");
            Director::getInstance()->end();
            break;
        }
    }, .1f, "flow-probe");
    scene->scheduleOnce([](float) { throw std::runtime_error("Main flow timeout"); }, 100, "flow-timeout");
    return true;
}
