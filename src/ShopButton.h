#pragma once
#include "axmol.h"
#include "Assets.h"
#include "ShopText.h"
namespace rb
{
inline ax::Node* shopUnlockButton(int language)
{
    auto* node = ax::Node::create();
    node->setContentSize({180, 28});
    node->setAnchorPoint({.5f, .5f});
    node->setCascadeOpacityEnabled(true);
    for (int pressed = 0; pressed < 2; ++pressed)
    {
        auto* face = ax::DrawNode::create();
        node->addChild(face, 0, pressed + 1);
        face->drawSolidRect({0, 0}, {180, 28}, ax::Color4F(.69f, .66f, .40f, 1));
        face->drawSolidRect({1, 1}, {179, 27},
                            pressed ? ax::Color4F(.40f, .42f, .20f, 1) : ax::Color4F(.20f, .22f, .12f, 1));
        face->setVisible(pressed == 0);
    }
    auto* text = rb::label(rb::shopUnlockText(language), language, 13);
    text->setPosition(90, 14);
    node->addChild(text, 1);
    return node;
}
inline void pressShopUnlock(ax::Node* node, bool pressed)
{
    node->getChildByTag(1)->setVisible(!pressed);
    node->getChildByTag(2)->setVisible(pressed);
}
}  // namespace rb
