#include "engine/audio/WavFile.h"

#include <algorithm>
#include <cstring>
#include <fstream>

namespace incdaw::engine {
namespace {

// RIFF is little-endian by definition, and every platform INCDAW targets is
// little-endian; the byte-level helpers below still avoid type-punning, which
// is undefined behaviour regardless of endianness.

std::uint32_t readU32(const std::uint8_t* bytes) noexcept
{
    return static_cast<std::uint32_t>(bytes[0]) | (static_cast<std::uint32_t>(bytes[1]) << 8)
         | (static_cast<std::uint32_t>(bytes[2]) << 16) | (static_cast<std::uint32_t>(bytes[3]) << 24);
}

std::uint16_t readU16(const std::uint8_t* bytes) noexcept
{
    return static_cast<std::uint16_t>(static_cast<std::uint16_t>(bytes[0])
                                      | (static_cast<std::uint16_t>(bytes[1]) << 8));
}

void writeU32(std::vector<std::uint8_t>& out, std::uint32_t value)
{
    out.push_back(static_cast<std::uint8_t>(value & 0xFFu));
    out.push_back(static_cast<std::uint8_t>((value >> 8) & 0xFFu));
    out.push_back(static_cast<std::uint8_t>((value >> 16) & 0xFFu));
    out.push_back(static_cast<std::uint8_t>((value >> 24) & 0xFFu));
}

void writeU16(std::vector<std::uint8_t>& out, std::uint16_t value)
{
    out.push_back(static_cast<std::uint8_t>(value & 0xFFu));
    out.push_back(static_cast<std::uint8_t>((value >> 8) & 0xFFu));
}

constexpr std::uint16_t formatPcm   = 1;
constexpr std::uint16_t formatFloat = 3;
constexpr std::uint16_t formatExtensible = 0xFFFE;

struct ParsedHeader {
    std::uint16_t format        = 0;
    std::uint16_t channels      = 0;
    std::uint32_t sampleRate    = 0;
    std::uint16_t bitsPerSample = 0;
    std::size_t   dataOffset    = 0;
    std::size_t   dataSize      = 0;
};

WavFile::Result parseHeader(const std::vector<std::uint8_t>& bytes, ParsedHeader& header)
{
    WavFile::Result result;

    if (bytes.size() < 12 || std::memcmp(bytes.data(), "RIFF", 4) != 0
        || std::memcmp(bytes.data() + 8, "WAVE", 4) != 0) {
        result.error = "not a RIFF/WAVE file";
        return result;
    }

    // Walk chunks rather than assuming fmt-then-data: files from broadcast
    // recorders carry bext/LIST/junk chunks in between, and they are valid.
    std::size_t offset  = 12;
    bool        sawFmt  = false;
    bool        sawData = false;

    while (offset + 8 <= bytes.size()) {
        const std::uint32_t chunkSize = readU32(bytes.data() + offset + 4);
        const std::uint8_t* chunkId   = bytes.data() + offset;
        const std::size_t   body      = offset + 8;

        if (std::memcmp(chunkId, "fmt ", 4) == 0 && body + 16 <= bytes.size()) {
            header.format        = readU16(bytes.data() + body);
            header.channels      = readU16(bytes.data() + body + 2);
            header.sampleRate    = readU32(bytes.data() + body + 4);
            header.bitsPerSample = readU16(bytes.data() + body + 14);

            // WAVE_FORMAT_EXTENSIBLE wraps the real format in a sub-GUID whose
            // first two bytes are the classic code.
            if (header.format == formatExtensible && body + 26 <= bytes.size())
                header.format = readU16(bytes.data() + body + 24);

            sawFmt = true;
        } else if (std::memcmp(chunkId, "data", 4) == 0) {
            header.dataOffset = body;
            header.dataSize   = std::min<std::size_t>(chunkSize, bytes.size() - body);
            sawData = true;
        }

        // Chunks are word-aligned; an odd size is followed by a pad byte.
        offset = body + chunkSize + (chunkSize & 1u);
    }

    if (!sawFmt)  { result.error = "no fmt chunk";  return result; }
    if (!sawData) { result.error = "no data chunk"; return result; }

    if (header.channels == 0 || header.sampleRate == 0) {
        result.error = "fmt chunk describes no audio";
        return result;
    }

    const bool supported = (header.format == formatPcm
                            && (header.bitsPerSample == 16 || header.bitsPerSample == 24
                                || header.bitsPerSample == 32))
                        || (header.format == formatFloat && header.bitsPerSample == 32);

    if (!supported) {
        result.error = "unsupported format: code " + std::to_string(header.format) + ", "
                     + std::to_string(header.bitsPerSample) + " bits";
        return result;
    }

    result.succeeded = true;
    return result;
}

WavFile::Result loadAndParse(const std::filesystem::path& path, std::vector<std::uint8_t>& bytes,
                             ParsedHeader& header)
{
    WavFile::Result result;

    std::ifstream file{path, std::ios::binary | std::ios::ate};
    if (!file) {
        result.error = "cannot open: " + path.string();
        return result;
    }

    const std::streamsize size = file.tellg();
    file.seekg(0);

    bytes.resize(static_cast<std::size_t>(size));
    if (!file.read(reinterpret_cast<char*>(bytes.data()), size)) {
        result.error = "read failed: " + path.string();
        return result;
    }

    return parseHeader(bytes, header);
}

void fillMetadata(const ParsedHeader& header, AudioFileData& out)
{
    const std::size_t bytesPerSample = header.bitsPerSample / 8u;
    const std::size_t frameBytes     = bytesPerSample * header.channels;

    out.sampleRate   = static_cast<SampleRate>(header.sampleRate);
    out.channelCount = header.channels;
    out.frameCount   = frameBytes > 0 ? static_cast<FrameCount>(header.dataSize / frameBytes) : 0;
    out.channels.clear();
}

} // namespace

WavFile::Result WavFile::probe(const std::filesystem::path& path, AudioFileData& out)
{
    std::vector<std::uint8_t> bytes;
    ParsedHeader              header;

    Result result = loadAndParse(path, bytes, header);
    if (!result)
        return result;

    fillMetadata(header, out);
    return result;
}

WavFile::Result WavFile::read(const std::filesystem::path& path, AudioFileData& out)
{
    std::vector<std::uint8_t> bytes;
    ParsedHeader              header;

    Result result = loadAndParse(path, bytes, header);
    if (!result)
        return result;

    fillMetadata(header, out);

    out.channels.assign(out.channelCount, std::vector<Sample>(static_cast<std::size_t>(out.frameCount)));

    const std::uint8_t* data           = bytes.data() + header.dataOffset;
    const std::size_t   bytesPerSample = header.bitsPerSample / 8u;

    for (FrameCount frame = 0; frame < out.frameCount; ++frame) {
        for (std::size_t channel = 0; channel < out.channelCount; ++channel) {
            const std::uint8_t* sample = data
                + (static_cast<std::size_t>(frame) * out.channelCount + channel) * bytesPerSample;

            Sample value = 0.0f;

            if (header.format == formatFloat) {
                std::uint32_t raw = readU32(sample);
                float decoded = 0.0f;
                std::memcpy(&decoded, &raw, sizeof(decoded));
                value = decoded;
            } else if (header.bitsPerSample == 16) {
                const auto raw = static_cast<std::int16_t>(readU16(sample));
                value = static_cast<Sample>(raw) / 32768.0f;
            } else if (header.bitsPerSample == 24) {
                // Sign-extend 24 bits via a shift up to 32 and back down.
                const std::int32_t raw = static_cast<std::int32_t>(
                    (static_cast<std::uint32_t>(sample[0]) << 8)
                    | (static_cast<std::uint32_t>(sample[1]) << 16)
                    | (static_cast<std::uint32_t>(sample[2]) << 24)) >> 8;
                value = static_cast<Sample>(raw) / 8388608.0f;
            } else {   // PCM 32
                const auto raw = static_cast<std::int32_t>(readU32(sample));
                value = static_cast<Sample>(static_cast<double>(raw) / 2147483648.0);
            }

            out.channels[channel][static_cast<std::size_t>(frame)] = value;
        }
    }

    return result;
}

WavFile::Result WavFile::write(const std::filesystem::path& path, const AudioFileData& data,
                               Format format)
{
    Result result;

    if (data.channelCount == 0 || data.channels.size() < data.channelCount) {
        result.error = "no audio to write";
        return result;
    }

    const std::uint16_t bits = format == Format::pcm16 ? 16
                             : format == Format::pcm24 ? 24 : 32;
    const std::uint16_t code = format == Format::float32 ? formatFloat : formatPcm;

    const std::size_t bytesPerSample = bits / 8u;
    const std::size_t frameBytes     = bytesPerSample * data.channelCount;
    const std::size_t dataBytes      = frameBytes * static_cast<std::size_t>(data.frameCount);

    std::vector<std::uint8_t> bytes;
    bytes.reserve(44 + dataBytes);

    const auto append = [&bytes](const char* text) {
        bytes.insert(bytes.end(), text, text + 4);
    };

    append("RIFF");
    writeU32(bytes, static_cast<std::uint32_t>(36 + dataBytes));
    append("WAVE");
    append("fmt ");
    writeU32(bytes, 16);
    writeU16(bytes, code);
    writeU16(bytes, static_cast<std::uint16_t>(data.channelCount));
    writeU32(bytes, static_cast<std::uint32_t>(data.sampleRate));
    writeU32(bytes, static_cast<std::uint32_t>(data.sampleRate * static_cast<double>(frameBytes)));
    writeU16(bytes, static_cast<std::uint16_t>(frameBytes));
    writeU16(bytes, bits);
    append("data");
    writeU32(bytes, static_cast<std::uint32_t>(dataBytes));

    for (FrameCount frame = 0; frame < data.frameCount; ++frame) {
        for (std::size_t channel = 0; channel < data.channelCount; ++channel) {
            const std::size_t index = static_cast<std::size_t>(frame);
            const Sample value = index < data.channels[channel].size()
                                     ? data.channels[channel][index] : 0.0f;

            if (format == Format::float32) {
                std::uint32_t raw = 0;
                std::memcpy(&raw, &value, sizeof(raw));
                writeU32(bytes, raw);
            } else if (format == Format::pcm16) {
                const float clamped = std::clamp(value, -1.0f, 1.0f);
                const auto quantised = static_cast<std::int16_t>(std::lround(clamped * 32767.0f));
                writeU16(bytes, static_cast<std::uint16_t>(quantised));
            } else {   // pcm24
                const float clamped = std::clamp(value, -1.0f, 1.0f);
                const auto quantised = static_cast<std::int32_t>(std::lround(clamped * 8388607.0f));
                bytes.push_back(static_cast<std::uint8_t>(quantised & 0xFF));
                bytes.push_back(static_cast<std::uint8_t>((quantised >> 8) & 0xFF));
                bytes.push_back(static_cast<std::uint8_t>((quantised >> 16) & 0xFF));
            }
        }
    }

    std::ofstream file{path, std::ios::binary | std::ios::trunc};
    if (!file || !file.write(reinterpret_cast<const char*>(bytes.data()),
                             static_cast<std::streamsize>(bytes.size()))) {
        result.error = "write failed: " + path.string();
        return result;
    }

    result.succeeded = true;
    return result;
}

} // namespace incdaw::engine
