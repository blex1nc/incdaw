#pragma once

#include "engine/core/Time.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace incdaw::engine {

/// A named point or span inside a file.
///
/// Stored IN the audio file, as RIFF's own `cue `/`adtl` chunks, rather than
/// in the project. Three reasons, and the third is the one that matters most:
/// a marker belongs to the sound, not to the song it happens to be used in;
/// every other audio editor reads and writes them there, so a file marked up
/// in INCDAW opens marked up elsewhere; and INCDAW's own edit commands used to
/// destroy them — read, edit, write, and every cue another application had
/// written was silently gone, because the reader skipped the chunk and the
/// writer never emitted it.
struct AudioMarker {
    std::string   name;
    FramePosition start = 0;

    /// Zero is a point; anything else is a region [start, start + length).
    FrameCount length = 0;

    [[nodiscard]] bool isRegion() const noexcept { return length > 0; }
    [[nodiscard]] FramePosition end() const noexcept { return start + length; }

    [[nodiscard]] friend bool operator==(const AudioMarker&, const AudioMarker&) = default;
};

/// Decoded audio, planar, float — the engine's native shape.
struct AudioFileData {
    SampleRate                       sampleRate   = 0.0;
    std::size_t                      channelCount = 0;
    FrameCount                       frameCount   = 0;
    std::vector<std::vector<Sample>> channels;    ///< channelCount vectors of frameCount

    /// Sorted by `start`. Kept coherent by the edit verbs: audio inserted
    /// before a marker pushes it later, audio removed under one takes it with
    /// it (engine/audio/AudioEdits.h).
    std::vector<AudioMarker> markers;
};

/// Reads and writes RIFF/WAVE files.
///
/// This is the gate for Phase 9b (audio clips), 11b (automation recording) and
/// the audio editor: an AudioAsset has been serializable since Phase 4 with
/// nothing able to open the file it names. WAV first because it is the format
/// recordings are born in — lossless, seekable, and writable incrementally.
/// AIFF/FLAC/MP3 are readers to add behind the same AudioFileData, not reasons
/// to design an abstraction before there are two of anything.
///
/// Supports PCM 16/24/32-bit and IEEE float 32; mono to arbitrary channel
/// counts; walks chunks properly, so files with LIST/bext/junk chunks load.
/// Never called from the audio thread — this is asset I/O, it allocates and
/// blocks by design. Streaming from disk is a separate subsystem built on top.
class WavFile {
public:
    struct Result {
        bool        succeeded = false;
        std::string error;

        explicit operator bool() const noexcept { return succeeded; }
    };

    /// Reads the whole file into memory.
    [[nodiscard]] static Result read(const std::filesystem::path& path, AudioFileData& out);

    /// Header-only probe: fills everything except `channels`. What the browser
    /// and the relinker need — content identity without decoding minutes of
    /// audio.
    [[nodiscard]] static Result probe(const std::filesystem::path& path, AudioFileData& out);

    /// The file's markers, without decoding a sample.
    ///
    /// Seeks chunk to chunk and reads only the metadata bodies. `probe` and
    /// `read` both pull the whole file into memory, which for an hour of audio
    /// is hundreds of megabytes to answer a question about a few dozen bytes —
    /// and the editor asks it on every reload.
    [[nodiscard]] static Result readMarkers(const std::filesystem::path& path,
                                            std::vector<AudioMarker>&    out);

    enum class Format : std::uint16_t { pcm16, pcm24, float32 };

    /// Writes planar float data. Float32 is the default: recordings pass
    /// through gain staging before export, and quantising them at capture would
    /// bake the noise floor in.
    [[nodiscard]] static Result write(const std::filesystem::path& path, const AudioFileData& data,
                                      Format format = Format::float32);
};

} // namespace incdaw::engine
