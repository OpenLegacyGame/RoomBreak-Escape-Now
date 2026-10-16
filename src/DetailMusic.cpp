#include "DetailMusic.h"
#include "SheetSprite.h"
using namespace ax;
DetailMusic* DetailMusic::make(const rb::GameData& data, int id, Vec2 origin, int language, int hands)
{
    return createDetail<DetailMusic>(data, id, origin, language, hands);
}
void DetailMusic::setup(const rb::Row&)
{
    makeBoard("e2_s3_det1_1.png");
    success_ = true;
    for (int i = 0; i < 20; ++i)
    {
        auto* note = SheetSprite::create("e2_s3_det1_3.png", 20, 1);
        note->frame(i);
        board_->addChild(note);
        track(note);
        place(board_, note, {note->getContentSize().width * (i + .5f), 16});
        notes_.push_back(note);
    }
    addImage("e2_s3_det1_2.png", 0, 0);
    schedule([this](float) {
        if (!closing_)
            updateVisibility();
    }, "visibility");
}
void DetailMusic::updateVisibility()
{
    for (auto* note : notes_)
    {
        const float half = note->getContentSize().width / 2, x = note->getPositionX();
        note->setOpacity(x >= half && x <= board_->getContentSize().width - half ? 255 : 0);
    }
}
Vec2 DetailMusic::controlCenter(int index) const
{
    auto* note = notes_.at(index);
    return note->convertToWorldSpace(note->getContentSize() / 2);
}
void DetailMusic::press(Vec2 p)
{
    last_     = p;
    dragging_ = false;
    for (auto* note : notes_)
        if (contains(note, p))
        {
            dragging_ = note->getOpacity() != 0;
            break;
        }
}
void DetailMusic::move(Vec2 p)
{
    const float dx = p.x - last_.x;
    last_          = p;
    // Original 0x2290c tests the current endpoints before applying the drag.
    if (dragging_ && notes_.front()->getPositionX() < 50 &&
        notes_.back()->getPositionX() > board_->getContentSize().width - 50)
        for (auto* note : notes_)
            note->setPositionX(note->getPositionX() + dx);
}
void DetailMusic::release(Vec2)
{
    dragging_         = false;
    float offset      = 0;
    const float first = notes_.front()->getPositionX(), width = notes_.front()->getContentSize().width;
    if (first > width)
        offset = width - first;
    const float last = notes_.back()->getPositionX(), limit = board_->getContentSize().width - 25;
    if (last < limit)
        offset = limit - last;
    if (offset)
        for (auto* note : notes_)
            note->runAction(EaseIn::create(MoveBy::create(.3f, {offset, 0}), .25f));
}
