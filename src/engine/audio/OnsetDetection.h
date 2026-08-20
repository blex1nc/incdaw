#pragma once

#include "engine/audio/WavFile.h"

#include <vector>

namespace incdaw::engine::audio {

/// Attack positions in decoded audio — what a slicer cuts at.
///
/// Energy-based: a short block whose energy leaps past its predecessor marks
/// an onset, refined to the first frame inside the block that carries real
/// level, with a minimum gap so one drum hit is one slice. `sensitivity`
/// scales the required leap: 1 is conservative (clean drum loops), larger
/// values cut softer attacks too.
[[nodiscard]] std::vector<FrameCount> detectOnsets(const AudioFileData& audio,
                                                   double sensitivity = 1.0);

} // namespace incdaw::engine::audio
