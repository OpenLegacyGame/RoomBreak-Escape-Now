#include "GameData.h"
#include <cstdlib>
#include <algorithm>
#include <stdexcept>

namespace rb
{
int integer(const Row& row, const char* key)
{
    const auto& v = row[key];
    if (v.IsInt())
        return v.GetInt();
    if (v.IsBool())
        return v.GetBool() ? 1 : 0;
    if (v.IsString())
        return std::atoi(v.GetString());
    if (v.IsNull())
        return 0;
    throw std::runtime_error(std::string("Invalid integer: ") + key);
}
std::string string(const Row& row, const char* key)
{
    const auto& v = row[key];
    return v.IsString() ? std::string(v.GetString(), v.GetStringLength()) : std::string();
}
void GameData::load(std::string_view json)
{
    indexes_.clear();
    stageIndexes_.clear();
    data_.Parse(json.data(), json.size());
    if (data_.HasParseError() || !data_.IsObject())
        throw std::runtime_error("GameData.json is invalid");
}
const Row& GameData::table(const char* name) const
{
    if (!data_.HasMember(name) || !data_[name].IsArray())
        throw std::runtime_error(std::string("Missing table: ") + name);
    return data_[name];
}
const std::unordered_map<int, GameData::Rows>& GameData::index(const char* name, const char* key) const
{
    auto [it, inserted] = indexes_.try_emplace({name, key});
    if (inserted)
    {
        try
        {
            for (const auto& row : table(name).GetArray())
                it->second[integer(row, key)].push_back(&row);
        }
        catch (...)
        {
            indexes_.erase(it);
            throw;
        }
    }
    return it->second;
}
const Row& GameData::byId(const char* name, const char* key, int id) const
{
    const auto& rows = index(name, key);
    if (auto it = rows.find(id); it != rows.end())
        return *it->second.front();
    throw std::runtime_error(std::string("Missing ") + name + "." + key + "=" + std::to_string(id));
}
std::vector<const Row*> GameData::where(const char* name, const char* key, int value) const
{
    const auto& rows = index(name, key);
    const auto it    = rows.find(value);
    return it == rows.end() ? Rows{} : it->second;
}
std::vector<const Row*> GameData::stage(const char* name, int episode, int stageNumber) const
{
    auto [it, inserted] = stageIndexes_.try_emplace(name);
    if (inserted)
    {
        try
        {
            for (const auto& row : table(name).GetArray())
                it->second[{integer(row, "episodeNumber"), integer(row, "stageNumber")}].push_back(&row);
        }
        catch (...)
        {
            stageIndexes_.erase(it);
            throw;
        }
    }
    const auto found = it->second.find({episode, stageNumber});
    return found == it->second.end() ? Rows{} : found->second;
}
std::string GameData::text(int id, int language) const
{
    const char* tables[] = {"TextChiData", "TextKorData", "TextEngData", "TextJapData"};
    if (language < 0 || language > 3)
        throw std::runtime_error("Invalid language");
    return string(byId(tables[language], "textID", id), "text");
}
std::vector<Command> parseCommands(std::string_view source)
{
    static const std::map<std::string_view, Command::Type> types = {
        {"get", Command::Type::GetItem}, {"det", Command::Type::Detail},       {"act", Command::Type::Action},
        {"clear", Command::Type::Clear}, {"mov", Command::Type::MoveRoom},     {"eve", Command::Type::Scene},
        {"cry", Command::Type::Crystal}, {"pos", Command::Type::MovePosition}, {"eff", Command::Type::Effect},
        {"snd", Command::Type::Sound},   {"och", Command::Type::ChangeState},  {"vie", Command::Type::Show},
        {"hid", Command::Type::Hide},    {"dir", Command::Type::Direction},    {"dia", Command::Type::Dialog},
        {"sha", Command::Type::Shake},   {"vibrator", Command::Type::Vibrate}, {"par", Command::Type::Particle}};
    std::vector<Command> out;
    while (!source.empty())
    {
        if (source.front() != '&')
            throw std::runtime_error("Event must start with &: " + std::string(source));
        const auto end        = source.find('&', 1);
        const auto part       = source.substr(1, end == std::string_view::npos ? end : end - 1);
        const auto underscore = part.find('_');
        const auto name       = part.substr(0, underscore);
        const auto it         = types.find(name);
        if (it == types.end())
            throw std::runtime_error("Unknown original event: " + std::string(name));
        out.push_back({it->second, underscore == std::string_view::npos ? "" : std::string(part.substr(underscore + 1)),
                       "&" + std::string(part)});
        if (end == std::string_view::npos)
            break;
        source.remove_prefix(end);
    }
    return out;
}
void StageState::reset(const GameData& data, int e, int s, bool goldKeyCollected)
{
    episode = e;
    stage   = s;
    room    = 0;
    objects.clear();
    items.clear();
    firedConditions.clear();
    inventory_.clear();
    auto stages = data.stage("StageTable", e, s);
    if (stages.size() != 1)
        throw std::runtime_error("StageTable identity mismatch");
    health = integer(*stages.front(), "maxHealth");
    for (auto* r : data.stage("RoomData", e, s))
    {
        const int id = integer(*r, "roomID");
        if (integer(*r, "isBasic"))
            room = id;
        for (auto* o : data.where("ObjectData", "roomID", id))
        {
            if (integer(*o, "objectType") == 2 && goldKeyCollected)
                continue;
            const int oid = integer(*o, "objectID");
            auto& value   = objects[oid];
            value.active  = integer(*o, "visible") != 0;
            for (auto* state : data.where("ObjectState", "objectID", oid))
                if (integer(*state, "isBasic"))
                    value.state = integer(*state, "stateID");
        }
    }
    if (!room)
        throw std::runtime_error("Stage has no initial room");
    for (auto* item : data.stage("ItemData", e, s))
        items[integer(*item, "itemID")] = false;
}
void StageState::addItem(int id)
{
    items.at(id) = true;
    inventory_.push_back(id);
}
void StageState::removeItem(int id)
{
    auto it = std::find(inventory_.begin(), inventory_.end(), id);
    if (it == inventory_.end())
        throw std::runtime_error("Inventory item is absent");
    inventory_.erase(it);
    items.at(id) = std::find(inventory_.begin(), inventory_.end(), id) != inventory_.end();
}
static std::vector<int> itemList(std::string_view text)
{
    std::vector<int> result;
    while (!text.empty())
    {
        const auto comma = text.find(',');
        const int id     = std::stoi(std::string(text.substr(0, comma)));
        if (id > 0)
            result.push_back(id);
        if (comma == std::string_view::npos)
            break;
        text.remove_prefix(comma + 1);
    }
    return result;
}
std::vector<int> StageState::combineResult(const GameData& data, int selected, int target) const
{
    // Original asks the drop target's recipe list about the dragged item.
    for (auto* row : data.where("ItemCombine", "itemID", target))
        if (integer(*row, "targetItem") == selected)
            return itemList(string(*row, "getItem"));
    return {};
}
std::vector<int> StageState::separateResult(const GameData& data, int id) const
{
    return itemList(string(data.byId("ItemData", "itemID", id), "separate"));
}
void StageState::combineItems(const GameData& data, int selected, int target)
{
    const auto result = combineResult(data, selected, target);
    if (selected == target || !items.at(selected) || !items.at(target) || result.empty())
        throw std::runtime_error("Invalid inventory combination");
    for (int id : result)
        (void)items.at(id);  // validate the whole result before mutation
    removeItem(selected);
    removeItem(target);
    for (int id : result)
        addItem(id);
}
void StageState::separateItem(const GameData& data, int id)
{
    const auto result = separateResult(data, id);
    auto it           = std::find(inventory_.begin(), inventory_.end(), id);
    if (it == inventory_.end() || result.size() != 2)
        throw std::runtime_error("Invalid inventory separation");
    for (int part : result)
        (void)items.at(part);
    // The first part replaces the original slot; the second goes at the end.
    *it                 = result[0];
    items.at(id)        = std::find(inventory_.begin(), inventory_.end(), id) != inventory_.end();
    items.at(result[0]) = true;
    addItem(result[1]);
}
bool StageState::conditionsMatch(const GameData& data, int id) const
{
    for (auto* c : data.where("ConditionInfoTable", "conID", id))
    {
        auto it = objects.find(integer(*c, "objectID"));
        if (it == objects.end() || it->second.state != integer(*c, "stateID"))
            return false;
    }
    return true;
}
int StageState::eventFor(const GameData& data, int object, int cursor, int selectedItem) const
{
    const auto& value = objects.at(object);
    if (selectedItem)
    {
        for (auto* row : data.where("ItemCursorEventTable", "stateID", value.state))
            if (integer(*row, "usedItem") == selectedItem)
                return integer(*row, "eventID");
        return 0;
    }
    const char* keys[] = {"handsID", "searchID", "footsID"};
    if (cursor < 0 || cursor > 2)
        throw std::runtime_error("Invalid cursor");
    int event = 0;
    for (auto* row : data.where("CursorEventTable", "stateID", value.state))
        event = integer(*row, keys[cursor]);
    return event;
}
int StageState::objectAt(const GameData& data, float x, float y) const
{
    std::vector<const Row*> matches;
    for (auto* row : data.where("ObjectData", "roomID", room))
    {
        const int id = integer(*row, "objectID");
        auto it      = objects.find(id);
        if (it == objects.end() || !it->second.active || it->second.disappeared)
            continue;
        for (auto* area : data.where("AreaData", "objectID", id))
        {
            // DataControl 0x7f3c divides signed integer coordinates before use.
            const int left = integer(*area, "AreaX") / 2, top = integer(*area, "AreaY") / 2;
            const int width = integer(*area, "AreaWidth") / 2, height = integer(*area, "AreaHeight") / 2;
            if (x > left && x < left + width && y > top && y < top + height)
            {
                matches.push_back(row);
                break;
            }
        }
    }
    if (matches.size() == 1)
        return integer(*matches.front(), "objectID");
    const int max = integer(data.byId("RoomData", "roomID", room), "maxLayer");
    for (int layer = 1; layer <= max; ++layer)
        for (auto* row : matches)
            if (integer(data.byId("LayerData", "layerID", integer(*row, "layerID")), "layerSequence") == layer)
                return integer(*row, "objectID");
    return 0;
}
}  // namespace rb
