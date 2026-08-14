#pragma once

#include "engine/audio/WavFile.h"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <vector>

namespace incdaw::engine {

/// Writes a WAV file incrementally — the disk half of recording.
///
/// A recording cannot be buffered whole and written at the end: a take is
/// minutes long, and losing it to a crash in minute five because nothing was
/// on disk yet is unacceptable. This writer streams data as it arrives and
/// patches the two RIFF size fields on finalize, so at any moment the file on
/// disk is at worst one header-patch away from valid.
///
/// Produces byte-identical output to WavFile::write for the same data — both
/// go through the encoders in WavBytes.h, and the round-trip test asserts it.
///
/// Not for the audio thread: append() does file I/O and may grow its scratch
/// buffer. The audio thread hands samples to AudioRecorder's lock-free FIFO;
/// the recorder's writer thread calls this.
class WavStreamWriter {
public:
    WavStreamWriter() = default;

    /// Finalizes on destruction if still open, so a forgotten finalize loses
    /// nothing — but the explicit call is the one that can report an error.
    ~WavStreamWriter();

    WavStreamWriter(const WavStreamWriter&)            = delete;
    WavStreamWriter& operator=(const WavStreamWriter&) = delete;

    /// Creates the file and writes a header with placeholder sizes.
    [[nodiscard]] WavFile::Result open(const std::filesystem::path& path, SampleRate sampleRate,
                                       std::size_t channelCount,
                                       WavFile::Format format = WavFile::Format::float32);

    /// Appends planar audio: `channels[c]` holds `frameCount` samples.
    [[nodiscard]] WavFile::Result append(const Sample* const* channels, std::size_t channelCount,
                                         FrameCount frameCount);

    /// Appends interleaved audio: `frameCount * channelCount` samples, frame by
    /// frame. The shape AudioRecorder's FIFO delivers.
    [[nodiscard]] WavFile::Result appendInterleaved(const Sample* samples, FrameCount frameCount);

    /// Patches the RIFF and data sizes and closes the file.
    [[nodiscard]] WavFile::Result finalize();

    [[nodiscard]] bool       isOpen()     const noexcept { return file_.is_open(); }
    [[nodiscard]] FrameCount frameCount() const noexcept { return framesWritten_; }

private:
    [[nodiscard]] WavFile::Result flushEncoded();

    std::ofstream             file_;
    std::filesystem::path     path_;
    std::vector<std::uint8_t> encoded_;       ///< reused per append; grows once, then stable
    WavFile::Format           format_       = WavFile::Format::float32;
    std::size_t               channelCount_ = 0;
    FrameCount                framesWritten_ = 0;
};

} // namespace incdaw::engine
