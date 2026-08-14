#include "app/AutomationWriteSession.h"

#include "app/commands/AutomationCommands.h"

#include <cmath>

namespace incdaw::app {
namespace {

/// A point survives thinning only when the envelope actually bends there: if
/// it sits within this of the straight line from the last kept point to its
/// successor, the two segments would play identically without it. A hand on
/// a fader emits hundreds of points; a ramp needs two.
constexpr double thinningDeviationEpsilon = 0.002;

std::vector<project::AutomationPoint> thin(const std::vector<project::AutomationPoint>& raw)
{
    if (raw.size() <= 2)
        return raw;

    std::vector<project::AutomationPoint> kept;
    kept.push_back(raw.front());

    for (std::size_t index = 1; index + 1 < raw.size(); ++index) {
        const project::AutomationPoint& previous = kept.back();
        const project::AutomationPoint& point    = raw[index];
        const project::AutomationPoint& next     = raw[index + 1];

        const double span = static_cast<double>(next.tick - previous.tick);
        const double mix  = span > 0.0
                                ? static_cast<double>(point.tick - previous.tick) / span
                                : 1.0;
        const double onLine = previous.value + (next.value - previous.value) * mix;

        if (std::abs(point.value - onLine) > thinningDeviationEpsilon)
            kept.push_back(point);
    }

    kept.push_back(raw.back());
    return kept;
}

} // namespace

void AutomationWriteSession::capture(project::EntityId target, const std::string& parameterKey,
                                     project::Tick tick, double normalizedValue)
{
    if (!enabled_)
        return;

    Stream* stream = nullptr;
    for (Stream& candidate : streams_)
        if (candidate.target == target && candidate.key == parameterKey)
            stream = &candidate;

    if (stream == nullptr) {
        streams_.push_back({target, parameterKey, {}});
        stream = &streams_.back();
    }

    project::AutomationPoint point;
    point.tick  = tick;
    point.value = normalizedValue;

    // The transport can hand out the same tick for two UI events, and a loop
    // wrap can hand out an earlier one. Same tick: the later value wins.
    // Earlier tick: the pass wrapped — keep it simple and honest by starting
    // the stream over from the wrap; loop-aware overdub is latch-mode work.
    if (!stream->points.empty() && tick < stream->points.back().tick)
        stream->points.clear();

    if (!stream->points.empty() && tick == stream->points.back().tick)
        stream->points.back() = point;
    else
        stream->points.push_back(point);
}

std::vector<std::unique_ptr<Command>> AutomationWriteSession::finish()
{
    std::vector<std::unique_ptr<Command>> commands;

    for (Stream& stream : streams_) {
        if (stream.points.empty())
            continue;

        commands.push_back(std::make_unique<WriteAutomationCommand>(
            stream.target, stream.key, thin(stream.points)));
    }

    streams_.clear();
    return commands;
}

} // namespace incdaw::app
