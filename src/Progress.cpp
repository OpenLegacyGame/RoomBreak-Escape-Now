#include "Progress.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include <stdexcept>
#include <limits>
#include <algorithm>
namespace rb
{
int databaseEpisode(int e)
{
    return e == 1 ? 3 : e == 2 ? 1 : e == 3 ? 2 : e;
}
int displayedEpisode(int e)
{
    return e == 3 ? 1 : e == 1 ? 2 : e == 2 ? 3 : e;
}
void Progress::initialize(const GameData& data)
{
    openEpisodes.clear();
    seenIntros.clear();
    seenEndings.clear();
    achievements.clear();
    crystals.clear();
    stages.clear();
    for (const auto& row : data.table("EpisodeData").GetArray())
    {
        const int e = integer(row, "episodeNumber");
        if (integer(row, "isOpened"))
            openEpisodes.insert(e);
        if (integer(row, "isIntroOpened"))
            seenIntros.insert(e);
        if (integer(row, "isEndingOpened"))
            seenEndings.insert(e);
    }
    for (const auto& row : data.table("StageTable").GetArray())
        stages[integer(row, "stageID")] = {integer(row, "isOpened") != 0, integer(row, "isGetGoldKey") != 0,
                                           integer(row, "isGetApPoint") != 0, string(row, "getRanked")};
    const auto& info = data.table("GameInfoData")[0];
    ap               = integer(info, "apPoint");
    goldKeys         = integer(info, "goldKey");
    score            = integer(info, "gameScore");
    eyes             = integer(info, "abilityEyes");
    insight          = integer(info, "abilityInsight");
    hands            = integer(info, "abilityHands");
    feet             = integer(info, "abilityFoots");
    for (const auto& row : data.table("AchieveData").GetArray())
        if (integer(row, "isAchieve"))
            achievements.insert(integer(row, "achID"));
    for (const auto& row : data.table("CrystalTable").GetArray())
        if (integer(row, "isGeted"))
            crystals.insert(integer(row, "crystalID"));
}
void Progress::claimShopProduct(int product)
{
    static constexpr int amounts[6][4] = {{2, 0, 0, 0}, {0, 2, 0, 0}, {0, 0, 2, 0},
                                          {0, 0, 0, 1}, {6, 6, 0, 0}, {0, 0, 6, 3}};
    if (product < 0 || product >= 6)
        throw std::runtime_error("Invalid shop product");
    int* counts[] = {&eyes, &insight, &hands, &feet};
    for (int i = 0; i < 4; ++i)
        *counts[i] += std::min(amounts[product][i], std::numeric_limits<int>::max() - *counts[i]);
}
void Progress::unlockAllStages(const GameData& data)
{
    for (const auto& row : data.table("EpisodeData").GetArray())
        openEpisodes.insert(integer(row, "episodeNumber"));
    for (auto& [id, stage] : stages)
        stage.opened = true;
}
bool Progress::stageOpened(const GameData& data, int e, int s) const
{
    const auto rows = data.stage("StageTable", e, s);
    return rows.size() == 1 && stages.at(integer(*rows.front(), "stageID")).opened;
}
std::string Progress::clear(const GameData& data, int e, int s, int touches)
{
    const auto rows = data.stage("StageTable", e, s);
    if (rows.size() != 1)
        throw std::runtime_error("Invalid completed stage");
    const auto& row  = *rows.front();
    const char* rank = "d";
    if (touches <= integer(row, "rankS"))
        rank = "s";
    else if (touches <= integer(row, "rankA"))
        rank = "a";
    else if (touches <= integer(row, "rankB"))
        rank = "b";
    else if (touches <= integer(row, "rankC"))
        rank = "c";
    auto& saved  = stages.at(integer(row, "stageID"));
    auto quality = [](std::string_view r) {
        return r == "s" ? 5 : r == "a" ? 4 : r == "b" ? 3 : r == "c" ? 2 : r == "d" ? 1 : 0;
    };
    if (quality(rank) > quality(saved.rank))
        saved.rank = rank;
    if (e && s < 5)
    {
        auto next = data.stage("StageTable", e, s + 1);
        if (next.size() != 1)
            throw std::runtime_error("Missing next stage");
        stages.at(integer(*next.front(), "stageID")).opened = true;
    }
    if (e && s == 5)
    {
        const int display = displayedEpisode(e);
        // Offline rule chosen by the user: episode 6 opens after episode 5.
        if (display < 6)
            openEpisodes.insert(display + 1);
    }
    return rank;
}
std::string Progress::encode() const
{
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> w(buffer);
    w.StartObject();
    w.Key("version");
    w.Int(1);
    auto number = [&](const char* key, int n) {
        w.Key(key);
        w.Int(n);
    };
    number("ap", ap);
    number("goldKeys", goldKeys);
    number("eyes", eyes);
    number("insight", insight);
    number("hands", hands);
    number("feet", feet);
    number("score", score);
    auto set = [&](const char* key, const std::set<int>& values) {
        w.Key(key);
        w.StartArray();
        for (int n : values)
            w.Int(n);
        w.EndArray();
    };
    set("openEpisodes", openEpisodes);
    set("seenIntros", seenIntros);
    set("seenEndings", seenEndings);
    set("achievements", achievements);
    set("crystals", crystals);
    w.Key("stages");
    w.StartArray();
    for (const auto& [id, s] : stages)
    {
        w.StartObject();
        number("id", id);
        number("opened", s.opened);
        number("goldKey", s.goldKey);
        number("apPoint", s.apPoint);
        w.Key("rank");
        w.String(s.rank.c_str());
        w.EndObject();
    }
    w.EndArray();
    w.EndObject();
    return {buffer.GetString(), buffer.GetSize()};
}
void Progress::load(std::string_view json)
{
    rapidjson::Document d;
    d.Parse(json.data(), json.size());
    if (d.HasParseError() || !d.IsObject() || !d.HasMember("version") || !d["version"].IsInt() ||
        d["version"].GetInt() != 1)
        throw std::runtime_error("Unsupported progress file");
    const char* numbers[] = {"ap", "goldKeys", "eyes", "insight", "hands", "feet", "score"};
    for (auto key : numbers)
        if (!d.HasMember(key) || !d[key].IsInt() || d[key].GetInt() < 0)
            throw std::runtime_error("Invalid progress counter");
    auto readSet = [&](const char* key, std::set<int>& values) {
        if (!d.HasMember(key) || !d[key].IsArray())
            throw std::runtime_error("Invalid progress list");
        values.clear();
        for (const auto& v : d[key].GetArray())
        {
            if (!v.IsInt())
                throw std::runtime_error("Invalid progress identifier");
            values.insert(v.GetInt());
        }
    };
    Progress next = *this;
    next.ap       = integer(d, "ap");
    next.goldKeys = integer(d, "goldKeys");
    next.eyes     = integer(d, "eyes");
    next.insight  = integer(d, "insight");
    next.hands    = integer(d, "hands");
    next.feet     = integer(d, "feet");
    next.score    = integer(d, "score");
    readSet("openEpisodes", next.openEpisodes);
    readSet("seenIntros", next.seenIntros);
    readSet("seenEndings", next.seenEndings);
    readSet("achievements", next.achievements);
    readSet("crystals", next.crystals);
    if (!d.HasMember("stages") || !d["stages"].IsArray() || d["stages"].Size() != stages.size())
        throw std::runtime_error("Invalid progress stage count");
    std::set<int> loaded;
    for (const auto& row : d["stages"].GetArray())
    {
        if (!row.IsObject())
            throw std::runtime_error("Invalid stage progress record");
        for (auto key : {"id", "opened", "goldKey", "apPoint"})
            if (!row.HasMember(key) || !row[key].IsInt())
                throw std::runtime_error("Invalid stage progress field");
        if (!row.HasMember("rank") || !row["rank"].IsString())
            throw std::runtime_error("Invalid rank");
        const int id = integer(row, "id");
        if (!next.stages.contains(id) || !loaded.insert(id).second)
            throw std::runtime_error("Invalid stage identity");
        next.stages[id] = {integer(row, "opened") != 0, integer(row, "goldKey") != 0, integer(row, "apPoint") != 0,
                           string(row, "rank")};
    }
    *this = std::move(next);
}
}  // namespace rb
