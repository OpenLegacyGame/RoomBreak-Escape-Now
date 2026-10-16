#include "RoomView.h"
#include "Assets.h"
#include <stdexcept>
using namespace ax;
namespace
{
ActionInterval* pulse(uint8_t high, uint8_t low)
{
    return RepeatForever::create(Sequence::create(FadeTo::create(.5f, high), FadeTo::create(.5f, low), nullptr));
}
}  // namespace
void RoomView::addDirectionAction(std::string name, Sprite* sprite, ActionInterval* action, int z)
{
    directionActions_.insert_or_assign(std::move(name), RoomAction{state_->room, z, sprite, action});
    background_->addChild(sprite, z);
    sprite->runAction(action->clone());
}
void RoomView::addDirectionAnimation(std::string name, SheetSprite* sprite, int frames, float interval, int z)
{
    directionAnimations_.insert_or_assign(std::move(name), RoomAnimation{state_->room, frames, z, interval, sprite});
    background_->addChild(sprite, z);
    sprite->animate(frames, interval);
}
void RoomView::fadeDirectionAction(const std::string& name, float seconds)
{
    if (auto it = directionActions_.find(name); it != directionActions_.end())
        it->second.sprite->runAction(FadeOut::create(seconds));
}
void RoomView::fadeDirectionAnimation(const std::string& name, float seconds)
{
    if (auto it = directionAnimations_.find(name); it != directionAnimations_.end())
        it->second.sprite->runAction(FadeOut::create(seconds));
}
void RoomView::removeDirectionAction(const std::string& name, bool stopActions)
{
    if (auto it = directionActions_.find(name); it != directionActions_.end())
    {
        if (stopActions)
            it->second.sprite->stopAllActions();
        it->second.sprite->removeFromParent();
        directionActions_.erase(it);
    }
}
void RoomView::removeDirectionAnimation(const std::string& name)
{
    if (auto it = directionAnimations_.find(name); it != directionAnimations_.end())
    {
        it->second.sprite->stopAnimation();
        it->second.sprite->removeFromParent();
        directionAnimations_.erase(it);
    }
}
void RoomView::removeDirectionParticle(const std::string& name)
{
    if (auto it = directionParticles_.find(name); it != directionParticles_.end())
    {
        it->second->stopSystem();
        it->second->removeFromParent();
        directionParticles_.erase(it);
    }
}
Vec2 RoomView::roomPoint(Node* node, float x, float y) const
{
    return rb::topLeft(background_, node, x, y);
}
LayerColor* RoomView::whiteFlash()
{
    auto* white = LayerColor::create(Color4B(255, 255, 255, 0), 480, 320);
    getParent()->addChild(white, 6);
    return white;
}
void RoomView::hideObject(int object)
{
    if (auto* old = stateImage(state_->objects.at(object).state))
        old->setOpacity(0);
}
void RoomView::revealState(int object, int state)
{
    state_->objects.at(object).state = state;
    if (auto* target = stateImage(state))
        target->setOpacity(255);
}
void RoomView::playStateChange(int object,
                               int state,
                               SheetSprite* sheet,
                               int frames,
                               float interval,
                               std::function<void()> complete,
                               bool reverse)
{
    sheet->playOnce(frames, interval, [this, sheet, object, state, complete = std::move(complete)] {
        sheet->removeFromParent();
        revealState(object, state);
        complete();
    }, reverse);
}
bool RoomView::direction(int id, std::function<void()> complete)
{
    if (id == 49)
    {
        hideObject(607);
        auto* floor = SheetSprite::create("e5_r2_dir1.png", 3, 1);
        floor->setPosition(roomPoint(floor, 347, 259));
        background_->addChild(floor, 1);
        playStateChange(607, 1254, floor, 3, .2f, std::move(complete));
        return true;
    }
    if (id >= 30 && id <= 41)
    {
        const int type           = (id - 30) / 2;
        const std::string tags[] = {"__DIR_EP4_GATE_PANDANT__",  "__DIR_EP4_GATE_EARTH__",
                                    "__DIR_EP4_GATE_ALIEN__",    "__DIR_EP4_GATE_CRYSTAL__",
                                    "__DIR_EP4_GATE_ALPHABET__", "__DIR_EP4_GATE_SNAKE_HEAD__"};
        const auto name          = tags[type];
        if (id % 2 == 0)
        {
            auto* s = rb::sprite("e4_r6_obj" + std::to_string(12 + type) + ".png");
            s->setPosition(roomPoint(s, 313, 124));
            s->setOpacity(0);
            addDirectionAction(name, s, pulse(200, 150), 1);
            after(2, std::move(complete));
        }
        else
        {
            fadeDirectionAction(name, 1);
            after(1, [this, name, complete = std::move(complete)] {
                removeDirectionAction(name);
                complete();
            });
        }
        return true;
    }
    if (id == 27)
    {
        auto* s = SheetSprite::create("e4_r6_dir4.png", 10, 1);
        s->setPosition(roomPoint(s, 526, 19));
        addDirectionAnimation("__DIR_EP4_TORCH_LIGHT__", s, 10, .1f, 0);
        after(1, std::move(complete));
        return true;
    }
    if (id == 48)
    {
        auto* white = LayerColor::create(Color4B(255, 255, 255, 0), 480, 5);
        white->setPosition(0, 248);
        background_->addChild(white, 2);
        auto acceleration = std::make_shared<float>(0);
        auto grow         = [white, acceleration, complete = std::move(complete)] {
            white->schedule([white, acceleration, complete](float) {
                if (white->getScaleY() >= 200)
                {
                    white->unschedule("grow-light");
                    white->runAction(Sequence::create(FadeOut::create(1), CallFunc::create([white, complete] {
                        white->removeFromParent();
                        complete();
                    }),
                                                      nullptr));
                }
                else
                {
                    *acceleration += 1.5f;
                    white->setScaleY(white->getScaleY() + *acceleration);
                }
            }, "grow-light");
        };
        white->runAction(Sequence::create(FadeIn::create(1), CallFunc::create(std::move(grow)), nullptr));
        return true;
    }
    if (id == 54)
    {
        hideObject(685);
        auto* door = SheetSprite::create("e4_r5_dir1.png", 6, 2);
        door->setPosition(roomPoint(door, 615, 78));
        background_->addChild(door, 1);
        const auto p = background_->getPosition();
        background_->runAction(Repeat::create(
            Sequence::create(MoveTo::create(.05f, p - Vec2(10, 0)), MoveTo::create(.05f, p), nullptr), 20));
        playStateChange(685, 1509, door, 11, .1f, std::move(complete));
        return true;
    }
    if (id == 20 || id == 53)
    {
        for (int i = 0; i < 2; ++i)
        {
            auto* s = rb::sprite(i ? "e4_r4_obj49.png" : "e4_r4_obj48.png");
            s->setPosition(roomPoint(s, i ? 460 : 172, 196));
            addDirectionAction(i ? "__DIR_EP4_GEAR_RIGHT_BG__" : "__DIR_EP4_GEAR_LEFT_BG__", s, DelayTime::create(0),
                               1);
        }
        const Vec2 positions[]  = {{212, 199}, {212, 217}, {252, 210}, {255, 200}, {229, 205}, {175, 196},
                                   {464, 213}, {514, 202}, {517, 216}, {535, 202}, {479, 203}};
        const float durations[] = {.6f, .5f, .56f, .8f, 1.2f, .85f, .35f, 1.2f, 1.5f, .9f, .75f};
        for (int i = 0; i < 11; ++i)
        {
            auto* s = rb::sprite("e4_r4_obj" + std::to_string(16 + i) + ".png");
            s->setPosition(roomPoint(s, positions[i].x, positions[i].y));
            const float t = durations[i];
            addDirectionAction("__DIR_EP4_GEAR" + std::to_string(i) + "_ON__", s,
                               RepeatForever::create(Sequence::create(RotateTo::create(t, i % 2 ? 240 : 120),
                                                                      RotateTo::create(t, i % 2 ? 120 : 240),
                                                                      RotateTo::create(t, i % 2 ? 0 : 360), nullptr)),
                               1);
        }
        after(1, std::move(complete));
        return true;
    }
    if (id == 21)
    {
        hideObject(424);
        auto* door = SheetSprite::create("e4_r4_dir1.png", 7, 2);
        door->setPosition(roomPoint(door, 327, 114));
        background_->addChild(door);
        auto* crystal = rb::place(background_, "e4_r4_obj3.png", 370, 137);
        crystal->setOpacity(255);
        crystal->runAction(Spawn::create(FadeOut::create(.5f), EaseIn::create(ScaleTo::create(.5f, 3), .25f), nullptr));
        auto* white  = whiteFlash();
        auto opening = [this, door, crystal, white, complete = std::move(complete)] {
            white->removeFromParent();
            door->playOnce(13, .1f, [this, door, crystal, complete] {
                door->removeFromParent();
                crystal->removeFromParent();
                revealState(424, 645);
                complete();
            });
        };
        white->runAction(Sequence::create(DelayTime::create(.5f), FadeIn::create(1), FadeOut::create(1),
                                          CallFunc::create(std::move(opening)), nullptr));
        return true;
    }
    if (id == 9)
    {
        auto* prism = SheetSprite::create("e4_r3_dir2.png", 3, 1);
        prism->setPosition(roomPoint(prism, 133, 145));
        background_->addChild(prism);
        auto* light = rb::place(background_, "e4_r3_dir3.png", 0, 0);
        light->setOpacity(0);
        const Vec2 positions[] = {{0, -15},   {208, 106}, {79, 219},  {197, 149}, {238, 201},
                                  {393, 100}, {393, 172}, {516, 171}, {545, 214}};
        const Vec2 offsets[]   = {{10, 20},  {-15, 20}, {20, -20}, {15, -20}, {5, -5},
                                  {10, -15}, {-5, 10},  {10, -5},  {25, 20}};
        std::vector<Sprite*> planets;
        for (int i = 0; i < 9; ++i)
        {
            auto* s =
                rb::place(background_, "e4_r3_obj" + std::to_string(54 + i) + ".png", positions[i].x, positions[i].y);
            s->setOpacity(0);
            planets.push_back(s);
        }
        auto* white = whiteFlash();
        auto gather = [this, prism, light, white, planets, offsets, complete = std::move(complete)] {
            for (int i = 0; i < 9; ++i)
            {
                auto* s           = planets[i];
                const Vec2 target = roomPoint(s, 502, 135);
                s->runAction(Sequence::create(FadeIn::create(1),
                                              EaseIn::create(MoveTo::create(1, s->getPosition() + offsets[i]), .25f),
                                              Spawn::create(EaseOut::create(MoveTo::create(.5f, target), .25f),
                                                            EaseOut::create(ScaleTo::create(.5f, 0), .25f), nullptr),
                                              nullptr));
            }
            white->runAction(Sequence::create(DelayTime::create(2.5f), FadeOut::create(1),
                                              CallFunc::create([prism, light, white, planets, complete] {
                for (auto* s : planets)
                    s->removeFromParent();
                prism->removeFromParent();
                light->removeFromParent();
                white->removeFromParent();
                complete();
            }),
                                              nullptr));
        };
        prism->playOnce(3, .1f, [this, prism, light, white, gather = std::move(gather)] {
            prism->setVisible(false);
            light->runAction(Sequence::create(FadeIn::create(.1f), CallFunc::create([this, light, white, gather] {
                light->setOpacity(0);
                white->runAction(FadeTo::create(.5f, 150));
                after(.5f, gather);
            }),
                                              nullptr));
        });
        return true;
    }
    if (id == 10)
    {
        auto* circle = rb::place(background_, "e4_r3_obj25.png", 401, 103);
        circle->setOpacity(255);
        auto* white = whiteFlash();
        circle->runAction(Spawn::create(FadeOut::create(.5f), EaseIn::create(ScaleTo::create(.5f, 2), .25f), nullptr));
        white->runAction(Sequence::create(DelayTime::create(.5f), FadeIn::create(1), FadeOut::create(1),
                                          CallFunc::create([circle, white, complete = std::move(complete)] {
            circle->removeFromParent();
            white->removeFromParent();
            complete();
        }),
                                          nullptr));
        return true;
    }
    if (id == 80)
    {
        hideObject(316);
        auto* door = SheetSprite::create("e4_r3_dir1.png", 9, 1);
        door->setPosition(roomPoint(door, 83, 88));
        background_->addChild(door, 1);
        playStateChange(316, 468, door, 9, .1f, std::move(complete));
        return true;
    }
    if (id == 6)
    {
        auto* sprite = SheetSprite::create("e4_r2_dir6.png", 6, 1);
        sprite->setPosition(roomPoint(sprite, 6, 36));
        sprite->setOpacity(0);
        addDirectionAnimation("__DIR_EP4_HOLOGRAM__", sprite, 6, .2f, 1);
        sprite->runAction(FadeIn::create(1));
        after(.5f, std::move(complete));
        return true;
    }
    if (id == 23)
    {
        fadeDirectionAnimation("__DIR_EP4_HOLOGRAM__", 1);
        after(1, [this, complete = std::move(complete)] {
            removeDirectionAnimation("__DIR_EP4_HOLOGRAM__");
            complete();
        });
        return true;
    }
    if (id == 26)
    {
        auto* sprite = rb::sprite("e4_r2_dir4.png");
        sprite->setPosition(roomPoint(sprite, 349, 91));
        addDirectionAction("__DIR_EP4_LEFT_EYES__", sprite, pulse(200, 100), 1);
        after(1, std::move(complete));
        return true;
    }
    if (id == 24)
    {
        auto left = directionActions_.at("__DIR_EP4_LEFT_EYES__").sprite;
        left->stopAllActions();
        left->runAction(FadeOut::create(.5f));
        after(.5f, [this, complete = std::move(complete)] {
            auto* eyes = rb::sprite("e4_r2_dir3.png");
            eyes->setPosition(roomPoint(eyes, 336, 76));
            eyes->setOpacity(0);
            addDirectionAction("__DIR_EP4_EYES_LIGHT__", eyes, pulse(200, 100), 1);
            after(1, [this, complete] {
                auto* number = SheetSprite::create("e4_r2_dir5.png", 6, 1);
                number->setPosition(roomPoint(number, 374, 246));
                addDirectionAction("__DIR_EP4_EYES_NUMBER__", number, DelayTime::create(0), 1);
                number->playOnce(6, .1f, [this, number, complete] {
                    number->frame(5);
                    removeDirectionAction("__DIR_EP4_LEFT_EYES__", false);
                    complete();
                });
            });
        });
        return true;
    }
    if (id == 25)
    {
        static const char* const names[] = {"__DIR_EP4_EYES_LIGHT__", "__DIR_EP4_EYES_NUMBER__"};
        const bool both                  = directionActions_.contains(names[0]) && directionActions_.contains(names[1]);
        if (both)
            for (const auto* name : names)
                directionActions_.at(name).sprite->runAction(FadeOut::create(.5f));
        after(both ? .5f : .3f, [this, complete = std::move(complete)] {
            for (const auto* name : names)
                removeDirectionAction(name);
            complete();
        });
        return true;
    }
    if (id == 8 || id == 73)
    {
        if (id == 73)
            hideObject(265);
        auto* door = SheetSprite::create("e4_r2_dir7.png", 9, 1);
        door->setPosition(roomPoint(door, 690, 59));
        background_->addChild(door, id == 73 ? 1 : 0);
        auto run = [this, id, door, complete = std::move(complete)] {
            door->playOnce(9, .1f, [this, id, door, complete] {
                door->removeFromParent();
                if (id == 73)
                    revealState(265, 2463);
                complete();
            });
        };
        if (id == 8)
        {
            auto* white = whiteFlash();
            white->runAction(Sequence::create(FadeIn::create(1), FadeOut::create(1), CallFunc::create([white, run] {
                white->removeFromParent();
                run();
            }),
                                              nullptr));
        }
        else
            run();
        return true;
    }
    if (id == 7)
    {
        hideObject(257);
        auto* sphinx = rb::place(background_, "e4_r2_dir1.png", 248, 36);
        auto* rubble = rb::place(background_, "e4_r2_obj35.png", 232, 180);
        rubble->setOpacity(0);
        auto* main = rb::place(background_, "e4_r2_dir2.png", 245, 51);
        main->setOpacity(0);
        main->runAction(pulse(250, 200));
        std::vector<Sprite*> pieces;
        const Vec2 positions[] = {{165, 179}, {416, 0}, {241, 0}, {465, 20}, {0, 46}};
        const float delays[]   = {0, .5f, .8f, .9f, 1};
        for (int i = 0; i < 5; ++i)
        {
            auto* s =
                rb::place(background_, "e4_r2_dir2_" + std::to_string(i + 1) + ".png", positions[i].x, positions[i].y);
            s->setOpacity(0);
            pieces.push_back(s);
            s->runAction(Sequence::create(DelayTime::create(2 + delays[i]), FadeIn::create(.1f), nullptr));
        }
        auto collapse = [sphinx, rubble, main, pieces] {
            main->stopAllActions();
            main->setOpacity(0);
            sphinx->setOpacity(0);
            for (auto* s : pieces)
                s->setOpacity(0);
            rubble->setOpacity(255);
        };
        auto* white  = whiteFlash();
        auto cleanup = [this, sphinx, rubble, main, pieces, white, complete = std::move(complete)] {
            for (auto* s : pieces)
                s->removeFromParent();
            sphinx->removeFromParent();
            rubble->removeFromParent();
            main->removeFromParent();
            white->removeFromParent();
            revealState(257, 1353);
            complete();
        };
        white->runAction(Sequence::create(DelayTime::create(3.1f), FadeIn::create(1),
                                          CallFunc::create(std::move(collapse)), FadeOut::create(1),
                                          CallFunc::create(std::move(cleanup)), nullptr));
        return true;
    }
    if (id == 12 || id == 15 || id == 16)
    {
        const auto& row  = data_->byId("DirectionTable", "directionID", id);
        const int object = rb::integer(row, "objectID"), next = rb::integer(row, "stateID");
        hideObject(object);
        auto done = [this, object, next, complete = std::move(complete)] {
            revealState(object, next);
            complete();
        };
        struct Sheet
        {
            const char* file;
            int columns, rows, frames;
            Vec2 topLeft;
        };
        const Sheet sheet = id == 12   ? Sheet{"e4_r1_dir1.png", 5, 2, 10, {615, 70}}
                            : id == 15 ? Sheet{"e4_r1_dir2.png", 3, 3, 9, {266, 68}}
                                       : Sheet{"e4_r1_dir5.png", 7, 1, 7, {324, 80}};
        const int frames  = sheet.frames;
        auto* sprite      = SheetSprite::create(sheet.file, sheet.columns, sheet.rows);
        sprite->setPosition(roomPoint(sprite, sheet.topLeft.x, sheet.topLeft.y));
        background_->addChild(sprite, id == 16 ? 0 : 1);
        if (id == 16)
        {
            sprite->playOnce(frames, .1f, [this, sprite, done = std::move(done)] {
                auto* mark = rb::sprite("e4_r1_dir6.png");
                mark->setPosition(roomPoint(mark, 396, 296));
                mark->setOpacity(0);
                addDirectionAction("__DIR_EP4_MARK__", mark, DelayTime::create(0), 1);
                mark->runAction(Sequence::create(FadeIn::create(1), CallFunc::create([this, sprite, done] {
                    auto* floor = SheetSprite::create("e4_r1_dir4.png", 3, 2);
                    floor->setPosition(roomPoint(floor, 382, 266));
                    addDirectionAnimation("__DIR_EP4_FLOOR__", floor, 5, .1f, 1);
                    after(1, [sprite, done] {
                        sprite->removeFromParent();
                        done();
                    });
                }),
                                                 nullptr));
            });
        }
        else if (id == 15)
        {
            sprite->setOpacity(0);
            sprite->runAction(Sequence::create(FadeIn::create(1), CallFunc::create([sprite, frames, done] {
                sprite->playOnce(frames, .1f, [sprite, done] {
                    sprite->removeFromParent();
                    done();
                });
            }),
                                               nullptr));
        }
        else
            sprite->playOnce(frames, .1f, [sprite, done] {
                sprite->removeFromParent();
                done();
            });
        return true;
    }
    if (id == 17)
    {
        fadeDirectionAnimation("__DIR_EP4_FLOOR__", 1);
        fadeDirectionAction("__DIR_EP4_MARK__", 1);
        after(1, [this, complete = std::move(complete)] {
            removeDirectionAnimation("__DIR_EP4_FLOOR__");
            removeDirectionAction("__DIR_EP4_MARK__", false);
            complete();
        });
        return true;
    }
    if (id == 18 || id == 22)
    {
        const Vec2 target(id == 18 ? 0 : 480 - background_->getContentSize().width, background_->getPositionY());
        background_->runAction(Sequence::create(EaseIn::create(MoveTo::create(.5f, target), .25f),
                                                CallFunc::create(std::move(complete)), nullptr));
        return true;
    }
    if (id == 19)
    {
        auto* sprite = rb::place(background_, "e4_r1_obj56.png", 266, 68);
        sprite->setOpacity(0);
        sprite->runAction(Sequence::create(FadeIn::create(1), FadeOut::create(1),
                                           CallFunc::create([sprite, complete = std::move(complete)] {
            sprite->removeFromParent();
            complete();
        }),
                                           nullptr));
        return true;
    }
    if (id == 71)
    {
        auto* sprite = rb::sprite("e4_r1_obj44.png");
        const Vec2 p = roomPoint(sprite, 459, 278);
        sprite->setPosition(p);
        sprite->setOpacity(0);
        addDirectionAction("__DIR_EP4_PENDANT__", sprite,
                           RepeatForever::create(
                               Sequence::create(MoveTo::create(.5f, p + Vec2(0, 5)), MoveTo::create(.5f, p), nullptr)),
                           2);
        sprite->runAction(FadeIn::create(.5f));
        after(.5f, std::move(complete));
        return true;
    }
    if (id == 72)
    {
        fadeDirectionAction("__DIR_EP4_PENDANT__", .5f);
        after(.5f, [this, complete = std::move(complete)] {
            removeDirectionAction("__DIR_EP4_PENDANT__");
            complete();
        });
        return true;
    }
    if (id == 57 || id == 62)
    {
        if (id == 57)
            hideObject(829);
        auto* fan = rb::place(background_, "e3_r2_obj14.png", 686, 139.5f);
        rb::place(background_, "e3_r2_obj15.png", 683, 136);
        rb::rotateGear(fan, .2f, true);
        after(.3f, [this, id, complete = std::move(complete)] {
            if (id == 57)
                state_->objects.at(829).state = 1996;
            complete();
        });
        return true;
    }
    if (id == 11)
    {
        auto* sprite = SheetSprite::create("e2_r6_dir1.png", 4, 2);
        sprite->setPosition(roomPoint(sprite, 199, 140));
        background_->addChild(sprite, 1);
        sprite->playOnce(8, .1f, [sprite, complete = std::move(complete)] {
            sprite->removeFromParent();
            complete();
        });
        return true;
    }
    if (id == 5)
    {
        auto* sprite = SheetSprite::create("e2_r3_dir1.png", 5, 3);
        sprite->setPosition(roomPoint(sprite, 578, 25));
        background_->addChild(sprite);
        sprite->playOnce(15, .1f, [] {});
        auto* white = whiteFlash();
        white->runAction(Sequence::create(DelayTime::create(.5f), FadeIn::create(1), DelayTime::create(.5f),
                                          FadeOut::create(1),
                                          CallFunc::create([sprite, white, complete = std::move(complete)] {
            sprite->removeFromParent();
            white->removeFromParent();
            complete();
        }),
                                          nullptr));
        return true;
    }
    if (id == 3 || id == 4)
    {
        const auto& direction = data_->byId("DirectionTable", "directionID", id);
        const int object = rb::integer(direction, "objectID"), next = rb::integer(direction, "stateID");
        hideObject(object);
        auto* sprite = SheetSprite::create("e2_r2_dir1.png", 3, 2);
        sprite->setPosition(roomPoint(sprite, 399, 40));
        background_->addChild(sprite, 1);
        playStateChange(object, next, sprite, 6, .1f, std::move(complete), id == 4);
        return true;
    }
    if (id == 1)
    {
        auto* sprite = rb::sprite("e2_r2_dir2.png");
        sprite->setPosition(roomPoint(sprite, 206, 49));
        sprite->setOpacity(80);
        auto* action = RepeatForever::create(Sequence::create(FadeTo::create(.5f, 200), FadeTo::create(.3f, 80),
                                                              FadeTo::create(.5f, 150), FadeTo::create(.3f, 80),
                                                              FadeTo::create(1, 150), nullptr));
        addDirectionAction("__DIR_EP2_BEAM__", sprite, action, 2);
        after(1, std::move(complete));
        return true;
    }
    if (id == 2)
    {
        fadeDirectionAction("__DIR_EP2_BEAM__", .5f);
        after(.5f, [this, complete = std::move(complete)] {
            removeDirectionAction("__DIR_EP2_BEAM__");
            complete();
        });
        return true;
    }
    if (id == 13)
    {
        auto* white = whiteFlash();
        white->runAction(Sequence::create(FadeIn::create(1), DelayTime::create(.5f), FadeOut::create(1),
                                          CallFunc::create([this, white, complete = std::move(complete)] {
            struct Number
            {
                int image, digit;
                float x, y, dx, dy, seconds;
            };
            // E1S1BlindDirection: four independent, persistent floating clues.
            static const Number numbers[] = {{20, 0, 45, 117, 3, 5, 1},
                                             {21, 2, 93, 133, -5, 2, 1.5f},
                                             {22, 1, 134, 153, 3, -2, 1},
                                             {23, 3, 172, 162, 5, 2, 2}};
            for (const auto& number : numbers)
            {
                auto* sprite = rb::sprite("e1_r1_obj" + std::to_string(number.image) + ".png");
                const auto p = roomPoint(sprite, number.x, number.y);
                sprite->setPosition(p);
                auto* action = RepeatForever::create(
                    Sequence::create(MoveTo::create(number.seconds, p + Vec2(number.dx, number.dy)),
                                     MoveTo::create(number.seconds, p - Vec2(number.dx, number.dy)), nullptr));
                addDirectionAction("__DIR_EP1_BLIND_" + std::to_string(number.digit) + "__", sprite, action, 1);
            }
            white->removeFromParent();
            after(1, complete);
        }),
                                          nullptr));
        return true;
    }
    if (id == 14 || id == 50 || id == 51 || id == 52)
    {
        auto* white      = whiteFlash();
        auto revealClues = [this, id, white, complete = std::move(complete)] {
            struct Clue
            {
                int direction;
                const char* file;
                const char* name;
                float x, y, dx, dy;
                int opacity;
            };
            static const Clue clues[] = {{14, "e1_r2_obj2.png", "__DIR_EP1_BLIND_BLOOD__", 370, 90, 5, 5, 255},
                                         {14, "e1_r2_obj3.png", "__DIR_EP1_BLIND_ERASE__", 394, 135, -5, -2, 255},
                                         {50, "e1_r3_obj1.png", "__DIR_EP1_BLIND_GOAWAY__", 662, 133, 2, 5, 150},
                                         {50, "e1_r3_obj2.png", "__DIR_EP1_BLIND_ROOF__", 680, 230, -2, -5, 150},
                                         {51, "e1_r3_obj3.png", "__DIR_EP1_BLIND_YOU__", 146, 160, -5, 2, 150},
                                         {51, "e1_r3_obj4.png", "__DIR_EP1_BLIND_ARREST__", 193, 165, 5, -2, 150},
                                         {52, "e1_r5_obj25.png", "__DIR_EP1_BLIND_LIE__", 342, 127, -2, 5, 255}};
            for (const auto& c : clues)
            {
                if (c.direction != id)
                    continue;
                auto* sprite = rb::sprite(c.file);
                const Vec2 p = roomPoint(sprite, c.x, c.y);
                sprite->setPosition(p);
                sprite->setOpacity(c.opacity);
                auto* action = RepeatForever::create(Sequence::create(
                    Spawn::create(ScaleTo::create(2, 1.2f), MoveTo::create(2, p + Vec2(c.dx, c.dy)), nullptr),
                    Spawn::create(MoveTo::create(2, p - Vec2(c.dx, c.dy)), ScaleTo::create(2, 1), nullptr), nullptr));
                addDirectionAction(c.name, sprite, action, 1);
            }
            white->removeFromParent();
            after(1, complete);
        };
        white->runAction(Sequence::create(FadeIn::create(1), DelayTime::create(.5f), FadeOut::create(1),
                                          CallFunc::create(std::move(revealClues)), nullptr));
        return true;
    }
    struct Loop
    {
        int id;
        const char* file;
        const char* name;
        int frames;
        float interval, x, y;
    };
    static const Loop loops[] = {{55, "e3_r1_dir1.png", "__DIR_EP3_TV__", 3, .1f, 368, 185.5f},
                                 {60, "e3_r1_dir1.png", "__DIR_EP3_TV__", 3, .1f, 368, 185.5f},
                                 {64, "e3_r1_dir1.png", "__DIR_EP3_TV__", 3, .1f, 368, 185.5f},
                                 {68, "e3_r4_dir1.png", "__DIR_EP3_TV__", 3, .1f, 368, 185.5f},
                                 {59, "e3_r2_dir2.png", "__DIR_EP3_RAIN__", 5, .1f, 40.5f, 52},
                                 {63, "e3_r2_dir2.png", "__DIR_EP3_RAIN__", 5, .1f, 40.5f, 52},
                                 {67, "e3_r4_dir2.png", "__DIR_EP3_RAIN__", 5, .1f, 40.5f, 52},
                                 {70, "e3_r4_dir2.png", "__DIR_EP3_RAIN__", 5, .1f, 40.5f, 52},
                                 {65, "e3_r3_dir4.png", "__DIR_EP3_MUSIC_BOX__", 6, .2f, 513, 255.5f},
                                 {74, "e1_r3_dir4.png", "__DIR_EP3_WATER__", 7, .1f, 271, 152},
                                 {75, "e1_r3_dir3.png", "__DIR_EP1_FIRE__", 10, .1f, 394, 199}};
    for (const auto& loop : loops)
    {
        if (loop.id != id)
            continue;
        auto* sprite = SheetSprite::create(loop.file, loop.frames, 1);
        sprite->setPosition(roomPoint(sprite, loop.x, loop.y));
        addDirectionAnimation(loop.name, sprite, loop.frames, loop.interval, 1);
        after(.3f, std::move(complete));
        return true;
    }
    if (id == 76 || id == 56 || id == 61 || id == 66 || id == 69)
    {
        const std::string name = id == 76 ? "__DIR_EP1_FIRE__" : "__DIR_EP3_TV__";
        fadeDirectionAnimation(name, .5f);
        after(.5f, [this, name, complete = std::move(complete)] {
            removeDirectionAnimation(name);
            complete();
        });
        return true;
    }
    struct Particle
    {
        int id;
        const char* file;
        const char* name;
        float x, y;
    };
    static const Particle particles[] = {{28, "case14.plist", "__DIR_EP4_GATE_ON__", 380, 195},
                                         {42, "case13.plist", "__DIR_EP4_GATE_POWER_1__", 448, 128},
                                         {43, "case13.plist", "__DIR_EP4_GATE_POWER_2__", 375, 99},
                                         {44, "case13.plist", "__DIR_EP4_GATE_POWER_3__", 303, 128},
                                         {45, "case13.plist", "__DIR_EP4_GATE_POWER_4__", 303, 250},
                                         {46, "case13.plist", "__DIR_EP4_GATE_POWER_5__", 448, 250},
                                         {77, "case11.plist", "__DIR_EP3_SHINEE__", 60, 100}};
    for (const auto& p : particles)
    {
        if (p.id != id)
            continue;
        auto* node = ParticleSystemQuad::create(p.file);
        if (!node)
            throw std::runtime_error("Missing direction particle");
        directionParticles_[p.name] = node;
        background_->addChild(node, 1);
        node->setPosition(roomPoint(node, p.x, p.y));
        after(.5f, std::move(complete));
        return true;
    }
    if (id == 29)
        removeDirectionParticle("__DIR_EP4_GATE_ON__");
    else if (id == 78)
        removeDirectionParticle("__DIR_EP3_SHINEE__");
    else if (id == 47)
        for (int i = 1; i <= 5; ++i)
            removeDirectionParticle("__DIR_EP4_GATE_POWER_" + std::to_string(i) + "__");
    else
        return false;
    after(.5f, std::move(complete));
    return true;
}
