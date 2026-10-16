#include "UserSettings.h"
#include "axmol.h"
namespace rb
{
UserSettings UserSettings::load()
{
    auto* store = ax::UserDefault::getInstance();
    UserSettings settings;
    settings.language  = store->getIntegerForKey("rb.language", 0);
    settings.textSpeed = store->getIntegerForKey("rb.textSpeed", 1);
    settings.music     = store->getBoolForKey("rb.bgm", true);
    settings.effects   = store->getBoolForKey("rb.effect", true);
    settings.vibration = store->getBoolForKey("rb.vibration", true);
    return settings;
}
void UserSettings::save() const
{
    auto* store = ax::UserDefault::getInstance();
    store->setIntegerForKey("rb.language", language);
    store->setIntegerForKey("rb.textSpeed", textSpeed);
    store->setBoolForKey("rb.bgm", music);
    store->setBoolForKey("rb.effect", effects);
    store->setBoolForKey("rb.vibration", vibration);
    store->flush();
}
}  // namespace rb
