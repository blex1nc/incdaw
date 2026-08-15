#pragma once

#include "engine/core/Time.h"
#include "engine/transport/TempoMap.h"

#include <filesystem>
#include <string>
#include <vector>

namespace incdaw::engine {

/// One note in a Standard MIDI File track, in the engine's own terms.
struct SmfNote {
    Tick startTick   = 0;
    Tick lengthTicks = 0;
    int  channel     = 0;
    int  key         = 60;
    int  velocity    = 100;
};

struct SmfTrack {
    std::string          name;
    std::vector<SmfNote> notes;
};

/// A Standard MIDI File as INCDAW sees one: a tempo map and note tracks.
///
/// Ticks are INCDAW's own resolution; the reader rescales whatever division
/// the file declares, so a document round-trips through any PPQN.
struct SmfDocument {
    std::vector<TempoEvent>         tempo;
    std::vector<TimeSignatureEvent> timeSignatures;
    std::vector<SmfTrack>           tracks;
};

/// Reads and writes Standard MIDI Files (format 1).
///
/// Writing: track 0 carries the tempo map, one MTrk per note track. Reading:
/// note on/off pairs become notes (running status handled, note-on velocity
/// 0 treated as note-off, as the spec demands); unknown events are skipped
/// by their declared length. CC/pitch-bend import is future work — recorded,
/// not silently half-done.
class SmfFile {
public:
    struct Result {
        bool        succeeded = false;
        std::string error;

        explicit operator bool() const noexcept { return succeeded; }
    };

    [[nodiscard]] static Result write(const std::filesystem::path& path,
                                      const SmfDocument& document);

    [[nodiscard]] static Result read(const std::filesystem::path& path, SmfDocument& document);
};

} // namespace incdaw::engine
