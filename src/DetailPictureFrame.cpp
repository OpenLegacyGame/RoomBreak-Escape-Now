#include "DetailPictureFrame.h"
#include "SheetSprite.h"
using namespace ax;
DetailPictureFrame* DetailPictureFrame::make(const rb::GameData& data, int id, Vec2 origin, int language, int hands)
{
    return createDetail<DetailPictureFrame>(data, id, origin, language, hands);
}
void DetailPictureFrame::setup(const rb::Row&)
{
    makeBoard("e1_s3_det1_1.png");
    for (int i = 0; i < 5; ++i)
    {
        auto* eye = SheetSprite::create("e1_s3_det1_2.png", 5, 1);
        eye->frame(i);
        eyes_[i] = eye;
        board_->addChild(eyes_[i]);
        place(board_, eyes_[i], {53, 112});
        eyes_[i]->setOpacity(0);
    }
    scheduleOnce([this](float) { animateEyes(); }, 1, "picture-eyes-start");
}
void DetailPictureFrame::animateEyes()
{
    if (closing_)
        return;
    // Original table 0x1186D0 and callback 0x1B770: six directions, then closed eyes.
    constexpr int directions[] = {0, 1, 2, 3, 0, 3, 4};
    direction_                 = directions[step_];
    eyes_[direction_]->runAction(Sequence::create(EaseOut::create(FadeIn::create(.5f), .25f), DelayTime::create(1),
                                                  EaseOut::create(FadeOut::create(.5f), .25f), CallFunc::create([this] {
        step_ = (step_ + 1) % 7;
        animateEyes();
    }),
                                                  nullptr));
}
void DetailPictureFrame::close()
{
    unschedule("picture-eyes-start");
    for (auto* eye : eyes_)
    {
        eye->stopAllActions();
        eye->setVisible(false);
    }
    DetailView::close();
}
