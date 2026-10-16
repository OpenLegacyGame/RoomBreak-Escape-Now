#pragma once

namespace rb
{
// Player preferences persisted in the platform's UserDefault store.
struct UserSettings
{
    int language = 0, textSpeed = 1;
    bool music = true, effects = true, vibration = true;
    static UserSettings load();
    void save() const;
};
}  // namespace rb
