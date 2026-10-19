/****************************************************************************
 Copyright (c) 2017-2018 Xiamen Yaji Software Co., Ltd.
 Copyright (c) 2019-present Axmol Engine contributors (see AUTHORS.md).

 https://axmol.dev/

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 ****************************************************************************/

#include "AppDelegate.h"
#if defined(_WIN32)
#    include <filesystem>
#    include <stdexcept>
#endif
#include "MainScene.h"
#include "Viewport.h"

#define USE_AUDIO_ENGINE 1

#if USE_AUDIO_ENGINE
#    include "audio/AudioEngine.h"
#endif

#if _AX_TESTS
#    include "doctest/doctest.h"
#endif

using namespace ax;

static ax::Size designResolutionSize = ax::Size(480, 320);

AppDelegate::AppDelegate() {}

AppDelegate::~AppDelegate() {}

// if you want a different context, modify the value of gfxContextAttrs
// it will affect all platforms
void AppDelegate::initGfxContextAttrs()
{
    // set graphics context attributes: red,green,blue,alpha,depth,stencil,multisamplesCount
    GfxContextAttrs gfxContextAttrs = {8, 8, 8, 8, 24, 8, 0};
    // since axmol-2.2 vsync was enabled in engine by default
    // gfxContextAttrs.vsync = false;

    RenderView::setGfxContextAttrs(gfxContextAttrs);
}

bool AppDelegate::applicationDidFinishLaunching()
{
#if defined(_WIN32)
    // Keep the established save location when the executable's display name changes.
    auto* files    = FileUtils::getInstance();
    auto directory = std::filesystem::u8path(files->getWritablePath());
    if (directory.filename().empty())
        directory = directory.parent_path();
    directory          = directory.parent_path() / "RoomBreakRebuilt";
    const auto encoded = directory.generic_u8string();
    std::string stableDirectory(reinterpret_cast<const char*>(encoded.data()), encoded.size());
    stableDirectory += '/';
    if (!files->createDirectories(stableDirectory))
        throw std::runtime_error("Cannot create save directory");
    files->setWritablePath(stableDirectory);
#endif
    // initialize director
    auto director   = Director::getInstance();
    auto renderView = director->getRenderView();
    if (!renderView)
    {
#if (AX_TARGET_PLATFORM != AX_PLATFORM_ANDROID) && (AX_TARGET_PLATFORM != AX_PLATFORM_IOS)
        renderView = RenderViewImpl::createWithRect("RoomBreak Escape Now", ax::Rect(0, 0, 960, 640), 1, true);
#else
        renderView = RenderViewImpl::create("RoomBreak Escape Now");
#endif
        director->setRenderView(renderView);
#if (AX_TARGET_PLATFORM == AX_PLATFORM_WIN32) || (AX_TARGET_PLATFORM == AX_PLATFORM_MAC)
        static_cast<RenderViewImpl*>(renderView)->setWindowSizeLimits(480, 320);
#endif
    }

    // turn on display FPS
    director->setStatsDisplay(false);
    Image::setPNGPremultipliedAlphaEnabled(false);

    // set FPS. the default value is 1.0/60 if you don't call this
    director->setAnimationInterval(1.0f / 60);

    // Set the design resolution
    renderView->setDesignResolutionSize(designResolutionSize.width, designResolutionSize.height,
                                        ResolutionPolicy::SHOW_ALL);
    rb::updateViewport();

#if !_AX_TESTS
    // create a scene. it's an autorelease object
    auto scene = utils::createInstance<MainScene>();

    // run
    director->runWithScene(scene);
#endif

    return true;
}

// This function will be called when the app is inactive. Note, when receiving a phone call it is invoked.
void AppDelegate::applicationDidEnterBackground()
{
    Director::getInstance()->stopAnimation();

#if USE_AUDIO_ENGINE
    pausedMusic_ = -1;
    if (auto* scene = dynamic_cast<MainScene*>(Director::getInstance()->getRunningScene()))
    {
        const int id = scene->musicId();
        if (AudioEngine::getState(id) == AudioEngine::AudioState::PAUSED)
            pausedMusic_ = id;
    }
    AudioEngine::pauseAll();
#endif
}

// this function will be called when the app is active again
void AppDelegate::applicationWillEnterForeground()
{
    Director::getInstance()->startAnimation();

#if USE_AUDIO_ENGINE
    // resumeAll also resumes music that was already paused by the menu.
    const float volume = pausedMusic_ >= 0 ? AudioEngine::getVolume(pausedMusic_) : 0;
    if (pausedMusic_ >= 0)
        AudioEngine::setVolume(pausedMusic_, 0);
    AudioEngine::resumeAll();
    if (pausedMusic_ >= 0)
    {
        AudioEngine::pause(pausedMusic_);
        AudioEngine::setVolume(pausedMusic_, volume);
        pausedMusic_ = -1;
    }
#endif
}

void AppDelegate::applicationWillQuit() {}

#if _AX_TESTS
int AppDelegate::run(int argc, char** argv)
{
    AXLOGI("Running unit tests...\n");
    fflush(stdout);
    AXLOGI("Default resource path: {}\n", FileUtils::getInstance()->getDefaultResourceRootPath());
    AXLOGI("Writable path: {}\n", FileUtils::getInstance()->getWritablePath());
    {
        for (auto& path : FileUtils::getInstance()->getSearchPaths())
            AXLOGI("Search path: {}\n", path);
    }
    fflush(stdout);

    ax::Director::getInstance()->init();

    doctest::Context context;

    // context.addFilter("test-case-exclude", "*math*"); // exclude test cases with "math" in their name
    // context.setOption("abort-after", 5);              // stop test execution after 5 failed assertions

    // context.setOption("order-by", "name");            // sort the test cases by their name

    context.applyCommandLine(argc, argv);

    // overrides
    context.setOption("no-breaks", true);  // don't break in the debugger when assertions fail

    int res = context.run();  // run
    return res;
}
#endif
