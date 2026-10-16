#include "GetItemDialog.h"
#include "NodeFactory.h"
#include "Assets.h"
#include <stdexcept>
using namespace ax;
GetItemDialog* GetItemDialog::create(const rb::GameData& data, int item, int language)
{
    auto* value = rb::make<GetItemDialog>();
    value->setContentSize({480, 320});
    const auto& row = data.byId("ItemData", "itemID", item);
    auto* bg        = rb::sprite("get_bg-1.png");
    value->addChild(bg);
    bg->setPosition(240, 180);
    value->fading_.push_back(bg);
    value->light_ = rb::place(value, "get_light.png", 98, 75);
    value->light_->setOpacity(0);
    auto* icon = rb::place(bg, rb::string(row, "imageName"), -39, 0);
    icon->setScale(.55f);
    value->fading_.push_back(icon);
    // Original text places each glyph separately, truncating each accumulated
    // advance to an integer before centering the complete string (0x492fc).
    const auto name = data.text(rb::integer(row, "itemName"), language);
    std::vector<Label*> glyphs;
    int x       = 0;
    float width = 0;
    Color3B color(255, 255, 0);
    for (std::size_t offset = 0; offset < name.size();)
    {
        // Item names are single-line; legacy translations contain trailing tabs.
        const auto byte = static_cast<unsigned char>(name[offset]);
        if (byte < 0x20 || byte == 0x7f)
        {
            ++offset;
            continue;
        }
        if (name[offset] == '#' && offset + 1 < name.size())
        {
            color = name[offset + 1] == 'Y' ? Color3B(255, 255, 0) : Color3B::WHITE;
            offset += 2;
            continue;
        }
        const auto c             = static_cast<unsigned char>(name[offset]);
        const std::size_t length = c < 0x80 ? 1 : c < 0xe0 ? 2 : c < 0xf0 ? 3 : 4;
        auto* glyph              = rb::label(name.substr(offset, length), language, 12);
        bg->addChild(glyph);
        const auto size = glyph->getContentSize();
        glyph->setPosition(x + size.width / 2, bg->getContentSize().height - 66 - size.height / 2);
        glyph->setColor(color);
        glyphs.push_back(glyph);
        value->fading_.push_back(glyph);
        x = static_cast<int>(x + size.width);
        width += size.width;
        offset += length;
    }
    for (auto* glyph : glyphs)
        glyph->setPositionX(glyph->getPositionX() + 176 - width / 2);
    for (auto* node : value->fading_)
        node->setOpacity(0);
    auto* touch = EventListenerTouchOneByOne::create();
    touch->setSwallowTouches(true);
    touch->onTouchBegan = [](Touch*, Event*) { return true; };
    touch->onTouchEnded = [value](Touch*, Event*) { value->close(); };
    value->_eventDispatcher->addEventListenerWithSceneGraphPriority(touch, value);
    return value;
}
void GetItemDialog::open()
{
    if (opened_)
        throw std::runtime_error("Item dialog already opened");
    opened_ = true;
    for (auto* node : fading_)
        node->runAction(EaseOut::create(FadeIn::create(.5f), .25f));
    scheduleOnce([this](float) {
        if (sound)
            sound("snd_f16");
    }, .3f, "item-sound");
    scheduleOnce([this](float) {
        ready_ = true;
        light_->runAction(Sequence::create(FadeIn::create(.3f), FadeOut::create(.3f), nullptr));
    }, .5f, "item-open");
}
void GetItemDialog::close()
{
    if (!ready_)
        return;
    ready_ = false;
    for (auto* node : fading_)
        node->runAction(EaseOut::create(FadeOut::create(.5f), .25f));
    scheduleOnce([this](float) {
        auto callback = std::move(complete);
        if (callback)
            callback();
    }, .5f, "item-close");
}
