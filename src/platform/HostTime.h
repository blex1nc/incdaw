#pragma once

#include <cstdint>

namespace incdaw::platform {

/// The system's monotonic high-resolution clock, in nanoseconds.
///
/// This is the common currency between the audio callback and MIDI input. Both
/// CoreAudio and CoreMIDI timestamp in mach absolute time; converting to
/// nanoseconds here — once, in the platform layer — means the engine can relate
/// a MIDI event to a sample position without knowing what a mach timebase is.
///
/// Getting this wrong is the difference between MIDI that lands on the beat and
/// MIDI that lands somewhere near it (docs/REQUIREMENTS.md §1.1).
[[nodiscard]] std::uint64_t hostTimeNowNanos() noexcept;

/// Converts a platform-native host timestamp to nanoseconds.
[[nodiscard]] std::uint64_t hostTimeToNanos(std::uint64_t hostTime) noexcept;

/// The inverse: nanoseconds back to the platform's native tick count.
///
/// Needed by anything that hands a *future* time to the system rather than
/// reading a past one — MIDI output schedules a message for the host time at
/// which its frame will be heard, and CoreMIDI wants that in mach ticks. On
/// Apple silicon the two units differ, so passing nanoseconds through
/// unconverted schedules every message at the wrong moment.
[[nodiscard]] std::uint64_t nanosToHostTime(std::uint64_t nanos) noexcept;

} // namespace incdaw::platform
