#include "RoomView.h"
#include "Viewport.h"
#include "NodeFactory.h"
#include "Assets.h"
#include <algorithm>
using namespace ax;
RoomView* RoomView::create(const rb::GameData& data, rb::StageState& state)
{
    auto* view   = rb::make<RoomView>();
    view->data_  = &data;
    view->state_ = &state;
    view->setContentSize({480, 320});
    view->viewport_ = rb::viewportBounds();
    view->loadRoom(state.room);
    rb::onViewport(view, [view] { view->updateViewport(); });
    view->schedule([view](float) { view->updateClip(); }, "viewport-clip");
    return view;
}
std::pair<float, float> RoomView::horizontalLimits() const
{
    const float width = background_->getContentSize().width;
    if (width <= viewport_.size.width)
    {
        const float center = viewport_.getMidX() - width / 2;
        return {center, center};
    }
    return {viewport_.getMaxX() - width, viewport_.getMinX()};
}
float RoomView::positionX(int position) const
{
    const auto [left, right] = horizontalLimits();
    return right + (left - right) * std::clamp(position, 0, 4) / 4;
}
void RoomView::updateViewport()
{
    const float center = viewport_.getMidX() - background_->getPositionX();
    viewport_ = rb::viewportBounds();
    const auto [left, right] = horizontalLimits();
    background_->setPosition(std::clamp(viewport_.getMidX() - center, left, right),
                             viewport_.getMidY() - background_->getContentSize().height / 2);
    beginDrag();
    updateClip();
}
void RoomView::updateClip()
{
    const auto image = ax::Rect(background_->getPosition(), background_->getContentSize());
    const float left = std::max(viewport_.getMinX(), image.getMinX());
    const float bottom = std::max(viewport_.getMinY(), image.getMinY());
    const float right = std::min(viewport_.getMaxX(), image.getMaxX());
    const float top = std::min(viewport_.getMaxY(), image.getMaxY());
    setClippingRegion({left, bottom, std::max(0.f, right - left), std::max(0.f, top - bottom)});
}
void RoomView::after(float seconds, std::function<void()> callback)
{
    runAction(Sequence::create(DelayTime::create(seconds), CallFunc::create(std::move(callback)), nullptr));
}
Sprite* RoomView::image(std::string_view filename) const
{
    auto it = sprites_.find(std::string(filename));
    return it == sprites_.end() ? nullptr : it->second;
}
Sprite* RoomView::stateImage(int stateId) const
{
    return image(rb::string(data_->byId("ObjectState", "stateID", stateId), "imageName"));
}
void RoomView::loadRoom(int id, int position)
{
    for (auto& [name, animation] : directionAnimations_)
        if (animation.sprite->getParent())
        {
            animation.sprite->stopAnimation();
            animation.sprite->removeFromParent();
        }
    for (auto& [name, action] : directionActions_)
        if (action.sprite->getParent())
        {
            action.sprite->stopAllActions();
            action.sprite->removeFromParent();
        }
    unschedule("room-position");
    unschedule("room-slide");
    unschedule("room-settle");
    unschedule("drag-time");
    state_->room = id;
    removeAllChildren();
    sprites_.clear();
    shine_.clear();
    const auto& room = data_->byId("RoomData", "roomID", id);
    background_      = rb::sprite(rb::string(room, "imageName"));
    addChild(background_);
    background_->setAnchorPoint(Vec2::ZERO);
    const auto size = background_->getContentSize();
    background_->setPosition(positionX(position), viewport_.getMidY() - size.height / 2);
    updateClip();
    const auto objects = data_->where("ObjectData", "roomID", id);
    for (int layer = rb::integer(room, "maxLayer"); layer >= 1; --layer)
        for (auto* object : objects)
        {
            const int oid = rb::integer(*object, "objectID");
            auto it       = state_->objects.find(oid);
            if (it == state_->objects.end() || it->second.disappeared)
                continue;
            const int order =
                rb::integer(data_->byId("LayerData", "layerID", rb::integer(*object, "layerID")), "layerSequence");
            if (order != layer)
                continue;
            for (auto* state : data_->where("ObjectState", "objectID", oid))
            {
                auto filename = rb::string(*state, "imageName");
                if (filename.empty() || sprites_.contains(filename))
                    continue;
                auto* s = rb::place(background_, filename, rb::integer(*state, "imageX") / 2,
                                    rb::integer(*state, "imageY") / 2);
                s->setOpacity(0);
                sprites_[filename] = s;
            }
        }
    for (auto* object : objects)
    {
        auto it = state_->objects.find(rb::integer(*object, "objectID"));
        if (it == state_->objects.end() || it->second.disappeared || !it->second.active)
            continue;
        if (auto* s = stateImage(it->second.state))
            s->setOpacity(255);
    }
    for (auto& [name, animation] : directionAnimations_)
        if (animation.room == id)
        {
            background_->addChild(animation.sprite.get(), animation.z);
            animation.sprite->animate(animation.frames, animation.interval);
        }
    for (auto& [name, action] : directionActions_)
        if (action.room == id)
        {
            background_->addChild(action.sprite.get(), action.z);
            action.sprite->runAction(action.action->clone());
        }
    refreshShining();
}
void RoomView::refreshShining()
{
    for (auto it = shine_.begin(); it != shine_.end();)
    {
        const auto& state = state_->objects.at(it->first);
        if (state.disappeared || !state.shining)
        {
            it->second->removeFromParent();
            it = shine_.erase(it);
        }
        else
            ++it;
    }
    for (auto* row : data_->where("ObjectData", "roomID", state_->room))
    {
        const int id = rb::integer(*row, "objectID"), type = rb::integer(*row, "objectType");
        // Previously collected gold keys are omitted when the stage is restored.
        if (type == 2 && !state_->objects.contains(id))
            continue;
        const auto& state = state_->objects.at(id);
        if ((type != 1 && type != 2) || !state.shining || state.disappeared || shine_.contains(id))
            continue;
        auto* particle = ParticleSystemQuad::create("par_itemshinee.plist");
        if (!particle)
            throw std::runtime_error("Missing object shine particle");
        background_->addChild(particle);
        particle->setPosition(objectPosition(id));
        shine_[id] = particle;
    }
}
Vec2 RoomView::objectPosition(int object) const
{
    int left = 0, right = 0, top = 0, bottom = 0;
    for (auto* row : data_->where("AreaData", "objectID", object))
    {
        const int x = rb::integer(*row, "AreaX") / 2,
                  y = static_cast<int>(background_->getContentSize().height) - rb::integer(*row, "AreaY") / 2;
        const int w = rb::integer(*row, "AreaWidth") / 2, h = rb::integer(*row, "AreaHeight") / 2;
        left   = left == 0 ? x : std::min(left, x);
        right  = std::max(right, x + w);
        top    = std::max(top, y);
        bottom = bottom == 0 ? y - h : std::min(bottom, y - h);
    }
    return {(left + right) / 2.f, (top + bottom) / 2.f};
}
bool RoomView::containsPoint(Vec2 worldPoint) const
{
    return getClippingRegion().containsPoint(convertToNodeSpace(worldPoint));
}
int RoomView::hitObject(Vec2 worldPoint) const
{
    if (!containsPoint(worldPoint))
        return 0;
    const auto local = background_->convertToNodeSpace(worldPoint);
    return state_->objectAt(*data_, local.x, background_->getContentSize().height - local.y);
}
void RoomView::changeState(int object, int next, std::function<void()> complete)
{
    auto& state         = state_->objects.at(object);
    const auto oldName  = rb::string(data_->byId("ObjectState", "stateID", state.state), "imageName");
    const auto nextName = rb::string(data_->byId("ObjectState", "stateID", next), "imageName");
    if (!state.active || !state.disappeared)
        state.state = next;
    if (state.active && !state.disappeared && oldName != nextName)
    {
        if (auto* old = image(oldName))
            old->runAction(FadeOut::create(.3f));
        if (auto* value = image(nextName))
            value->runAction(FadeIn::create(.3f));
    }
    after(.3f, std::move(complete));
}
void RoomView::setActive(int object, bool visible, std::function<void()> complete)
{
    // Original getObjectData returns nil for stale references (e.g. event7187
    // shows deleted object69). View/HideObject then complete after .3 seconds.
    const auto found = state_->objects.find(object);
    if (found == state_->objects.end())
    {
        after(.3f, std::move(complete));
        return;
    }
    auto& state = found->second;
    if (!state.disappeared && state.active != visible)
    {
        state.active = visible;
        if (auto* s = stateImage(state.state))
            s->runAction(FadeTo::create(.3f, visible ? 255 : 0));
    }
    after(.3f, std::move(complete));
}
void RoomView::movePosition(int p, float seconds, std::function<void()> complete)
{
    moveCamera([this, p] { return positionX(p); }, seconds, .5f, std::move(complete));
}
void RoomView::focusObject(int object, float seconds, std::function<void()> complete)
{
    moveCamera([this, object] {
        const auto [left, right] = horizontalLimits();
        return std::clamp(viewport_.getMidX() - objectPosition(object).x, left, right);
    }, seconds, .25f, std::move(complete));
}
void RoomView::moveCamera(std::function<float()> target, float seconds, float easing, std::function<void()> complete)
{
    // Recompute the destination during a resize without losing the completion barrier.
    const float startX = background_->getPositionX();
    const auto startLimits = horizontalLimits();
    const float startFraction = startLimits.second > startLimits.first
        ? (startLimits.second - startX) / (startLimits.second - startLimits.first) : .5f;
    auto elapsed = std::make_shared<float>(0);
    schedule([this, target = std::move(target), seconds, easing, startFraction, elapsed, complete = std::move(complete)](float dt) {
        *elapsed += dt;
        const float t = seconds > 0 ? std::min(*elapsed / seconds, 1.f) : 1.f;
        const auto limits = horizontalLimits();
        const float from = limits.second + (limits.first - limits.second) * startFraction;
        background_->setPositionX(from + (target() - from) * std::pow(t, easing));
        if (t >= 1)
        {
            unschedule("room-position");
            complete();
        }
    }, "room-position");
}
void RoomView::beginDrag()
{
    unschedule("room-slide");
    unschedule("room-settle");
    velocity_  = 0;
    dragTicks_ = 0;
}
void RoomView::drag(float delta)
{
    const auto [min, max] = horizontalLimits();
    if (min == max)
    {
        background_->setPositionX(min);
        velocity_ = 0;
        return;
    }
    background_->setPositionX(std::clamp(background_->getPositionX() + delta * .8f, min - 100, max + 100));
    velocity_ = delta;
    if (!isScheduled("drag-time"))
        schedule([this](float) {
            if (++dragTicks_ >= 8)
            {
                velocity_  = 0;
                dragTicks_ = 0;
            }
        }, .01f, "drag-time");
}
void RoomView::settle()
{
    unschedule("drag-time");
    schedule([this](float) {
        const auto [min, max] = horizontalLimits();
        velocity_ /= 1.2f;
        const float x = std::clamp(background_->getPositionX() + velocity_, min - 100, max + 100);
        background_->setPositionX(x);
        if (std::abs(velocity_) <= .1f || x < min || x > max)
        {
            velocity_ = 0;
            unschedule("room-slide");
            schedule([this](float) {
                const auto [min, max] = horizontalLimits();
                float x         = background_->getPositionX();
                if (x > max)
                {
                    x = max + (x - max) / 1.2f;
                    if (x <= max + .1f)
                        x = max;
                }
                if (x < min)
                {
                    x = min + (x - min) / 1.2f;
                    if (x >= min - .1f)
                        x = min;
                }
                background_->setPositionX(x);
                if (x >= min && x <= max)
                    unschedule("room-settle");
            }, .01f, "room-settle");
        }
    }, .01f, "room-slide");
}

