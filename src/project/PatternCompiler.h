#pragma once

#include "engine/midi/NoteSequence.h"
#include "project/Model.h"

#include <vector>

namespace incdaw::project {

/// Compiles a pattern into the form the audio engine plays.
///
/// The direction of this conversion matters: `engine/` sits below `project/`
/// and cannot see a Pattern, so the translation has to live here. It is also
/// where musical decisions belong that the engine has no business making —
/// note probability, for one.
///
/// `randomSeed` makes probability reproducible. A pattern that sounded one way
/// on playback and another on export would be unusable; deterministic
/// evaluation is what lets the offline render match what was heard.
[[nodiscard]] std::vector<engine::SequencedNote> compilePattern(const Pattern& pattern,
                                                                std::uint64_t randomSeed = 0);

/// Compiles a pattern into an existing sequence, replacing its contents.
void compilePatternInto(engine::NoteSequence& sequence, const Pattern& pattern,
                        std::uint64_t randomSeed = 0);

} // namespace incdaw::project
