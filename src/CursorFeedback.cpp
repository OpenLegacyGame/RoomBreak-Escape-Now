#include "CursorFeedback.h"
#include "NodeFactory.h"
#include "Assets.h"
#include <stdexcept>
using namespace ax;
CursorFeedback* CursorFeedback::create(const rb::GameData& data)
{
    auto* value         = rb::make<CursorFeedback>();
    value->data_        = &data;
    const char* files[] = {"par_cur_hands_def.plist", "par_cur_hands.plist", "par_cur_search_def.plist",
                           "par_cur_foots_def.plist", "par_cur_foots.plist", "par_cur_item_def.plist",
                           "par_useitem.plist"};
    for (int i = 0; i < 7; ++i)
    {
        auto* p = ParticleSystemQuad::create(files[i]);
        if (!p)
            throw std::runtime_error(std::string("Missing particle: ") + files[i]);
        value->particles_[i] = p;
        value->addChild(p);
        p->stopSystem();
        p->setVisible(false);
    }
    return value;
}
void CursorFeedback::emit(int index, Vec2 point)
{
    auto* p = particles_.at(index);
    p->setPosition(point);
    p->setVisible(true);
    p->resetSystem();
}
void CursorFeedback::play(Vec2 point, int cursor, int outcome, int item)
{
    if (cursor == 1)
        outcome = 1;
    if (outcome == 6)
        return;
    switch (outcome)
    {
    case 0:
        emit(1, point);
        break;
    case 1:
        emit(2, point);
        break;
    case 2:
        emit(4, point);
        break;
    case 3:
        if (item)
            itemAction(item, point, true);
        break;
    default:
        if (cursor == 0)
            emit(0, point);
        else if (cursor == 2)
            emit(3, point);
        else if (cursor == 3)
        {
            if (item)
                itemAction(item, point, false);
            emit(5, point);
        }
    }
}
void CursorFeedback::itemAction(int item, Vec2 point, bool success)
{
    if (itemSprite_)
        itemSprite_->removeFromParent();
    itemSprite_ = rb::sprite(rb::string(data_->byId("ItemData", "itemID", item), "imageName"));
    addChild(itemSprite_);
    itemSprite_->setPosition(point);
    itemSprite_->setScale(.2f);
    itemSprite_->setOpacity(success ? 50 : 180);
    if (!success)
    {
        itemSprite_->runAction(Spawn::create(EaseIn::create(ScaleTo::create(.5f, .3f), .25f),
                                             EaseIn::create(FadeOut::create(.5f), .25f), nullptr));
        if (sound)
            sound("snd_f13");
        return;
    }
    itemSprite_->runAction(Sequence::create(Spawn::create(EaseOut::create(ScaleTo::create(.2f, .35f), .25f),
                                                          EaseOut::create(FadeTo::create(.2f, 200), .25f), nullptr),
                                            Spawn::create(EaseOut::create(ScaleTo::create(.1f, .3f), .25f),
                                                          EaseOut::create(FadeTo::create(.1f, 150), .25f), nullptr),
                                            CallFunc::create([this, point] {
        emit(6, point);
        if (sound)
            sound("snd_f18");
    }),
                                            EaseOut::create(FadeOut::create(.3f), .25f), nullptr));
}
