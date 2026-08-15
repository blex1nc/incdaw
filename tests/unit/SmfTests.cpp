// Phase 17 — Standard MIDI File exchange.
//
// The load-bearing test is the round trip: what INCDAW writes, INCDAW reads
// back identically — notes, tempo, signatures, names. Import lands as an
// editable pattern, which is the pattern workflow's answer to foreign MIDI.

#include "doctest.h"

#include "engine/midi/SmfFile.h"
#include "project/MidiFile.h"
#include "project/Model.h"

#include <filesystem>
#include <fstream>
#include <string>

using namespace incdaw;
using incdaw::engine::ticksPerQuarterNote;

namespace fs = std::filesystem;

namespace {

struct ScratchDirectory {
    explicit ScratchDirectory(const std::string& name)
        : path(fs::temp_directory_path()
               / ("incdaw-smf-" + name + "-" + std::to_string(nextSerial())))
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

} // namespace

TEST_CASE("an SMF document round-trips through the file")
{
    ScratchDirectory scratch{"roundtrip"};
    const fs::path file = scratch.path / "song.mid";

    engine::SmfDocument original;
    original.tempo          = {{0, 120.0}, {ticksPerQuarterNote * 8, 90.0}};
    original.timeSignatures = {{0, {4, 4}}, {ticksPerQuarterNote * 8, {7, 8}}};

    engine::SmfTrack lead;
    lead.name  = "Lead";
    lead.notes = {
        {0, ticksPerQuarterNote, 0, 60, 100},
        {ticksPerQuarterNote, ticksPerQuarterNote / 2, 0, 64, 80},
        {ticksPerQuarterNote * 2, ticksPerQuarterNote * 4, 0, 67, 127},
    };

    engine::SmfTrack drums;
    drums.name  = "Drums";
    drums.notes = {
        {0, 120, 9, 36, 110},
        {ticksPerQuarterNote, 120, 9, 38, 90},
    };

    original.tracks = {lead, drums};

    REQUIRE(engine::SmfFile::write(file, original));

    engine::SmfDocument loaded;
    REQUIRE(engine::SmfFile::read(file, loaded));

    REQUIRE(loaded.tempo.size() == 2);
    CHECK(loaded.tempo[0].beatsPerMinute == doctest::Approx(120.0).epsilon(0.001));
    CHECK(loaded.tempo[1].beatsPerMinute == doctest::Approx(90.0).epsilon(0.001));
    CHECK(loaded.tempo[1].tick == ticksPerQuarterNote * 8);

    REQUIRE(loaded.timeSignatures.size() == 2);
    CHECK(loaded.timeSignatures[1].signature.numerator == 7);
    CHECK(loaded.timeSignatures[1].signature.denominator == 8);

    REQUIRE(loaded.tracks.size() == 2);
    CHECK(loaded.tracks[0].name == "Lead");
    CHECK(loaded.tracks[1].name == "Drums");

    REQUIRE(loaded.tracks[0].notes.size() == 3);
    for (std::size_t index = 0; index < 3; ++index) {
        CHECK(loaded.tracks[0].notes[index].startTick == lead.notes[index].startTick);
        CHECK(loaded.tracks[0].notes[index].lengthTicks == lead.notes[index].lengthTicks);
        CHECK(loaded.tracks[0].notes[index].key == lead.notes[index].key);
        CHECK(loaded.tracks[0].notes[index].velocity == lead.notes[index].velocity);
    }

    CHECK(loaded.tracks[1].notes[0].channel == 9);
}

TEST_CASE("a foreign division rescales into INCDAW ticks")
{
    // Hand-built minimal SMF at 96 PPQN: one note, a quarter long, starting
    // one quarter in. INCDAW reads it at its own 960.
    ScratchDirectory scratch{"division"};
    const fs::path file = scratch.path / "division.mid";

    const std::uint8_t bytes[] = {
        'M', 'T', 'h', 'd', 0, 0, 0, 6, 0, 1, 0, 1, 0, 96,
        'M', 'T', 'r', 'k', 0, 0, 0, 12,
        0x60, 0x90, 60, 100,   // delta 96 (one quarter), note on C4
        0x60, 0x80, 60, 64,    // delta 96, note off
        0x00, 0xFF, 0x2F, 0x00,
    };

    {
        std::ofstream out(file, std::ios::binary);
        out.write(reinterpret_cast<const char*>(bytes), sizeof(bytes));
    }

    engine::SmfDocument loaded;
    REQUIRE(engine::SmfFile::read(file, loaded));

    REQUIRE(loaded.tracks.size() == 1);
    REQUIRE(loaded.tracks[0].notes.size() == 1);
    CHECK(loaded.tracks[0].notes[0].startTick == ticksPerQuarterNote);
    CHECK(loaded.tracks[0].notes[0].lengthTicks == ticksPerQuarterNote);
}

TEST_CASE("the arrangement exports and imports as an editable pattern")
{
    ScratchDirectory scratch{"project"};
    const fs::path file = scratch.path / "arrangement.mid";

    // A project whose arrangement holds two placements of one pattern.
    project::Project source;
    const auto channel = source.addChannel("Keys").id;
    const auto track   = source.addTrack(project::TrackType::instrument, "T").id;

    auto& pattern  = source.addPattern("Motif");
    pattern.length = ticksPerQuarterNote * 4;

    project::MidiEvent note;
    note.type     = project::MidiEventType::note;
    note.duration = ticksPerQuarterNote;
    note.key      = 62;
    note.value    = 96;
    pattern.contentFor(channel).events.push_back(note);

    for (const project::Tick start : {project::Tick{0}, ticksPerQuarterNote * 4}) {
        auto& clip       = source.addClip(project::ClipType::pattern, track, pattern.id);
        clip.startTick   = start;
        clip.lengthTicks = pattern.length;
    }

    REQUIRE(project::exportArrangement(source, file));

    // Into a fresh project: a new pattern, a channel per track, notes intact.
    project::Project destination;
    const auto imported = project::importAsPattern(destination, file);
    REQUIRE(imported);

    REQUIRE(imported.newChannels.size() == 1);
    CHECK(destination.findChannel(imported.newChannels[0])->name == "Keys");

    const project::Pattern* landed = destination.findPattern(imported.pattern);
    REQUIRE(landed != nullptr);

    const auto* events = landed->events(imported.newChannels[0]);
    REQUIRE(events != nullptr);
    REQUIRE(events->size() == 2);   // both placements, flattened by the export

    CHECK((*events)[0].tick == 0);
    CHECK((*events)[1].tick == ticksPerQuarterNote * 4);
    CHECK((*events)[0].key == 62);
    CHECK((*events)[0].value == 96);

    // The pattern is long enough to hold its content, in whole bars.
    CHECK(landed->length >= ticksPerQuarterNote * 5);
    CHECK(landed->length % (ticksPerQuarterNote * 4) == 0);
}

TEST_CASE("unreadable MIDI files refuse cleanly")
{
    ScratchDirectory scratch{"bad"};
    const fs::path file = scratch.path / "not-midi.mid";

    {
        std::ofstream out(file, std::ios::binary);
        out << "this is not a midi file";
    }

    engine::SmfDocument document;
    CHECK(!engine::SmfFile::read(file, document));

    project::Project project;
    CHECK(!project::importAsPattern(project, file));
    CHECK(project.patterns().empty());
}
