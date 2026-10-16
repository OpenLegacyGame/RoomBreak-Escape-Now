#pragma once
#include "axmol.h"
#include <optional>
namespace rb
{
// Original -hd images have twice the logical dimensions of their base images.
ax::Sprite* sprite(std::string_view filename);
ax::Sprite* place(ax::Node* parent, std::string_view filename, float x, float y, int z = 0);
// Adds node to parent with its top-left corner at (x, y) in parent's top-down coordinates.
void attach(ax::Node* parent, ax::Node* node, float x, float y, int z = 0);
void rotateGear(ax::Node* node, float seconds, bool clockwise);
const char* font(int language);
// Entries of other_texts*.txt; findOtherText returns nullopt for a missing key.
std::optional<std::string> findOtherText(int language, std::string_view key);
std::string otherText(int language, std::string_view key);
// Center position that puts child's top-left corner at (x, y) in parent's top-down coordinates.
ax::Vec2 topLeft(ax::Node* parent, ax::Node* child, float x, float y);
// True when a world-space point falls inside node's untransformed content rectangle.
bool hitTest(ax::Node* node, ax::Vec2 world);
ax::Label* label(std::string_view text, int language, float size);
}  // namespace rb
