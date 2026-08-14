// Phase 8 — the pattern system.
//
// The exit criterion in docs/ROADMAP.md is the load-bearing test here: one
// pattern placed several times in the arrangement must play identically at
// every placement, and editing it must change all of them. That property is
// what makes a pattern a pattern rather than a copy.

#include "doctest.h"

#include "engine/core/AudioBufferPool.h"
#include "engine/transport/TempoMap.h"
#include "project/Model.h"
#include "project/PatternCompiler.h"
#include "project/ProjectGraphCompiler.h"

#include <chrono>
#include <vector>

using namespace incdaw;
using incdaw::engine::SequencedNote;
using incdaw::engine::Tick;
using incdaw::engine::ticksPerQuarterNote;

namespace {

project::MidiEvent note(Tick tick, int key = 60, Tick duration = 120, int velocity = 100)
{
    project::MidiEvent event;
    event.type     = project::MidiEventType::note;
    event.tick     = tick;
    event.duration = duration;
    event.key      = key;
    event.value    = velocity;
    return event;
}

std::vector<Tick> startsOf(const std::vector<SequencedNote>& notes)
{
    std::vector<Tick> ticks;
    ticks.reserve(notes.size());
    for (const SequencedNote& entry : notes)
        ticks.push_back(entry.startTick);

    return ticks;
}

/// Note starts relative to the first, which is what "the same phrase" means
/// once it has been moved somewhere else on the timeline.
std::vector<Tick> shapeOf(const std::vector<SequencedNote>& notes, Tick origin)
{
    std::vector<Tick> shape;
    shape.reserve(notes.size());
    for (const SequencedNote& entry : notes)
        shape.push_back(entry.startTick - origin);

    return shape;
}

} // namespace

// ── Per-channel content ───────────────────────────────────────────────────────

TEST_CASE("a pattern holds independent content per channel")
{
    project::Project project;
    const auto kick  = project.addChannel("Kick").id;
    const auto snare = project.addChannel("Snare").id;

    auto& pattern = project.addPattern("Beat");
    pattern.contentFor(kick).events.push_back(note(0, 36));
    pattern.contentFor(kick).events.push_back(note(ticksPerQuarterNote * 2, 36));
    pattern.contentFor(snare).events.push_back(note(ticksPerQuarterNote, 38));

    const auto kickNotes  = project::compilePattern(pattern, kick);
    const auto snareNotes = project::compilePattern(pattern, snare);

    REQUIRE(kickNotes.size() == 2);
    REQUIRE(snareNotes.size() == 1);
    CHECK(kickNotes[0].key == 36);
    CHECK(snareNotes[0].key == 38);

    // A channel with no content in this pattern compiles to nothing, rather
    // than to the other channels' notes.
    CHECK(project::compilePattern(pattern, project.addChannel("Hat").id).empty());
}

TEST_CASE("content blocks are created on demand and reused")
{
    project::Project project;
    const auto channel = project.addChannel("Lead").id;

    auto& pattern = project.addPattern("Riff");
    CHECK(pattern.content(channel) == nullptr);

    pattern.contentFor(channel).events.push_back(note(0));
    pattern.contentFor(channel).events.push_back(note(240));

    REQUIRE(pattern.channels.size() == 1);          // not a second block
    CHECK(pattern.totalEventCount() == 2);
    CHECK(pattern.events(channel) != nullptr);
}

// ── Swing ─────────────────────────────────────────────────────────────────────

