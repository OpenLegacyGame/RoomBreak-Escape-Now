#include "GameAudio.h"
#include "audio/AudioEngine.h"
using ax::AudioEngine;
namespace rb
{
void GameAudio::playMusic(std::string_view file)
{
    if (!file.empty() && !settings_.music)
        return;
    std::string path(file);
    if (!path.empty() && path.find(".mp3") == std::string::npos)
        path += ".mp3";
    if (path == "EP5_3.mp3")
        path = "EP5-3.mp3";
    // AudioPlayer::playAudio: resumes an unchanged track (original 0x796fc).
    if (path.empty() || path != musicFile_)
    {
        if (music_ >= 0)
            AudioEngine::stop(music_);
        music_     = -1;
        musicFile_ = std::move(path);
    }
    if (!musicFile_.empty())
    {
        if (music_ < 0)
            music_ = AudioEngine::play2d(musicFile_, true);
        else
            AudioEngine::resume(music_);
    }
}
void GameAudio::play(std::string_view file, bool music)
{
    if (music)
    {
        // A nil default clears playFile without stopping the loaded track.
        // Explicit scene teardown uses playMusic("") instead (releaseAudio).
        if (file.empty())
        {
            if (settings_.music)
                musicFile_.clear();
        }
        else
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
void GameAudio::setMenuAudio(bool stopped, std::string_view defaultMusic)
{
    if (stopped)
    {
        if (music_ >= 0)
        {
            // Axmol ignores pause while loading. Cancel only a fresh, unready player;
            // a paused player is retained, so resuming never reloads or seeks the stream.
            if (AudioEngine::getState(music_) == AudioEngine::AudioState::INITIALIZING)
            {
                AudioEngine::stop(music_);
                music_ = -1;
            }
            else
                AudioEngine::pause(music_);
        }
    }
    else if (settings_.music)
    {
        if (musicFile_.empty())
            play(defaultMusic, true);
        else
            playMusic(musicFile_);
    }
}
}  // namespace rb
