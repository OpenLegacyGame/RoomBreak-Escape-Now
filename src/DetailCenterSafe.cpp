#include "DetailCenterSafe.h"
#include "Assets.h"
#include <cmath>
#include <stdexcept>
using namespace ax;
namespace
{
constexpr std::array<int, 7> password = {1, 2, 2, 3, 2, 1, 2};
}
DetailCenterSafe* DetailCenterSafe::make(const rb::GameData& data, int id, Vec2 origin, int language, int hands)
{
    return createDetail<DetailCenterSafe>(data, id, origin, language, hands);
}
void DetailCenterSafe::setup(const rb::Row&)
{
    makeBoard("e5_s3_det3_1.png");
    for (int c = 0; c < 7; ++c)
        for (int r = 0; r < 3; ++r)
        {
            auto* s       = rb::sprite("e5_s3_det3_2.png");
            blocks_[c][r] = s;
            board_->addChild(s);
            track(s);
            const auto size = s->getContentSize();
            place(board_, s, {10 + size.width * c, 10 + size.height * (r + 1)});
            auto* p        = rb::sprite("e5_s3_det3_3.png");
            pressed_[c][r] = p;
            board_->addChild(p);
            p->setPosition(s->getPosition());
            p->setVisible(false);
        }
}
Vec2 DetailCenterSafe::controlCenter(int i) const
{
    if (i < 0 || i >= 21)
        throw std::runtime_error("Invalid center block");
    auto* s = blocks_[i / 3][i % 3];
    return s->convertToWorldSpace(s->getContentSize() / 2);
}
void DetailCenterSafe::press(Vec2 p)
{
    last_ = p;
    for (int c = 0; c < 7; ++c)
        for (int r = 0; r < 3; ++r)
            if (contains(blocks_[c][r], p))
            {
                column_   = c;
                row_      = r;
                dragging_ = true;
                pressed_[c][r]->setVisible(true);
                break;
            }
}
void DetailCenterSafe::move(Vec2 p)
{
    float dy = p.y - last_.y;
    last_    = p;
    if (!dragging_)
        return;
    int gap = input_[column_];
    if (row_ != gap && row_ != gap - 1)
        return;
    if ((row_ == gap && dy < 0) || (row_ == gap - 1 && dy > 0))
        return;
    checkLength_ += dy;
    auto* block = blocks_[column_][row_];
    if (std::abs(checkLength_ + dy) >= block->getContentSize().height / 2)
    {
        dragging_   = false;
        float delta = block->getContentSize().height - std::abs(moved_);
        if (checkLength_ < 0)
        {
            delta = -delta;
            --input_[column_];
        }
        else
            ++input_[column_];
        animateBlock(column_, row_, delta);
        checkLength_ = 0;
    }
    else
    {
        block->setPositionY(block->getPositionY() + dy);
        pressed_[column_][row_]->setPosition(block->getPosition());
        moved_ += dy;
    }
}
void DetailCenterSafe::animateBlock(int c, int r, float delta, bool check)
{
    busy_       = true;
    auto target = blocks_[c][r]->getPosition() + Vec2(0, delta);
    for (auto* s : {blocks_[c][r], pressed_[c][r]})
    {
        s->stopAllActions();
        s->runAction(EaseIn::create(MoveTo::create(.3f, target), .25f));
    }
    scheduleOnce([this, check](float) {
        busy_ = false;
        if (check && !solved_ && input_ == password)
            solve();
    }, .3f, "center-move-" + std::to_string(c) + "-" + std::to_string(r));
    playSound("snd_p08");
}
void DetailCenterSafe::release(Vec2)
{
    for (auto& col : pressed_)
        for (auto* s : col)
            s->setVisible(false);
    if (dragging_ && moved_ != 0)
    {
        busy_       = true;
        auto target = blocks_[column_][row_]->getPosition() - Vec2(0, moved_);
        for (auto* s : {blocks_[column_][row_], pressed_[column_][row_]})
            s->runAction(MoveTo::create(.3f, target));
        scheduleOnce([this](float) { busy_ = false; }, .3f, "center-restore");
    }
    moved_  = 0;
    column_ = row_ = 0;
    dragging_      = false;
}
void DetailCenterSafe::cancel()
{
    release({});
}
void DetailCenterSafe::resetBlocks()
{
    for (int c = 0; c < 7; ++c)
    {
        for (int r = 0; r < input_[c]; ++r)
            animateBlock(c, r, -blocks_[c][r]->getContentSize().height);
        input_[c] = 0;
    }
}
void DetailCenterSafe::retry()
{
    if (!closing_)
        resetBlocks();
}
void DetailCenterSafe::solve()
{
    if (solved_ || closing_)
        return;
    solved_ = true;
    success();
    resetBlocks();
    scheduleOnce([this](float) {
        for (int c = 0; c < 7; ++c)
            for (int r = 0; r < password[c]; ++r)
                animateBlock(c, r, blocks_[c][r]->getContentSize().height, false);
    }, .3f, "center-success");
}
