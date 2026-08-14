#include "engine/transport/TempoMap.h"

#include <algorithm>
#include <cmath>

namespace incdaw::engine {
namespace {

/// Tempo bounds. Outside this range the frames-per-tick figure stops being
/// meaningful and a zero or negative value would divide by zero downstream.
constexpr double minimumTempo = 1.0;
constexpr double maximumTempo = 999.0;

double clampTempo(double beatsPerMinute) noexcept
{
    if (!(beatsPerMinute > 0.0))      // also catches NaN
        return 120.0;

    return std::clamp(beatsPerMinute, minimumTempo, maximumTempo);
}

} // namespace

TempoMap::TempoMap(double beatsPerMinute, SampleRate rate)
    : tempoEvents_{{0, clampTempo(beatsPerMinute)}},
      signatureEvents_{{0, TimeSignature{4, 4}}},
      sampleRate_(rate > 0.0 ? rate : 48000.0)
{
    rebuild();
}

void TempoMap::setTempoEvents(std::vector<TempoEvent> events)
{
    std::sort(events.begin(), events.end(),
              [](const TempoEvent& a, const TempoEvent& b) { return a.tick < b.tick; });

    // Negative positions are not musical time; drop them rather than letting
    // them produce a segment that starts before the song.
    events.erase(std::remove_if(events.begin(), events.end(),
                                [](const TempoEvent& event) { return event.tick < 0; }),
                 events.end());

    // Later events win at the same tick — the user's most recent edit.
    events.erase(std::unique(events.begin(), events.end(),
                             [](const TempoEvent& a, const TempoEvent& b) { return a.tick == b.tick; }),
                 events.end());

    for (TempoEvent& event : events)
        event.beatsPerMinute = clampTempo(event.beatsPerMinute);

    // The map must be total: a query before the first event has to have an
    // answer, so tick 0 always carries one.
    if (events.empty() || events.front().tick != 0)
        events.insert(events.begin(), TempoEvent{0, events.empty() ? 120.0 : events.front().beatsPerMinute});

    tempoEvents_ = std::move(events);
    rebuild();
}

void TempoMap::setTimeSignatureEvents(std::vector<TimeSignatureEvent> events)
{
    std::sort(events.begin(), events.end(),
              [](const TimeSignatureEvent& a, const TimeSignatureEvent& b) { return a.tick < b.tick; });

    events.erase(std::remove_if(events.begin(), events.end(),
                                [](const TimeSignatureEvent& event) {
                                    return event.tick < 0 || !event.signature.isValid();
                                }),
                 events.end());

    events.erase(std::unique(events.begin(), events.end(),
                             [](const TimeSignatureEvent& a, const TimeSignatureEvent& b) {
                                 return a.tick == b.tick;
                             }),
                 events.end());

    if (events.empty() || events.front().tick != 0)
        events.insert(events.begin(), TimeSignatureEvent{0, TimeSignature{4, 4}});

    signatureEvents_ = std::move(events);
}

void TempoMap::setSampleRate(SampleRate rate)
{
    if (rate > 0.0 && rate != sampleRate_) {
        sampleRate_ = rate;
        rebuild();
    }
}

void TempoMap::rebuild()
{
    segments_.clear();
    segments_.reserve(tempoEvents_.size());

    FramePosition frame = 0;

    for (std::size_t index = 0; index < tempoEvents_.size(); ++index) {
        const TempoEvent& event = tempoEvents_[index];

        Segment segment;
        segment.startTick      = event.tick;
        segment.beatsPerMinute = event.beatsPerMinute;

        // seconds per tick = (60 / bpm) / ticksPerQuarterNote
        segment.framesPerTick = (60.0 / event.beatsPerMinute)
                              / static_cast<double>(ticksPerQuarterNote) * sampleRate_;

        if (index == 0) {
            segment.startFrame = 0;
        } else {
            const Segment&   previous = segments_.back();
            const Tick       span     = event.tick - previous.startTick;
            const double     frames   = static_cast<double>(span) * previous.framesPerTick;

            // Rounded, not truncated: truncation at every tempo change would
            // accumulate a backward drift across a song with many of them.
            frame += static_cast<FramePosition>(frames + 0.5);
            segment.startFrame = frame;
        }

        segments_.push_back(segment);
    }
}

const TempoMap::Segment& TempoMap::segmentForTick(Tick tick) const noexcept
{
    // Upper bound then step back: the segment that governs `tick` is the last
    // one starting at or before it.
    const auto next = std::upper_bound(segments_.begin(), segments_.end(), tick,
                                       [](Tick value, const Segment& segment) {
                                           return value < segment.startTick;
                                       });

    return next == segments_.begin() ? segments_.front() : *(next - 1);
}

const TempoMap::Segment& TempoMap::segmentForFrame(FramePosition frame) const noexcept
{
    const auto next = std::upper_bound(segments_.begin(), segments_.end(), frame,
                                       [](FramePosition value, const Segment& segment) {
                                           return value < segment.startFrame;
                                       });

    return next == segments_.begin() ? segments_.front() : *(next - 1);
}

double TempoMap::tempoAtTick(Tick tick) const noexcept
{
    return segmentForTick(tick).beatsPerMinute;
}

double TempoMap::tempoAtFrame(FramePosition frame) const noexcept
{
    return segmentForFrame(frame).beatsPerMinute;
}

TimeSignature TempoMap::timeSignatureAtTick(Tick tick) const noexcept
{
    if (signatureEvents_.empty())
        return TimeSignature{4, 4};

    const auto next = std::upper_bound(signatureEvents_.begin(), signatureEvents_.end(), tick,
                                       [](Tick value, const TimeSignatureEvent& event) {
                                           return value < event.tick;
                                       });

    return next == signatureEvents_.begin() ? signatureEvents_.front().signature : (next - 1)->signature;
}

FramePosition TempoMap::frameForTick(Tick tick) const noexcept
{
    const Segment& segment = segmentForTick(tick);
    const double   frames  = static_cast<double>(tick - segment.startTick) * segment.framesPerTick;

    return segment.startFrame + static_cast<FramePosition>(frames >= 0.0 ? frames + 0.5 : frames - 0.5);
}

Tick TempoMap::tickForFrame(FramePosition frame) const noexcept
{
    const Segment& segment = segmentForFrame(frame);

    if (segment.framesPerTick <= 0.0)
        return segment.startTick;

    const double ticks = static_cast<double>(frame - segment.startFrame) / segment.framesPerTick;

    return segment.startTick + static_cast<Tick>(ticks >= 0.0 ? ticks + 0.5 : ticks - 0.5);
}

MusicalPosition TempoMap::musicalPositionForTick(Tick tick) const noexcept
{
    // Walk the signature changes so that bar numbering stays correct across
    // them: a 7/8 section does not have the same bar length as the 4/4 before it.
    MusicalPosition position;
    position.bar = 1;

    Tick          cursor    = 0;
    std::int64_t  barNumber = 0;

    for (std::size_t index = 0; index < signatureEvents_.size(); ++index) {
        const TimeSignature signature = signatureEvents_[index].signature;
        const Tick          barTicks  = signature.ticksPerBar();

        const Tick sectionEnd = (index + 1 < signatureEvents_.size())
                                    ? signatureEvents_[index + 1].tick
                                    : tick;

        if (barTicks <= 0)
            continue;

        if (tick < sectionEnd || index + 1 == signatureEvents_.size()) {
            const Tick into = tick - cursor;
            const auto local = ticksToMusicalPosition(into, signature);

            position.bar  = barNumber + local.bar;
            position.beat = local.beat;
            position.tick = local.tick;
            return position;
        }

        const Tick sectionTicks = sectionEnd - cursor;
        barNumber += sectionTicks / barTicks;
        cursor = sectionEnd;
    }

    return position;
}

FramePosition TempoMap::nextTempoChangeAfter(FramePosition frame) const noexcept
{
    for (const Segment& segment : segments_)
        if (segment.startFrame > frame)
            return segment.startFrame;

    return noTempoChange;
}

} // namespace incdaw::engine
