#pragma once
#include "axmol.h"
#include <functional>
#include <string>
#include <vector>

// EventSceneControl text animation, pagination, and one-character color codes.
class DialogText : public ax::Node
{
public:
    static DialogText* create(int language, int speed, float width, int lines, ax::Color3B color);
    void setText(std::string_view text);
    void nextPage();
    void skip();
    bool animating() const { return animating_; }
    bool hasMore() const { return offset_ < tokens_.size(); }
    std::function<void()> finished;

private:
    struct Token
    {
        std::string text;
        ax::Color3B color;
    };
    void reveal(std::size_t index);
    std::vector<Token> tokens_;
    std::vector<ax::Label*> glyphs_;
    std::size_t offset_ = 0, visible_ = 0;
    int language_ = 0, speed_ = 1, lines_ = 2;
    float width_ = 0;
    ax::Color3B color_;
    bool animating_ = false;
};
