#pragma once

#include "engine/audio/WavFile.h"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace incdaw::engine {

/// Random-access WAV reading — the disk half of streamed playback.
///
/// `WavFile::read` loads a whole file, which is right for takes and wrong for
/// an hour of audio. This reader parses the header once, then decodes any
/// frame range on demand: `readAt` seeks, reads exactly the bytes asked for,
/// and decodes through the same `WavBytes.h` decoder `WavFile` uses, so the
/// two can never disagree about a file's contents.
///
/// Not for the audio thread: `readAt` does file I/O by design. The realtime
/// side reads from AudioStream's prefetched window; the DiskStreamer's
/// service thread is what calls this.
class WavStreamReader {
public:
    /// Parses the header (a streaming chunk walk — the file body is not
    /// loaded) and leaves the reader positioned for `readAt`.
    [[nodiscard]] WavFile::Result open(const std::filesystem::path& path);

    void close();

    [[nodiscard]] bool isOpen() const noexcept { return file_.is_open(); }

    [[nodiscard]] SampleRate  sampleRate()   const noexcept { return sampleRate_; }
    [[nodiscard]] std::size_t channelCount() const noexcept { return channelCount_; }
    [[nodiscard]] FrameCount  frameCount()   const noexcept { return frameCount_; }

    /// Decodes `frameCount` frames starting at `firstFrame` into planar
    /// `channels` (each `frameCount` samples). Ranges beyond the file are
    /// zero-filled — the caller asked about time that has no audio, which is
    /// silence, not an error. Returns false only on an I/O failure.
    [[nodiscard]] bool readAt(FrameCount firstFrame, FrameCount frameCount,
                              Sample* const* channels, std::size_t channelCountOut);

private:
    std::ifstream             file_;
    std::filesystem::path     path_;
    std::vector<std::uint8_t> raw_;   ///< encoded bytes for one readAt, reused

    SampleRate    sampleRate_    = 0.0;
    std::size_t   channelCount_  = 0;
    FrameCount    frameCount_    = 0;
    std::uint64_t dataOffset_    = 0;
    std::uint16_t format_        = 0;
    std::uint16_t bitsPerSample_ = 0;
};

} // namespace incdaw::engine
