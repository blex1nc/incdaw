#pragma once

#include "engine/core/Time.h"

#include <cstddef>
#include <vector>

namespace incdaw::engine {

/// A tempo change at a musical position.
struct TempoEvent {
    Tick   tick           = 0;
    double beatsPerMinute = 120.0;

    [[nodiscard]] friend bool operator==(const TempoEvent&, const TempoEvent&) = default;
};

/// A time-signature change at a musical position.
struct TimeSignatureEvent {
    Tick          tick      = 0;
    TimeSignature signature = {4, 4};

    [[nodiscard]] friend bool operator==(const TimeSignatureEvent&, const TimeSignatureEvent&) = default;
};

/// Converts between musical time and sample frames across tempo changes.
///
/// The map is built on a non-realtime thread and then only read, so lookups are
/// branch-light binary searches with no allocation (docs/AUDIO_ENGINE.md §1).
///
/// Frame positions for each segment boundary are precomputed at build time
/// rather than integrated per lookup. Integrating on the fly would make
/// `frameForTick` cost O(number of tempo changes) and — worse — make the answer
/// depend on how you got there, which is exactly how a DAW ends up with a
/// playhead that disagrees with its own export.
class TempoMap {
public:
    /// A constant-tempo map. The common case, and the starting state of every
    /// new project.
    explicit TempoMap(double beatsPerMinute = 120.0, SampleRate sampleRate = 48000.0);

    /// Replaces the tempo events. They are sorted and de-duplicated; an event
    /// at tick 0 is synthesised if absent, so the map is always total.
    void setTempoEvents(std::vector<TempoEvent> events);
    void setTimeSignatureEvents(std::vector<TimeSignatureEvent> events);

    /// Rebuilds the frame table. Must be called after the sample rate changes,
    /// and is called automatically by the setters.
    void setSampleRate(SampleRate sampleRate);

    [[nodiscard]] SampleRate sampleRate() const noexcept { return sampleRate_; }

    [[nodiscard]] const std::vector<TempoEvent>&         tempoEvents()         const noexcept { return tempoEvents_; }
    [[nodiscard]] const std::vector<TimeSignatureEvent>& timeSignatureEvents() const noexcept { return signatureEvents_; }

    // ── Realtime-safe queries ────────────────────────────────────────────────

    [[nodiscard]] double tempoAtTick(Tick tick) const noexcept;
    [[nodiscard]] double tempoAtFrame(FramePosition frame) const noexcept;

    [[nodiscard]] TimeSignature timeSignatureAtTick(Tick tick) const noexcept;

    /// Exact frame position of a musical position.
    [[nodiscard]] FramePosition frameForTick(Tick tick) const noexcept;

    /// Musical position of a frame. Inverse of `frameForTick` to within one
    /// tick — the round-trip is exact at every segment boundary.
    [[nodiscard]] Tick tickForFrame(FramePosition frame) const noexcept;

    /// Musical position decomposed for a ruler, honouring signature changes.
    [[nodiscard]] MusicalPosition musicalPositionForTick(Tick tick) const noexcept;

    /// First tempo change strictly after `frame`, or `noTempoChange` if none.
    ///
    /// Used to split a processing block so that a tempo change takes effect on
    /// the exact frame it is written on, rather than at the next block boundary
    /// (docs/AUDIO_ENGINE.md §6).
    static constexpr FramePosition noTempoChange = -1;
    [[nodiscard]] FramePosition nextTempoChangeAfter(FramePosition frame) const noexcept;

    [[nodiscard]] std::size_t segmentCount() const noexcept { return segments_.size(); }

private:
    struct Segment {
        Tick          startTick     = 0;
        FramePosition startFrame    = 0;
        double        framesPerTick = 0.0;
        double        beatsPerMinute = 120.0;
    };

    void rebuild();

    [[nodiscard]] const Segment& segmentForTick(Tick tick) const noexcept;
    [[nodiscard]] const Segment& segmentForFrame(FramePosition frame) const noexcept;

    std::vector<TempoEvent>         tempoEvents_;
    std::vector<TimeSignatureEvent> signatureEvents_;
    std::vector<Segment>            segments_;
    SampleRate                      sampleRate_ = 48000.0;
};

} // namespace incdaw::engine
