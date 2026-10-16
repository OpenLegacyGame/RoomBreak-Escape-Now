#include "InventoryView.h"
#include "NodeFactory.h"
#include "Assets.h"
#include <algorithm>
#include <stdexcept>
using namespace ax;
InventoryView* InventoryView::create(const rb::GameData& data, rb::StageState& state, int language)
{
    auto* value      = rb::make<InventoryView>();
    value->data_     = &data;
    value->state_    = &state;
    value->language_ = language;
    value->setup();
    return value;
}
void InventoryView::setup()
{
    setContentSize({480, 320});
    const char* suffix = language_ == 1 ? "kor" : language_ == 3 ? "jap" : "eng";
    auto localized     = [suffix](const char* base) { return std::string(base) + "_" + suffix + ".png"; };
    bg_                = rb::sprite(localized("inv_bg"));
    addChild(bg_);
    const auto size = bg_->getContentSize();
    bg_->setPosition(-size.width / 2 - 24, size.height / 2);
    cell_ = rb::place(bg_, "inv_cell.png", 0, 75);
    cell_->setOpacity(0);
    open_  = rb::place(bg_, "inv_open.png", 429, 50);
    openP_ = rb::place(bg_, "inv_open_p.png", 429, 50);
    openP_->setVisible(false);
    close_  = rb::place(bg_, "inv_close.png", 429, 50);
    closeP_ = rb::place(bg_, "inv_close_p.png", 429, 50);
    close_->setVisible(false);
    closeP_->setVisible(false);
    highlight_ = rb::place(bg_, "inv_selected.png", 0, 114);
    highlight_->setOpacity(0);
    const char* names[] = {"use", "mix", "dev"};
    const float x[]     = {39, 114, 189};
    for (int i = 0; i < 3; ++i)
    {
        tabs_[i] = rb::place(bg_, localized((std::string("inv_") + names[i] + "_p").c_str()), x[i], 10);
        tabs_[i]->setVisible(false);
        selectedTabs_[i] = rb::place(bg_, localized((std::string("inv_") + names[i] + "_s").c_str()), x[i], 10);
        selectedTabs_[i]->setVisible(false);
    }
    for (int i = 0; i < 10; ++i)
    {
        auto* b = rb::place(bg_, "inv_item_index1.png", 141 + 15 * i, 78, 1);
        b->setOpacity(0);
        indexBg_.push_back(b);
        auto* s = rb::place(bg_, "inv_item_index2.png", 141 + 15 * i, 78, 1);
        s->setOpacity(0);
        index_.push_back(s);
    }
    indexSelected_ = rb::place(bg_, "inv_item_index3.png", 141, 78, 2);
    indexSelected_->setOpacity(0);
    const float xs[2][3] = {{59, 44, 19}, {363, 371, 386}};
    for (int side = 0; side < 2; ++side)
        for (int i = 0; i < 3; ++i)
        {
            auto* a = rb::place(bg_, "inv_arrow_" + std::to_string(3 - i) + ".png", xs[side][i], 147);
            a->setFlippedX(side == 1);
            a->setOpacity(0);
            arrow_[side][i] = a;
        }
    possible_ = rb::sprite("inv_possi.png");
    bg_->addChild(possible_, 4);
    possible_->setVisible(false);
    impossible_ = rb::sprite("inv_impos.png");
    bg_->addChild(impossible_, 4);
    impossible_->setVisible(false);
    insight_  = rb::place(bg_, "ability_short_insight.png", 275, 25);
    insightP_ = rb::place(bg_, "ability_short_insight_p.png", 275, 25);
    insightP_->setVisible(false);
    insightLabel_ = rb::label("0", 2, 10);
    insight_->addChild(insightLabel_);
    auto labelSize = insightLabel_->getContentSize();
    insightLabel_->setPosition(insight_->getContentSize().width / 2 - 2 + labelSize.width / 2,
                               insight_->getContentSize().height - 34 - labelSize.height / 2);
    insight_->setVisible(false);
    auto* touch = EventListenerTouchOneByOne::create();
    touch->setSwallowTouches(true);
    touch->onTouchBegan     = [this](Touch* t, Event*) { return beginTouch(t->getLocation()); };
    touch->onTouchMoved     = [this](Touch* t, Event*) { moveTouch(t->getLocation()); };
    touch->onTouchEnded     = [this](Touch* t, Event*) { endTouch(t->getLocation()); };
    touch->onTouchCancelled = [this](Touch* t, Event*) {
        dragged_ = true;
        endTouch(t->getLocation());
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(touch, this);
}
void InventoryView::show(bool visible)
{
    shown_ = visible;
    bg_->stopAllActions();
    bg_->runAction(EaseIn::create(
        MoveTo::create(.5f, {-bg_->getContentSize().width / 2 - (visible ? 0 : 24), bg_->getContentSize().height / 2}),
        .25f));
}
void InventoryView::refreshAbility()
{
    insight_->setVisible(opened_ && mode_ == 1 && !items_.empty());
    insightLabel_->setString(std::to_string(insightCount ? insightCount() : 0));
}
void InventoryView::refresh()
{
    std::vector<Sprite*> next;
    for (int id : state_->inventory())
    {
        auto it = std::find(ids_.begin(), ids_.end(), id);
        if (it != ids_.end())
        {
            const auto index = std::distance(ids_.begin(), it);
            next.push_back(items_[index]);
            items_[index] = nullptr;
            *it           = -1;
        }
        else
        {
            auto* item = rb::sprite(rb::string(data_->byId("ItemData", "itemID", id), "imageName"));
            bg_->addChild(item);
            item->setPosition(bg_->getContentSize() / 2);
            next.push_back(item);
        }
    }
    for (auto* item : items_)
        if (item)
            item->removeFromParent();
    items_ = std::move(next);
    ids_   = state_->inventory();
    refreshAbility();
}
void InventoryView::open()
{
    if (opened_)
        return;
    opened_  = true;
    sliding_ = false;
    open_->setVisible(false);
    openP_->setVisible(false);
    close_->setVisible(true);
    if (otherUI)
        otherUI(false);
    refresh();
    selected_ = std::max(0, static_cast<int>(items_.size()) - 1);
    mode_     = 0;
    selectMode(1);
    bg_->stopAllActions();
    bg_->runAction(EaseOut::create(MoveTo::create(.5f, bg_->getContentSize() / 2), 5));
    if (sound)
        sound("snd_f09");
}
void InventoryView::clearLabels()
{
    if (labels_)
    {
        labels_->removeFromParent();
        labels_ = nullptr;
    }
}
void InventoryView::close()
{
    if (!opened_ || operation_)
        return;
    opened_  = false;
    sliding_ = false;
    unschedule("inventory-hold");
    held_ = false;
    bg_->stopAllActions();
    bg_->runAction(
        EaseOut::create(MoveTo::create(.5f, {-bg_->getContentSize().width / 2, bg_->getContentSize().height / 2}), 5));
    if (otherUI)
        otherUI(true);
    open_->setVisible(true);
    close_->setVisible(false);
    openP_->setVisible(false);
    closeP_->setVisible(false);
    insight_->setVisible(false);
    insightP_->setVisible(false);
    clearLabels();
    for (auto* tab : selectedTabs_)
        tab->setVisible(false);
    for (auto* node : indexBg_)
        node->setOpacity(0);
    for (auto* node : index_)
        node->setOpacity(0);
    indexSelected_->setOpacity(0);
    for (auto& side : arrow_)
        for (auto* a : side)
        {
            a->stopAllActions();
            a->setOpacity(0);
        }
    if (sound)
        sound("snd_f01");
}
void InventoryView::sortUse(float duration)
{
    for (std::size_t i = 0; i < items_.size(); ++i)
    {
        auto* item = items_[i];
        item->stopAllActions();
        const int d = static_cast<int>(i) - selected_;
        item->runAction(
            Spawn::create(EaseIn::create(MoveTo::create(duration, bg_->getContentSize() / 2 + Vec2(d * 100, 0)), .25f),
                          EaseIn::create(ScaleTo::create(duration, d == 0 ? 1 : .5f), .25f),
                          EaseIn::create(FadeTo::create(duration, d == 0             ? 255
                                                                  : std::abs(d) == 1 ? 120
                                                                                     : 0),
                                         .25f),
                          nullptr));
    }
}
void InventoryView::labels()
{
    clearLabels();
    if (items_.empty())
        return;
    labels_ = Node::create();
    bg_->addChild(labels_, 3);
    const auto& row = data_->byId("ItemData", "itemID", state_->inventory().at(selected_));
    auto* name      = rb::label(data_->text(rb::integer(row, "itemName"), language_), language_, 12);
    labels_->addChild(name);
    name->setPosition(210, 63);
    name->setColor({151, 239, 250});
    auto* description = rb::label(data_->text(rb::integer(row, "contents"), language_), language_, 12);
    labels_->addChild(description);
    description->setPosition(210, 35);
}
void InventoryView::indices()
{
    for (std::size_t i = 0; i < indexBg_.size(); ++i)
    {
        indexBg_[i]->runAction(FadeIn::create(.3f));
        index_[i]->setOpacity(i < items_.size() ? 255 : 0);
    }
    indexSelected_->setOpacity(items_.empty() ? 0 : 255);
    indexSelected_->stopAllActions();
    indexSelected_->runAction(EaseIn::create(
        MoveTo::create(.35f, {141 + 15.f * selected_ + indexSelected_->getContentSize().width / 2,
                              bg_->getContentSize().height - 78 - indexSelected_->getContentSize().height / 2}),
        .25f));
}
void InventoryView::arrows()
{
    for (int side = 0; side < 2; ++side)
        for (int i = 0; i < 3; ++i)
        {
            auto* a = arrow_[side][i];
            a->stopAllActions();
            const bool active =
                !items_.empty() && (side == 0 ? selected_ > 0 : selected_ + 1 < static_cast<int>(items_.size()));
            a->setOpacity(active ? 100 : 0);
            if (active)
                a->runAction(RepeatForever::create(Sequence::create(DelayTime::create(i * .1f),
                                                                    FadeTo::create(.3f, 200), FadeTo::create(.3f, 100),
                                                                    DelayTime::create(.9f - i * .1f), nullptr)));
        }
}
bool InventoryView::beginTouch(Vec2 point)
{
    if (!shown_ || (blocked && blocked()))
        return false;
    if (operation_ || splitDragging_)
        return true;
    previous_ = start_ = point;
    pressed_           = -1;
    dragged_           = false;
    moving_            = 0;
    sliding_           = false;
    held_              = false;
    splitReady_        = false;
    target_            = -1;
    unschedule("inventory-hold");
    if (!opened_)
    {
        if (!rb::hitTest(open_, point))
            return false;
        pressed_ = 0;
        openP_->setVisible(true);
        bg_->stopAllActions();
        return true;
    }
    if (rb::hitTest(close_, point))
    {
        pressed_ = 1;
        closeP_->setVisible(true);
        bg_->stopAllActions();
        return true;
    }
    for (int i = 0; i < 3; ++i)
        if (rb::hitTest(tabs_[i], point))
        {
            pressed_ = 2 + i;
            tabs_[i]->setVisible(true);
            if (sound)
                sound("snd_f39");
            return true;
        }
    if (insight_->isVisible() && rb::hitTest(insight_, point))
    {
        pressed_ = 5;
        insightP_->setVisible(true);
        return true;
    }
    if (mode_ != 1)
    {
        for (std::size_t i = 0; i < items_.size(); ++i)
            if (rb::hitTest(items_[i], point))
            {
                selected_  = static_cast<int>(i);
                holdTicks_ = 0;
                schedule([this](float) {
                    if (++holdTicks_ >= 5)
                    {
                        unschedule("inventory-hold");
                        holdItem();
                    }
                }, .05f, "inventory-hold");
                break;
            }
        return true;
    }
    if (!items_.empty() && Rect(0, 102, 400, 140).containsPoint(convertToNodeSpace(point)))
    {
        sliding_ = true;
        for (auto* item : items_)
            item->stopAllActions();
        highlight_->stopAllActions();
        highlight_->runAction(FadeIn::create(.3f));
    }
    return true;
}
void InventoryView::moveTouch(Vec2 point)
{
    const auto delta = point - previous_;
    previous_        = point;
    if (operation_ && !splitDragging_)
        return;
    if (splitDragging_)
    {
        items_.back()->setPosition(bg_->convertToNodeSpace(point) + Vec2(-25, 25));
        return;
    }
    if (held_ && mode_ == 3)
    {
        items_[selected_]->setPosition(items_[selected_]->getPosition() + delta);
        target_ = -1;
        possible_->setVisible(false);
        impossible_->setVisible(false);
        for (std::size_t i = 0; i < items_.size(); ++i)
            if (static_cast<int>(i) != selected_ && rb::hitTest(items_[i], point))
            {
                const bool valid = !state_->combineResult(*data_, ids_[selected_], ids_[i]).empty();
                auto* marker     = valid ? possible_ : impossible_;
                marker->setVisible(true);
                marker->setPosition(items_[i]->getPosition());
                if (valid)
                    target_ = static_cast<int>(i);
                break;
            }
        return;
    }
    if (splitReady_ && mode_ == 2)
    {
        const auto stretch = point - start_;
        if (std::abs(static_cast<int>(delta.x)) > 35 || std::abs(static_cast<int>(delta.y)) > 35)
        {
            returnItem();
            return;
        }
        if (std::abs(static_cast<int>(stretch.x)) > 35 || std::abs(static_cast<int>(stretch.y)) > 35)
        {
            if (state_->separateResult(*data_, ids_[selected_]).size() == 2)
                separate(point);
            else
            {
                returnItem();
                if (sound)
                    sound("snd_f43");
            }
            return;
        }
        auto* item = items_[selected_];
        item->setScaleX(.6f + std::abs(stretch.x) / 100);
        item->setScaleY(.6f + std::abs(stretch.y) / 100);
        item->setOpacity(static_cast<uint8_t>(150 - 3 * std::max(std::abs(stretch.x), std::abs(stretch.y))));
        return;
    }
    if (pressed_ == 0 && !opened_)
    {
        bg_->setPositionX(bg_->getPositionX() + delta.x);
        if (bg_->getPositionX() > -bg_->getContentSize().width / 2 + 80)
        {
            open();
            pressed_ = -1;
        }
    }
    else if (pressed_ == 1 && opened_)
    {
        if (delta.x < 0)
            bg_->setPositionX(bg_->getPositionX() + delta.x);
        if (bg_->getPositionX() < bg_->getContentSize().width / 2 - 80)
        {
            close();
            pressed_ = -1;
        }
    }
    else if (sliding_ && opened_)
    {
        dragged_ = true;
        moving_ -= delta.x;
        if (std::abs(moving_) > 50)
        {
            selected_ = std::clamp(selected_ + (moving_ < 0 ? -1 : 1), 0, static_cast<int>(items_.size()) - 1);
            labels();
            indices();
            arrows();
            sortUse(.35f);
            moving_  = 0;
            sliding_ = false;
            if (sound)
                sound("snd_f40");
        }
        else
            for (std::size_t i = 0; i < items_.size(); ++i)
                items_[i]->setPositionX(items_[i]->getPositionX() +
                                        delta.x * (static_cast<int>(i) == selected_ ? 1 : .2f));
    }
}
void InventoryView::endTouch(Vec2 point)
{
    unschedule("inventory-hold");
    if (splitDragging_)
    {
        finishSeparate();
        clearLabels();
        return;
    }
    if (operation_)
        return;
    if (held_ && mode_ != 1)
    {
        if (mode_ == 3 && target_ >= 0)
            combine();
        else
        {
            returnItem();
            if (mode_ == 3 && sound)
                sound("snd_f43");
        }
        possible_->setVisible(false);
        impossible_->setVisible(false);
        clearLabels();
        return;
    }
    if (pressed_ == 5)
    {
        if (rb::hitTest(insight_, point) && requestInsight)
            requestInsight(ids_.at(selected_));
        insightP_->setVisible(false);
    }
    else if (pressed_ >= 2)
    {
        const int tab = pressed_ - 2;
        if (rb::hitTest(tabs_[tab], point))
            selectMode(tab == 0 ? 1 : tab == 1 ? 3 : 2);
    }
    else if (sliding_ && !dragged_ && !items_.empty())
    {
        const int id = state_->inventory().at(selected_);
        close();
        if (selectItem)
            selectItem(id);
    }
    if (pressed_ == 0 || pressed_ == 1)
    {
        bg_->stopAllActions();
        bg_->runAction(EaseIn::create(MoveTo::create(.5f, {(opened_ ? 1 : -1) * bg_->getContentSize().width / 2,
                                                           bg_->getContentSize().height / 2}),
                                      .25f));
    }
    for (auto* tab : tabs_)
        tab->setVisible(false);
    openP_->setVisible(false);
    closeP_->setVisible(false);
    pressed_ = -1;
    highlight_->runAction(FadeOut::create(.3f));
    if (opened_ && mode_ == 1)
        sortUse(.5f);
    sliding_ = false;
}
Vec2 InventoryView::gridPosition(int index) const
{
    return {bg_->getContentSize().width / 2 - 167 + (index % 5) * 65,
            bg_->getContentSize().height - 112 - (index / 5) * 55};
}
void InventoryView::sortGrid()
{
    for (std::size_t i = 0; i < items_.size(); ++i)
    {
        auto* item = items_[i];
        item->stopAllActions();
        item->setRotation(0);
        item->setLocalZOrder(0);
        item->runAction(EaseOut::create(Spawn::create(MoveTo::create(.5f, gridPosition(i)), ScaleTo::create(.5f, .4f),
                                                      FadeIn::create(.5f), nullptr),
                                        2.5f));
    }
}
void InventoryView::selectMode(int mode)
{
    if (mode < 1 || mode > 3)
        throw std::runtime_error("Invalid inventory mode");
    if (operation_ || mode_ == mode)
        return;
    unschedule("inventory-hold");
    held_       = false;
    splitReady_ = false;
    clearLabels();
    possible_->setVisible(false);
    impossible_->setVisible(false);
    for (int i = 0; i < 3; ++i)
        selectedTabs_[i]->setVisible(i == (mode == 1 ? 0 : mode == 3 ? 1 : 2));
    cell_->stopAllActions();
    cell_->runAction(FadeTo::create(mode == 1 ? .5f : 1.f, mode == 1 ? 0 : 255));
    mode_ = mode;
    refreshAbility();
    if (mode == 1)
    {
        selected_ = std::max(0, static_cast<int>(items_.size()) - 1);
        sortUse(.5f);
        labels();
        indices();
        arrows();
    }
    else
    {
        for (auto* a : indexBg_)
            a->runAction(FadeOut::create(.3f));
        for (auto* a : index_)
            a->runAction(FadeOut::create(.3f));
        indexSelected_->runAction(FadeOut::create(.3f));
        for (auto& side : arrow_)
            for (auto* a : side)
            {
                a->stopAllActions();
                a->runAction(FadeOut::create(.3f));
            }
        sortGrid();
    }
}
void InventoryView::holdItem()
{
    held_ = true;
    labels();
    auto* item = items_.at(selected_);
    item->stopAllActions();
    item->setLocalZOrder(2);
    if (mode_ == 3)
    {
        item->runAction(Sequence::create(Spawn::create(EaseIn::create(ScaleTo::create(.15f, .792f), .25f),
                                                       EaseIn::create(MoveBy::create(.15f, {-25, 25}), .25f), nullptr),
                                         Spawn::create(EaseOut::create(ScaleTo::create(.15f, .72f), .25f),
                                                       EaseOut::create(FadeTo::create(.15f, 85), .25f), nullptr),
                                         nullptr));
    }
    else
    {
        const bool valid = state_->separateResult(*data_, ids_[selected_]).size() == 2;
        auto* marker     = valid ? possible_ : impossible_;
        marker->setPosition(item->getPosition());
        marker->setVisible(true);
        item->runAction(Sequence::create(
            Spawn::create(EaseIn::create(MoveBy::create(.3f, {-25, 25}), .25f),
                          EaseIn::create(ScaleTo::create(.3f, .6f), .25f), FadeTo::create(.3f, 150), nullptr),
            CallFunc::create([this] { splitReady_ = true; }), nullptr));
    }
}
void InventoryView::returnItem()
{
    held_       = false;
    splitReady_ = false;
    operation_  = true;
    possible_->setVisible(false);
    impossible_->setVisible(false);
    auto* item = items_.at(selected_);
    item->stopAllActions();
    if (mode_ == 3)
        item->runAction(Spawn::create(EaseIn::create(MoveTo::create(.3f, gridPosition(selected_)), .25f),
                                      EaseIn::create(ScaleTo::create(.3f, .4f), .25f),
                                      EaseIn::create(FadeIn::create(.3f), .25f), nullptr));
    else
        item->runAction(Sequence::create(
            Spawn::create(EaseIn::create(MoveTo::create(.15f, gridPosition(selected_)), .25f), FadeIn::create(.15f),
                          EaseOut::create(ScaleTo::create(.15f, .3f), .25f), nullptr),
            EaseIn::create(ScaleTo::create(.15f, .4f), .25f), nullptr));
    scheduleOnce([this, item](float) {
        item->setLocalZOrder(0);
        operation_ = false;
    }, .3f, "inventory-return");
}
void InventoryView::combine()
{
    const int source = ids_.at(selected_), target = ids_.at(target_);
    const auto result = state_->combineResult(*data_, source, target);
    auto* a           = items_.at(selected_);
    auto* b           = items_.at(target_);
    operation_        = true;
    held_             = false;
    a->stopAllActions();
    b->stopAllActions();
    a->runAction(Sequence::create(EaseIn::create(MoveTo::create(.3f, b->getPosition()), .25f),
                                  EaseOut::create(ScaleTo::create(1, 0), .25f), nullptr));
    rb::rotateGear(a, .1f, true);
    b->runAction(Sequence::create(
        Spawn::create(EaseIn::create(ScaleTo::create(.3f, .72f), .25f), FadeTo::create(.3f, 85), nullptr),
        EaseOut::create(ScaleTo::create(1, 0), .25f), nullptr));
    rb::rotateGear(b, .15f, false);
    if (sound)
        sound("snd_f41");
    scheduleOnce([this, source, target, result](float) {
        state_->combineItems(*data_, source, target);
        refresh();
        sortGrid();
        const auto first = items_.size() - result.size();
        for (std::size_t i = first; i < items_.size(); ++i)
        {
            auto* item = items_[i];
            item->stopAllActions();
            item->setPosition(gridPosition(i));
            item->setScale(0);
            item->runAction(Sequence::create(EaseOut::create(ScaleTo::create(.8f, .44f), .25f),
                                             EaseIn::create(ScaleTo::create(.3f, .4f), .25f), nullptr));
        }
        if (changed)
            changed();
        scheduleOnce([this](float) { operation_ = false; }, 1.1f, "inventory-combine-end");
    }, 1.3f, "inventory-combine");
}
void InventoryView::separate(Vec2 point)
{
    auto* original      = items_.at(selected_);
    const auto position = original->getPosition();
    const float sx      = original->getScaleX();
    const auto opacity  = original->getOpacity();
    state_->separateItem(*data_, ids_.at(selected_));
    refresh();
    auto* first = items_.at(selected_);
    first->setPosition(position);
    first->setScale(sx);
    first->setOpacity(opacity);
    returnItem();
    splitDragging_ = true;
    auto* second   = items_.back();
    second->setLocalZOrder(2);
    second->setPosition(bg_->convertToNodeSpace(point));
    second->setScale(0);
    second->setOpacity(150);
    second->runAction(Sequence::create(EaseIn::create(ScaleTo::create(.3f, .44f), .25f),
                                       EaseIn::create(ScaleTo::create(.3f, .4f), .25f), nullptr));
    if (changed)
        changed();
    if (sound)
        sound("snd_f42");
}
void InventoryView::finishSeparate()
{
    splitDragging_ = false;
    operation_     = true;
    unschedule("inventory-return");
    auto* item = items_.back();
    item->stopAllActions();
    item->runAction(
        Sequence::create(Spawn::create(EaseOut::create(MoveTo::create(.3f, gridPosition(items_.size() - 1)), .25f),
                                       FadeIn::create(.3f), nullptr),
                         EaseOut::create(ScaleTo::create(.15f, .44f), .25f),
                         EaseOut::create(ScaleTo::create(.15f, .4f), .25f), nullptr));
    scheduleOnce([this, item](float) {
        item->setLocalZOrder(0);
        operation_ = false;
    }, .6f, "inventory-separate-end");
}
