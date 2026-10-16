#include "SheetSprite.h"
#include "Assets.h"
#include <stdexcept>

SheetSprite* SheetSprite::create(std::string_view file, int columns, int rows)
{
    if (columns <= 0 || rows <= 0)
        throw std::runtime_error("Invalid sprite sheet grid");
    auto* source = rb::sprite(file);
    auto* value  = new SheetSprite;
    if (!value->initWithTexture(source->getTexture()))
    {
        delete value;
        return nullptr;
    }
    value->autorelease();
    value->columns_      = columns;
    value->rows_         = rows;
    const auto physical  = source->getTextureRect().size;
    const auto logical   = source->getContentSize();
    value->textureFrame_ = {physical.width / columns, physical.height / rows};
    value->logicalFrame_ = {logical.width / columns, logical.height / rows};
    value->setStretchEnabled(true);
    value->frame(0);
    return value;
}
void SheetSprite::frame(int index)
{
    if (index < 0 || index >= columns_ * rows_)
        throw std::runtime_error("Sprite sheet frame out of range");
    index_ = index;
    setTextureRect(
        {(index % columns_) * textureFrame_.x, (index / columns_) * textureFrame_.y, textureFrame_.x, textureFrame_.y});
    setContentSize(logicalFrame_);
}
void SheetSprite::animate(int count, float interval)
{
    if (count <= 0 || count > columns_ * rows_ || interval <= 0)
        throw std::runtime_error("Invalid sprite sheet animation");
    stopAnimation();
    frame(0);
    schedule([this, count](float) { frame((index_ + 1) % count); }, interval, "sheet");
}
void SheetSprite::stopAnimation()
{
    unschedule("sheet");
}
void SheetSprite::setGrid(int columns, int rows)
{
    if (columns <= 0 || rows <= 0)
        throw std::runtime_error("Invalid sprite sheet grid");
    stopAnimation();
    textureFrame_ = {textureFrame_.x * columns_ / columns, textureFrame_.y * rows_ / rows};
    logicalFrame_ = {logicalFrame_.x * columns_ / columns, logicalFrame_.y * rows_ / rows};
    columns_      = columns;
    rows_         = rows;
    frame(0);
}
void SheetSprite::playOnce(int count, float interval, std::function<void()> complete, bool reverse)
{
    if (count <= 0 || count > columns_ * rows_ || interval <= 0)
        throw std::runtime_error("Invalid sprite sheet animation");
    stopAnimation();
    frame(reverse ? count - 1 : 0);
    schedule([this, count, complete, reverse](float) {
        const int next = index_ + (reverse ? -1 : 1);
        if (next >= 0 && next < count)
        {
            frame(next);
            return;
        }
        auto done = complete;
        stopAnimation();
        frame(0);
        if (done)
            done();
    }, interval, "sheet");
}