void RoomView::objectEffect(int object, int type, std::function<void()> complete)
{
    if (type < 1 || type > 3)
        throw std::runtime_error("Unknown object effect");
    const auto& current = state_->objects.at(object);
    auto* sprite        = stateImage(current.state);
    if (!sprite || effects_.contains(object))
    {
        after(.3f, std::move(complete));
        return;
    }
    effects_.insert(object);
    ax::ActionInterval* action = nullptr;
    if (type == 1)
    {
        const auto p = sprite->getPosition();
        action       = ax::Repeat::create(
            ax::Sequence::create(ax::EaseInOut::create(ax::MoveTo::create(.05f, p + ax::Vec2(2, 0)), 1),
                                 ax::EaseInOut::create(ax::MoveTo::create(.05f, p), 1), nullptr),
            2);
    }
    else if (type == 2)
    {
        action = ax::Sequence::create(
            ax::Repeat::create(ax::Sequence::create(ax::EaseInOut::create(ax::RotateTo::create(.1f, 2), 3),
                                                    ax::EaseInOut::create(ax::RotateTo::create(.1f, -2), 3), nullptr),
                               2),
            ax::RotateTo::create(.1f, 0), nullptr);
    }
    else
        action = ax::Repeat::create(ax::Sequence::create(ax::EaseIn::create(ax::ScaleTo::create(.1f, 1.05f), 3),
                                                         ax::EaseOut::create(ax::ScaleTo::create(.1f, 1), 3), nullptr),
                                    2);
    sprite->runAction(ax::Sequence::create(action, ax::CallFunc::create([this, object, complete = std::move(complete)] {
        effects_.erase(object);
        complete();
    }),
                                           nullptr));
}

void RoomView::particle(int id, ax::Vec2 world, std::function<void()> complete)
{
    const auto file = rb::string(data_->byId("ParticleData", "parID", id), "parFileName");
    auto* particle  = ax::ParticleSystemQuad::create(file);
    if (!particle)
        throw std::runtime_error("Missing particle: " + file);
    background_->addChild(particle, 4);
    particle->setPosition(background_->convertToNodeSpace(world));
    after(1, [particle = ax::RefPtr<ax::ParticleSystemQuad>(particle), complete = std::move(complete)] {
        particle->stopSystem();
        particle->removeFromParent();
        complete();
    });
}
