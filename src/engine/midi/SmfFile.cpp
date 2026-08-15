#include "engine/midi/SmfFile.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <map>

namespace incdaw::engine {

namespace {

// ── Byte plumbing ────────────────────────────────────────────────────────────

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

void appendVlq(std::vector<std::uint8_t>& out, std::uint32_t value)
{
    std::uint8_t bytes[5];
    int          count = 0;

    do {
        bytes[count++] = static_cast<std::uint8_t>(value & 0x7Fu);
        value >>= 7;
    } while (value != 0);

    for (int index = count - 1; index >= 0; --index)
        out.push_back(static_cast<std::uint8_t>(bytes[index] | (index > 0 ? 0x80u : 0u)));
}

/// One timed event being assembled into a track.
struct TimedBytes {
    Tick                      tick = 0;
    int                       order = 0;   ///< stable tiebreak: offs before ons
    std::vector<std::uint8_t> bytes;
};

std::vector<std::uint8_t> finishTrack(std::vector<TimedBytes> events)
{
    std::stable_sort(events.begin(), events.end(), [](const TimedBytes& a, const TimedBytes& b) {
        return a.tick != b.tick ? a.tick < b.tick : a.order < b.order;
    });

    std::vector<std::uint8_t> data;
    Tick                      last = 0;

    for (const TimedBytes& event : events) {
        appendVlq(data, static_cast<std::uint32_t>(std::max<Tick>(0, event.tick - last)));
        data.insert(data.end(), event.bytes.begin(), event.bytes.end());
        last = event.tick;
    }

    // End of track.
    appendVlq(data, 0);
    data.push_back(0xFF);
    data.push_back(0x2F);
    data.push_back(0x00);

    return data;
}

void appendChunk(std::vector<std::uint8_t>& file, const char id[4],
                 const std::vector<std::uint8_t>& data)
{
    file.insert(file.end(), id, id + 4);
    appendBigU32(file, static_cast<std::uint32_t>(data.size()));
    file.insert(file.end(), data.begin(), data.end());
}

// ── Reader plumbing ──────────────────────────────────────────────────────────

struct Cursor {
    const std::uint8_t* data = nullptr;
    std::size_t         size = 0;

    [[nodiscard]] bool readU8(std::uint8_t& value)
    {
        if (size < 1)
            return false;
        value = data[0];
        ++data;
        --size;
        return true;
    }

    [[nodiscard]] bool readBigU16(std::uint16_t& value)
    {
        if (size < 2)
            return false;
        value = static_cast<std::uint16_t>((data[0] << 8) | data[1]);
        data += 2;
        size -= 2;
        return true;
    }

    [[nodiscard]] bool readBigU32(std::uint32_t& value)
    {
        if (size < 4)
            return false;
        value = (static_cast<std::uint32_t>(data[0]) << 24)
              | (static_cast<std::uint32_t>(data[1]) << 16)
              | (static_cast<std::uint32_t>(data[2]) << 8) | static_cast<std::uint32_t>(data[3]);
        data += 4;
        size -= 4;
        return true;
    }

    [[nodiscard]] bool readVlq(std::uint32_t& value)
    {
        value = 0;
        for (int count = 0; count < 5; ++count) {
            std::uint8_t byte = 0;
            if (!readU8(byte))
                return false;

            value = (value << 7) | (byte & 0x7Fu);
            if ((byte & 0x80u) == 0)
                return true;
        }
        return false;   // a VLQ longer than five bytes is not a VLQ
    }

