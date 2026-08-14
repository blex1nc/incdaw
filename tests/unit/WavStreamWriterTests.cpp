// Phase 12 (part 2) — the streaming WAV writer.
//
// The load-bearing test is byte-identity: a file streamed block by block must
// equal, byte for byte, the file WavFile::write produces in one shot. That is
// what guarantees a recording and an export can never sound different.

#include "doctest.h"

#include "engine/audio/WavFile.h"
#include "engine/audio/WavStreamWriter.h"

#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <vector>

using namespace incdaw::engine;
namespace fs = std::filesystem;

namespace {

struct ScratchFile {
    fs::path path;

    explicit ScratchFile(const char* name)
        : path(fs::temp_directory_path() / name)
    {
        std::error_code code;
        fs::remove(path, code);
    }

    ~ScratchFile()
    {
        std::error_code code;
        fs::remove(path, code);
    }
};

AudioFileData makeTestSignal(std::size_t channels, FrameCount frames, double sampleRate)
{
    AudioFileData data;
    data.sampleRate   = sampleRate;
    data.channelCount = channels;
    data.frameCount   = frames;
    data.channels.assign(channels, std::vector<Sample>(static_cast<std::size_t>(frames)));

    for (std::size_t channel = 0; channel < channels; ++channel)
        for (FrameCount frame = 0; frame < frames; ++frame)
            data.channels[channel][static_cast<std::size_t>(frame)] = static_cast<Sample>(
                0.5 * std::sin(2.0 * 3.14159265358979 * (220.0 + 110.0 * static_cast<double>(channel))
                               * static_cast<double>(frame) / sampleRate));

    return data;
}

std::vector<std::uint8_t> fileBytes(const fs::path& path)
{
    std::ifstream file{path, std::ios::binary | std::ios::ate};
    REQUIRE(bool(file));

    const std::streamsize size = file.tellg();
    file.seekg(0);

    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
    REQUIRE(bool(file.read(reinterpret_cast<char*>(bytes.data()), size)));
    return bytes;
}

/// Streams `data` in ragged block sizes — recording never delivers neat
/// uniform blocks, and the writer must not care.
void streamInBlocks(WavStreamWriter& writer, const AudioFileData& data)
{
    const FrameCount blockSizes[] = {1, 7, 64, 113, 480};

    FrameCount position = 0;
    std::size_t which   = 0;

    while (position < data.frameCount) {
        const FrameCount want   = blockSizes[which % (sizeof(blockSizes) / sizeof(blockSizes[0]))];
        const FrameCount frames = std::min<FrameCount>(want, data.frameCount - position);

        std::vector<const Sample*> pointers(data.channelCount);
        for (std::size_t channel = 0; channel < data.channelCount; ++channel)
            pointers[channel] = data.channels[channel].data() + position;

        REQUIRE(bool(writer.append(pointers.data(), data.channelCount, frames)));

        position += frames;
        ++which;
    }
}

} // namespace

TEST_CASE("streamed file is byte-identical to the one-shot writer")
{
    const auto data = makeTestSignal(2, 4801, 48000.0);   // deliberately not a block multiple

    for (const auto format : {WavFile::Format::float32, WavFile::Format::pcm16, WavFile::Format::pcm24}) {
        ScratchFile oneShot{"incdaw-stream-oneshot.wav"};
        ScratchFile streamed{"incdaw-stream-blocks.wav"};

        REQUIRE(bool(WavFile::write(oneShot.path, data, format)));

        WavStreamWriter writer;
        REQUIRE(bool(writer.open(streamed.path, data.sampleRate, data.channelCount, format)));
        streamInBlocks(writer, data);
        REQUIRE(writer.frameCount() == data.frameCount);
        REQUIRE(bool(writer.finalize()));

        CHECK(fileBytes(streamed.path) == fileBytes(oneShot.path));
    }
}

