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

    /// Cue points, by their RIFF id, before names and lengths are attached.
    std::vector<std::pair<std::uint32_t, FramePosition>> cues;

    /// From the `adtl` list: `labl` gives a name, `ltxt` a length. Both are
    /// keyed by the cue id, and either may be absent — a bare `cue ` chunk
    /// with no list beside it is a perfectly ordinary file.
    std::vector<std::pair<std::uint32_t, std::string>> labels;
    std::vector<std::pair<std::uint32_t, FrameCount>>  lengths;
};

/// Reads a `cue ` chunk body into `header.cues`.
void parseCueChunk(const std::uint8_t* body, std::size_t available, ParsedHeader& header)
{
    if (available < 4)
        return;

    const std::uint32_t count = readU32(body);

    // 24 bytes each: id, play order, the chunk it points into, two starts we
    // do not use, and the sample offset that is the whole point.
    constexpr std::size_t pointBytes = 24;

    for (std::uint32_t index = 0; index < count; ++index) {
        const std::size_t at = 4 + static_cast<std::size_t>(index) * pointBytes;
        if (at + pointBytes > available)
            break;

        header.cues.emplace_back(readU32(body + at),
                                 static_cast<FramePosition>(readU32(body + at + 20)));
    }
}

/// Reads a `LIST` chunk body, picking up `labl` and `ltxt` when it is `adtl`.
void parseListChunk(const std::uint8_t* body, std::size_t available, ParsedHeader& header)
{
    if (available < 4 || std::memcmp(body, "adtl", 4) != 0)
        return;   // INFO lists and the rest are somebody else's metadata

    std::size_t offset = 4;

    while (offset + 8 <= available) {
        const std::uint8_t* id   = body + offset;
        const std::uint32_t size = readU32(body + offset + 4);
        const std::size_t   at   = offset + 8;

        if (at + size > available)
            break;

        if (std::memcmp(id, "labl", 4) == 0 && size >= 4) {
            const std::uint32_t cueId = readU32(body + at);

            // Null-terminated, and the terminator is inside the declared size.
            const auto* text   = reinterpret_cast<const char*>(body + at + 4);
            const std::size_t maximum = size - 4;
            const std::size_t length  = ::strnlen(text, maximum);

            header.labels.emplace_back(cueId, std::string(text, length));
        } else if (std::memcmp(id, "ltxt", 4) == 0 && size >= 8) {
            header.lengths.emplace_back(readU32(body + at),
                                        static_cast<FrameCount>(readU32(body + at + 4)));
        }

        offset = at + size + (size & 1u);
    }
}

/// Joins cues, labels and lengths into markers, sorted by position.
std::vector<AudioMarker> assembleMarkers(const ParsedHeader& header)
{
    std::vector<AudioMarker> markers;
    markers.reserve(header.cues.size());

    for (const auto& [id, position] : header.cues) {
        AudioMarker marker;
        marker.start = position;

        for (const auto& [labelId, text] : header.labels)
            if (labelId == id)
                marker.name = text;

        for (const auto& [lengthId, length] : header.lengths)
            if (lengthId == id)
                marker.length = length;

        markers.push_back(std::move(marker));
    }

    std::stable_sort(markers.begin(), markers.end(),
                     [](const AudioMarker& left, const AudioMarker& right) {
                         return left.start < right.start;
                     });

    return markers;
}

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
        } else if (std::memcmp(chunkId, "cue ", 4) == 0) {
            parseCueChunk(bytes.data() + body, std::min<std::size_t>(chunkSize, bytes.size() - body),
                          header);
        } else if (std::memcmp(chunkId, "LIST", 4) == 0) {
            parseListChunk(bytes.data() + body, std::min<std::size_t>(chunkSize, bytes.size() - body),
                           header);
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
    out.markers = assembleMarkers(header);
}

