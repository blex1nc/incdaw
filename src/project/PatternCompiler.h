#pragma once

#include "engine/midi/NoteSequence.h"
#include "project/Model.h"

#include <cstdint>
#include <vector>

namespace incdaw::project {

/// Compiles patterns into the form the audio engine plays.
///
/// The direction of this conversion matters: `engine/` sits below `project/`
/// and cannot see a Pattern, so the translation has to live here. It is also
/// where musical decisions belong that the engine has no business making —
/// note probability, swing and polymetric channel lengths, for three.
///
/// `randomSeed` makes probability reproducible. A pattern that sounded one way
/// on playback and another on export would be unusable; deterministic
/// evaluation is what lets the offline render match what was heard, and it is
/// also what makes the *same* pattern placed twice in an arrangement play
/// identically at both placements.
struct PatternCompileOptions {
    /// Compile only the notes belonging to this channel. Invalid means every
    /// note in the pattern regardless of channel.
    EntityId      channel;

    /// The channel a note with no channel id of its own belongs to. Notes
    /// written before the project had channels resolve here.
    EntityId      defaultChannel;

    std::uint64_t randomSeed = 0;

    /// Where the compiled notes are placed on the timeline, in ticks.
    Tick          startTick  = 0;

    /// Ticks of the pattern to emit from `startTick`. 0 means one pattern
    /// length. A longer span repeats the pattern, which is what a pattern clip
    /// stretched over several bars does.
    Tick          span       = 0;

    /// Ticks into the pattern where playback begins.
    Tick          sourceOffset = 0;

    /// Confine the result to the span: notes starting past the end are dropped
    /// and a note crossing it is cut.
    ///
    /// Off by default, because a pattern's length is a *loop marker*, not a
    /// guillotine — the transport wraps at it, and a note the user dragged past
    /// the end is still their note. A clip in the arrangement is the opposite
    /// case: it owns a fixed span of the timeline, and content bleeding out of
    /// it would overlap whatever comes next. Repeating a pattern within a
    /// longer span implies this, since a note running past the repeat point
    /// would collide with its own next repetition.
    bool          bounded = false;
};

[[nodiscard]] std::vector<engine::SequencedNote> compilePattern(const Pattern& pattern,
                                                                const PatternCompileOptions& options);

/// Compiles every note in a pattern, ignoring channels. Kept because a
/// single-channel project and most tests have no use for the distinction.
[[nodiscard]] std::vector<engine::SequencedNote> compilePattern(const Pattern& pattern,
                                                                std::uint64_t randomSeed = 0);

/// Compiles a pattern into an existing sequence, replacing its contents.
void compilePatternInto(engine::NoteSequence& sequence, const Pattern& pattern,
                        std::uint64_t randomSeed = 0);

void compilePatternInto(engine::NoteSequence& sequence, const Pattern& pattern,
                        const PatternCompileOptions& options);

/// Flattens every pattern clip in the arrangement into one absolute-tick
/// sequence for one channel — what song mode plays.
///
/// Clips carry frame positions, so this needs the tempo map to place them.
/// Every placement of a given pattern is compiled with the same seed, which is
/// what makes them sound the same.
[[nodiscard]] std::vector<engine::SequencedNote> compileArrangement(const Project& project,
                                                                    EntityId channel,
                                                                    std::uint64_t randomSeed = 0);

void compileArrangementInto(engine::NoteSequence& sequence, const Project& project,
                            EntityId channel, std::uint64_t randomSeed = 0);

/// End of the last pattern clip in the arrangement, in ticks. 0 when the
/// arrangement is empty.
[[nodiscard]] Tick arrangementLengthTicks(const Project& project);

} // namespace incdaw::project
