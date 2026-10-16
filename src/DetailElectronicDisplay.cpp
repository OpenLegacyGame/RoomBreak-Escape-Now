#include "DetailElectronicDisplay.h"
#include "Assets.h"
using namespace ax;
DetailElectronicDisplay* DetailElectronicDisplay::make(const rb::GameData& data,
                                                       int id,
                                                       Vec2 origin,
                                                       int language,
                                                       int hands)
{
    return createDetail<DetailElectronicDisplay>(data, id, origin, language, hands);
}
void DetailElectronicDisplay::setup(const rb::Row&)
{
    makeBoard("e1_s4_det1_1.png");
    success_ = true;
    for (int i = 0; i < 6; ++i)
    {
        auto* page = rb::sprite("e1_s4_det1_" + std::to_string(i + 2) + ".png");
        addChild(page);
        pages_.push_back(page);
        page->setPosition(240 + 480 * i, 160);
        page->setOpacity(0);
        page->runAction(Sequence::create(DelayTime::create(1), FadeIn::create(.5f), nullptr));
    }
    scheduleOnce([this](float) {
        if (!hiding_)
            displayBusy_ = false;
    }, 1.5f, "display-open");
}
void DetailElectronicDisplay::press(Vec2 p)
{
    if (displayBusy_)
        return;
    last_     = p;
    dragging_ = false;
    for (auto* page : pages_)
        if (contains(page, p))
        {
            for (auto* other : pages_)
                other->stopAllActions();
            dragging_ = true;
            break;
        }
}
void DetailElectronicDisplay::sortPages()
{
    // Cocos2d-iphone overwrites absolute positions when these actions overlap.
    // Axmol MoveTo accumulates simultaneous motion, so interpolate x directly.
    for (int i = 0; i < 6; ++i)
    {
        auto* page = pages_[i];
        page->runAction(EaseIn::create(ActionFloat::create(.5f, page->getPositionX(), 240.f + 480 * (i - selected_),
                                                           [page](float x) { page->setPositionX(x); }),
                                       .25f));
    }
}
void DetailElectronicDisplay::move(Vec2 p)
{
    const float dx = p.x - last_.x;
    last_          = p;
    if (!dragging_ || displayBusy_)
        return;
    for (auto* page : pages_)
        page->setPositionX(page->getPositionX() + dx);
    const float x = pages_[selected_]->getPositionX();
    if (x < 190)
    {
        if (selected_ < 5)
            ++selected_;
    }
    else if (x > 290)
    {
        if (selected_ > 0)
            --selected_;
    }
    else
        return;
    sortPages();
    dragging_ = false;
}
void DetailElectronicDisplay::release(Vec2)
{
    if (!displayBusy_)
        sortPages();
    dragging_ = false;
}
void DetailElectronicDisplay::close()
{
    if (hiding_ || closing_)
        return;
    hiding_      = true;
    displayBusy_ = true;
    dragging_    = false;
    for (auto* page : pages_)
    {
        page->stopAllActions();
        page->runAction(FadeOut::create(.5f));
    }
    scheduleOnce([this](float) { DetailView::close(); }, .5f, "display-close");
}