TEST_CASE("swing delays off-beats and leaves down-beats alone")
{
    project::Project project;
    const auto channel = project.addChannel("Hat").id;

    auto& pattern = project.addPattern("Shuffle");
    pattern.length    = ticksPerQuarterNote * 4;
    pattern.swingGrid = ticksPerQuarterNote / 2;    // eighths

    for (int index = 0; index < 8; ++index)
        pattern.contentFor(channel).events.push_back(
            note(static_cast<Tick>(index) * (ticksPerQuarterNote / 2)));

    const auto straight = startsOf(project::compilePattern(pattern, channel));

    pattern.swing = 1.0;
    const auto swung = startsOf(project::compilePattern(pattern, channel));

    REQUIRE(straight.size() == swung.size());

    for (std::size_t index = 0; index < swung.size(); ++index) {
        if (index % 2 == 0)
            CHECK(swung[index] == straight[index]);                        // down-beat, untouched
        else
            CHECK(swung[index] == straight[index] + ticksPerQuarterNote / 4);   // half a subdivision
    }
}

TEST_CASE("swing of zero changes nothing, and swing is deterministic")
{
    project::Project project;
    const auto channel = project.addChannel("Hat").id;

    auto& pattern = project.addPattern("Shuffle");
    for (int index = 0; index < 16; ++index)
        pattern.contentFor(channel).events.push_back(
            note(static_cast<Tick>(index) * (ticksPerQuarterNote / 4)));

    const auto reference = startsOf(project::compilePattern(pattern, channel));

    pattern.swing = 0.0;
    CHECK(startsOf(project::compilePattern(pattern, channel)) == reference);

    pattern.swing = 0.6;
    const auto once  = startsOf(project::compilePattern(pattern, channel));
    const auto twice = startsOf(project::compilePattern(pattern, channel));
    CHECK(once == twice);
    CHECK(once != reference);
}

TEST_CASE("swing leaves notes that are not on the grid alone")
{
    // A note played deliberately off the grid is expression. Shuffling it would
    // destroy the timing the user placed rather than add groove to it.
    project::Project project;
    const auto channel = project.addChannel("Keys").id;

    auto& pattern = project.addPattern("Loose");
    pattern.swing     = 1.0;
    pattern.swingGrid = ticksPerQuarterNote / 2;

    const Tick offGrid = ticksPerQuarterNote / 2 + 57;   // an off-beat, played late
    pattern.contentFor(channel).events.push_back(note(offGrid));

    const auto compiled = project::compilePattern(pattern, channel);
    REQUIRE(compiled.size() == 1);
    CHECK(compiled[0].startTick == offGrid);
}

// ── Polymetry ─────────────────────────────────────────────────────────────────

TEST_CASE("a channel with a shorter loop repeats inside the pattern")
{
    project::Project project;
    const auto three = project.addChannel("Three").id;
    const auto four  = project.addChannel("Four").id;

    auto& pattern = project.addPattern("Polymetric");
    pattern.length = ticksPerQuarterNote * 12;      // twelve beats

    auto& threeContent = pattern.contentFor(three);
    threeContent.loopLength = ticksPerQuarterNote * 3;
    threeContent.events.push_back(note(0, 60));

    auto& fourContent = pattern.contentFor(four);
    fourContent.loopLength = ticksPerQuarterNote * 4;
    fourContent.events.push_back(note(0, 64));

    const auto threeNotes = startsOf(project::compilePattern(pattern, three));
    const auto fourNotes  = startsOf(project::compilePattern(pattern, four));

    CHECK(threeNotes == std::vector<Tick>{0, ticksPerQuarterNote * 3,
                                         ticksPerQuarterNote * 6, ticksPerQuarterNote * 9});
    CHECK(fourNotes  == std::vector<Tick>{0, ticksPerQuarterNote * 4, ticksPerQuarterNote * 8});

    // Twelve beats is where a 3 and a 4 meet again — both start together only
    // at 0 and at the end of the pattern.
    CHECK(threeNotes.back() + ticksPerQuarterNote * 3 == pattern.length);
    CHECK(fourNotes.back() + ticksPerQuarterNote * 4 == pattern.length);
}

