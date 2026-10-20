#include "Viewport.h"
#include "GameSession.h"
#include "RoomView.h"
#include "PauseButton.h"
#include "AbilityInfo.h"
#include "InventoryView.h"
#include "Progress.h"
#include "StoryDialog.h"
#include "EpisodeDialog.h"
#include "PortraitDialog.h"
#include "Episode5Dialog.h"
#include "ResultFeedback.h"
#include <cstdlib>
#include <fstream>
#include <stdexcept>
using namespace ax;
namespace
{
void require(bool value, const char* message)
{
    if (!value) throw std::runtime_error(message);
}

template<class T> T* descendant(Node* root)
{
    for (auto* child : root->getChildren())
    {
        if (auto* match = dynamic_cast<T*>(child)) return match;
        if (auto* match = descendant<T>(child)) return match;
    }
    return nullptr;
}
bool runOverlayProbe(Scene* scene, Node* canvas, const rb::GameData& data)
{
    const char* output = std::getenv("RB_OVERLAY_VIEWPORT_PROBE");
    if (!output) return false;
#if (AX_TARGET_PLATFORM == AX_PLATFORM_MAC) || (AX_TARGET_PLATFORM == AX_PLATFORM_WIN32)
    auto* game = GameSession::create(data, 1, 3, 0, 1, false);
    canvas->addChild(game);
    auto log = std::make_shared<std::ofstream>(std::string(output) + "/checks.txt");
    auto failures = std::make_shared<int>(0);
    auto check = [log, failures](bool ok, const char* message) {
        *log << (ok ? "PASS " : "FAIL ") << message << '\n';
        log->flush();
        if (!ok) ++*failures;
    };
    auto index = std::make_shared<int>(0);
    const std::vector<Size> sizes = {{960,640}, {1440,640}, {640,800}};
    auto next = std::make_shared<std::function<void()>>();
    *next = [=, &data] {
        auto* view = static_cast<RenderViewImpl*>(Director::getInstance()->getRenderView());
        const auto size = sizes.at(*index);
        view->setWindowed(static_cast<int>(size.width), static_cast<int>(size.height));
        rb::updateViewport();
        *log << "Window " << size.width << 'x' << size.height << '\n';
        auto* result = ResultFeedback::create(true);
        canvas->addChild(result, 10);
        result->play();
        scene->scheduleOnce([=](float) {
            view->setWindowed(size.width == 1440 ? 960 : 1440, 640);
            rb::updateViewport();
            auto* bar = descendant<LayerColor>(result);
            const auto bounds = rb::viewportBounds();
            check(bar && std::abs(bar->getPositionX()-bounds.getMinX()) < .1f &&
                  std::abs(bar->getContentSize().width-bounds.size.width) < .1f,
                  "visible success band follows live resize");
            view->setWindowed(static_cast<int>(size.width), static_cast<int>(size.height));
            rb::updateViewport();
        }, .2f, "overlay-live-resize");
        scene->scheduleOnce([=, &data](float) {
            auto* bar = descendant<LayerColor>(result);
            const auto bounds = rb::viewportBounds();
            check(bar && std::abs(bar->getPositionX()-bounds.getMinX()) < .1f &&
                  std::abs(bar->getContentSize().width-bounds.size.width) < .1f,
                  "success band covers viewport width");
            Sprite* title = nullptr;
            for (auto* child : result->getChildren())
                if (auto* sprite = dynamic_cast<Sprite*>(child); sprite && !dynamic_cast<LayerColor*>(sprite))
                {
                    title = sprite;
                    break;
                }
            check(title && title->getPosition() == Vec2(240,160) &&
                  title->getScaleX() == 1 && title->getScaleY() == 1,
                  "success title stays centered and undistorted");
            utils::captureScreen([=, &data](bool ok, std::string_view) {
                require(ok, "Success screenshot failed");
                result->removeFromParent();
                // Actual EP2-3 monologue from issue #3: characterView=0, text 2821.
                auto* story = StoryDialog::create(data, 1, 101, 0, 1);
                canvas->addChild(story, 10);
                story->open();
                scene->scheduleOnce([=](float) {
                    auto* skin = descendant<EpisodeDialog>(story);
                    auto* portrait = skin ? descendant<Sprite>(skin) : nullptr;
                    check(portrait && !portrait->isVisible(), "monologue hides character portrait");
                    auto* clip = dynamic_cast<ClippingRectangleNode*>(skin);
                    check(clip && clip->getClippingRegion().equals(Rect(0,0,480,320)),
                          "dialog clips off-canvas entrance and exit artwork");
                    // The vertically sliding portrait uses the same off-canvas hiding convention.
                    auto* upper = Episode5Dialog::create(0, 1, false);
                    auto* upperPortrait = descendant<Sprite>(upper);
                    check(upperPortrait && !upperPortrait->isVisible(), "upper portrait hidden without character");
                    utils::captureScreen([=](bool ok, std::string_view) {
                        require(ok, "Monologue screenshot failed");
                        story->removeFromParent();
                        auto* visible = PortraitDialog::create(1, 0, 1, true);
                        canvas->addChild(visible, 10);
                        visible->open([=] {
                            auto* portrait = descendant<Sprite>(visible);
                            check(portrait && portrait->isVisible() && portrait->getBoundingBox().getMinX() >= 0,
                                  "character dialogue still shows full portrait");
                            visible->close([=] {
                                visible->removeFromParent();
                                if (++*index < static_cast<int>(sizes.size()))
                                    scene->scheduleOnce([next](float) { (*next)(); }, .1f, "overlay-next");
                                else
                                {
                                    *next = {};
                                    check(*failures == 0, "overlay viewport regression");
                                    if (*failures) throw std::runtime_error("Overlay viewport regression failed");
                                    Director::getInstance()->end();
                                }
                            });
                        });
                    }, std::string(output) + "/monologue-" + std::to_string(*index) + ".png");
                }, 1.2f, "overlay-monologue");
            }, std::string(output) + "/success-" + std::to_string(*index) + ".png");
        }, .7f, "overlay-success");
    };
    scene->scheduleOnce([next](float) { (*next)(); }, .5f, "overlay-start");
    return true;
#else
    return false;
#endif
}
}
bool runViewportProbe(Scene* scene, Node* canvas, const rb::GameData& data)
{
    if (runOverlayProbe(scene, canvas, data)) return true;
    const char* output = std::getenv("RB_VIEWPORT_PROBE");
    if (!output) return false;
#if (AX_TARGET_PLATFORM == AX_PLATFORM_MAC) || (AX_TARGET_PLATFORM == AX_PLATFORM_WIN32)
    auto progress = std::make_shared<rb::Progress>();
    progress->initialize(data);
    auto* game = GameSession::create(data, 2, std::getenv("RB_VIEWPORT_SMALL_ROOM") ? 1 : 2, 0, 1, false);
    canvas->addChild(game);
    game->bindProgress(*progress);
    game->showUI(true);
    auto results = std::make_shared<std::ofstream>(std::string(output) + "/checks.txt");
    auto index = std::make_shared<int>(0);
    const std::vector<Size> sizes = {{960,640}, {1280,720}, {1440,640}, {800,600}, {640,800}, {1440,480}, {960,640}};
    auto step = std::make_shared<std::function<void()>>();
    *step = [=, &data] {
        auto* view = static_cast<RenderViewImpl*>(Director::getInstance()->getRenderView());
        const auto frame = sizes.at(*index);
        view->setWindowed(static_cast<int>(frame.width), static_cast<int>(frame.height));
        rb::updateViewport();
        scene->scheduleOnce([=, &data](float) {
            auto* room = game->room();
            auto* bg = room->background();
            const auto bounds = rb::viewportBounds();
            require(std::abs(view->getScaleX() - view->getScaleY()) < .001f, "Nonuniform scale");
            require(std::abs(bounds.size.width / bounds.size.height - view->getFrameSize().width / view->getFrameSize().height) < .001f,
                    "Viewport aspect mismatch");
            const auto clip = room->getClippingRegion();
            require(std::abs(clip.size.width - std::min(bounds.size.width, bg->getContentSize().width)) < .1f,
                    "Room does not expose the available width");
            // Sample the full newly exposed area, including outside the old 480-point canvas.
            int hits = 0;
            for (float y = clip.getMinY() + 2; y < clip.getMaxY(); y += 11)
                for (float x = clip.getMinX() + 2; x < clip.getMaxX(); x += 11)
                {
                    const auto world = room->convertToWorldSpace({x,y});
                    const auto local = bg->convertToNodeSpace(world);
                    const int expected = game->state().objectAt(data, local.x, bg->getContentSize().height-local.y);
                    require(room->hitObject(world) == expected, "Object hit coordinate mismatch");
                    hits += expected != 0;
                }
            auto* pause = game->pauseButton();
            auto* button = pause->getChildren().front();
            const auto center = button->convertToWorldSpace(button->getContentSize()/2);
            auto safe = rb::safeBounds(); safe.origin += rb::canvasOrigin();
            require(safe.containsPoint(center), "Pause outside safe area");
            require(std::abs(safe.getMaxX() - center.x - button->getContentSize().width/2 - 5) < .1f,
                    "Pause not anchored to right edge");
            require(pause->beginTouch(center), "Pause hit missed after resize");
            int activated = 0;
            pause->activate = [&activated] { ++activated; };
            pause->endTouch(center);
            pause->activate = [] {};
            require(activated == 1, "Pause action lost");
            *results << frame.width << 'x' << frame.height << " viewport=" << bounds.size.width << 'x'
                     << bounds.size.height << " room=" << clip.size.width << " objectSamples=" << hits << " PASS\n";
            results->flush();
            const std::string path = std::string(output) + "/viewport-" + std::to_string(*index) + ".png";
            utils::captureScreen([=](bool ok, std::string_view) {
                require(ok, "Screenshot failed");
                if (++*index < static_cast<int>(sizes.size()))
                    scene->scheduleOnce([step](float) { (*step)(); }, .1f, "viewport-next");
                else
                {
                    *step = {};
                    scene->scheduleOnce([scene](float) {
                        EventKeyboard full(EventKeyboard::KeyCode::KEY_F11, false);
                        scene->getEventDispatcher()->dispatchEvent(&full);
                    }, .1f, "viewport-full-key");
                    scene->scheduleOnce([=](float) {
                        require(view->isFullscreen(), "F11 did not enter fullscreen");
                        EventKeyboard leave(EventKeyboard::KeyCode::KEY_ESCAPE, false);
                        scene->getEventDispatcher()->dispatchEvent(&leave);
                        scene->scheduleOnce([=](float) {
                            require(!view->isFullscreen(), "Escape did not leave fullscreen");
                            auto completed = std::make_shared<int>(0);
                            room->movePosition(4, 1, [completed] { ++*completed; });
                            scene->scheduleOnce([=](float) {
                                view->setWindowed(1440, 640);
                                rb::updateViewport();
                            }, .2f, "viewport-during-pan");
                            scene->scheduleOnce([=](float) {
                                require(*completed == 1, "Camera completion lost during resize");
                                const auto bounds = rb::viewportBounds();
                                const float width = bg->getContentSize().width;
                                const float expected = width <= bounds.size.width
                                    ? bounds.getMidX() - width/2 : bounds.getMaxX() - width;
                                require(std::abs(bg->getPositionX() - expected) < .1f,
                                        "Camera endpoint stale after resize");
                                const int health = game->state().health;
                                game->tap(room->convertToWorldSpace({bounds.getMinX()-1, 160}));
                                require(game->state().health == health, "Empty margin consumes health");
                                room->beginDrag(); room->drag(10000); room->settle();
                                scene->scheduleOnce([=](float) {
                                    const auto bounds = rb::viewportBounds();
                                    const float width = bg->getContentSize().width;
                                    const float expected = width <= bounds.size.width
                                        ? bounds.getMidX() - width/2 : bounds.getMinX();
                                    require(std::abs(bg->getPositionX() - expected) < .2f,
                                            "Drag did not settle at viewport boundary");
                                    *results << "PASS: resize, fullscreen, camera completion, drag bounds and margin input\n";
                                    results->flush();
                                    Director::getInstance()->end();
                                }, 2, "viewport-settle-check");
                            }, 1.5f, "viewport-pan-check");
                        }, .5f, "viewport-windowed");
                    }, .5f, "viewport-fullscreen");
                }
            }, path);
        }, 1.0f, "viewport-check");
    };
    scene->scheduleOnce([step](float) { (*step)(); }, 1, "viewport-start");
    return true;
#else
    return false;
#endif
}
