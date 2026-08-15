// Phase 19 — QA: fuzzing the readers.
//
// Every parser that touches bytes from outside — the project package, WAV,
// Standard MIDI Files — is fed truncations and seeded random corruption of
// valid documents. The contract under fuzz is binary: return an error or
// succeed, but never crash, never hang, never trip the sanitizer. The
// corpus is deterministic (fixed seeds), so a failure here reproduces
// exactly and becomes a permanent regression test by construction.

#include "doctest.h"

#include "engine/audio/WavFile.h"
#include "engine/midi/SmfFile.h"
#include "project/Model.h"
#include "project/ProjectFile.h"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

using namespace incdaw;

namespace fs = std::filesystem;

namespace {

struct ScratchDirectory {
    explicit ScratchDirectory(const std::string& name)
        : path(fs::temp_directory_path()
               / ("incdaw-fuzz-" + name + "-" + std::to_string(nextSerial())))
    {
        std::error_code code;
        fs::remove_all(path, code);
        fs::create_directories(path, code);
    }

    ~ScratchDirectory()
    {
        std::error_code code;
        fs::remove_all(path, code);
    }

    fs::path path;

private:
    static int nextSerial()
    {
        static int serial = 0;
        return ++serial;
    }
};

class Random {
public:
    explicit Random(std::uint64_t seed) : state_(seed | 1) {}

    [[nodiscard]] std::uint64_t next() noexcept
    {
        state_ ^= state_ << 13;
        state_ ^= state_ >> 7;
        state_ ^= state_ << 17;
        return state_;
    }