    [[nodiscard]] bool skip(std::size_t count)
    {
        if (size < count)
            return false;
        data += count;
        size -= count;
        return true;
    }
};

} // namespace

SmfFile::Result SmfFile::write(const std::filesystem::path& path, const SmfDocument& document)
{
    Result result;

    std::vector<std::uint8_t> file;

    // MThd: format 1, tempo track + note tracks, INCDAW's own division.
    std::vector<std::uint8_t> header;
    appendBigU16(header, 1);
    appendBigU16(header, static_cast<std::uint16_t>(1 + document.tracks.size()));
    appendBigU16(header, static_cast<std::uint16_t>(ticksPerQuarterNote));
    appendChunk(file, "MThd", header);

    // Track 0: the conductor — tempo and time signatures.
    {
        std::vector<TimedBytes> events;

        for (const TempoEvent& tempo : document.tempo) {
            const auto microseconds = static_cast<std::uint32_t>(
                std::llround(60'000'000.0 / std::max(1.0, tempo.beatsPerMinute)));

            TimedBytes event;
            event.tick  = tempo.tick;
            event.bytes = {0xFF, 0x51, 0x03,
                           static_cast<std::uint8_t>((microseconds >> 16) & 0xFFu),
                           static_cast<std::uint8_t>((microseconds >> 8) & 0xFFu),
                           static_cast<std::uint8_t>(microseconds & 0xFFu)};
            events.push_back(std::move(event));
        }

        for (const TimeSignatureEvent& signature : document.timeSignatures) {
            // The denominator travels as its base-two exponent.
            std::uint8_t exponent = 0;
            for (int value = signature.signature.denominator; value > 1; value /= 2)
                ++exponent;

            TimedBytes event;
            event.tick  = signature.tick;
            event.bytes = {0xFF, 0x58, 0x04,
                           static_cast<std::uint8_t>(signature.signature.numerator), exponent,
                           24, 8};
            events.push_back(std::move(event));
        }

        appendChunk(file, "MTrk", finishTrack(std::move(events)));
    }

    for (const SmfTrack& track : document.tracks) {
        std::vector<TimedBytes> events;

        if (!track.name.empty()) {
            TimedBytes name;
            name.tick  = 0;
            name.order = -1;
            name.bytes = {0xFF, 0x03};
            appendVlq(name.bytes, static_cast<std::uint32_t>(track.name.size()));
            name.bytes.insert(name.bytes.end(), track.name.begin(), track.name.end());
            events.push_back(std::move(name));
        }

        for (const SmfNote& note : track.notes) {
            const auto channel = static_cast<std::uint8_t>(note.channel & 0x0F);
            const auto key     = static_cast<std::uint8_t>(std::clamp(note.key, 0, 127));

            TimedBytes on;
            on.tick  = note.startTick;
            on.order = 1;   // offs (order 0) before ons at the same tick
            on.bytes = {static_cast<std::uint8_t>(0x90 | channel), key,
                        static_cast<std::uint8_t>(std::clamp(note.velocity, 1, 127))};
            events.push_back(std::move(on));

            TimedBytes off;
            off.tick  = note.startTick + std::max<Tick>(1, note.lengthTicks);
            off.order = 0;
            off.bytes = {static_cast<std::uint8_t>(0x80 | channel), key, 64};
            events.push_back(std::move(off));
        }

        appendChunk(file, "MTrk", finishTrack(std::move(events)));
    }

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

SmfFile::Result SmfFile::read(const std::filesystem::path& path, SmfDocument& document)
{
    Result result;
    document = SmfDocument{};

    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        result.error = "could not open: " + path.string();
        return result;
    }

    const std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(stream)),
                                          std::istreambuf_iterator<char>());

    Cursor cursor{bytes.data(), bytes.size()};

    std::uint32_t headerId = 0, headerSize = 0;
    std::uint16_t format = 0, trackCount = 0, division = 0;

    if (!cursor.readBigU32(headerId) || headerId != 0x4D546864u   // "MThd"
        || !cursor.readBigU32(headerSize) || headerSize < 6 || !cursor.readBigU16(format)
        || !cursor.readBigU16(trackCount) || !cursor.readBigU16(division)
        || !cursor.skip(headerSize - 6)) {
        result.error = "not a Standard MIDI File: " + path.string();
        return result;
    }

    if ((division & 0x8000u) != 0) {
        result.error = "SMPTE-division MIDI files are not supported";
        return result;
    }

    if (division == 0) {
        result.error = "MIDI file declares zero ticks per quarter";
        return result;
    }

    // Everything rescales into INCDAW's resolution as it is read.
    const double scale = static_cast<double>(ticksPerQuarterNote) / static_cast<double>(division);
    const auto   rescale = [scale](std::uint64_t tick) {
        return static_cast<Tick>(std::llround(static_cast<double>(tick) * scale));
    };

    for (std::uint16_t trackIndex = 0; trackIndex < trackCount; ++trackIndex) {
        std::uint32_t chunkId = 0, chunkSize = 0;
        if (!cursor.readBigU32(chunkId) || !cursor.readBigU32(chunkSize)
            || chunkSize > cursor.size) {
            result.error = "truncated MIDI file";
            return result;
        }

        if (chunkId != 0x4D54726Bu) {   // not "MTrk": skip an alien chunk
            (void)cursor.skip(chunkSize);
            continue;
        }

        Cursor track{cursor.data, chunkSize};
        (void)cursor.skip(chunkSize);

        SmfTrack readTrack;

        /// Key of a sounding note: (channel << 8) | key -> start tick, velocity.
        std::map<int, std::pair<std::uint64_t, int>> sounding;

        std::uint64_t tick          = 0;
        std::uint8_t  runningStatus = 0;

        while (track.size > 0) {
            std::uint32_t delta = 0;
            if (!track.readVlq(delta))
                break;
            tick += delta;

            std::uint8_t status = 0;
            if (!track.readU8(status))
                break;

            if (status < 0x80) {
                // Running status: the byte was data; reuse the previous status.
                if (runningStatus == 0)
                    break;
                --track.data;
                ++track.size;
                status = runningStatus;
            }

            if (status == 0xFF) {
                std::uint8_t  type   = 0;
                std::uint32_t length = 0;
                if (!track.readU8(type) || !track.readVlq(length) || length > track.size)
                    break;

                if (type == 0x51 && length == 3) {
                    const std::uint32_t microseconds =
                        (static_cast<std::uint32_t>(track.data[0]) << 16)
                        | (static_cast<std::uint32_t>(track.data[1]) << 8)
                        | static_cast<std::uint32_t>(track.data[2]);
                    document.tempo.push_back(
                        {rescale(tick), 60'000'000.0 / std::max(1u, microseconds)});
                } else if (type == 0x58 && length >= 2) {
                    document.timeSignatures.push_back(
                        {rescale(tick),
                         {static_cast<int>(track.data[0]), 1 << track.data[1]}});
                } else if (type == 0x03) {
                    readTrack.name.assign(reinterpret_cast<const char*>(track.data), length);
                }

                (void)track.skip(length);
                continue;
            }

            if (status == 0xF0 || status == 0xF7) {
                std::uint32_t length = 0;
                if (!track.readVlq(length) || !track.skip(length))
                    break;
                continue;
            }

            runningStatus = status;

            const std::uint8_t kind    = status & 0xF0u;
            const int          channel = status & 0x0Fu;

            const std::size_t dataBytes = (kind == 0xC0 || kind == 0xD0) ? 1 : 2;
            if (track.size < dataBytes)
                break;

            const std::uint8_t data1 = track.data[0];
            const std::uint8_t data2 = dataBytes > 1 ? track.data[1] : 0;
            (void)track.skip(dataBytes);

            const int noteKey = (channel << 8) | data1;

            if (kind == 0x90 && data2 > 0) {
                sounding[noteKey] = {tick, data2};
            } else if (kind == 0x80 || (kind == 0x90 && data2 == 0)) {
                const auto found = sounding.find(noteKey);
                if (found != sounding.end()) {
                    SmfNote note;
                    note.startTick   = rescale(found->second.first);
                    note.lengthTicks = std::max<Tick>(1, rescale(tick) - note.startTick);
                    note.channel     = channel;
                    note.key         = data1;
                    note.velocity    = found->second.second;
                    readTrack.notes.push_back(note);
                    sounding.erase(found);
                }
            }
        }

        // A note the file never turned off ends where the track's data did.
        for (const auto& [noteKey, startAndVelocity] : sounding) {
            SmfNote note;
            note.startTick   = rescale(startAndVelocity.first);
            note.lengthTicks = std::max<Tick>(1, rescale(tick) - note.startTick);
            note.channel     = (noteKey >> 8) & 0x0F;
            note.key         = noteKey & 0xFF;
            note.velocity    = startAndVelocity.second;
            readTrack.notes.push_back(note);
        }

        if (!readTrack.notes.empty() || !readTrack.name.empty())
            document.tracks.push_back(std::move(readTrack));
    }

    result.succeeded = true;
    return result;
}

} // namespace incdaw::engine
