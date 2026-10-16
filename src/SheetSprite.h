#pragma once
#include "axmol.h"

// Original SpriteManager's row-major sheets, in 480 x 320 logical coordinates.
class SheetSprite : public ax::Sprite
{
public:
    static SheetSprite* create(std::string_view file, int columns, int rows);
    void frame(int index);
    void animate(int count, float interval);
    void playOnce(int count, float interval, std::function<void()> complete, bool reverse = false);
    void setGrid(int columns, int rows);
    void stopAnimation();

private:
    int columns_ = 1, rows_ = 1, index_ = 0;
    ax::Vec2 textureFrame_, logicalFrame_;
};
