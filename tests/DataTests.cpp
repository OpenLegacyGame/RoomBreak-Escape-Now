#include "GameData.h"
#include "EventRunner.h"
#include "Progress.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <set>
int main(int argc, char** argv)
{
    assert(argc == 2);
    std::ifstream in(argv[1]);
    std::string bytes((std::istreambuf_iterator<char>(in)), {});
    rb::GameData data;
    data.load(bytes);
    assert(data.table("RoomData").Size() == 52);
    assert(data.table("DetailData").Size() == 96);
    std::size_t commands = 0;
    for (const auto& row : data.table("EventInfoTable").GetArray())
        commands += rb::parseCommands(rb::string(row, "eventData")).size();
    assert(commands == 7141);
    for (const auto& row : data.table("StageTable").GetArray())
    {
        rb::StageState state;
        state.reset(data, rb::integer(row, "episodeNumber"), rb::integer(row, "stageNumber"), false);
        for (const auto& [id, obj] : state.objects)
        {
            const auto& initial = data.byId("ObjectState", "stateID", obj.state);
            assert(rb::integer(initial, "objectID") == id);
        }
    }
    // Exercise recipes loaded by the original per-stage ItemData loop. This catches IDs
    // outside that stage and inventory mutations that lose/reorder outputs.
    int loadedRecipes = 0, unusedRecipes = 0;
    for (const auto& recipe : data.table("ItemCombine").GetArray())
    {
        const int target = rb::integer(recipe, "itemID"), selected = rb::integer(recipe, "targetItem");
        if (data.where("ItemData", "itemID", target).empty())
        {
            ++unusedRecipes;
            continue;
        }
        ++loadedRecipes;
        const auto& item = data.byId("ItemData", "itemID", target);
        rb::StageState state;
        state.reset(data, rb::integer(item, "episodeNumber"), rb::integer(item, "stageNumber"), false);
        state.addItem(selected);
        state.addItem(target);
        const auto result = state.combineResult(data, selected, target);
        assert(!result.empty());
        state.combineItems(data, selected, target);
        assert(state.inventory() == result);
        for (int id : result)
            assert(state.items.at(id));
    }
    assert(loadedRecipes == 90 && unusedRecipes == 54);
    for (const auto& item : data.table("ItemData").GetArray())
    {
        if (rb::string(item, "separate").empty())
            continue;
        rb::StageState state;
        state.reset(data, rb::integer(item, "episodeNumber"), rb::integer(item, "stageNumber"), false);
        const int id = rb::integer(item, "itemID");
        state.addItem(id);
        const auto result = state.separateResult(data, id);
        assert(result.size() == 2);
        state.separateItem(data, id);
        assert(state.inventory() == result);
    }
    rb::StageState inventory;
    inventory.reset(data, 2, 1, false);
    inventory.addItem(3);
    inventory.addItem(1);
    inventory.addItem(4);
    inventory.combineItems(data, 4, 3);
    assert((inventory.inventory() == std::vector<int>{1, 5}));
    inventory.reset(data, 0, 1, false);
    inventory.addItem(269);
    inventory.addItem(272);
    inventory.addItem(270);
    inventory.separateItem(data, 272);
    assert((inventory.inventory() == std::vector<int>{269, 273, 270, 277}));
    const auto previous = inventory.inventory();
    bool rejected       = false;
    try
    {
        inventory.combineItems(data, 269, 270);
    }
    catch (const std::runtime_error&)
    {
        rejected = true;
    }
    assert(rejected && inventory.inventory() == previous);
    auto group = rb::parseCommands("&och_50-57&vie_52&snd_9");
    assert(group.size() == 3 && group[0].type == rb::Command::Type::ChangeState && group[0].argument == "50-57");
    for (int language = 0; language < 4; ++language)
    {
        for (const auto& row : data.table("TextEngData").GetArray())
            data.text(rb::integer(row, "textID"), language);
    }
    // Synthetic callback order verifies parallel groups and stale completions.
    rb::GameData fixture;
    fixture.load(
        R"({"EventTable":[{"eventID":1,"isLink":1}],"EventInfoTable":[{"eventID":1,"eventData":"&och_1-2&snd_9"},{"eventID":1,"eventData":"&dia_3"}]})");
    std::vector<rb::EventRunner::Complete> pending;
    std::vector<std::string> seen;
    rb::EventRunner runner(fixture, [&](const rb::Command& c, auto finish) {
        seen.push_back(c.source);
        pending.push_back(finish);
    });
    runner.start(1);
    assert(seen.size() == 2 && runner.protectsTouch());
    auto first = pending[0], second = pending[1];
    second(true);
    assert(seen.size() == 2);
    first(true);
    assert(seen.size() == 3);
    auto last = pending[2];
    last(true);
    assert(!runner.busy());
    runner.start(1);
    auto stale = pending.back();
    runner.cancel();
    stale(true);
    assert(!runner.busy());
    runner.start(1);
    auto stop = pending.back();
    stop(false);
    assert(!runner.busy());
    std::size_t observedAtCondition = 0;
    runner.afterGroup               = [&] { observedAtCondition = seen.size(); };
    runner.start(1);
    auto a = pending[pending.size() - 2], b = pending.back();
    const auto beforeNext = seen.size();
    a(true);
    b(true);
    assert(observedAtCondition == beforeNext + 1);  // next row dispatched before condition evaluation
    pending.back()(true);
    int failureChecks = 0;
    runner.afterGroup = [&] { ++failureChecks; };
    runner.start(1);
    pending.back()(false);
    assert(failureChecks == 1 && runner.linked() && !runner.protectsTouch());
    int dispatches = 0, checks = 0;
    rb::EventRunner synchronous(fixture, [&](const rb::Command&, auto finish) {
        ++dispatches;
        finish(true);
    });
    synchronous.afterGroup = [&] {
        assert(dispatches == 3);
        ++checks;
    };
    synchronous.start(1);
    assert(checks == 2 && !synchronous.busy());
    // A non-linked sound/effect can remain active when another interaction starts.
    rb::GameData overlapping;
    overlapping.load(
        R"({"EventTable":[{"eventID":1,"isLink":0},{"eventID":2,"isLink":1}],"EventInfoTable":[{"eventID":1,"eventData":"&snd_9"},{"eventID":2,"eventData":"&och_1-2"},{"eventID":2,"eventData":"&dia_3"}]})");
    std::vector<rb::EventRunner::Complete> overlapPending;
    int overlapChecks = 0;
    rb::EventRunner overlap(overlapping, [&](const rb::Command&, auto done) { overlapPending.push_back(done); });
    overlap.afterGroup = [&] { ++overlapChecks; };
    overlap.start(1);
    overlap.start(1);
    assert(overlapPending.size() == 2 && !overlap.protectsTouch());
    overlapPending[0](true);
    assert(overlapChecks == 0);
    overlap.start(2);
    assert(overlapPending.size() == 3 && overlap.protectsTouch());
    overlapPending[2](true);
    assert(overlapPending.size() == 3);
    overlapPending[1](true);
    assert(overlapPending.size() == 4 && overlapChecks == 1);
    overlapPending.back()(true);
    assert(!overlap.busy() && !overlap.protectsTouch() && overlapChecks == 2);
    rb::Progress shop;
    shop.initialize(data);
    const auto& defaults = data.table("GameInfoData")[0];
    assert(shop.eyes == rb::integer(defaults, "abilityEyes"));
    assert(shop.insight == rb::integer(defaults, "abilityInsight"));
    assert(shop.hands == rb::integer(defaults, "abilityHands"));
    assert(shop.feet == rb::integer(defaults, "abilityFoots"));
    const int eyes = shop.eyes, insight = shop.insight, hands = shop.hands, feet = shop.feet, gold = shop.goldKeys;
    for (int repeat = 0; repeat < 2; ++repeat)
        for (int product = 0; product < 6; ++product)
            shop.claimShopProduct(product);
    assert(shop.eyes == eyes + 16 && shop.insight == insight + 16 && shop.hands == hands + 16 &&
           shop.feet == feet + 8 && shop.goldKeys == gold);
    --shop.eyes;
    --shop.hands;
    shop.stages.begin()->second.goldKey = true;
    const auto collectedID              = shop.stages.begin()->first;
    shop.unlockAllStages(data);
    for (int episode = 1; episode <= 6; ++episode)
    {
        assert(shop.episodeOpened(episode));
        for (int stage = 1; stage <= 5; ++stage)
            assert(shop.stageOpened(data, rb::databaseEpisode(episode), stage));
    }
    rb::Progress reopened;
    reopened.initialize(data);
    reopened.load(shop.encode());
    assert(reopened.encode() == shop.encode() && reopened.stages.at(collectedID).goldKey);
    for (const auto& row : data.table("StageTable").GetArray())
    {
        rb::StageState first, replay;
        const int e = rb::integer(row, "episodeNumber"), s = rb::integer(row, "stageNumber");
        first.reset(data, e, s, false);
        replay.reset(data, e, s, true);
        for (const auto& [id, object] : first.objects)
            if (rb::integer(data.byId("ObjectData", "objectID", id), "objectType") == 2)
                assert(!replay.objects.contains(id));
    }
    rb::Progress progress;
    progress.initialize(data);
    assert(progress.episodeOpened(1) && !progress.episodeOpened(6));
    for (int episode = 1; episode <= 5; ++episode)
    {
        int database = rb::databaseEpisode(episode);
        assert(progress.episodeOpened(episode));
        for (int stage = 1; stage <= 5; ++stage)
        {
            assert(progress.stageOpened(data, database, stage));
            assert(progress.clear(data, database, stage, 0) == "s");
            progress.clear(data, database, stage, 100000);
            auto row = data.stage("StageTable", database, stage).front();
            assert(progress.stages.at(rb::integer(*row, "stageID")).rank == "s");
        }
    }
    assert(progress.episodeOpened(6));
    rb::Progress restored;
    restored.initialize(data);
    restored.load(progress.encode());
    assert(restored.encode() == progress.encode());
    std::cout << "31 stages, 52 rooms, 96 details, " << commands << " commands, four languages, event barriers: PASS\n";
}
