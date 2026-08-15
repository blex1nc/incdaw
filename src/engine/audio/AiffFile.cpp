#include "engine/audio/AiffFile.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <vector>

namespace incdaw::engine {

namespace {

void appendBigU16(std::vector<std::uint8_t>& out, std::uint16_t value)
{
    out.push_back(static_cast<std::uint8_t>((value >> 8) & 0xFFu));
    out.push_back(static_cast<std::uint8_t>(value & 0xFFu));
}

void appendBigU32(std::vector<std::uint8_t>& out, std::uint32_t value)
{
    out.push_back(static_cast<std::uint8_t>((value >> 24) & 0xFFu));
    out.push_back(static_cast<std::uint8_t>((value >> 16) & 0xFFu));
    out.push_back(static_cast<std::uint8_t>((value >> 8) & 0xFFu));
    out.push_back(static_cast<std::uint8_t>(value & 0xFFu));
}

void appendId(std::vector<std::uint8_t>& out, const char id[4])
{
    out.insert(out.end(), id, id + 4);
}

/// The 80-bit IEEE 754 extended float COMM stores the sample rate in —
/// 1 sign+15 exponent bits, then a 64-bit mantissa with an explicit
/// integer bit. Sample rates are small positive integers, so the
/// conversion is exact.
void appendExtended(std::vector<std::uint8_t>& out, double value)
{
    std::uint16_t exponent = 0;
    std::uint64_t mantissa = 0;

    if (value > 0.0) {
        int exponent2 = 0;
        const double normalised = std::frexp(value, &exponent2);   // [0.5, 1)

        exponent = static_cast<std::uint16_t>(exponent2 + 16382);
        mantissa = static_cast<std::uint64_t>(
            std::ldexp(normalised, 64));   // top bit becomes the explicit integer bit
    }

    appendBigU16(out, exponent);
    for (int shift = 56; shift >= 0; shift -= 8)
        out.push_back(static_cast<std::uint8_t>((mantissa >> shift) & 0xFFu));
}

std::int32_t quantise(Sample value, std::int32_t limit) noexcept
{
    const double scaled = static_cast<double>(value) * static_cast<double>(limit);
    const double clamped =
        std::clamp(scaled, -static_cast<double>(limit) - 1.0, static_cast<double>(limit));
    return static_cast<std::int32_t>(std::lrint(clamped));
}

} // namespace

WavFile::Result AiffFile::write(const std::filesystem::path& path, const AudioFileData& data,
                                Format format)
{
    WavFile::Result result;

    if (data.channelCount == 0 || data.channels.size() != data.channelCount) {
        result.error = "AIFF write needs at least one channel";
        return result;
    }

    const int bitsPerSample = format == Format::pcm16 ? 16 : 24;
    const std::size_t bytesPerSample = static_cast<std::size_t>(bitsPerSample) / 8;

    const auto frames = static_cast<std::size_t>(data.frameCount);
    const std::size_t soundBytes = frames * data.channelCount * bytesPerSample;

    std::vector<std::uint8_t> file;
    file.reserve(soundBytes + 128);

    appendId(file, "FORM");
    appendBigU32(file, 0);   // patched at the end
    appendId(file, "AIFF");

    // COMM
    appendId(file, "COMM");
    appendBigU32(file, 18);
    appendBigU16(file, static_cast<std::uint16_t>(data.channelCount));
    appendBigU32(file, static_cast<std::uint32_t>(frames));
    appendBigU16(file, static_cast<std::uint16_t>(bitsPerSample));
    appendExtended(file, data.sampleRate);

    // SSND
    appendId(file, "SSND");
    appendBigU32(file, static_cast<std::uint32_t>(soundBytes + 8));
    appendBigU32(file, 0);   // offset
    appendBigU32(file, 0);   // block size

    for (std::size_t frame = 0; frame < frames; ++frame) {
        for (std::size_t channel = 0; channel < data.channelCount; ++channel) {
            const Sample sample = data.channels[channel][frame];

            if (format == Format::pcm16) {
                const auto value = static_cast<std::int16_t>(quantise(sample, 32767));
                file.push_back(static_cast<std::uint8_t>((value >> 8) & 0xFF));
                file.push_back(static_cast<std::uint8_t>(value & 0xFF));
            } else {
                const std::int32_t value = quantise(sample, 8388607);
                file.push_back(static_cast<std::uint8_t>((value >> 16) & 0xFF));
                file.push_back(static_cast<std::uint8_t>((value >> 8) & 0xFF));
                file.push_back(static_cast<std::uint8_t>(value & 0xFF));
            }
        }
    }

    // An odd chunk needs a pad byte, though PCM frames never produce one.
    if (file.size() % 2 != 0)
        file.push_back(0);

    const auto formSize = static_cast<std::uint32_t>(file.size() - 8);
    file[4] = static_cast<std::uint8_t>((formSize >> 24) & 0xFFu);
    file[5] = static_cast<std::uint8_t>((formSize >> 16) & 0xFFu);
    file[6] = static_cast<std::uint8_t>((formSize >> 8) & 0xFFu);
    file[7] = static_cast<std::uint8_t>(formSize & 0xFFu);

    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (!stream) {
        result.error = "could not open for writing: " + path.string();
        return result;
    }

    stream.write(reinterpret_cast<const char*>(file.data()),
                 static_cast<std::streamsize>(file.size()));

    if (!stream) {
        result.error = "write failed: " + path.string();
        return result;
    }

    result.succeeded = true;
    return result;
}

} // namespace incdaw::engine
