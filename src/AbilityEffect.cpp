#include "Viewport.h"
#include "AbilityEffect.h"
#include "NodeFactory.h"
#include <stdexcept>
using namespace ax;
AbilityEffect* AbilityEffect::create(int type)
{
    const char* particle = type == 0   ? "par_ability_eyes.plist"
                           : type == 2 ? "par_ability_hands.plist"
                           : type == 3 ? "par_ability_foots.plist"
                                       : nullptr;
    if (!particle)
        throw std::runtime_error("Invalid common ability type");
    auto* value  = rb::make<AbilityEffect>();
    value->type_ = type;
    value->setContentSize({480, 320});
    value->shade_ = rb::screenShade(Color4B(0, 0, 0, 0));
    value->addChild(value->shade_);
    value->particle_ = ParticleSystemQuad::create(particle);
    if (!value->particle_)
        throw std::runtime_error("Missing ability particle");
    value->addChild(value->particle_);
    value->particle_->setPosition(240, 160);
    value->particle_->stopSystem();
    value->particle_->setVisible(false);
    auto* touch = EventListenerTouchOneByOne::create();
    touch->setSwallowTouches(true);
    touch->onTouchBegan = [](Touch*, Event*) { return true; };
    value->_eventDispatcher->addEventListenerWithSceneGraphPriority(touch, value);
    return value;
}
void AbilityEffect::start()
{
    if (started_)
        throw std::runtime_error("Ability already started");
    started_ = true;
    if (sound && type_ != 3)
        sound(type_ == 0 ? "snd_f45" : "snd_f47");
    shade_->runAction(Sequence::create(FadeTo::create(1, 150), CallFunc::create([this] {
        particle_->setVisible(true);
        particle_->resetSystem();
    }),
                                       DelayTime::create(2), FadeTo::create(.5f, 0), CallFunc::create([this] {
        auto callback = std::move(complete);
        if (callback)
            callback();
    }),
                                       nullptr));
}
