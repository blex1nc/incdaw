#pragma once

#include "engine/core/Time.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace incdaw::engine {

/// Decoded audio, planar, float — the engine's native shape.
struct AudioFileData {
    SampleRate                       sampleRate   = 0.0;
    std::size_t                      channelCount = 0;
    FrameCount                       frameCount   = 0;
    std::vector<std::vector<Sample>> channels;    ///< channelCount vectors of frameCount
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

    enum class Format : std::uint16_t { pcm16, pcm24, float32 };

    /// Writes planar float data. Float32 is the default: recordings pass
    /// through gain staging before export, and quantising them at capture would
    /// bake the noise floor in.
    [[nodiscard]] static Result write(const std::filesystem::path& path, const AudioFileData& data,
                                      Format format = Format::float32);
};

} // namespace incdaw::engine
