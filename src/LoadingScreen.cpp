#include "LoadingScreen.h"
#include "NodeFactory.h"
#include "Assets.h"
#include <sstream>
using namespace ax;
LoadingScreen* LoadingScreen::create(int language, bool tips, std::function<void()> complete)
{
    auto* screen = rb::make<LoadingScreen>();
    screen->setup(language, tips, std::move(complete));
    return screen;
}
void LoadingScreen::setup(int language, bool tips, std::function<void()> complete)
{
    setContentSize({480, 320});
    addChild(LayerColor::create(Color4B::BLACK, 480, 320));
    // LoadingLayer advances its process-wide tip counter for both screen types.
    static int tip = 1;
    tip            = tip % 10 + 1;
    if (tips)
    {
        auto* box = rb::sprite("loading_1.png");
        addChild(box);
        box->setPosition(240, 160);
        std::istringstream lines(rb::otherText(language, "TIP_" + std::to_string(tip)));
        for (int i = 0; i < 2; ++i)
        {
            std::string line;
            std::getline(lines, line);
            auto* text = rb::label(line, language, 10);
            addChild(text);
            text->setPosition(240, 320 - (102 + 15 * i) - text->getContentSize().height / 2);
        }
    }
    auto* first  = rb::place(this, "loading_2.png", 385, 232);
    auto* second = rb::place(this, "loading_3.png", 385, 232);
    rb::place(this, "loading_4.png", 325, 261);
    rb::rotateGear(first, .5f, false);
    rb::rotateGear(second, .5f, true);
    auto* touch = EventListenerTouchOneByOne::create();
    touch->setSwallowTouches(true);
    touch->onTouchBegan = [](Touch*, Event*) { return true; };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(touch, this);
    scheduleOnce([this, complete = std::move(complete)](float) {
        complete();
        removeFromParent();
    }, 2.0f, "loading-complete");
}
