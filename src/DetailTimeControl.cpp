#include "DetailTimeControl.h"
#include "Assets.h"
#include <cstdio>
using namespace ax;
namespace
{
constexpr int ranges[][2] = {{11, 14},
                             {8, 11},
                             {15, 18},
                             {12, 15},
                             {7, 10},
                             {4, 5},
                             {16, 19},
                             {6, 8},
                             {17, 20},
                             {5, 7},
                             {1058642330, 1056964608}};
}
DetailTimeControl* DetailTimeControl::make(const rb::GameData& data, int id, Vec2 origin, int language, int hands)
{
    return createDetail<DetailTimeControl>(data, id, origin, language, hands);
}
void DetailTimeControl::setup(const rb::Row&)
{
    makeBoard("e4_s4_det1_1.png");
    finger_  = addImage("e4_s4_det1_2.png", 220, 28);
    fingerP_ = addImage("e4_s4_det1_3.png", 220, 28, false);
    fingerP_->setPosition(finger_->getPosition());
    fingerP_->setVisible(false);
    failure_ = addImage("e4_s4_det1_4.png", 204, 184, false);
    failure_->setOpacity(0);
    showTarget(192);
    track(targetLabel_);
}
void DetailTimeControl::showTarget(float y)
{
    if (!targetLabel_)
    {
        targetLabel_ = rb::label("", 2, 15);
        board_->addChild(targetLabel_);
    }
    targetLabel_->setString(std::to_string(ranges[target_][0]) + " sec --- " + std::to_string(ranges[target_][1]) +
                            " sec");
    place(board_, targetLabel_, {35, y});
}
Vec2 DetailTimeControl::controlCenter(int) const
{
    return finger_->convertToWorldSpace(finger_->getContentSize() / 2);
}
void DetailTimeControl::press(Vec2 p)
{
    pressed_ = contains(finger_, p);
    fingerP_->setVisible(pressed_);
    if (pressed_)
        start_ = std::chrono::system_clock::now();
}
void DetailTimeControl::cancel()
{
    pressed_ = false;
    fingerP_->setVisible(false);
}
void DetailTimeControl::release(Vec2 p)
{
    bool accepted = pressed_ && contains(finger_, p);
    cancel();
    if (!accepted)
        return;
    double elapsed = std::chrono::duration<double>(std::chrono::system_clock::now() - start_).count();
    if (resultLabel_)
        resultLabel_->removeFromParent();
    char text[64];
    std::snprintf(text, sizeof(text), "%.1f sec", elapsed);
    resultLabel_ = rb::label(text, 2, 15);
    board_->addChild(resultLabel_);
    place(board_, resultLabel_, {235, 193});
    if (elapsed >= ranges[target_][0] && elapsed <= ranges[target_][1])
    {
        solve();
        return;
    }
    busy_ = true;
    playSound("snd_p28");
    failure_->setOpacity(255);
    failure_->runAction(
        Sequence::create(Repeat::create(Sequence::create(FadeOut::create(.1f), FadeIn::create(.1f), nullptr), 5),
                         FadeOut::create(.3f), CallFunc::create([this] {
        busy_ = false;
        resultLabel_->runAction(FadeOut::create(.5f));
        retry();
    }),
                         nullptr));
}
void DetailTimeControl::retry()
{
    target_ = target_ >= 10 ? 0 : target_ + 1;
    showTarget(193);
}
void DetailTimeControl::solve()
{
    if (closing_)
        return;
    busy_ = true;
    success();
}
