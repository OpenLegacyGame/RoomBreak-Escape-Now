#pragma once
#include "axmol.h"
#include "Progress.h"
#include <functional>
#include <vector>
class GameMessageBox;
class SystemMenu : public ax::Node
{
public:
    struct Settings
    {
        bool effects = true, music = true, vibration = true;
        int textSpeed = 1;
    };
    static SystemMenu* create(const rb::Progress&, int language, Settings settings);
    void open(int mode = 1);
    void close();
    void setMode(int mode);
    void click(int index);
    bool beginTouch(ax::Vec2);
    void endTouch(ax::Vec2);
    int mode() const { return mode_; }
    GameMessageBox* messageBox() const { return message_; }
    const Settings& settings() const { return settings_; }
    std::function<void()> resumed, retry, exitGame, unlockAll;
    std::function<void(Settings)> settingsChanged;
    std::function<void(int)> purchase;
    std::function<void(std::string_view)> sound;

private:
    void setup();
    void refreshOptions();
    void refreshCounts();
    void confirm(const char* key, std::function<void()> action);
    int hit(ax::Vec2) const;
    void selectShop(int index);
    void fade(ax::Node*, int opacity, float duration = .3f);
    const rb::Progress* progress_ = nullptr;
    Settings settings_;
    int language_ = 0, mode_ = 0, pressed_ = -1, selectedShop_ = -1;
    bool opened_ = false, closing_ = false;
    ax::Sprite* bg_ = nullptr;
    ax::Sprite *titles_[3]{}, *play_ = nullptr, *playP_ = nullptr, *back_ = nullptr, *backP_ = nullptr;
    ax::Sprite *main_[4]{}, *mainP_[4]{}, *options_[4]{}, *optionsP_[4]{}, *off_[3]{}, *speed_[3]{};
    ax::Sprite *shop_[6]{}, *shopP_[6]{}, *shopCursor_ = nullptr;
    std::vector<ax::Node*> shopDecor_;
    ax::Label* counts_[4]{};
    ax::Node* unlock_ = nullptr;
    ax::Label* description_[2]{};
    GameMessageBox* message_ = nullptr;
};
