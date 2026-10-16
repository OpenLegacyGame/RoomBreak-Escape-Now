#pragma once
#include "axmol.h"
#include <functional>
// Shared animation from AbilityControlLayer, used by eyes, hands and feet.
class AbilityEffect : public ax::Node
{
public:
    static AbilityEffect* create(int type);
    void start();
    std::function<void()> complete;
    std::function<void(std::string_view)> sound;

private:
    ax::LayerColor* shade_            = nullptr;
    ax::ParticleSystemQuad* particle_ = nullptr;
    int type_                         = 0;
    bool started_                     = false;
};
