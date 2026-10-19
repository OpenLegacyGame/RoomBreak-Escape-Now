#include "Viewport.h"
#include <algorithm>
#include <cmath>
#if RB_VISUAL_PROBE
#include <cstdlib>
#include <cstdio>
#endif

namespace rb
{
namespace
{
constexpr const char* changed = "roombreak-viewport-changed";
ax::Rect safe(0, 0, 480, 320);
ax::Size size(480, 320);
bool closeEnough(float a, float b) { return std::abs(a - b) < .01f; }
ax::Rect readSafeArea(ax::RenderView* view)
{
#if RB_VISUAL_PROBE
    if (const char* insets = std::getenv("RB_VIEWPORT_INSETS"))
    {
        float left, top, right, bottom;
        if (std::sscanf(insets, "%f,%f,%f,%f", &left, &top, &right, &bottom) != 4)
            throw std::runtime_error("Expected left,top,right,bottom test insets");
        const auto frame = view->getFrameSize();
        return {left / view->getScaleX(), bottom / view->getScaleY(),
                (frame.width-left-right) / view->getScaleX(),
                (frame.height-top-bottom) / view->getScaleY()};
    }
#endif
    return view->getSafeAreaRect();
}
}
bool updateViewport()
{
    auto* view = ax::Director::getInstance()->getRenderView();
    const auto frame = view->getFrameSize();
    if (frame.width <= 0 || frame.height <= 0)
        return false;
    auto design = view->getDesignResolutionSize();
    // Start with an exact framebuffer aspect ratio, including after rotation.
    if (std::abs(design.width / design.height - frame.width / frame.height) > .00001f)
    {
        const float scale = std::min(frame.width / 480, frame.height / 320);
        design = frame / scale;
        view->setDesignResolutionSize(design.width, design.height, ResolutionPolicy::SHOW_ALL);
    }
    auto nextSafe = readSafeArea(view);
    if (nextSafe.size.width <= 0 || nextSafe.size.height <= 0)
        return false;
    // Keep a complete, undistorted UI canvas inside the device's safe area.
    const float factor = std::max(480 / nextSafe.size.width, 320 / nextSafe.size.height);
    if (std::abs(factor - 1) > .00001f)
    {
        design *= factor;
        view->setDesignResolutionSize(design.width, design.height, ResolutionPolicy::SHOW_ALL);
        nextSafe = readSafeArea(view);
    }
    if (closeEnough(size.width, design.width) && closeEnough(size.height, design.height) &&
        closeEnough(safe.origin.x, nextSafe.origin.x) && closeEnough(safe.origin.y, nextSafe.origin.y) &&
        closeEnough(safe.size.width, nextSafe.size.width) && closeEnough(safe.size.height, nextSafe.size.height))
        return false;
    size = design;
    safe = nextSafe;
    ax::Director::getInstance()->getEventDispatcher()->dispatchCustomEvent(changed);
    return true;
}
ax::Vec2 canvasOrigin()
{
    return {safe.getMidX() - 240, safe.getMidY() - 160};
}
ax::Rect viewportBounds()
{
    return {-canvasOrigin(), size};
}
ax::Rect safeBounds()
{
    return {safe.origin - canvasOrigin(), safe.size};
}
void onViewport(ax::Node* owner, std::function<void()> layout)
{
    layout();
    auto* listener = ax::EventListenerCustom::create(changed, [layout](ax::EventCustom*) { layout(); });
    owner->getEventDispatcher()->addEventListenerWithSceneGraphPriority(listener, owner);
}
void anchorToSafeArea(ax::Node* node, float horizontal, float vertical)
{
    // Clip sliding controls at the safe-area edge, including while hidden.
    auto* parent = node->getParent();
    const int order = node->getLocalZOrder();
    auto* clip = ax::ClippingRectangleNode::create();
    node->retain();
    node->removeFromParentAndCleanup(false);
    parent->addChild(clip, order);
    clip->addChild(node);
    node->release();
    onViewport(clip, [clip] { clip->setClippingRegion(safeBounds()); });
    onViewport(node, [node, horizontal, vertical] {
        const auto rect = safeBounds();
        node->setPosition(rect.origin.x + (rect.size.width - 480) * horizontal,
                          rect.origin.y + (rect.size.height - 320) * vertical);
    });
}
ax::LayerColor* screenShade(const ax::Color4B& color)
{
    auto* layer = ax::LayerColor::create(color);
    onViewport(layer, [layer] {
        const auto rect = viewportBounds();
        layer->setPosition(rect.origin);
        layer->setContentSize(rect.size);
    });
    return layer;
}
} // namespace rb