TEST_CASE("a repeat never spills past the end of its pattern")
{
    project::Project project;
    const auto channel = project.addChannel("Odd").id;

    auto& pattern = project.addPattern("Ragged");
    pattern.length = ticksPerQuarterNote * 5;       // not a multiple of the loop

    auto& content = pattern.contentFor(channel);
    content.loopLength = ticksPerQuarterNote * 2;
    content.events.push_back(note(0));
    content.events.push_back(note(ticksPerQuarterNote));

    for (const SequencedNote& compiled : project::compilePattern(pattern, channel))
        CHECK(compiled.startTick < pattern.length);
}

// ── Phase 8 exit criterion ────────────────────────────────────────────────────

TEST_CASE("one pattern placed several times plays identically at each placement")
{
    // docs/ROADMAP.md Phase 8. The placements share the pattern rather than
    // owning copies of it, which is the whole point of a pattern.
    project::Project project;
    const auto channel = project.addChannel("Lead").id;
    const auto track   = project.addTrack(project::TrackType::instrument, "Track 1").id;

    auto& pattern  = project.addPattern("Motif");
    pattern.length = ticksPerQuarterNote * 4;

    auto& content = pattern.contentFor(channel);
    content.events.push_back(note(0, 60));
    content.events.push_back(note(ticksPerQuarterNote, 64));
    content.events.push_back(note(ticksPerQuarterNote * 2 + 60, 67));   // deliberately off-grid

    const std::vector<Tick> placements{0, ticksPerQuarterNote * 4, ticksPerQuarterNote * 16};
    for (const Tick start : placements) {
        auto& clip = project.addClip(project::ClipType::pattern, track, pattern.id);
        clip.startTick   = start;
        clip.lengthTicks = pattern.length;
    }

    const auto compiled = project::compileArrangement(project, channel);
    REQUIRE(compiled.size() == content.events.size() * placements.size());

    // Each placement carries the same phrase, at its own offset.
    std::vector<SequencedNote> perPlacement[3];
    for (const SequencedNote& entry : compiled) {
        for (std::size_t index = 0; index < placements.size(); ++index) {
            if (entry.startTick >= placements[index]
                && entry.startTick < placements[index] + pattern.length)
                perPlacement[index].push_back(entry);
        }
    }

    const auto reference = shapeOf(perPlacement[0], placements[0]);
    REQUIRE(reference.size() == 3);
    for (std::size_t index = 1; index < placements.size(); ++index)
        CHECK(shapeOf(perPlacement[index], placements[index]) == reference);

    // Editing the pattern changes every placement, because none of them holds a
    // copy of it.
    content.events.push_back(note(ticksPerQuarterNote * 3, 72));

    const auto edited = project::compileArrangement(project, channel);
    CHECK(edited.size() == compiled.size() + placements.size());

    for (const Tick start : placements) {
        const bool present = std::any_of(edited.begin(), edited.end(),
                                         [start](const SequencedNote& entry) {
                                             return entry.key == 72
                                                 && entry.startTick == start + ticksPerQuarterNote * 3;
                                         });
        CHECK(present);
    }
}

TEST_CASE("a clip shorter than its pattern trims it, a longer one repeats it")
{
    project::Project project;
    const auto channel = project.addChannel("Lead").id;
    const auto track   = project.addTrack(project::TrackType::instrument, "Track 1").id;

    auto& pattern  = project.addPattern("Four");
    pattern.length = ticksPerQuarterNote * 4;

    auto& content = pattern.contentFor(channel);
    for (int index = 0; index < 4; ++index)
        content.events.push_back(note(static_cast<Tick>(index) * ticksPerQuarterNote));

    auto& trimmed = project.addClip(project::ClipType::pattern, track, pattern.id);
    trimmed.startTick   = 0;
    trimmed.lengthTicks = ticksPerQuarterNote * 2;      // half the pattern

    CHECK(startsOf(project::compileArrangement(project, channel))
          == std::vector<Tick>{0, ticksPerQuarterNote});

    project.clips().clear();

    auto& stretched = project.addClip(project::ClipType::pattern, track, pattern.id);
    stretched.startTick   = ticksPerQuarterNote * 8;
    stretched.lengthTicks = ticksPerQuarterNote * 8;    // two passes of the pattern

    const auto repeated = startsOf(project::compileArrangement(project, channel));
    CHECK(repeated.size() == 8);
    CHECK(repeated.front() == ticksPerQuarterNote * 8);
    CHECK(repeated.back() == ticksPerQuarterNote * 15);
}