    [[nodiscard]] std::size_t below(std::size_t bound) noexcept
    {
        return bound > 0 ? static_cast<std::size_t>(next() % bound) : 0;
    }

private:
    std::uint64_t state_;
};

std::vector<std::uint8_t> readFile(const fs::path& path)
{
    std::ifstream stream(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
}

void writeFile(const fs::path& path, const std::vector<std::uint8_t>& bytes)
{
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    stream.write(reinterpret_cast<const char*>(bytes.data()),
                 static_cast<std::streamsize>(bytes.size()));
}

/// Flips up to `flips` random bytes; deterministic per seed.
std::vector<std::uint8_t> corrupt(std::vector<std::uint8_t> bytes, Random& random, int flips)
{
    if (bytes.empty())
        return bytes;

    for (int flip = 0; flip < flips; ++flip)
        bytes[random.below(bytes.size())] = static_cast<std::uint8_t>(random.next());

    return bytes;
}

} // namespace

TEST_CASE("FUZZ: corrupted and truncated project packages never crash the loader")
{
    ScratchDirectory scratch{"project"};

    // A real package to mutate.
    const fs::path packagePath = scratch.path / "Song.incdaw";
    {
        project::Project original;
        auto& channel      = original.addChannel("Keys");
        channel.instrument = plugins::builtinSampler();

        project::ChannelSamplerZone zone;
        zone.asset = original.addAudioAsset("/nowhere/kick.wav").id;
        original.findChannel(channel.id)->samplerZones.push_back(zone);

        auto& pattern = original.addPattern("P");
        project::MidiEvent note;
        note.type = project::MidiEventType::note;
        note.duration = 240;
        pattern.contentFor(channel.id).events.push_back(note);

        original.addMidiMapping(74, "volume", original.masterMixerNode());
        REQUIRE(project::ProjectFile::save(original, packagePath));
    }

    const auto originalManifest = readFile(packagePath / "manifest.json");
    const auto originalDocument = readFile(packagePath / "project.json");
    REQUIRE(!originalManifest.empty());
    REQUIRE(!originalDocument.empty());

    Random random(0xF00DF00Du);

    SUBCASE("byte corruption of project.json")
    {
        for (int round = 0; round < 120; ++round) {
            writeFile(packagePath / "project.json",
                      corrupt(originalDocument, random, 1 + static_cast<int>(random.below(8))));

            project::Project loaded;
            const auto result = project::ProjectFile::load(loaded, packagePath);
            // Either outcome is fine; surviving the attempt is the test.
            (void)result;
        }
    }

    SUBCASE("truncation of project.json at every eighth byte")
    {
        for (std::size_t length = 0; length < originalDocument.size(); length += 8) {
            writeFile(packagePath / "project.json",
                      {originalDocument.begin(),
                       originalDocument.begin() + static_cast<std::ptrdiff_t>(length)});

            project::Project loaded;
            (void)project::ProjectFile::load(loaded, packagePath);
        }
    }

    SUBCASE("byte corruption of manifest.json")
    {
        writeFile(packagePath / "project.json", originalDocument);

        for (int round = 0; round < 120; ++round) {
            writeFile(packagePath / "manifest.json",
                      corrupt(originalManifest, random, 1 + static_cast<int>(random.below(4))));

            project::Project loaded;
            (void)project::ProjectFile::load(loaded, packagePath);
        }
    }

    SUBCASE("a corrupted pattern file costs the pattern, never the process")
    {
        writeFile(packagePath / "project.json", originalDocument);
        writeFile(packagePath / "manifest.json", originalManifest);

        for (const auto& entry : fs::directory_iterator(packagePath / "patterns")) {
            const auto patternBytes = readFile(entry.path());
            for (int round = 0; round < 40; ++round) {
                writeFile(entry.path(), corrupt(patternBytes, random, 4));

                project::Project loaded;
                (void)project::ProjectFile::load(loaded, packagePath);
            }
        }
    }
}

TEST_CASE("FUZZ: corrupted and truncated WAV files never crash the codec")
{
    ScratchDirectory scratch{"wav"};
    const fs::path wavPath = scratch.path / "tone.wav";

    {
        engine::AudioFileData data;
        data.sampleRate   = 48000.0;
        data.channelCount = 2;
        data.frameCount   = 2048;
        data.channels.assign(2, std::vector<engine::Sample>(2048, 0.25f));
        REQUIRE(engine::WavFile::write(wavPath, data));
    }

    const auto original = readFile(wavPath);
    Random     random(0xCAFED00Du);

    SUBCASE("corruption")
    {
        for (int round = 0; round < 150; ++round) {
            writeFile(wavPath,
                      corrupt(original, random, 1 + static_cast<int>(random.below(12))));

            engine::AudioFileData decoded;
            (void)engine::WavFile::read(wavPath, decoded);
            (void)engine::WavFile::probe(wavPath, decoded);
        }
    }

    SUBCASE("truncation")
    {
        for (std::size_t length = 0; length < original.size();
             length += std::max<std::size_t>(1, original.size() / 128)) {
            writeFile(wavPath, {original.begin(),
                                original.begin() + static_cast<std::ptrdiff_t>(length)});

            engine::AudioFileData decoded;
            (void)engine::WavFile::read(wavPath, decoded);
        }
    }
}

TEST_CASE("FUZZ: corrupted and truncated MIDI files never crash the reader")
{
    ScratchDirectory scratch{"smf"};
    const fs::path midiPath = scratch.path / "song.mid";

    {
        engine::SmfDocument document;
        document.tempo = {{0, 120.0}};

        engine::SmfTrack track;
        track.name = "Fuzz";
        for (int note = 0; note < 32; ++note)
            track.notes.push_back({note * 240, 240, 0, 48 + (note % 24), 100});
        document.tracks = {track};

        REQUIRE(engine::SmfFile::write(midiPath, document));
    }

    const auto original = readFile(midiPath);
    Random     random(0xBEEFFACEu);

    SUBCASE("corruption")
    {
        for (int round = 0; round < 150; ++round) {
            writeFile(midiPath,
                      corrupt(original, random, 1 + static_cast<int>(random.below(6))));

            engine::SmfDocument document;
            (void)engine::SmfFile::read(midiPath, document);
        }
    }

    SUBCASE("truncation at every fourth byte")
    {
        for (std::size_t length = 0; length < original.size(); length += 4) {
            writeFile(midiPath, {original.begin(),
                                 original.begin() + static_cast<std::ptrdiff_t>(length)});

            engine::SmfDocument document;
            (void)engine::SmfFile::read(midiPath, document);
        }
    }
}
