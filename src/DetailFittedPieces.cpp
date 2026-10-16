#include "DetailFittedPieces.h"
#include "Assets.h"
using namespace ax;
DetailFittedPieces* DetailFittedPieces::make(const rb::GameData& data, int id, Vec2 origin, int language, int hands)
{
    return createDetail<DetailFittedPieces>(data, id, origin, language, hands);
}
void DetailFittedPieces::setup(const rb::Row&)
{
    makeBoard("e6_s2_det1_1.png");
    const Vec2 starts[] = {{197, 154}, {199, 9},  {250, 69}, {278, 10}, {253, 159},
                           {224, 99},  {279, 67}, {196, 43}, {200, 184}};
    const Vec2 ends[]   = {{33, 45},  {59, 71},  {109, 46}, {33, 95},  {58, 120},
                           {34, 146}, {133, 46}, {133, 96}, {133, 146}};
    for (int i = 0; i < 9; ++i)
    {
        auto* p     = addImage("e6_s2_det1_" + std::to_string(i + 2) + ".png", starts[i].x, starts[i].y);
        pieces_[i]  = p;
        initial_[i] = p->getPosition();
        targets_[i] = {ends[i].x + p->getContentSize().width / 2,
                       board_->getContentSize().height - ends[i].y - p->getContentSize().height / 2};
    }
}
Vec2 DetailFittedPieces::controlCenter(int index) const
{
    return board_->convertToWorldSpace(pieces_.at(index)->getPosition());
}
Vec2 DetailFittedPieces::targetCenter(int index) const
{
    return board_->convertToWorldSpace(targets_.at(index));
}
void DetailFittedPieces::press(Vec2 p)
{
    last_     = p;
    selected_ = -1;
    for (int i = 0; i < 9; ++i)
        if (contains(pieces_[i], p))
        {
            selected_ = i;
            pieces_[i]->setLocalZOrder(1);
            break;
        }
}
void DetailFittedPieces::move(Vec2 p)
{
    if (selected_ >= 0)
        pieces_[selected_]->setPosition(pieces_[selected_]->getPosition() + p - last_);
    last_ = p;
}
void DetailFittedPieces::animate(int index, bool target)
{
    auto* p         = pieces_[index];
    const auto from = p->getPosition(), to = target ? targets_[index] : initial_[index];
    p->runAction(Sequence::create(
        EaseIn::create(
            ActionFloat::create(.5f, 0, 1, [p, from, to](float t) { p->setPosition(from + (to - from) * t); }), .25f),
        CallFunc::create([p] { p->setLocalZOrder(0); }), nullptr));
}
void DetailFittedPieces::release(Vec2)
{
    if (selected_ < 0)
        return;
    const int index  = selected_;
    selected_        = -1;
    const auto delta = pieces_[index]->getPosition() - targets_[index];
    fitted_[index]   = std::abs(delta.x) <= 10 && std::abs(delta.y) <= 10;
    animate(index, fitted_[index]);
    if (fitted_[index] && std::all_of(fitted_.begin(), fitted_.end(), [](bool value) { return value; }))
        solve();
}
void DetailFittedPieces::retry()
{
    if (closing_)
        return;
    for (int i = 0; i < 9; ++i)
        animate(i, false);
}
void DetailFittedPieces::solve()
{
    if (closing_)
        return;
    success();
    for (int i = 0; i < 9; ++i)
        animate(i, true);
}