TEST_CASE("a muted clip is silent and does not shift the others")
{
    project::Project project;
    const auto channel = project.addChannel("Lead").id;
    const auto track   = project.addTrack(project::TrackType::instrument, "Track 1").id;

    auto& pattern = project.addPattern("Motif");
    pattern.contentFor(channel).events.push_back(note(0));

    auto& audible = project.addClip(project::ClipType::pattern, track, pattern.id);
    audible.startTick = 0;

    auto& muted = project.addClip(project::ClipType::pattern, track, pattern.id);
    muted.startTick = ticksPerQuarterNote * 4;
    muted.muted     = true;

    CHECK(startsOf(project::compileArrangement(project, channel)) == std::vector<Tick>{0});
    CHECK(project::arrangementLengthTicks(project) == ticksPerQuarterNote * 8);
}

// ── Project graph ─────────────────────────────────────────────────────────────

TEST_CASE("the project compiles to a graph with one instrument per channel")
{
    project::Project project;
    const auto lead = project.addChannel("Lead").id;
    const auto bass = project.addChannel("Bass").id;

    auto& pattern = project.addPattern("Both");
    pattern.contentFor(lead).events.push_back(note(0, 72));
    pattern.contentFor(bass).events.push_back(note(0, 36));

    const engine::TempoMap map{120.0, 48000.0};

    project::GraphCompileOptions options;
    options.sampleRate   = 48000.0;
    options.maxBlockSize = 256;
    options.pattern      = pattern.id;

    const auto compiled = project::compileProjectGraph(project, map, options);

    REQUIRE(compiled);
    CHECK(compiled.channels.size() == 2);
    CHECK(compiled.instrumentFor(lead) != nullptr);
    CHECK(compiled.instrumentFor(bass) != nullptr);

    // Each instrument got its own channel's notes, not the pattern's whole
    // contents.
    REQUIRE(compiled.instrumentFor(lead)->sequence().noteCount() == 1);
    CHECK(compiled.instrumentFor(lead)->sequence().notes()[0].key == 72);
    CHECK(compiled.instrumentFor(bass)->sequence().notes()[0].key == 36);

    // Instrument + gain per channel, plus the master.
    CHECK(compiled.graph->nodeCount() == 5);
}

TEST_CASE("muting and soloing decide what reaches the graph")
{
    project::Project project;
    const auto lead = project.addChannel("Lead").id;
    const auto bass = project.addChannel("Bass").id;

    auto& pattern = project.addPattern("Both");
    pattern.contentFor(lead).events.push_back(note(0, 72));
    pattern.contentFor(bass).events.push_back(note(0, 36));

    const engine::TempoMap map{120.0, 48000.0};

    project::GraphCompileOptions options;
    options.pattern = pattern.id;

    SUBCASE("a muted channel is left out entirely")
    {
        project.channels()[0].muted = true;

        const auto compiled = project::compileProjectGraph(project, map, options);
        REQUIRE(compiled);
        CHECK(compiled.instrumentFor(lead) == nullptr);
        CHECK(compiled.instrumentFor(bass) != nullptr);
    }

    SUBCASE("solo silences everything that is not soloed")
    {
        project.channels()[0].soloed = true;

        const auto compiled = project::compileProjectGraph(project, map, options);
        REQUIRE(compiled);
        CHECK(compiled.instrumentFor(lead) != nullptr);
        CHECK(compiled.instrumentFor(bass) == nullptr);
    }

    SUBCASE("muting everything still yields a renderable graph")
    {
        project.channels()[0].muted = true;
        project.channels()[1].muted = true;

        const auto compiled = project::compileProjectGraph(project, map, options);
        REQUIRE(compiled);           // silence, not a failure
        CHECK(compiled.channels.empty());

        engine::AudioBufferPool output;
        output.allocate(1, 2, 256);
        compiled.graph->process(output.buffer(0), 256, 0);

        CHECK(output.buffer(0).peak() == doctest::Approx(0.0f));
    }
}

