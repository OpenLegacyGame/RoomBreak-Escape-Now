#include "HeroHealth.h"
#include "NodeFactory.h"
#include "SheetSprite.h"
#include <stdexcept>
using namespace ax;
HeroHealth* HeroHealth::create(int maximum, int health)
{
    if (maximum <= 0)
        throw std::runtime_error("Invalid health maximum");
    auto* value     = rb::make<HeroHealth>();
    value->maximum_ = maximum;
    for (int i = 0; i < 5; ++i)
    {
        auto* bar = SheetSprite::create("health_bar.png", 1, 5);
        bar->frame(i);
        value->addChild(bar);
        value->bars_[i] = bar;
        const auto size = bar->getContentSize();
        bar->setPosition(-size.width / 2, size.height / 2 + 69 - size.height * i);
    }
    value->updateHealth(health);
    return value;
}
void HeroHealth::updateHealth(int health)
{
    const float ratio = static_cast<float>(health) / maximum_;
    for (int i = 0; i < 5; ++i)
    {
        const float missing = ((i + 1) * .2f - ratio) / .2f;
        const int opacity   = missing <= 0 ? 255 : missing >= 1 ? 0 : static_cast<int>(255 - 255 * missing);
        bars_[4 - i]->setOpacity(static_cast<uint8_t>(opacity));
    }
}
void HeroHealth::show(bool visible)
{
    for (int i = 0; i < 5; ++i)
    {
        auto* bar       = bars_[i];
        const auto size = bar->getContentSize();
        bar->stopAllActions();
        bar->runAction(EaseIn::create(MoveTo::create(.5f, {visible ? size.width / 2 + 1 : -size.width / 2,
                                                           size.height / 2 + 69 - size.height * i}),
                                      .25f));
    }
}
void HeroHealth::fill()
{
    for (auto* bar : bars_)
        bar->runAction(FadeIn::create(.5f));
}
