#include "GameAudio.h"
#include "audio/AudioEngine.h"
using ax::AudioEngine;
namespace rb
{
void GameAudio::playMusic(std::string_view file)
{
    if (music_ >= 0)
        AudioEngine::stop(music_);
    music_     = -1;
    musicFile_ = file;
    if (file.empty())
        return;
    if (musicFile_.find(".mp3") == std::string::npos)
        musicFile_ += ".mp3";
    if (musicFile_ == "EP5_3.mp3")
        musicFile_ = "EP5-3.mp3";
    if (settings_.music)
        music_ = AudioEngine::play2d(musicFile_, true);
}
void GameAudio::play(std::string_view file, bool music)
{
    if (music)
    {
        playMusic(file);
        return;
    }
    if (!settings_.effects || file.empty())
        return;
    std::string path(file);
    if (path.find(".wav") == std::string::npos)
        path += ".wav";
    AudioEngine::play2d(path);
}
void GameAudio::setMenuAudio(bool stopped)
{
    if (stopped)
    {
        if (music_ >= 0)
            AudioEngine::stop(music_);
        music_ = -1;
    }
    else if (settings_.music && !musicFile_.empty() && music_ < 0)
        music_ = AudioEngine::play2d(musicFile_, true);
}
}  // namespace rb
