#pragma once

#include "engine/audio/WavFile.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

namespace incdaw::engine::wav {

/// Byte-level RIFF/WAVE helpers shared by WavFile (one-shot) and
/// WavStreamWriter (incremental). One encoder, two writers: if the two paths
/// encoded samples separately they would drift apart, and "the recording
/// sounds different from the export" is a bug nobody would think to look
/// for here.
///
/// RIFF is little-endian by definition, and every platform INCDAW targets is
/// little-endian; these helpers still avoid type-punning, which is undefined
/// behaviour regardless of endianness.

[[nodiscard]] inline std::uint32_t readU32(const std::uint8_t* bytes) noexcept
{
    return static_cast<std::uint32_t>(bytes[0]) | (static_cast<std::uint32_t>(bytes[1]) << 8)
         | (static_cast<std::uint32_t>(bytes[2]) << 16) | (static_cast<std::uint32_t>(bytes[3]) << 24);
}

[[nodiscard]] inline std::uint16_t readU16(const std::uint8_t* bytes) noexcept
{
    return static_cast<std::uint16_t>(static_cast<std::uint16_t>(bytes[0])
                                      | (static_cast<std::uint16_t>(bytes[1]) << 8));
}

inline void writeU32(std::vector<std::uint8_t>& out, std::uint32_t value)
{
    out.push_back(static_cast<std::uint8_t>(value & 0xFFu));
    out.push_back(static_cast<std::uint8_t>((value >> 8) & 0xFFu));
    out.push_back(static_cast<std::uint8_t>((value >> 16) & 0xFFu));
    out.push_back(static_cast<std::uint8_t>((value >> 24) & 0xFFu));
}

inline void writeU16(std::vector<std::uint8_t>& out, std::uint16_t value)
{
    out.push_back(static_cast<std::uint8_t>(value & 0xFFu));
    out.push_back(static_cast<std::uint8_t>((value >> 8) & 0xFFu));
}

inline constexpr std::uint16_t formatPcm        = 1;
inline constexpr std::uint16_t formatFloat      = 3;
inline constexpr std::uint16_t formatExtensible = 0xFFFE;

[[nodiscard]] inline constexpr std::uint16_t bitsFor(WavFile::Format format) noexcept
{
    return format == WavFile::Format::pcm16 ? 16
         : format == WavFile::Format::pcm24 ? 24 : 32;
}

[[nodiscard]] inline constexpr std::uint16_t codeFor(WavFile::Format format) noexcept
{
    return format == WavFile::Format::float32 ? formatFloat : formatPcm;
}

/// Encodes one sample in the requested format, appending its bytes.
inline void encodeSample(std::vector<std::uint8_t>& out, Sample value, WavFile::Format format)
{
    if (format == WavFile::Format::float32) {
        std::uint32_t raw = 0;
        std::memcpy(&raw, &value, sizeof(raw));
        writeU32(out, raw);
    } else if (format == WavFile::Format::pcm16) {
        const float clamped = std::clamp(value, -1.0f, 1.0f);
        const auto quantised = static_cast<std::int16_t>(std::lround(clamped * 32767.0f));
        writeU16(out, static_cast<std::uint16_t>(quantised));
    } else {   // pcm24
        const float clamped = std::clamp(value, -1.0f, 1.0f);
        const auto quantised = static_cast<std::int32_t>(std::lround(clamped * 8388607.0f));
        out.push_back(static_cast<std::uint8_t>(quantised & 0xFF));
        out.push_back(static_cast<std::uint8_t>((quantised >> 8) & 0xFF));
        out.push_back(static_cast<std::uint8_t>((quantised >> 16) & 0xFF));
    }
}

/// Appends the canonical 44-byte header. `dataBytes` may be a placeholder;
/// WavStreamWriter patches the two size fields on finalize.
inline void appendCanonicalHeader(std::vector<std::uint8_t>& out, WavFile::Format format,
                                  std::size_t channelCount, double sampleRate, std::size_t dataBytes)
{
    const auto append = [&out](const char* text) {
        out.insert(out.end(), text, text + 4);
    };

    const std::uint16_t bits       = bitsFor(format);
    const std::size_t   frameBytes = (bits / 8u) * channelCount;

    append("RIFF");
    writeU32(out, static_cast<std::uint32_t>(36 + dataBytes));
    append("WAVE");
    append("fmt ");
    writeU32(out, 16);
    writeU16(out, codeFor(format));
    writeU16(out, static_cast<std::uint16_t>(channelCount));
    writeU32(out, static_cast<std::uint32_t>(sampleRate));
    writeU32(out, static_cast<std::uint32_t>(sampleRate * static_cast<double>(frameBytes)));
    writeU16(out, static_cast<std::uint16_t>(frameBytes));
    writeU16(out, bits);
    append("data");
    writeU32(out, static_cast<std::uint32_t>(dataBytes));
}

/// Byte offsets of the two size fields the streaming writer patches.
inline constexpr std::size_t riffSizeOffset = 4;    ///< RIFF chunk size: 36 + dataBytes
inline constexpr std::size_t dataSizeOffset = 40;   ///< data chunk size: dataBytes
inline constexpr std::size_t headerBytes    = 44;

} // namespace incdaw::engine::wav
