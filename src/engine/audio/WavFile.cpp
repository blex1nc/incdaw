#include "engine/audio/WavFile.h"

#include "engine/audio/WavBytes.h"

#include <algorithm>
#include <cstring>
#include <fstream>

namespace incdaw::engine {
namespace {

// Byte-level helpers live in WavBytes.h, shared with WavStreamWriter so the
// one-shot and streaming writers cannot encode differently.

using wav::readU16;
using wav::readU32;
using wav::writeU16;
using wav::writeU32;
using wav::formatExtensible;
using wav::formatFloat;
using wav::formatPcm;

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
            wav::FormatInfo info;
            if (wav::interpretFmtChunk(bytes.data() + body, bytes.size() - body, info)) {
                header.format        = info.format;
                header.channels      = info.channels;
                header.sampleRate    = info.sampleRate;
                header.bitsPerSample = info.bitsPerSample;
                sawFmt = true;
            }
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

    wav::FormatInfo info;
    info.format        = header.format;
    info.channels      = header.channels;
    info.sampleRate    = header.sampleRate;
    info.bitsPerSample = header.bitsPerSample;

    if (!wav::isSupportedFormat(info)) {
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

            out.channels[channel][static_cast<std::size_t>(frame)] =
                wav::decodeSample(sample, header.format, header.bitsPerSample);
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

    const std::size_t bytesPerSample = wav::bitsFor(format) / 8u;
    const std::size_t frameBytes     = bytesPerSample * data.channelCount;
    const std::size_t dataBytes      = frameBytes * static_cast<std::size_t>(data.frameCount);

    std::vector<std::uint8_t> bytes;
    bytes.reserve(wav::headerBytes + dataBytes);

    wav::appendCanonicalHeader(bytes, format, data.channelCount, data.sampleRate, dataBytes);

    for (FrameCount frame = 0; frame < data.frameCount; ++frame) {
        for (std::size_t channel = 0; channel < data.channelCount; ++channel) {
            const std::size_t index = static_cast<std::size_t>(frame);
            const Sample value = index < data.channels[channel].size()
                                     ? data.channels[channel][index] : 0.0f;

            wav::encodeSample(bytes, value, format);
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
