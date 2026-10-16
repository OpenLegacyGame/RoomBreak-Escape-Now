#include "DetailTelevision.h"
#include "SheetSprite.h"
using namespace ax;
DetailTelevision* DetailTelevision::make(const rb::GameData& data, int id, Vec2 origin, int language, int hands)
{
    return createDetail<DetailTelevision>(data, id, origin, language, hands);
}
void DetailTelevision::setup(const rb::Row&)
{
    makeBoard("e6_s3_det1_1.png");
    board_->setRotation(20);
    const char* languages[] = {"chi", "kor", "eng", "jap"};
    auto* image             = SheetSprite::create(std::string("e6_s3_det1_2_") + languages[language_] + ".png", 2, 2);
    board_->addChild(image);
    track(image);
    place(board_, image, {22, 27});
    schedule([image](float) { image->playOnce(4, .2f, {}); }, 2, "television-noise");
}
void DetailTelevision::close()
{
    unschedule("television-noise");
    DetailView::close();
}