/// Appends the `cue ` and `LIST adtl` chunks describing `markers`.
///
/// After the data chunk rather than before it, so that the canonical header —
/// which the streaming writer patches by fixed offset — stays exactly as it
/// was. RIFF does not care about chunk order, and every reader that walks
/// chunks properly (this one included) finds them either way.
void appendMarkerChunks(std::vector<std::uint8_t>& bytes, const std::vector<AudioMarker>& markers)
{
    if (markers.empty())
        return;

    const auto appendId = [&bytes](const char* text) {
        bytes.insert(bytes.end(), text, text + 4);
    };

    // Ids are 1-based and index-derived: unique within the file, which is all
    // RIFF asks, and stable across a rewrite so an external editor's notion of
    // "the same cue" survives.
    const auto idOf = [](std::size_t index) { return static_cast<std::uint32_t>(index + 1); };

    appendId("cue ");
    writeU32(bytes, static_cast<std::uint32_t>(4 + markers.size() * 24));
    writeU32(bytes, static_cast<std::uint32_t>(markers.size()));

    for (std::size_t index = 0; index < markers.size(); ++index) {
        const auto position = static_cast<std::uint32_t>(std::max<FramePosition>(markers[index].start, 0));

        writeU32(bytes, idOf(index));   // dwName
        writeU32(bytes, position);      // dwPosition, in the play order
        appendId("data");               // fccChunk
        writeU32(bytes, 0);             // dwChunkStart
        writeU32(bytes, 0);             // dwBlockStart
        writeU32(bytes, position);      // dwSampleOffset
    }

    // The list is built into its own buffer first: a LIST carries its total
    // size up front, and that is not known until its children are written.
    // A four-character id as bytes, without the terminator a string literal
    // would drag along.
    const auto appendListId = [](std::vector<std::uint8_t>& out, const char (&text)[5]) {
        out.insert(out.end(), text, text + 4);
    };

    std::vector<std::uint8_t> list;
    appendListId(list, "adtl");

    for (std::size_t index = 0; index < markers.size(); ++index) {
        const AudioMarker& marker = markers[index];

        if (!marker.name.empty()) {
            const std::size_t textBytes = marker.name.size() + 1;   // the terminator counts

            appendListId(list, "labl");
            writeU32(list, static_cast<std::uint32_t>(4 + textBytes));
            writeU32(list, idOf(index));
            list.insert(list.end(), marker.name.begin(), marker.name.end());
            list.push_back(0);

            // Chunks are word-aligned. Omitting the pad byte on an odd name
            // shifts every chunk after it by one, which readers see as
            // garbage rather than as a missing label.
            if (textBytes & 1u)
                list.push_back(0);
        }

        if (marker.length > 0) {
            appendListId(list, "ltxt");
            writeU32(list, 20);
            writeU32(list, idOf(index));
            writeU32(list, static_cast<std::uint32_t>(marker.length));
            appendListId(list, "rgn ");   // dwPurpose
            writeU16(list, 0);   // country
            writeU16(list, 0);   // language
            writeU16(list, 0);   // dialect
            writeU16(list, 0);   // code page
        }
    }

    if (list.size() > 4) {
        appendId("LIST");
        writeU32(bytes, static_cast<std::uint32_t>(list.size()));
        bytes.insert(bytes.end(), list.begin(), list.end());

        if (list.size() & 1u)
            bytes.push_back(0);
    }
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

    appendMarkerChunks(bytes, data.markers);

    // The RIFF size covers everything after the first eight bytes, and the
    // canonical header wrote it knowing only the data chunk.
    const auto riffSize = static_cast<std::uint32_t>(bytes.size() - 8);
    bytes[wav::riffSizeOffset + 0] = static_cast<std::uint8_t>(riffSize & 0xFFu);
    bytes[wav::riffSizeOffset + 1] = static_cast<std::uint8_t>((riffSize >> 8) & 0xFFu);
    bytes[wav::riffSizeOffset + 2] = static_cast<std::uint8_t>((riffSize >> 16) & 0xFFu);
    bytes[wav::riffSizeOffset + 3] = static_cast<std::uint8_t>((riffSize >> 24) & 0xFFu);

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
