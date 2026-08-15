#pragma once

#include "engine/audio/WavFile.h"

#include <filesystem>

namespace incdaw::engine {

/// Writes AIFF files — the export container macOS tools expect beside WAV.
///
/// Write-only for now: INCDAW exports AIFF, and imports arrive through the
/// WAV/streaming path. PCM 16 and 24 bit, big-endian, interleaved; the
/// sample rate travels in COMM's 80-bit extended float, as the format
/// demands. AIFF-C compression variants are deliberately not written.
class AiffFile {
public:
    enum class Format { pcm16, pcm24 };

    [[nodiscard]] static WavFile::Result write(const std::filesystem::path& path,
                                               const AudioFileData& data,
                                               Format format = Format::pcm24);
};

} // namespace incdaw::engine
