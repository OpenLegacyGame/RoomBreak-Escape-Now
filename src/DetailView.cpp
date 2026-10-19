#include "Viewport.h"
#include "DetailTelevision.h"
#include "DetailElevator.h"
#include "DetailCircuit.h"
#include "DetailWomanStatue.h"
#include "DetailCenterSafe.h"
#include "DetailDropBook.h"
#include "DetailFurnitureLeft.h"
#include "DetailPyramidBlock.h"
#include "DetailHistoricSafe.h"
#include "DetailTimeControl.h"
#include "DetailHistoricBook.h"
#include "DetailAlienPad.h"
#include "DetailWatchBox.h"
#include "DetailSecretPattern.h"
#include "DetailWineBox.h"
#include "DetailDustingFrame.h"
#include "DetailFingerScan.h"
#include "DetailGearHandle.h"
#include "DetailSwitch.h"
#include "DetailMonitorControl.h"
#include "DetailView.h"
#include "DetailPad.h"
#include "DetailMusic.h"
#include "DetailElectronicDisplay.h"
#include "DetailScreenSafe.h"
#include "DetailSafe.h"
#include "DetailDoorLock.h"
#include "DetailPictureFrame.h"
#include "DetailArrowSafe.h"
#include "DetailElectronicDoorLock.h"
#include "DetailTutorial.h"
#include "DetailFittedPieces.h"
#include "Assets.h"
#include "ResultFeedback.h"
#include <stdexcept>
using namespace ax;
namespace
{
class StandardDetail final : public DetailView
{
public:
    static StandardDetail* make(const rb::GameData& data, int id, Vec2 origin, int language, int hands)
    {
        return createDetail<StandardDetail>(data, id, origin, language, hands);
    }

private:
    void setup(const rb::Row& row) override
    {
        std::string image = rb::string(row, "imageName");
        if (id_ == 80 || id_ == 81 || id_ == 82 || id_ == 86 || id_ == 90)
        {
            const char* languages[] = {"chi", "kor", "eng", "jap"};
            const int number        = id_ == 80 ? 3 : id_ == 81 ? 4 : id_ == 82 ? 5 : id_ == 86 ? 9 : 13;
            image                   = "e0_s1_det" + std::to_string(number) + "_1_" + languages[language_] + ".png";
        }
        makeBoard(image);
        success_ = true;
    }
};
}  // namespace
DetailView* DetailView::create(const rb::GameData& data, int id, Vec2 origin, int language, int hands)
{
    const auto& row = data.byId("DetailData", "detailID", id);
    if (rb::integer(row, "detailType") == 0)
        return StandardDetail::make(data, id, origin, language, hands);
    if (id == 80 || id == 81 || id == 82 || id == 86 || id == 90)
        return StandardDetail::make(data, id, origin, language, hands);
    if (id == 83 || id == 84 || id == 85 || id == 87 || id == 88 || id == 89)
        return DetailTutorial::make(data, id, origin, language, hands);
    if (id == 94)
        return DetailFittedPieces::make(data, id, origin, language, hands);
    switch (id)
    {
    case 96:
        return DetailTelevision::make(data, id, origin, language, hands);
    case 72:
        return DetailElevator::make(data, id, origin, language, hands);
    case 70:
        return DetailWomanStatue::make(data, id, origin, language, hands);
    case 48:
        return DetailCenterSafe::make(data, id, origin, language, hands);
    case 49:
        return DetailDropBook::make(data, id, origin, language, hands);
    case 42:
        return DetailFurnitureLeft::make(data, id, origin, language, hands);
    case 22:
    case 27:
    case 28:
    case 29:
    case 30:
    case 31:
    case 73:
        return DetailAlienPad::make(data, id, origin, language, hands);
    case 20:
        return DetailHistoricBook::make(data, id, origin, language, hands);
    case 18:
        return DetailTimeControl::make(data, id, origin, language, hands);
    case 17:
        return DetailHistoricSafe::make(data, id, origin, language, hands);
    case 13:
        return DetailPyramidBlock::make(data, id, origin, language, hands);
    case 67:
        return DetailDustingFrame::make(data, id, origin, language, hands);
    case 11:
        return DetailWineBox::make(data, id, origin, language, hands);
    case 9:
        return DetailSecretPattern::make(data, id, origin, language, hands);
    case 7:
        return DetailWatchBox::make(data, id, origin, language, hands);
    case 3:
        return DetailCircuit::make(data, id, origin, language, hands);
    case 1:
    case 2:
        return DetailFingerScan::make(data, id, origin, language, hands);
    case 61:
    case 75:
        return DetailGearHandle::make(data, id, origin, language, hands);
    case 62:
        return DetailSwitch::make(data, id, origin, language, hands);
    case 60:
        return DetailMonitorControl::make(data, id, origin, language, hands);
    case 56:
        return DetailElectronicDoorLock::make(data, id, origin, language, hands);
    case 51:
        return DetailArrowSafe::make(data, id, origin, language, hands);
    case 50:
        return DetailPictureFrame::make(data, id, origin, language, hands);
    case 25:
        return DetailDoorLock::make(data, id, origin, language, hands);
    case 15:
    case 24:
    case 43:
    case 57:
    case 58:
    case 59:
        return DetailSafe::make(data, id, origin, language, hands);
    case 4:
    case 40:
    case 65:
    case 68:
    case 76:
    case 77:
    case 93:
    case 95:
        return DetailScreenSafe::make(data, id, origin, language, hands);
    case 54:
        return DetailElectronicDisplay::make(data, id, origin, language, hands);
    case 6:
        return DetailMusic::make(data, id, origin, language, hands);
    case 5:
    case 12:
    case 38:
    case 47:
    case 71:
        return DetailPad::make(data, id, origin, language, hands);
    }
    throw std::runtime_error("Detail controller not implemented: " + std::to_string(id));
}
bool DetailView::initialize(const rb::GameData& data, int id, Vec2 origin, int language, int hands)
{
    if (!Node::init())
        return false;
    id_             = id;
    language_       = language;
    origin_         = origin;
    const auto& row = data.byId("DetailData", "detailID", id);
    hasResult_      = rb::integer(row, "isResult");
    shade_          = rb::screenShade(Color4B(0, 0, 0, 0));
    addChild(shade_);
    setup(row);
    if (!board_)
        throw std::runtime_error("Detail has no root sprite");
    board_->setPosition(origin_);
    board_->setScale(0);
    auto* controls = Node::create();
    addChild(controls, 100);
    rb::anchorToSafeArea(controls, 1, 1);
    const char* files[]   = {"ability_short_hands.png", "det_retry-1.png", "det_close-1.png"};
    const char* pressed[] = {"ability_short_hands_p.png", "det_retry_p-1.png", "det_close_p-1.png"};
    const float tops[]    = {118, 172, 226};
    for (int i = 0; i < 3; ++i)
    {
        menus_[i]         = rb::sprite(files[i]);
        pressedImages_[i] = rb::sprite(pressed[i]);
        controls->addChild(menus_[i]);
        controls->addChild(pressedImages_[i]);
        for (auto* s : {menus_[i], pressedImages_[i]})
            s->setPosition(480 + s->getContentSize().width / 2, 320 - tops[i] - s->getContentSize().height / 2);
        pressedImages_[i]->setVisible(false);
    }
    handsLabel_ = rb::label(std::to_string(hands), 2, 10);
    menus_[0]->addChild(handsLabel_);
    place(menus_[0], handsLabel_, {menus_[0]->getContentSize().width / 2 - 2, 33});
    auto* touch = EventListenerTouchOneByOne::create();
    touch->setSwallowTouches(true);
    touch->onTouchBegan = [this](Touch* t, Event*) {
        beginTouch(t->getLocation());
        return true;
    };
    touch->onTouchMoved     = [this](Touch* t, Event*) { moveTouch(t->getLocation()); };
    touch->onTouchEnded     = [this](Touch* t, Event*) { endTouch(t->getLocation()); };
    touch->onTouchCancelled = [this](Touch*, Event*) { cancelTouch(); };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(touch, this);
    return true;
}
bool DetailView::contains(Node* node, Vec2 p)
{
    return node->isVisible() && rb::hitTest(node, p);
}
void DetailView::place(Node* parent, Node* child, Vec2 p)
{
    child->setPosition(rb::topLeft(parent, child, p.x, p.y));
}
Sprite* DetailView::makeBoard(std::string_view file)
{
    board_ = rb::sprite(file);
    addChild(board_);
    track(board_);
    return board_;
}
Sprite* DetailView::addImage(std::string_view file, float x, float y, bool fade)
{
    auto* sprite = rb::place(board_, file, x, y);
    if (fade)
        track(sprite);
    return sprite;
}
void DetailView::setHands(int count)
{
    handsLabel_->setString(std::to_string(count));
}
void DetailView::open()
{
    if (opened_)
        return;
    opened_ = true;
    busy_   = true;
    if (gameUI)
        gameUI(false);
    playSound("snd_f15");
    shade_->runAction(FadeTo::create(1, 150));
    for (auto* node : fading_)
        if (node->getOpacity())
        {
            node->setOpacity(0);
            node->runAction(FadeIn::create(1));
        }
    board_->runAction(Sequence::create(Spawn::create(EaseIn::create(MoveTo::create(1.1f, {240, 160}), .25f),
                                                     EaseIn::create(ScaleTo::create(1.1f, 1), .25f), nullptr),
                                       CallFunc::create([this] { busy_ = false; }), nullptr));
    for (int i = 0; i < 3; ++i)
        if (hasResult_ || i == 2)
            for (auto* node : {menus_[i], pressedImages_[i]})
                node->runAction(EaseIn::create(
                    MoveTo::create(.5f, {465 - node->getContentSize().width / 2, node->getPositionY()}), .25f));
}
void DetailView::close()
{
    if (closing_)
        return;
    closing_ = true;
    busy_    = true;
    cancelTouch();
    shade_->stopAllActions();
    shade_->runAction(FadeOut::create(1));
    board_->stopAllActions();
    for (auto* node : fading_)
    {
        node->stopAllActions();
        node->runAction(FadeOut::create(1));
    }
    board_->runAction(Sequence::create(Spawn::create(EaseOut::create(MoveTo::create(1, origin_), .25f),
                                                     EaseOut::create(ScaleTo::create(1, 0), .25f), nullptr),
                                       CallFunc::create([this] {
        if (gameUI)
            gameUI(true);
        closeMenu();
    }),
                                       nullptr));
}
void DetailView::closeMenu()
{
    for (int i = 0; i < 3; ++i)
        for (auto* node : {menus_[i], pressedImages_[i]})
            node->runAction(EaseOut::create(
                MoveTo::create(.5f, {480 + node->getContentSize().width / 2, node->getPositionY()}), .25f));
    scheduleOnce([this](float) {
        auto callback   = std::move(complete);
        const bool pass = success_;
        removeFromParent();
        if (callback)
            callback(pass);
    }, .5f, "complete");
}
void DetailView::beginTouch(Vec2 p)
{
    pressed_ = -1;
    if (closing_ || (blocked && blocked()))
        return;
    for (int i = 0; i < 3; ++i)
        if ((hasResult_ || i == 2) && contains(menus_[i], p))
        {
            pressed_ = i;
            pressedImages_[i]->setVisible(true);
            return;
        }
    if (!busy_)
        press(p);
}
void DetailView::endTouch(Vec2 p)
{
    const int index = pressed_;
    pressed_        = -1;
    for (auto* image : pressedImages_)
        image->setVisible(false);
    if (closing_ || (blocked && blocked()))
        return;
    if (index >= 0)
    {
        if (!contains(menus_[index], p))
            return;
        if (index == 0)
        {
            if (handsRequested)
                handsRequested();
        }
        else if (index == 1)
            retry();
        else
            close();
        return;
    }
    if (!busy_ || releaseWhileBusy())
        release(p);
}
void DetailView::cancelTouch()
{
    pressed_ = -1;
    for (auto* image : pressedImages_)
        if (image)
            image->setVisible(false);
    cancel();
}
void DetailView::solve()
{
    if (!closing_)
        success();
}
void DetailView::success()
{
    busy_ = true;
    cancelTouch();
    auto* result = ResultFeedback::create(true);
    addChild(result, 20);
    result->sound    = [this](std::string_view file) { playSound(file); };
    result->complete = [this] {
        success_ = true;
        close();
    };
    result->play();
}
void DetailView::failed()
{
    busy_ = true;
    cancelTouch();
    auto* result = ResultFeedback::create(false);
    addChild(result, 20);
    result->sound    = [this](std::string_view file) { playSound(file); };
    result->complete = [this] { failedComplete(); };
    result->play();
}

void DetailView::moveTouch(Vec2 p)
{
    if (!closing_ && !busy_ && pressed_ < 0 && !(blocked && blocked()))
        move(p);
}
