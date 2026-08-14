#pragma once

#include "engine/midi/MidiRecorder.h"
#include "project/Model.h"

#include <vector>

namespace incdaw::project {

/// Turns engine-level recorded events into pattern content.
///
/// This conversion lives in `project/` rather than `engine/` because it is the
/// point at which raw performance data becomes editable musical data — where
/// per-note properties (label, probability, pan, fine tune) exist at all. The
/// engine has no business knowing about them.
void appendRecordedEvents(Pattern& pattern, const std::vector<engine::RecordedEvent>& events);

/// Snaps note starts to a grid.
///
/// `strength` in [0,1] moves each note that fraction of the way to the grid —
/// 1.0 is rigid, and anything less preserves some of the performance. A
/// quantiser without strength is the reason quantised parts sound dead.
///
/// Durations are preserved rather than snapped: shortening a note to fit a grid
/// changes the articulation, which is not what the user asked for.
void quantizeNoteStarts(Pattern& pattern, Tick grid, double strength = 1.0);

/// Displaces note starts randomly by up to `maxTicks`, deterministically for a
/// given `seed` so that the result is reproducible and undoable.
void humanizeNoteStarts(Pattern& pattern, Tick maxTicks, std::uint64_t seed);

} // namespace incdaw::project
