#pragma once

#include <cstdint>

namespace incdaw::engine {

/// Audio sample format used throughout the engine. Single precision is the
/// industry norm for realtime paths; the mixer accumulates in the same type to
/// keep the realtime and offline paths bit-identical (docs/AUDIO_ENGINE.md §9).
using Sample = float;

/// A count of sample frames. Signed so that differences and rewinds are
/// expressible without wrapping. 64-bit because 32 bits overflows after ~13
/// hours at 48 kHz, which is well within a long session.
using FrameCount = std::int64_t;

/// An absolute position on the timeline, in sample frames from song start.
using FramePosition = std::int64_t;

/// Sample rate in Hz. Double because 44100/1.0001 pull-up rates exist and
/// accumulate error badly in single precision.
using SampleRate = double;

/// Musical position in pulses-per-quarter-note ticks.
using Tick = std::int64_t;

/// Ticks per quarter note used by INCDAW's internal musical timeline.
///
/// 960 is divisible by 2, 3, 4, 5, 6, 8, 10, 12, 15, 16, 20, 24, 30, 32, 64 and
/// 96, so every common note division and tuplet lands on an exact integer tick.
/// It is also the de-facto standard for high-resolution MIDI files, which keeps
/// import and export lossless.
inline constexpr Tick ticksPerQuarterNote = 960;

// ── Frame ↔ seconds ──────────────────────────────────────────────────────────

[[nodiscard]] constexpr double framesToSeconds(FrameCount frames, SampleRate rate) noexcept
{
    return rate > 0.0 ? static_cast<double>(frames) / rate : 0.0;
}

/// Rounds to nearest rather than truncating: truncation accumulates a
/// systematic backward drift when converting many independent positions.
[[nodiscard]] constexpr FrameCount secondsToFrames(double seconds, SampleRate rate) noexcept
{
    const double frames = seconds * rate;
    return static_cast<FrameCount>(frames >= 0.0 ? frames + 0.5 : frames - 0.5);
}

// ── Ticks ↔ frames at a fixed tempo ──────────────────────────────────────────
//
// These assume a constant tempo. Tempo-map-aware conversion arrives with the
// transport in Phase 3; these are the primitives it will be built from.

[[nodiscard]] constexpr double ticksToSeconds(Tick ticks, double beatsPerMinute) noexcept
{
    if (beatsPerMinute <= 0.0)
        return 0.0;

    const double quarterNotes = static_cast<double>(ticks) / static_cast<double>(ticksPerQuarterNote);
    return quarterNotes * (60.0 / beatsPerMinute);
}

[[nodiscard]] constexpr Tick secondsToTicks(double seconds, double beatsPerMinute) noexcept
{
    if (beatsPerMinute <= 0.0)
        return 0;

    const double quarterNotes = seconds * (beatsPerMinute / 60.0);
    const double ticks        = quarterNotes * static_cast<double>(ticksPerQuarterNote);
    return static_cast<Tick>(ticks >= 0.0 ? ticks + 0.5 : ticks - 0.5);
}

// ── Time signature ───────────────────────────────────────────────────────────

struct TimeSignature {
    int numerator   = 4;   ///< beats per bar
    int denominator = 4;   ///< note value that gets one beat (4 = quarter)

    [[nodiscard]] constexpr bool isValid() const noexcept
    {
        // The denominator must be a power of two: it names a note value
        // (1, 2, 4, 8, 16...), not an arbitrary division.
        return numerator > 0 && denominator > 0 && (denominator & (denominator - 1)) == 0;
    }

    [[nodiscard]] constexpr Tick ticksPerBeat() const noexcept
    {
        return denominator > 0 ? ticksPerQuarterNote * 4 / denominator : 0;
    }

    [[nodiscard]] constexpr Tick ticksPerBar() const noexcept
    {
        return ticksPerBeat() * numerator;
    }

    [[nodiscard]] friend constexpr bool operator==(TimeSignature, TimeSignature) noexcept = default;
};

/// A musical position, decomposed for display. Bars and beats are 1-based
/// because that is what every musician and every DAW ruler shows.
struct MusicalPosition {
    std::int64_t bar  = 1;
    int          beat = 1;
    Tick         tick = 0;

    [[nodiscard]] friend constexpr bool operator==(MusicalPosition, MusicalPosition) noexcept = default;
};

[[nodiscard]] MusicalPosition ticksToMusicalPosition(Tick ticks, TimeSignature signature) noexcept;
[[nodiscard]] Tick            musicalPositionToTicks(MusicalPosition position, TimeSignature signature) noexcept;

} // namespace incdaw::engine
