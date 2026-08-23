#pragma once

#include "project/Model.h"
#include "project/Json.h"

#include <filesystem>
#include <string>
#include <vector>

namespace incdaw::project {

/// The format version written by this build.
///
/// CLAUDE.md §2 is absolute: never create an unversioned project format. This
/// is stamped into the very first file INCDAW ever saves.
inline constexpr int projectFormatMajor = 1;

/// 1.1 (Phase 8a): a pattern's notes are stored per channel rather than as one
/// flat list, and pattern clips are placed in ticks rather than frames. Both
/// are read back from 1.0 files — see `ProjectFile::migrate`.
///
/// 1.2 (Phase 8b): a channel carries the pitch its step sequencer steps are
/// written at. A 1.1 file simply has no such field and reads back as middle C,
/// which is what those projects meant.
///
/// 1.3 (Phase 14): a channel carries sampler zones — the mapped samples the
/// builtin sampler plays, each naming an audio asset by id. Additive: earlier
/// files have none and read back with an empty program.
///
/// 1.4 (Phase 16): the project carries MIDI controller mappings — hardware
/// controls bound to parameters by registry key. Additive: earlier files
/// have none.
///
/// 1.5 (UI build-out increment 4): a channel carries stored instrument
/// parameter values, applied through the instrument's sink at every compile
/// (docs/DECISIONS.md D-034). Additive: earlier files have none and their
/// instruments play at catalogue defaults, which is what they always did.
///
/// 1.6 (FL2026 P3/P5): the project carries timeline markers and regions —
/// named musical positions with an optional length — and a mixer node carries
/// stereo separation. Additive: earlier files have neither and read back with
/// no markers and untouched width.
///
/// 1.7 (TRACK B / B4): a track carries whether it is collapsed. `TrackType`
/// has always serialized as an integer and `Track::parent` has always been
/// written, so folder tracks themselves need no conversion — a 1.6 file simply
/// has no `collapsed` field and reads back with every folder open, which is
/// the state those projects were last seen in.
///
/// 1.8 (TRACK B / B5): a clip carries the group it belongs to. Additive: a 1.7
/// file has no `group` and reads back with every clip on its own, which is
/// what those projects had.
///
/// 1.9 (TRACK B / B6): a clip carries which lane of its track it occupies. A
/// track's lane count is derived from its clips, so nothing else moved.
/// Additive: a 1.8 file has no `lane` and reads back on lane zero, which is
/// the single lane those projects had.
///
/// 1.10 (TRACK B / B7): a clip carries whether each of its edges crossfades
/// with the clip it overlaps. The fade lengths stay derived from the overlap,
/// so nothing else moved. Additive: a 1.9 file has neither flag and plays the
/// manual fades it always did.
///
/// 1.11 (TRACK B / B11): a project holds several arrangements — timelines that
/// share the patterns, channels, tracks, mixer and automation — and the clips
/// and markers live inside them. A 1.10 document has one timeline written at
/// the top level; the reader wraps it into a single arrangement named
/// "Arrangement 1", which is the whole of that migration.
inline constexpr int projectFormatMinor = 11;

[[nodiscard]] std::string projectFormatVersionString();

/// Reads and writes `.incdaw` packages (docs/PROJECT_FORMAT.md).
///
/// A package is a directory, not a single opaque file: partial saves, streaming
/// loads, and per-file corruption resilience all follow from that, and a
/// corrupted pattern costs one pattern rather than the session.
class ProjectFile {
public:
    struct Result {
        bool        succeeded = false;
        std::string error;

        /// Set when a project was written by an older format and upgraded on
        /// load. The original is preserved in `history/` first.
        bool        migrated = false;
        std::string migratedFrom;

        explicit operator bool() const noexcept { return succeeded; }
    };

    /// Writes `project` to `path`, creating the package if needed.
    ///
    /// Writes are staged and then moved into place, so an interrupted save
    /// leaves the previous version intact rather than a half-written one.
    [[nodiscard]] static Result save(const Project& project, const std::filesystem::path& path);

    [[nodiscard]] static Result load(Project& project, const std::filesystem::path& path);

    /// True if `path` looks like an INCDAW package.
    [[nodiscard]] static bool isProjectPackage(const std::filesystem::path& path);

    /// Version stamped in a package's manifest, or empty if unreadable.
    [[nodiscard]] static std::string versionOf(const std::filesystem::path& path);

private:
    [[nodiscard]] static Result migrate(Json& document, int major, int minor);
};

} // namespace incdaw::project
