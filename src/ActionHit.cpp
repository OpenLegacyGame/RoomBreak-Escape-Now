#include "ActionHit.h"
#include "Assets.h"
#include <sstream>
#include <stdexcept>
using namespace ax;
namespace
{
ActionInterval* hitFadeOut(Node* node, float seconds)
{
    // Original CCFadeOut::update: always writes 255 * (1 - t), even
    // when an earlier hit is still fading this shared sprite.
    return ActionFloat::create(seconds, 255.f, 0.f,
                               [node](float opacity) { node->setOpacity(static_cast<uint8_t>(opacity)); });
}
}  // namespace
ActionHit* ActionHit::create(int level, int hands)
{
    return createAction<ActionHit>(level, hands);
}
void ActionHit::setup(int level)
{
    auto text = FileUtils::getInstance()->getStringFromFile("act_hit_levlist.txt");
    std::istringstream lines(text);
    std::string line;
    while (std::getline(lines, line))
    {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        if (line != "LV" + std::to_string(level))
            continue;
        if (!std::getline(lines, line))
            break;
        const auto slash = line.find('/');
        if (slash == std::string::npos)
            break;
        speed_ = std::stof(line.substr(0, slash));
        line   = line.substr(slash + 1);
        std::replace(line.begin(), line.end(), ',', ' ');
        std::istringstream values(line);
        int value;
        while (values >> value)
        {
            if (value < 0 || value > 9)
                throw std::runtime_error("Invalid hit target");
            sequence_.push_back(value);
        }
        break;
    }
    if (sequence_.empty() || speed_ <= 0)
        throw std::runtime_error("Missing hit level: " + std::to_string(level));
    successOuter_  = rb::place(this, "act_hit_obj4.png", 0, 0, 1);
    successCenter_ = rb::place(this, "act_hit_cen4.png", 203, 122, 1);
    failedOuter_   = rb::place(this, "act_hit_obj3.png", 0, 0, 1);
    failedCenter_  = rb::place(this, "act_hit_cen3.png", 203, 122, 1);
    item_          = rb::place(this, "act_hit_item1_1.png", 221, 144, 1);
    itemAction_    = rb::place(this, "act_hit_item1_2.png", 221, 144, 1);
    for (auto* n : {successOuter_, successCenter_, failedOuter_, failedCenter_, itemAction_})
        n->setVisible(false);
    for (int i = 0; i < 9; ++i)
    {
        targets_[i] = rb::sprite(i == 8 ? "act_hit_cen1.png" : "act_hit_obj1.png");
        backs_[i]   = rb::sprite(i == 8 ? "act_hit_cen2.png" : "act_hit_obj2.png");
        addChild(targets_[i], 0);
        addChild(backs_[i], 2);
        targets_[i]->setPosition(240, 160);
        backs_[i]->setPosition(240, 160);
    }
    // Both source packages omit texture_3.png and fire.png. The original
    // texture loader returns nil, and its additive particle draw binds 0.
    // Keep this effect absent when the source texture is absent.
    if (FileUtils::getInstance()->isFileExist("texture_3.png"))
    {
        particles_ = ParticleSun::createWithTotalParticles(300);
        addChild(particles_);
        particles_->setVisible(false);
        particles_->setTexture(rb::sprite("texture_3.png")->getTexture());
        particles_->setDuration(.2f);
        particles_->setStartColor(Color4F(.3f, .3f, .3f, .9f));
        particles_->setStartSize(15);
        particles_->setEndSize(30);
        particles_->setLife(1);
        particles_->setEmissionRate(100);
    }
}
void ActionHit::start()
{
    failed_  = false;
    step_    = 0;
    playing_ = true;
    unschedule("hit-sequence");
    if (initialized_)
    {
        busy_ = false;
        schedule([this](float) { spawnCircle(); }, speed_ / 2, "hit-sequence");
        return;
    }
    initialized_           = true;
    busy_                  = true;
    const Vec2 positions[] = {{216, 61},  {269, 83},  {291, 136}, {269, 189},
                              {216, 211}, {163, 189}, {141, 136}, {163, 83}};
    for (int i = 0; i < 8; ++i)
    {
        auto* target    = targets_[i];
        const auto size = target->getContentSize();
        const Vec2 p(positions[i].x + size.width / 2, 320 - positions[i].y - size.height / 2);
        target->runAction(Spawn::create(EaseIn::create(MoveTo::create(1, p), .25f), RotateTo::create(1, 180), nullptr));
        auto* back       = backs_[i];
        const auto bsize = back->getContentSize();
        back->runAction(EaseIn::create(
            MoveTo::create(1, {positions[i].x + 4 + bsize.width / 2, 320 - positions[i].y - 4 - bsize.height / 2}),
            .25f));
    }
    schedule([this](float) { tick(); }, "hit-circles");
    scheduleOnce([this](float) {
        busy_ = false;
        schedule([this](float) { spawnCircle(); }, speed_ / 2, "hit-sequence");
    }, 1, "hit-open");
}
void ActionHit::spawnCircle()
{
    if (step_ >= sequenceSize())
    {
        if (circles_.empty() && !failed_)
            solve();
        return;
    }
    const int number = sequence_[step_++];
    if (!number)
        return;
    const int radius = static_cast<int>(backs_[number - 1]->getContentSize().width / 2);
    auto* drawing    = DrawNode::create();
    drawing->setBlendFunc(BlendFunc::ADDITIVE);
    addChild(drawing);
    circles_.push_back({number, static_cast<float>(3 * radius), static_cast<float>(radius - 2),
                        static_cast<float>(3 * radius), drawing, std::chrono::steady_clock::now()});
}
void ActionHit::tick()
{
    const auto now = std::chrono::steady_clock::now();
    for (std::size_t i = 0; i < circles_.size();)
    {
        auto& c         = circles_[i];
        const float age = std::chrono::duration<float>(now - c.born).count();
        c.radius        = c.max - c.max * age / speed_;
        if (age >= speed_ && !failed_)
            miss(c.number);
        if (failed_)
        {
            clearCircles();
            return;
        }
        if (pressedNumber_ == c.number)
        {
            const int number = c.number;
            const bool valid = std::abs(c.radius - c.valid) <= 10;
            pressedNumber_   = 0;
            if (!valid)
            {
                miss(number);
                clearCircles();
                return;
            }
            hit(number);
            c.drawing->removeFromParent();
            circles_.erase(circles_.begin() + i);
            continue;
        }
        c.drawing->clear();
        c.drawing->drawCircle(targetCenter(c.number), std::max(0.f, c.radius), 0, 100, false, Color4F(1, 1, 0, 1),
                              2.5f);
        ++i;
    }
}
int ActionHit::playableTarget() const
{
    for (const auto& c : circles_)
        if (std::abs(c.radius - c.valid) < 3)
            return c.number;
    return 0;
}
void ActionHit::press(Vec2 p)
{
    if (!playing_)
        return;
    for (int i = 0; i < 9; ++i)
    {
        auto* n = targets_[i];
        if (!rb::hitTest(n, p))
            continue;
        n->stopAllActions();
        n->setScale(1);
        n->runAction(Sequence::create(EaseIn::create(ScaleTo::create(.15f, 1.2f), 3),
                                      EaseIn::create(ScaleTo::create(.15f, 1), 3), nullptr));
        pressedNumber_ = i + 1;
        return;
    }
}
void ActionHit::hit(int number)
{
    auto* glow = number == 9 ? successCenter_ : successOuter_;
    glow->setPosition(targetCenter(number));
    glow->setVisible(true);
    glow->setOpacity(255);
    glow->setScale(1);
    glow->runAction(Spawn::create(hitFadeOut(glow, .5f), EaseIn::create(ScaleTo::create(.5f, 1.5f), .25f), nullptr));
    if (shake)
        shake();
    if (particles_)
    {
        particles_->resetSystem();
        particles_->setPosition(targetCenter(number));
        particles_->setVisible(true);
    }
    itemAction_->setVisible(true);
    itemAction_->setOpacity(255);
    itemAction_->runAction(Sequence::create(
        Spawn::create(hitFadeOut(itemAction_, .5f), EaseIn::create(ScaleTo::create(.5f, 1.5f), .25f), nullptr),
        CallFunc::create([this] {
        (pressedNumber_ < 9 ? successOuter_ : successCenter_)->setScale(1);
        itemAction_->setVisible(false);
    }),
        nullptr));
    playSound(number == 9 ? "snd_f32" : "snd_f31");
}
void ActionHit::miss(int number)
{
    failed_    = true;
    playing_   = false;
    auto* glow = number == 9 ? failedCenter_ : failedOuter_;
    glow->setPosition(targetCenter(number));
    glow->setVisible(true);
    glow->setOpacity(255);
    glow->setScale(1);
    glow->runAction(Sequence::create(
        Spawn::create(hitFadeOut(glow, .5f), EaseIn::create(ScaleTo::create(.5f, 1.5f), .25f), nullptr),
        CallFunc::create([this, glow] {
        glow->setScale(1);
        unschedule("hit-sequence");
        fail();
    }),
        nullptr));
    playSound("snd_f33");
}
void ActionHit::clearCircles()
{
    for (auto& c : circles_)
        c.drawing->removeFromParent();
    circles_.clear();
}
void ActionHit::solve()
{
    playing_ = false;
    unschedule("hit-sequence");
    ActionMode::solve();
}
void ActionHit::hideContent()
{
    playing_ = false;
    unschedule("hit-sequence");
    unschedule("hit-open");
    unschedule("hit-circles");
    clearCircles();
    for (int i = 0; i < 9; ++i)
    {
        if (i < 8)
            targets_[i]->runAction(
                Spawn::create(EaseIn::create(MoveTo::create(1, {240, 160}), .25f), RotateTo::create(1, 0), nullptr));
        backs_[i]->runAction(EaseIn::create(MoveTo::create(1, {240, 160}), .25f));
    }
}
