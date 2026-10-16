#pragma once
#include "GameData.h"
#include <cstdint>
#include <deque>
#include <functional>
#include <set>

namespace rb
{
// Corresponds to EventHandle (0x48508..0x490b0). A row is a parallel group;
// linked rows advance only after every operation reports completion.
class EventRunner
{
public:
    using Complete = std::function<void(bool)>;
    using Dispatch = std::function<void(const Command&, Complete)>;
    EventRunner(const GameData& data, Dispatch dispatch) : data_(data), dispatch_(std::move(dispatch)) {}
    void start(int eventId);
    void append(int eventId);
    void cancel();
    bool busy() const { return !active_.empty() || !queue_.empty(); }
    bool protectsTouch() const { return linked_ && running_; }
    bool linked() const { return linked_; }
    std::function<void()> afterGroup;

private:
    void pump();
    bool dispatchGroup(std::vector<Command> group);
    void complete(std::uint64_t generation, std::uint64_t token, bool pass);
    const GameData& data_;
    Dispatch dispatch_;
    std::deque<std::vector<Command>> queue_;
    std::set<std::uint64_t> active_;
    std::uint64_t generation_ = 0, token_ = 0;
    unsigned dispatchDepth_ = 0;
    bool linked_ = false, running_ = false, pumping_ = false;
};
}  // namespace rb
