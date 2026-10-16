#pragma once
#include "axmol.h"
#include <stdexcept>

namespace rb
{
// Two-phase construction for game nodes: allocate, run Node::init and hand
// ownership to the autorelease pool. Callers configure the returned node.
template <class T>
T* make()
{
    auto* node = new T;
    if (!node->init())
    {
        delete node;
        throw std::runtime_error("Cannot initialize node");
    }
    node->autorelease();
    return node;
}
}  // namespace rb
