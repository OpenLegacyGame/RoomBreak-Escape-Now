#include "DetailTutorial.h"
#include "Assets.h"
using namespace ax;
DetailTutorial* DetailTutorial::make(const rb::GameData& data, int id, Vec2 origin, int language, int hands)
{
    return createDetail<DetailTutorial>(data, id, origin, language, hands);
}
void DetailTutorial::setup(const rb::Row&)
{
    makeBoard("e0_s1_det15_1.png");
    success_                = true;
    const int number        = id_ - 77;
    const int count         = id_ <= 84 ? 2 : id_ == 85 ? 3 : 5;
    const char* languages[] = {"chi", "kor", "eng", "jap"};
    for (int i = 0; i < count; ++i)
    {
        std::string name = "e0_s1_det" + std::to_string(number) + "_" + std::to_string(i + 1);
        if (!(id_ == 85 && i == 2))
            name += "_" + std::string(languages[language_]);
        auto* page = rb::sprite(name + ".png");
        addChild(page);
        pages_.push_back(page);
        page->setPosition(240 + 480 * i, 160);
        page->setOpacity(0);
        page->runAction(Sequence::create(DelayTime::create(1), FadeIn::create(.5f), nullptr));
        auto* dot = rb::sprite("e4_s4_det3_5.png");
        addChild(dot);
        track(dot);
        dots_.push_back(dot);
        dot->setPosition(240 - 10 * (count - 1) + 20 * i + dot->getContentSize().width / 2,
                         160 - page->getContentSize().height / 2 - 50);
    }
    selection_ = rb::sprite("e4_s4_det3_6.png");
    addChild(selection_);
    track(selection_);
    selection_->setPosition(dots_[0]->getPosition());
    scheduleOnce([this](float) {
        if (!hiding_)
            displayBusy_ = false;
    }, 1.5f, "tutorial-open");
}
void DetailTutorial::press(Vec2 p)
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
void DetailTutorial::sortPages()
{
    for (int i = 0; i < pageCount(); ++i)
    {
        auto* page = pages_[i];
        page->runAction(EaseIn::create(ActionFloat::create(.5f, page->getPositionX(), 240.f + 480 * (i - selected_),
                                                           [page](float x) { page->setPositionX(x); }),
                                       .25f));
    }
    selection_->setPosition(dots_[selected_]->getPosition());
}
void DetailTutorial::move(Vec2 p)
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
        if (selected_ + 1 < pageCount())
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
void DetailTutorial::release(Vec2)
{
    if (!displayBusy_)
        sortPages();
    dragging_ = false;
}
void DetailTutorial::close()
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
    scheduleOnce([this](float) { DetailView::close(); }, .5f, "tutorial-close");
}
