#include "DetailCircuit.h"
#include "Assets.h"
using namespace ax;
const std::array<int, 105> DetailCircuit::grid_ = {
    2, 5, 0, 3, 1, 3, 0, 3, 1, 3, 0, 5, 2, 5, 0, 5, 6, 4, 6, 4, 6, 4, 6, 4, 6, 4, 6, 5, 6, 4, 0, 3, 1, 3, 1,
    3, 1, 3, 1, 3, 1, 3, 1, 3, 1, 5, 6, 5, 6, 4, 6, 4, 6, 4, 6, 4, 6, 4, 6, 5, 2, 5, 2, 5, 0, 3, 1, 3, 0, 3,
    1, 3, 0, 5, 2, 5, 6, 5, 6, 4, 6, 5, 6, 4, 6, 4, 6, 5, 6, 5, 2, 5, 2, 5, 2, 5, 2, 3, 1, 3, 0, 5, 2, 5, 2};
DetailCircuit* DetailCircuit::make(const rb::GameData& data, int id, Vec2 origin, int language, int hands)
{
    return createDetail<DetailCircuit>(data, id, origin, language, hands);
}
void DetailCircuit::setup(const rb::Row&)
{
    makeBoard("e2_s2_det2_1.png");
    for (int i = 0; i < 105; ++i)
    {
        int type = grid_[i], file = type == 0 ? 4 : (type == 1 ? 7 : (type == 3 ? 2 : (type == 4 ? 3 : 0)));
        if (!file)
            continue;
        auto* s   = rb::sprite("e2_s2_det2_" + std::to_string(file) + ".png");
        cells_[i] = s;
        board_->addChild(s, type < 2 ? 1 : 0);
        place(board_, s, {float(20 * (i % 15) + (type == 4 ? 17 : 7)), float(20 * (i / 15) + (type == 3 ? 30 : 20))});
        if (type < 2)
            track(s);
        else
            s->setOpacity(0);
    }
}
void DetailCircuit::open()
{
    DetailView::open();
    start();
}
Vec2 DetailCircuit::controlCenter(int i) const
{
    return cells_.at(i)->convertToWorldSpace(cells_.at(i)->getContentSize() / 2);
}
void DetailCircuit::press(Vec2 p)
{
    pressed_ = -1;
    for (int i = 0; i < 105; ++i)
        if (grid_[i] < 2 && contains(cells_[i], p))
        {
            pressed_ = i;
            break;
        }
}
void DetailCircuit::release(Vec2 p)
{
    int i = pressed_;
    cancel();
    if (i < 0 || !contains(cells_[i], p))
        return;
    auto it = std::find_if(ways_.begin(), ways_.end(), [i](auto pair) { return pair.first == i; });
    if (it != ways_.end())
    {
        int line = it->second;
        lightPoint(i);
        lightLine(line);
    }
}
Sprite* DetailCircuit::overlay(int i, int file, int z)
{
    auto* s = rb::sprite("e2_s2_det2_" + std::to_string(file) + ".png");
    board_->addChild(s, z);
    s->setPosition(cells_[i]->getPosition());
    s->setOpacity(0);
    return s;
}
void DetailCircuit::lightPoint(int i)
{
    auto* s = overlay(i, grid_[i] == 0 ? 5 : 8, 1);
    points_.push_back(s);
    visited_[i] = true;
    current_    = i;
    s->runAction(Sequence::create(FadeTo::create(.3f, 255), CallFunc::create([this] {
        if (closing_)
            return;
        if (!busy_ && std::all_of(visited_.begin(), visited_.end(), [](bool b) { return b; }))
            solve();
        else
            checkWays();
    }),
                                  nullptr));
    playSound("snd_p01");
}
void DetailCircuit::lightLine(int i)
{
    auto* s = overlay(i, grid_[i] == 3 ? 2 : 3, 0);
    lines_.push_back(s);
    s->runAction(FadeTo::create(.3f, 255));
}
void DetailCircuit::checkWays()
{
    ways_.clear();
    int x = current_ % 15, y = current_ / 15;
    for (auto delta : std::array<Vec2, 4>{Vec2(-2, 0), Vec2(2, 0), Vec2(0, -2), Vec2(0, 2)})
    {
        int nx = x + int(delta.x), ny = y + int(delta.y);
        if (nx < 0 || nx >= 15 || ny < 0 || ny >= 7)
            continue;
        int point = ny * 15 + nx;
        if (grid_[point] <= 1 && !visited_[point])
            ways_.push_back({point, (point + current_) / 2});
    }
    if (ways_.empty())
        failure();
}
void DetailCircuit::failure()
{
    busy_ = true;
    for (int i = 0; i < 105; ++i)
        if (grid_[i] <= 1 && !visited_[i])
            wrong_.push_back(overlay(i, grid_[i] == 0 ? 6 : 9, 1));
    for (std::size_t i = 0; i < wrong_.size(); ++i)
    {
        auto* blink = Repeat::create(Sequence::create(FadeIn::create(.1f), FadeOut::create(.1f), nullptr), 3);
        if (i + 1 == wrong_.size())
            wrong_[i]->runAction(Sequence::create(blink, CallFunc::create([this] {
                if (!closing_)
                {
                    busy_ = false;
                    retry();
                }
            }),
                                                  nullptr));
        else
            wrong_[i]->runAction(blink);
    }
    playSound("snd_p27");
}
void DetailCircuit::finishReset()
{
    for (auto* s : lines_)
        s->removeFromParent();
    for (auto* s : points_)
        s->removeFromParent();
    current_ = 30;
    lightPoint(current_);
    lines_.clear();
    points_.clear();
    busy_ = false;
}
void DetailCircuit::start()
{
    for (int i = 0; i < 105; ++i)
        visited_[i] = grid_[i] >= 2 && grid_[i] <= 6;
    busy_ = true;
    if (points_.empty())
    {
        finishReset();
        return;
    }
    for (auto* s : lines_)
        s->runAction(FadeOut::create(.3f));
    for (std::size_t i = 0; i < points_.size(); ++i)
        if (i + 1 == points_.size())
            points_[i]->runAction(Sequence::create(FadeOut::create(.3f), CallFunc::create([this] {
                if (!closing_)
                    finishReset();
            }),
                                                   nullptr));
        else
            points_[i]->runAction(FadeOut::create(.3f));
}
void DetailCircuit::retry()
{
    if (closing_)
        return;
    for (auto* s : wrong_)
        s->removeFromParent();
    wrong_.clear();
    start();
}
void DetailCircuit::solve()
{
    if (closing_)
        return;
    success();
    for (int i = 0; i < 105; ++i)
        if (!visited_[i])
        {
            if (grid_[i] <= 1)
                lightPoint(i);
            if (grid_[i] == 3 || grid_[i] == 4)
                lightLine(i);
        }
}
