#pragma once
#include "axmol.h"
#include "GameData.h"
#include "SheetSprite.h"
#include <functional>
#include <set>
class RoomView : public ax::ClippingRectangleNode
{
public:
    static RoomView* create(const rb::GameData& data, rb::StageState& state);
    void loadRoom(int id, int position = 0);
    ax::Vec2 objectPosition(int object) const;
    void refreshShining();
    bool isShining(int object) const { return shine_.contains(object); }
    bool containsPoint(ax::Vec2 worldPoint) const;
    int hitObject(ax::Vec2 worldPoint) const;
    void changeState(int object, int state, std::function<void()> complete);
    void setActive(int object, bool visible, std::function<void()> complete);
    void movePosition(int position, float duration, std::function<void()> complete);
    void focusObject(int object, float duration, std::function<void()> complete);
    bool direction(int id, std::function<void()> complete);
    std::size_t directionAnimationCount() const { return directionAnimations_.size() + directionActions_.size(); }
    std::size_t directionParticleCount() const { return directionParticles_.size(); }
    void particle(int id, ax::Vec2 world, std::function<void()> complete);
    void objectEffect(int object, int type, std::function<void()> complete);
    void drag(float delta);
    void beginDrag();
    void settle();
    ax::Sprite* background() const { return background_; }
    ax::Sprite* image(std::string_view filename) const;
    // Sprite showing an ObjectState row, if it belongs to the current room.
    ax::Sprite* stateImage(int stateId) const;

private:
    void moveCamera(std::function<float()> target, float duration, float easing, std::function<void()> complete);
    void updateViewport();
    void updateClip();
    std::pair<float, float> horizontalLimits() const;
    float positionX(int position) const;
    ax::Rect viewport_{0, 0, 480, 320};
    void after(float seconds, std::function<void()> callback);
    const rb::GameData* data_ = nullptr;
    rb::StageState* state_    = nullptr;
    ax::Sprite* background_   = nullptr;
    std::map<std::string, ax::Sprite*> sprites_;
    std::map<int, ax::ParticleSystemQuad*> shine_;
    std::set<int> effects_;
    std::map<std::string, ax::RefPtr<ax::ParticleSystemQuad>> directionParticles_;
    struct RoomAnimation
    {
        int room, frames, z;
        float interval;
        ax::RefPtr<SheetSprite> sprite;
    };
    std::map<std::string, RoomAnimation> directionAnimations_;
    struct RoomAction
    {
        int room, z;
        ax::RefPtr<ax::Sprite> sprite;
        ax::RefPtr<ax::ActionInterval> action;
    };
    std::map<std::string, RoomAction> directionActions_;
    void addDirectionAction(std::string name, ax::Sprite*, ax::ActionInterval*, int z);
    void addDirectionAnimation(std::string name, SheetSprite*, int frames, float interval, int z);
    void fadeDirectionAction(const std::string& name, float seconds);
    void fadeDirectionAnimation(const std::string& name, float seconds);
    void removeDirectionAction(const std::string& name, bool stopActions = true);
    void removeDirectionAnimation(const std::string& name);
    void removeDirectionParticle(const std::string& name);
    ax::Vec2 roomPoint(ax::Node* node, float x, float y) const;
    ax::LayerColor* whiteFlash();
    void hideObject(int object);
    void revealState(int object, int state);
    void playStateChange(int object,
                         int state,
                         SheetSprite* sheet,
                         int frames,
                         float interval,
                         std::function<void()> complete,
                         bool reverse = false);
    float velocity_ = 0;
    int dragTicks_  = 0;
};
