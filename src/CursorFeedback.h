#pragma once
#include "axmol.h"
#include "GameData.h"
#include <array>
class CursorFeedback : public ax::Node
{
public:
    static CursorFeedback* create(const rb::GameData&);
    void play(ax::Vec2 point, int cursor, int outcome, int item);
    std::function<void(std::string_view)> sound;

private:
    void emit(int index, ax::Vec2 point);
    void itemAction(int item, ax::Vec2 point, bool success);
    const rb::GameData* data_ = nullptr;
    std::array<ax::ParticleSystemQuad*, 7> particles_{};
    ax::Sprite* itemSprite_ = nullptr;
};
