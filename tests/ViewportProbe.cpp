#include "Viewport.h"
#include "GameSession.h"
#include "RoomView.h"
#include "PauseButton.h"
#include "AbilityInfo.h"
#include "InventoryView.h"
#include "Progress.h"
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
}
bool runViewportProbe(Scene* scene, Node* canvas, const rb::GameData& data)
{
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
