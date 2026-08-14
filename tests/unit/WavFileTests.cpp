// Phase 12 (part 1) — the WAV codec.
//
// The load-bearing test is the round trip: what is written is what is read
// back, bit-exactly for float and within one quantisation step for PCM. A
// codec that is approximately right corrupts every recording it ever touches.

#include "doctest.h"

#include "engine/audio/WavFile.h"

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

} // namespace

TEST_CASE("float32 round trip is bit-exact")
{
    ScratchFile scratch{"incdaw-roundtrip-f32.wav"};
    const AudioFileData written = makeTestSignal(2, 4800, 48000.0);

    REQUIRE(WavFile::write(scratch.path, written));

    AudioFileData read;
    REQUIRE(WavFile::read(scratch.path, read));

    CHECK(read.sampleRate == doctest::Approx(48000.0));
    CHECK(read.channelCount == 2);
    CHECK(read.frameCount == 4800);

    for (std::size_t channel = 0; channel < 2; ++channel)
        for (std::size_t frame = 0; frame < 4800; ++frame)
            REQUIRE(read.channels[channel][frame] == written.channels[channel][frame]);
}

TEST_CASE("pcm16 and pcm24 round trips stay within one quantisation step")
{
    const AudioFileData written = makeTestSignal(1, 480, 44100.0);

    SUBCASE("16-bit")
    {
        ScratchFile scratch{"incdaw-roundtrip-16.wav"};
        REQUIRE(WavFile::write(scratch.path, written, WavFile::Format::pcm16));

        AudioFileData read;
        REQUIRE(WavFile::read(scratch.path, read));
        REQUIRE(read.frameCount == 480);

        for (std::size_t frame = 0; frame < 480; ++frame)
            REQUIRE(std::abs(read.channels[0][frame] - written.channels[0][frame]) < 1.0f / 32768.0f);
    }

    SUBCASE("24-bit")
    {
        ScratchFile scratch{"incdaw-roundtrip-24.wav"};
        REQUIRE(WavFile::write(scratch.path, written, WavFile::Format::pcm24));

        AudioFileData read;
        REQUIRE(WavFile::read(scratch.path, read));
        REQUIRE(read.frameCount == 480);

        // 24-bit negative values exercise the sign extension; check explicitly.
        for (std::size_t frame = 0; frame < 480; ++frame)
            REQUIRE(std::abs(read.channels[0][frame] - written.channels[0][frame]) < 1.0f / 8388608.0f);
    }
}

TEST_CASE("probe reports metadata without decoding")
{
    ScratchFile scratch{"incdaw-probe.wav"};
    REQUIRE(WavFile::write(scratch.path, makeTestSignal(2, 9600, 96000.0)));

    AudioFileData info;
    REQUIRE(WavFile::probe(scratch.path, info));

    CHECK(info.sampleRate == doctest::Approx(96000.0));
    CHECK(info.channelCount == 2);
    CHECK(info.frameCount == 9600);
    CHECK(info.channels.empty());   // probe must not decode
}

TEST_CASE("chunk walking skips foreign chunks")
{
    // A file with a LIST chunk between fmt and data, the shape broadcast
    // recorders produce. Built by hand so the test does not depend on the
    // writer's own layout.
    ScratchFile scratch{"incdaw-chunks.wav"};

    AudioFileData source = makeTestSignal(1, 4, 48000.0);
    REQUIRE(WavFile::write(scratch.path, source, WavFile::Format::pcm16));

    std::ifstream in{scratch.path, std::ios::binary};
    std::vector<char> bytes{std::istreambuf_iterator<char>(in), {}};
    in.close();

    // Splice a LIST chunk after the fmt chunk (which ends at offset 36 in the
    // writer's layout): a 7-byte body, odd, so the spec requires a pad byte —
    // which is exactly the case a naive walker gets wrong.
    const char list[] = {'L','I','S','T', 7,0,0,0, 'I','N','F','O','x','y','z', 0};
    bytes.insert(bytes.begin() + 36, list, list + sizeof(list));

    // Patch the RIFF size.
    const auto riffSize = static_cast<std::uint32_t>(bytes.size() - 8);
    bytes[4] = static_cast<char>(riffSize & 0xFF);
    bytes[5] = static_cast<char>((riffSize >> 8) & 0xFF);
    bytes[6] = static_cast<char>((riffSize >> 16) & 0xFF);
    bytes[7] = static_cast<char>((riffSize >> 24) & 0xFF);

    std::ofstream out{scratch.path, std::ios::binary | std::ios::trunc};
    out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    out.close();

    AudioFileData read;
    REQUIRE(WavFile::read(scratch.path, read));
    CHECK(read.frameCount == 4);
}

TEST_CASE("garbage is refused with a reason, not decoded into noise")
{
    ScratchFile scratch{"incdaw-garbage.wav"};

    std::ofstream out{scratch.path, std::ios::binary};
    out << "this is not a wav file at all, but it is long enough to try";
    out.close();

    AudioFileData read;
    const auto result = WavFile::read(scratch.path, read);
    CHECK_FALSE(result);
    CHECK(!result.error.empty());

    AudioFileData missing;
    CHECK_FALSE(WavFile::read(fs::temp_directory_path() / "incdaw-does-not-exist.wav", missing));
}
