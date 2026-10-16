#pragma once
#include "UserSettings.h"
#include <string>
#include <string_view>

namespace rb
{
// Background music and sound effects, honoring the player's audio settings.
class GameAudio
{
public:
    explicit GameAudio(const UserSettings& settings) : settings_(settings) {}
    // Replaces the current track; an empty name stops music.
    void playMusic(std::string_view file);
    void play(std::string_view file, bool music);
    // Pauses music while a menu is open, resuming the remembered track afterwards.
    void setMenuAudio(bool stopped);
    const std::string& musicFile() const { return musicFile_; }
    int musicId() const { return music_; }

private:
    const UserSettings& settings_;
    std::string musicFile_;
    int music_ = -1;
};
}  // namespace rb
