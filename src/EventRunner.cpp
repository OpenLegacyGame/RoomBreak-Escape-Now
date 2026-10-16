#include "EventRunner.h"
#include <stdexcept>
namespace rb
{
void EventRunner::start(int id)
{
    linked_   = integer(data_.byId("EventTable", "eventID", id), "isLink") != 0;
    auto rows = data_.where("EventInfoTable", "eventID", id);
    if (linked_)
    {
        running_ = true;
        for (auto* row : rows)
            queue_.push_back(parseCommands(string(*row, "eventData")));
        // runEventLoop immediately launches the first queued row even when a
        // previous non-linked event still has outstanding callbacks.
        if (!queue_.empty())
        {
            auto group = std::move(queue_.front());
            queue_.pop_front();
            if (dispatchGroup(std::move(group)))
            {
                pump();
                if (afterGroup)
                    afterGroup();
            }
        }
        else
            pump();
    }
    else if (!rows.empty())
    {
        if (dispatchGroup(parseCommands(string(*rows.front(), "eventData"))))
        {
            pump();
            if (afterGroup)
                afterGroup();
        }
    }
}
void EventRunner::append(int id)
{
    for (auto* row : data_.where("EventInfoTable", "eventID", id))
        queue_.push_back(parseCommands(string(*row, "eventData")));
}
void EventRunner::cancel()
{
    ++generation_;
    queue_.clear();
    active_.clear();
    linked_  = false;
    running_ = false;
}
void EventRunner::complete(std::uint64_t generation, std::uint64_t token, bool pass)
{
    if (generation != generation_ || !active_.erase(token))
        return;
    if (!pass)
    {
        // _stopLinkedEvent leaves _isLinkedEvent unchanged. Conditions are still
        // checked after an event refuses to continue (0x486c8).
        ++generation_;
        queue_.clear();
        active_.clear();
        running_ = false;
        if (afterGroup)
            afterGroup();
        return;
    }
    if (active_.empty() && !dispatchDepth_)
    {
        pump();
        if (afterGroup)
            afterGroup();
    }
}
bool EventRunner::dispatchGroup(std::vector<Command> group)
{
    const auto generation = generation_;
    std::vector<std::uint64_t> tokens;
    for (std::size_t i = 0; i < group.size(); ++i)
    {
        tokens.push_back(++token_);
        active_.insert(token_);
    }
    ++dispatchDepth_;
    for (std::size_t i = 0; i < group.size() && generation == generation_; ++i)
        dispatch_(group[i], [this, generation, token = tokens[i]](bool pass) { complete(generation, token, pass); });
    --dispatchDepth_;
    return generation == generation_ && active_.empty() && !dispatchDepth_;
}
void EventRunner::pump()
{
    if (pumping_ || dispatchDepth_ || !active_.empty())
        return;
    pumping_                        = true;
    unsigned synchronousCompletions = 0;
    while (active_.empty() && !queue_.empty())
    {
        auto group = std::move(queue_.front());
        queue_.pop_front();
        const auto generation = generation_;
        if (dispatchGroup(std::move(group)))
            ++synchronousCompletions;
        if (generation != generation_)
            break;
    }
    pumping_ = false;
    if (active_.empty() && queue_.empty())
        running_ = false;
    // The original advances the linked queue before evaluating conditions.
    // Flatten synchronous completion recursion without changing that ordering.
    for (unsigned i = 0; i < synchronousCompletions; ++i)
    {
        if (afterGroup)
            afterGroup();
    }
}
}  // namespace rb
