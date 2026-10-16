#pragma once
#include "rapidjson/document.h"
#include <map>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace rb
{
using Row = rapidjson::Value;
int integer(const Row& row, const char* key);
std::string string(const Row& row, const char* key);

// Immutable, rowid-ordered copy of the original SQLite tables. Lookups are
// served from lazily built per-column indexes that preserve rowid order.
class GameData
{
public:
    void load(std::string_view json);
    const Row& table(const char* name) const;
    const Row& byId(const char* table, const char* key, int id) const;
    std::vector<const Row*> where(const char* table, const char* key, int value) const;
    std::vector<const Row*> stage(const char* table, int episode, int stage) const;
    std::string text(int id, int language) const;

private:
    using Rows = std::vector<const Row*>;
    const std::unordered_map<int, Rows>& index(const char* table, const char* key) const;
    rapidjson::Document data_;
    mutable std::map<std::pair<std::string, std::string>, std::unordered_map<int, Rows>> indexes_;
    mutable std::map<std::string, std::map<std::pair<int, int>, Rows>> stageIndexes_;
};

struct Command
{
    enum class Type
    {
        GetItem      = 1,
        Detail       = 2,
        Action       = 3,
        Clear        = 4,
        MoveRoom     = 5,
        Scene        = 6,
        Crystal      = 7,
        MovePosition = 8,
        Effect       = 11,
        Sound        = 12,
        ChangeState  = 13,
        Show         = 14,
        Hide         = 15,
        Direction    = 16,
        Dialog       = 17,
        Shake        = 18,
        Vibrate      = 19,
        Particle     = 20
    };
    Type type;
    std::string argument;
    std::string source;
};
std::vector<Command> parseCommands(std::string_view source);

struct ObjectState
{
    int state        = 0;
    bool active      = false;
    bool disappeared = false;
    bool shining     = false;
};
class StageState
{
public:
    void reset(const GameData& data, int episode, int stage, bool goldKeyCollected);
    bool conditionsMatch(const GameData& data, int conditionId) const;
    int eventFor(const GameData& data, int object, int cursor, int selectedItem = 0) const;
    int objectAt(const GameData& data, float roomX, float roomYFromTop) const;
    void addItem(int id);
    void removeItem(int id);
    std::vector<int> combineResult(const GameData&, int selected, int target) const;
    std::vector<int> separateResult(const GameData&, int id) const;
    void combineItems(const GameData&, int selected, int target);
    void separateItem(const GameData&, int id);
    const std::vector<int>& inventory() const { return inventory_; }
    int episode = 0, stage = 0, room = 0, health = 0;
    std::map<int, ObjectState> objects;
    std::map<int, bool> items;
    std::map<int, bool> firedConditions;

private:
    std::vector<int> inventory_;
};
}  // namespace rb
