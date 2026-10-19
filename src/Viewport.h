#pragma once
#include "axmol.h"

namespace rb
{
// Artwork and puzzle coordinates stay in the original 480 x 320 canvas.
// The room viewport may extend beyond it; all scaling remains uniform.
bool updateViewport();
ax::Vec2 canvasOrigin();
ax::Rect viewportBounds();
ax::Rect safeBounds();
void onViewport(ax::Node* owner, std::function<void()> layout);
void anchorToSafeArea(ax::Node* node, float horizontal, float vertical);
ax::LayerColor* screenShade(const ax::Color4B& color);
} // namespace rb