TEST_CASE("a channel with no instrument is silent rather than an error")
{
    project::Project project;
    project.addChannel("Empty");
    project.addPattern("Pattern");

    const engine::TempoMap map{120.0, 48000.0};

    project::GraphCompileOptions options;
    options.instrumentFactory = [](const project::Channel&) {
        return std::unique_ptr<engine::Instrument>{};
    };

    const auto compiled = project::compileProjectGraph(project, map, options);

    REQUIRE(compiled);
    CHECK(compiled.channels.empty());
}

TEST_CASE("the arrangement drives the graph in song mode")
{
    project::Project project;
    const auto channel = project.addChannel("Lead").id;
    const auto track   = project.addTrack(project::TrackType::instrument, "Track 1").id;

    auto& pattern  = project.addPattern("Motif");
    pattern.length = ticksPerQuarterNote * 4;
    pattern.contentFor(channel).events.push_back(note(0, 60));

    for (const Tick start : {Tick{0}, ticksPerQuarterNote * 8}) {
        auto& clip = project.addClip(project::ClipType::pattern, track, pattern.id);
        clip.startTick   = start;
        clip.lengthTicks = pattern.length;
    }

    const engine::TempoMap map{120.0, 48000.0};

    project::GraphCompileOptions options;
    options.source = project::PlaybackSource::arrangement;

    const auto compiled = project::compileProjectGraph(project, map, options);

    REQUIRE(compiled);
    const engine::InstrumentNode* node = compiled.instrumentFor(channel);
    REQUIRE(node != nullptr);
    REQUIRE(node->sequence().noteCount() == 2);
    CHECK(node->sequence().notes()[1].startTick == ticksPerQuarterNote * 8);
}

// ── Performance ───────────────────────────────────────────────────────────────

TEST_CASE("a full recompile stays well inside an edit's budget")
{
    // Every edit recompiles the whole graph (docs/ARCHITECTURE.md §7). Edits
    // happen at human speed, but a recompile that took tens of milliseconds
    // would still be felt as lag on a drag.
    project::Project project;
    const auto track = project.addTrack(project::TrackType::instrument, "Track 1").id;

    std::vector<project::EntityId> channels;
    for (int index = 0; index < 16; ++index)
        channels.push_back(project.addChannel("Channel").id);

    auto& pattern  = project.addPattern("Dense");
    pattern.length = ticksPerQuarterNote * 16;

    for (const project::EntityId channel : channels) {
        auto& content = pattern.contentFor(channel);
        for (int index = 0; index < 500; ++index)
            content.events.push_back(note(static_cast<Tick>(index) * 15, 40 + index % 40));
    }

    for (int index = 0; index < 32; ++index) {
        auto& clip = project.addClip(project::ClipType::pattern, track, pattern.id);
        clip.startTick   = static_cast<Tick>(index) * pattern.length;
        clip.lengthTicks = pattern.length;
    }

    const engine::TempoMap map{120.0, 48000.0};

    project::GraphCompileOptions options;
    options.source = project::PlaybackSource::arrangement;

    const auto started = std::chrono::steady_clock::now();
    const auto compiled = project::compileProjectGraph(project, map, options);
    const auto elapsed = std::chrono::duration<double, std::milli>(
                             std::chrono::steady_clock::now() - started).count();

    REQUIRE(compiled);
    CHECK(compiled.channels.size() == 16);

    MESSAGE("16 channels x 500 notes x 32 placements recompiled in " << elapsed << " ms");
    CHECK(elapsed < 250.0);   // generous: this also runs under a Debug build
}
