#include "DialogText.h"
#include "NodeFactory.h"
#include "Assets.h"
#include <algorithm>
using namespace ax;
DialogText* DialogText::create(int language, int speed, float width, int lines, Color3B color)
{
    auto* text      = rb::make<DialogText>();
    text->language_ = language;
    text->speed_    = std::clamp(speed, 0, 2);
    text->width_    = width;
    text->lines_    = lines;
    text->color_    = color;
    return text;
}
void DialogText::setText(std::string_view value)
{
    tokens_.clear();
    offset_       = 0;
    Color3B color = color_;
    for (size_t i = 0; i < value.size();)
    {
        if (value[i] == '#' && i + 1 < value.size())
        {
            switch (value[i + 1])
            {
            case 'R':
                color = Color3B(255, 0, 0);
                break;
            case 'G':
                color = Color3B(0, 255, 0);
                break;
            case 'B':
                color = Color3B(0, 255, 255);
                break;
            case 'Y':
                color = Color3B(255, 255, 0);
                break;
            case 'W':
                color = Color3B::WHITE;
                break;
            default:
                color = Color3B::BLACK;
                break;
            }
            i += 2;
            continue;
        }
        const auto byte     = static_cast<unsigned char>(value[i]);
        const size_t length = byte < 128 ? 1 : byte < 224 ? 2 : byte < 240 ? 3 : 4;
        tokens_.push_back({std::string(value.substr(i, length)), color});
        i += length;
        color = color_;
    }
    nextPage();
}
void DialogText::nextPage()
{
    unschedule("typewriter");
    removeAllChildren();
    glyphs_.clear();
    visible_ = 0;
    int x = 0, line = 0;
    while (offset_ < tokens_.size())
    {
        // Japanese and Chinese break at glyphs; Korean and English use words.
        size_t end = offset_ + 1;
        if (language_ == 1 || language_ == 2)
            while (end < tokens_.size() && tokens_[end - 1].text != " ")
                ++end;
        std::vector<Label*> word;
        int width = 0;
        for (size_t j = offset_; j < end; ++j)
        {
            auto* glyph = rb::label(tokens_[j].text, language_, 12);
            glyph->setColor(tokens_[j].color);
            word.push_back(glyph);
            width += static_cast<int>(glyph->getContentSize().width);
        }
        if (x && x + width > width_)
        {
            x = 0;
            ++line;
        }
        if (line >= lines_)
            break;
        for (auto* glyph : word)
        {
            const int advance = static_cast<int>(glyph->getContentSize().width);
            if (x && x + advance > width_)
            {
                x = 0;
                ++line;
            }
            if (line >= lines_)
                break;
            addChild(glyph);
            glyph->setPosition(x + glyph->getContentSize().width / 2, -line * 15 - glyph->getContentSize().height / 2);
            glyph->setOpacity(0);
            glyph->setScale(1.4f);
            glyphs_.push_back(glyph);
            x += advance;
            ++offset_;
        }
        if (line >= lines_)
            break;
    }
    animating_ = !glyphs_.empty();
    if (!animating_)
    {
        if (finished)
            finished();
        return;
    }
    schedule([this](float) {
        reveal(visible_++);
        if (visible_ == glyphs_.size())
        {
            unschedule("typewriter");
            animating_ = false;
            if (finished)
                finished();
        }
    }, .08f - speed_ * .03f, "typewriter");
}
void DialogText::reveal(size_t index)
{
    const float duration = .2f + speed_ * .1f;
    glyphs_.at(index)->runAction(Spawn::create(FadeIn::create(duration), ScaleTo::create(duration, 1), nullptr));
}
void DialogText::skip()
{
    if (!animating_)
        return;
    unschedule("typewriter");
    while (visible_ < glyphs_.size())
        reveal(visible_++);
    animating_ = false;
    if (finished)
        finished();
}
