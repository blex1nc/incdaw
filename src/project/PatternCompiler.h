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
/// where the musical decisions belong that the engine has no business making —
/// swing, polymetric repeats, and note probability are all resolved here.
///
/// Everything in this file is deterministic for a given seed. A pattern that
/// sounded one way on playback and another on export would be unusable;
/// deterministic compilation is what lets the offline render match what was
/// heard.

/// One channel's notes for one pattern, in ticks relative to the pattern start.
///
/// Resolves, in this order: polymetric repeats, swing, probability. Swing is
/// applied after the repeats because it follows the pattern's grid, not the
/// channel's loop — a channel looping every three sixteenths still swings on
/// the pattern's off-beats.
[[nodiscard]] std::vector<engine::SequencedNote> compilePattern(const Pattern& pattern,
                                                                EntityId      channel,
                                                                std::uint64_t randomSeed = 0);

/// Every note a channel plays across the arrangement, in absolute ticks.
///
/// Walks the project's pattern clips. A clip shorter than its pattern trims it;
/// a clip longer repeats it — which is what dragging a pattern clip out does in
/// every DAW that has one. The pattern itself is never copied, so editing it
/// changes every placement.
[[nodiscard]] std::vector<engine::SequencedNote> compileArrangement(const Project& project,
                                                                    EntityId       channel,
                                                                    std::uint64_t  randomSeed = 0);

/// Compiles into an existing sequence, replacing its contents.
void compilePatternInto(engine::NoteSequence& sequence, const Pattern& pattern,
                        EntityId channel, std::uint64_t randomSeed = 0);

void compileArrangementInto(engine::NoteSequence& sequence, const Project& project,
                            EntityId channel, std::uint64_t randomSeed = 0);

/// Last tick any pattern clip reaches. Zero when the arrangement is empty.
[[nodiscard]] Tick arrangementLengthTicks(const Project& project);

} // namespace incdaw::project