TEST_CASE("interleaved append matches planar append")
{
    const auto data = makeTestSignal(2, 960, 48000.0);

    ScratchFile planar{"incdaw-stream-planar.wav"};
    ScratchFile interleaved{"incdaw-stream-interleaved.wav"};

    {
        WavStreamWriter writer;
        REQUIRE(bool(writer.open(planar.path, data.sampleRate, data.channelCount)));
        streamInBlocks(writer, data);
        REQUIRE(bool(writer.finalize()));
    }

    {
        std::vector<Sample> frames(static_cast<std::size_t>(data.frameCount) * data.channelCount);
        for (FrameCount frame = 0; frame < data.frameCount; ++frame)
            for (std::size_t channel = 0; channel < data.channelCount; ++channel)
                frames[static_cast<std::size_t>(frame) * data.channelCount + channel]
                    = data.channels[channel][static_cast<std::size_t>(frame)];

        WavStreamWriter writer;
        REQUIRE(bool(writer.open(interleaved.path, data.sampleRate, data.channelCount)));

        // Two appends, split at an odd frame, to exercise the running count.
        REQUIRE(bool(writer.appendInterleaved(frames.data(), 313)));
        REQUIRE(bool(writer.appendInterleaved(frames.data() + 313 * data.channelCount,
                                              data.frameCount - 313)));
        REQUIRE(bool(writer.finalize()));
    }

    CHECK(fileBytes(interleaved.path) == fileBytes(planar.path));
}

TEST_CASE("streamed file round-trips through the reader")
{
    const auto data = makeTestSignal(1, 2048, 44100.0);

    ScratchFile scratch{"incdaw-stream-roundtrip.wav"};

    WavStreamWriter writer;
    REQUIRE(bool(writer.open(scratch.path, data.sampleRate, data.channelCount)));
    streamInBlocks(writer, data);
    REQUIRE(bool(writer.finalize()));

    AudioFileData loaded;
    REQUIRE(bool(WavFile::read(scratch.path, loaded)));

    REQUIRE(loaded.frameCount == data.frameCount);
    REQUIRE(loaded.channelCount == data.channelCount);

    for (FrameCount frame = 0; frame < data.frameCount; ++frame)
        REQUIRE(loaded.channels[0][static_cast<std::size_t>(frame)]
                == data.channels[0][static_cast<std::size_t>(frame)]);
}

TEST_CASE("an unfinalized file reads as an empty take, not garbage")
{
    // The header is written with zero sizes and patched on finalize; a crash
    // before the patch must leave a file that probes as zero frames rather
    // than one whose header lies.
    const auto data = makeTestSignal(2, 512, 48000.0);

    ScratchFile scratch{"incdaw-stream-unfinalized.wav"};

    {
        WavStreamWriter writer;
        REQUIRE(bool(writer.open(scratch.path, data.sampleRate, data.channelCount)));
        streamInBlocks(writer, data);

        // Simulate the crash: drop the file handle without patching by
        // closing the underlying stream through finalize... which we cannot
        // skip via the public API — the destructor finalizes deliberately.
        // So instead copy the file mid-stream, before finalize runs.
        std::ifstream source{scratch.path, std::ios::binary};
        std::ofstream copy{scratch.path.string() + ".crash", std::ios::binary};
        copy << source.rdbuf();
        REQUIRE(bool(writer.finalize()));
    }

    AudioFileData probed;
    REQUIRE(bool(WavFile::probe(scratch.path.string() + ".crash", probed)));
    CHECK(probed.frameCount == 0);

    std::error_code code;
    fs::remove(scratch.path.string() + ".crash", code);
}

TEST_CASE("append after finalize and mismatched channel counts are refused")
{
    const auto data = makeTestSignal(2, 64, 48000.0);

    ScratchFile scratch{"incdaw-stream-misuse.wav"};

    WavStreamWriter writer;
    REQUIRE(bool(writer.open(scratch.path, data.sampleRate, 2)));

    const Sample* pointers[] = {data.channels[0].data(), data.channels[1].data()};
    CHECK_FALSE(bool(writer.append(pointers, 1, 64)));       // wrong channel count
    REQUIRE(bool(writer.append(pointers, 2, 64)));

    REQUIRE(bool(writer.finalize()));
    CHECK_FALSE(bool(writer.append(pointers, 2, 64)));       // closed
    CHECK_FALSE(bool(writer.finalize()));                    // double finalize
}
