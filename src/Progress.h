#pragma once
#include "GameData.h"
#include <set>
namespace rb
{
int databaseEpisode(int displayedEpisode);
int displayedEpisode(int databaseEpisode);
struct StageProgress
{
    bool opened = false, goldKey = false, apPoint = false;
    std::string rank;
};
class Progress
{
public:
    void initialize(const GameData& data);
    void load(std::string_view json);
    void claimShopProduct(int product);
    void unlockAllStages(const GameData& data);
    std::string encode() const;
    std::string clear(const GameData& data, int episode, int stage, int touchCount);
    bool episodeOpened(int displayed) const { return openEpisodes.contains(displayed); }
    bool stageOpened(const GameData& data, int episode, int stage) const;
    std::set<int> openEpisodes, seenIntros, seenEndings, achievements, crystals;
    std::map<int, StageProgress> stages;
    int ap = 0, goldKeys = 0, eyes = 0, insight = 0, hands = 0, feet = 0, score = 0;
};
}  // namespace rb
