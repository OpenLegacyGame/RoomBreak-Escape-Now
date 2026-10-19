#include "ActionCut.h"
#include "Assets.h"
#include <sstream>
#include <stdexcept>
using namespace ax;
ActionCut* ActionCut::create(int level, int hands)
{
    return createAction<ActionCut>(level, hands);
}
void ActionCut::setup(int level)
{
    auto text = FileUtils::getInstance()->getStringFromFile("act_cut_levlist.txt");
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
        std::replace(line.begin(), line.end(), ',', ' ');
        std::istringstream values(line);
        int value;
        while (values >> value)
        {
            if (value < 0 || value > 7)
                throw std::runtime_error("Invalid cut direction");
            directions_.push_back(value);
        }
        break;
    }
    if (directions_.empty())
        throw std::runtime_error("Missing cut level: " + std::to_string(level));
    for (int i = 0; i < 8; ++i)
    {
        arrows_[i] = rb::sprite("act_cut_arr1.png");
        addChild(arrows_[i]);
        arrows_[i]->setPosition(240, 160);
        arrows_[i]->setRotation(i * 45);
        arrows_[i]->setOpacity(0);
    }
    startTop_    = rb::sprite("act_cut_action1.png");
    startBottom_ = rb::sprite("act_cut_action2.png");
    successText_ = rb::sprite("act_cut_success.png");
    for (auto* s : {startTop_, startBottom_, successText_})
    {
        addChild(s);
        s->setOpacity(0);
    }
    successText_->setPosition(240, 160);
    trails_ = Node::create();
    addChild(trails_);
}
void ActionCut::start()
{
    unschedule("cut-reveal");
    unschedule("cut-banner");
    unschedule("cut-complete");
    cancelGesture();
    for (auto* n : boxes_)
        n->removeFromParent();
    for (auto* n : copies_)
        n->removeFromParent();
    boxes_.clear();
    copies_.clear();
    for (auto* n : arrows_)
    {
        n->stopAllActions();
        n->setOpacity(0);
    }
    for (auto* n : {startTop_, startBottom_, successText_})
    {
        n->stopAllActions();
        n->setOpacity(0);
        n->setRotation(0);
    }
    startTop_->setPosition(240, 160 + startTop_->getContentSize().height / 2);
    startBottom_->setPosition(240, 160 - startBottom_->getContentSize().height / 2);
    step_ = revealStep_ = 0;
    busy_               = true;
    playing_            = true;
    schedule([this](float) { reveal(); }, .7f, "cut-reveal");
}
void ActionCut::reveal()
{
    auto* arrow = arrows_[directions_[revealStep_]];
    arrow->stopAllActions();
    arrow->runAction(Sequence::create(EaseIn::create(FadeIn::create(.3f), .25f), DelayTime::create(.3f),
                                      EaseOut::create(FadeOut::create(.3f), .25f), nullptr));
    auto* box = rb::sprite("act_cut_ui1.png");
    addChild(box);
    box->setPosition(43 + 41 * revealStep_ + box->getContentSize().width / 2,
                     320 - 43 - box->getContentSize().height / 2);
    boxes_.push_back(box);
    box->setOpacity(0);
    box->runAction(FadeIn::create(1));
    if (++revealStep_ == stepCount())
    {
        unschedule("cut-reveal");
        scheduleOnce([this](float) { startBanner(); }, 1.5f, "cut-banner");
    }
}
void ActionCut::startBanner()
{
    startTop_->runAction(Sequence::create(
        FadeIn::create(.5f), DelayTime::create(.5f), CallFunc::create([this] { playSound("snd_f35"); }),
        Spawn::create(FadeOut::create(1),
                      EaseIn::create(MoveTo::create(1.5f, startTop_->getPosition() + Vec2(-20, 25)), .25f),
                      EaseIn::create(RotateTo::create(1.5f, 10), .25f), nullptr),
        nullptr));
    startBottom_->runAction(Sequence::create(
        FadeIn::create(.5f), DelayTime::create(.5f), CallFunc::create([this] { busy_ = false; }),
        Spawn::create(FadeOut::create(1),
                      EaseIn::create(MoveTo::create(1.5f, startBottom_->getPosition() + Vec2(15, -25)), .25f),
                      EaseIn::create(RotateTo::create(1.5f, 350), .25f), nullptr),
        nullptr));
}
void ActionCut::press(Vec2 p)
{
    if (!playing_ || step_ >= stepCount())
        return;
    checkpoint_ = last_ = p;
    slicing_            = true;
    started_            = std::chrono::steady_clock::now();
    schedule([this](float) {
        if (std::chrono::duration<float>(std::chrono::steady_clock::now() - started_).count() >= 1)
        {
            playing_ = false;
            fail();
        }
    }, 0, "cut-timer");
}
void ActionCut::move(Vec2 p)
{
    if (!slicing_)
        return;
    if (p != last_)
    {
        auto* trail = DrawNode::create();
        trail->setBlendFunc(BlendFunc::ADDITIVE);
        trails_->addChild(trail);
        const auto a = trails_->convertToNodeSpace(last_), b = trails_->convertToNodeSpace(p);
        auto age = std::make_shared<float>(0);
        trail->drawLine(a, b, Color4F(0, 1, 0, 1), 12);
        trail->schedule([trail, a, b, age](float dt) {
            *age += dt;
            if (*age >= .2f)
            {
                trail->removeFromParent();
                return;
            }
            trail->clear();
            trail->drawLine(a, b, Color4F(*age / .2f, 1, 0, 1), 12);
        }, "life");
        last_ = p;
    }
    const auto delta     = p - checkpoint_;
    const float distance = delta.length();
    if (distance < 50)
        return;
    total_ += distance;
    const Vec2 vectors[] = {{-1, 0}, {-1, 1}, {0, 1}, {1, 1}, {1, 0}, {1, -1}, {0, -1}, {-1, -1}};
    const float cosine   = delta.dot(vectors[direction()]) / (distance * vectors[direction()].length());
    if (cosine < .9f && !right_)
        wrong_ = true;
    if (!wrong_ && total_ >= 150)
        right_ = true;
    checkpoint_ = p;
}
void ActionCut::release(Vec2)
{
    if (!slicing_)
        return;
    const bool pass = right_;
    cancelGesture();
    if (pass)
        cutSuccess();
    else
    {
        playing_ = false;
        fail();
    }
}
void ActionCut::cancelGesture()
{
    unschedule("cut-timer");
    slicing_ = right_ = wrong_ = false;
    total_                     = 0;
}
void ActionCut::cutSuccess()
{
    successText_->stopAllActions();
    successText_->setOpacity(255);
    successText_->setScale(1);
    successText_->runAction(Sequence::create(EaseIn::create(ScaleTo::create(.3f, 1.1f), .25f),
                                             EaseIn::create(ScaleTo::create(.3f, 1), .25f), FadeOut::create(.5f),
                                             nullptr));
    auto* arrow = rb::sprite("act_cut_arr1.png");
    addChild(arrow);
    copies_.push_back(arrow);
    arrow->setPosition(240, 160);
    arrow->setRotation(direction() * 45);
    auto* box = boxes_[step_];
    arrow->runAction(Sequence::create(EaseIn::create(FadeIn::create(.3f), .25f),
                                      Spawn::create(EaseOut::create(MoveTo::create(1, box->getPosition()), .25f),
                                                    EaseIn::create(ScaleTo::create(1, .15f), .5f), nullptr),
                                      CallFunc::create([box] {
        box->runAction(Sequence::create(EaseOut::create(ScaleTo::create(.15f, 1.2f), .25f),
                                        EaseIn::create(ScaleTo::create(.15f, 1), .25f), nullptr));
    }),
                                      nullptr));
    ++step_;
    playSound("snd_f36");
    if (step_ == stepCount())
    {
        busy_ = true;
        scheduleOnce([this](float) { solve(); }, 2, "cut-complete");
    }
}
void ActionCut::solve()
{
    playing_ = false;
    ActionMode::solve();
}
void ActionCut::hideContent()
{
    playing_ = false;
    unschedule("cut-reveal");
    unschedule("cut-banner");
    unschedule("cut-complete");
    for (auto* n : boxes_)
        n->runAction(FadeOut::create(.5f));
    for (auto* n : copies_)
        n->runAction(FadeOut::create(.5f));
}
