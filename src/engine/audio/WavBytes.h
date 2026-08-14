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

/// Decodes one sample. The exact inverse of `encodeSample`, and the ONLY
/// decoder: WavFile (whole-file) and WavStreamReader (random access) both
/// call this, so they cannot disagree about what a file contains.
[[nodiscard]] inline Sample decodeSample(const std::uint8_t* bytes, std::uint16_t format,
                                         std::uint16_t bitsPerSample) noexcept
{
    if (format == formatFloat) {
        const std::uint32_t raw = readU32(bytes);
        float decoded = 0.0f;
        std::memcpy(&decoded, &raw, sizeof(decoded));
        return decoded;
    }

    if (bitsPerSample == 16) {
        const auto raw = static_cast<std::int16_t>(readU16(bytes));
        return static_cast<Sample>(raw) / 32768.0f;
    }

    if (bitsPerSample == 24) {
        // Sign-extend 24 bits via a shift up to 32 and back down.
        const std::int32_t raw = static_cast<std::int32_t>(
            (static_cast<std::uint32_t>(bytes[0]) << 8)
            | (static_cast<std::uint32_t>(bytes[1]) << 16)
            | (static_cast<std::uint32_t>(bytes[2]) << 24)) >> 8;
        return static_cast<Sample>(raw) / 8388608.0f;
    }

    // PCM 32.
    const auto raw = static_cast<std::int32_t>(readU32(bytes));
    return static_cast<Sample>(static_cast<double>(raw) / 2147483648.0);
}

/// What both readers extract from a fmt chunk.
struct FormatInfo {
    std::uint16_t format        = 0;
    std::uint16_t channels      = 0;
    std::uint32_t sampleRate    = 0;
    std::uint16_t bitsPerSample = 0;
};

/// Interprets a fmt chunk body (16 bytes minimum; EXTENSIBLE unwrapped when
/// the body carries the sub-format GUID).
[[nodiscard]] inline bool interpretFmtChunk(const std::uint8_t* body, std::size_t size,
                                            FormatInfo& info) noexcept
{
    if (size < 16)
        return false;

    info.format        = readU16(body);
    info.channels      = readU16(body + 2);
    info.sampleRate    = readU32(body + 4);
    info.bitsPerSample = readU16(body + 14);

    // WAVE_FORMAT_EXTENSIBLE wraps the real format in a sub-GUID whose first
    // two bytes are the classic code.
    if (info.format == formatExtensible && size >= 26)
        info.format = readU16(body + 24);

    return true;
}

[[nodiscard]] inline bool isSupportedFormat(const FormatInfo& info) noexcept
{
    return (info.format == formatPcm
            && (info.bitsPerSample == 16 || info.bitsPerSample == 24 || info.bitsPerSample == 32))
        || (info.format == formatFloat && info.bitsPerSample == 32);
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
