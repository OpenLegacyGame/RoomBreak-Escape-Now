#include "DetailDustingFrame.h"
using namespace ax;
DetailDustingFrame* DetailDustingFrame::make(const rb::GameData& data, int id, Vec2 origin, int language, int hands)
{
    return createDetail<DetailDustingFrame>(data, id, origin, language, hands);
}
void DetailDustingFrame::setup(const rb::Row&)
{
    makeBoard("e3_s1_det1_1.png");
    const int files[] = {6, 3, 5, 2, 4, 7};
    for (int i = 0; i < 6; ++i)
        dust_[i] = addImage("e3_s1_det1_" + std::to_string(files[i]) + ".png", 32, 32, false);
}
Vec2 DetailDustingFrame::controlCenter(int i) const
{
    return dust_.at(i)->convertToWorldSpace(dust_.at(i)->getContentSize() / 2);
}
void DetailDustingFrame::press(Vec2 p)
{
    for (int i = 0; i < 6; ++i)
        if (contains(dust_[i], p) && hitPoints_[i])
        {
            selected_ = i;
            touching_ = true;
        }
    last_ = p;
}
void DetailDustingFrame::move(Vec2 p)
{
    auto delta = p - last_;
    if (touching_)
    {
        distance_ += std::max(std::abs(delta.x), std::abs(delta.y));
        particle(p);
        if (distance_ > 80)
        {
            consume(selected_);
            touching_ = false;
        }
    }
    last_ = p;
}
void DetailDustingFrame::particle(Vec2 p)
{
    if (particles_.size() == 5)
        return;
    auto* effect = ParticleSystemQuad::create("par_det_dust.plist");
    addChild(effect);
    effect->setPosition(convertToNodeSpace(p));
    particles_.push_back(effect);
    runAction(Sequence::create(DelayTime::create(3), CallFunc::create([this] {
        if (!particles_.empty())
        {
            particles_.front()->removeFromParent();
            particles_.erase(particles_.begin());
        }
    }),
                               nullptr));
}
void DetailDustingFrame::consume(int i)
{
    busy_ = true;
    dust_[i]->runAction(Sequence::create(Spawn::create(FadeOut::create(1), ScaleTo::create(1, 2), nullptr),
                                         CallFunc::create([this, i] {
        if (closing_)
            return;
        busy_ = false;
        dust_[i]->setVisible(false);
        hitPoints_[i] = 0;
        if (remaining() == 0)
            solve();
    }),
                                         nullptr));
    playSound("snd_p42");
}
void DetailDustingFrame::retry()
{
    if (closing_)
        return;
    for (int i = 0; i < 6; ++i)
    {
        dust_[i]->setVisible(true);
        dust_[i]->setScale(1);
        dust_[i]->runAction(FadeTo::create(.5f, 255));
        hitPoints_[i] = 1;
    }
}
void DetailDustingFrame::solve()
{
    if (closing_ || solved_)
        return;
    solved_ = true;
    success();
    for (auto* dust : dust_)
        dust->runAction(FadeTo::create(.5f, 0));
}
