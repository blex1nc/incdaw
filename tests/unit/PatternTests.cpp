#include "doctest.h"

#include "app/CommandRegistry.h"
#include "app/PianoRollModel.h"
#include "app/StepSequencerModel.h"
#include "app/commands/ArrangementCommands.h"
#include "app/commands/PatternCommands.h"
#include "engine/core/AudioBufferPool.h"
#include "engine/dsp/ChannelStripNode.h"
#include "engine/instrument/InstrumentNode.h"
#include "project/GraphCompiler.h"
#include "project/PatternCompiler.h"

#include <algorithm>
#include <memory>
#include <vector>

using namespace incdaw;
using engine::SequencedNote;
using engine::Tick;
using engine::ticksPerQuarterNote;
using project::EntityId;
using project::MidiEvent;
using project::Pattern;
using project::Project;

namespace {

MidiEvent noteAt(Tick tick, int key, EntityId channel = {}, Tick duration = 120)
{
    MidiEvent note;
    note.type      = project::MidiEventType::note;
    note.tick      = tick;
    note.duration  = duration;
    note.key       = key;
    note.value     = 100;
    note.channelId = channel;
    return note;
}

/// Notes as (start, key) pairs, sorted — what "the same music" means when two
/// placements are compared.
std::vector<std::pair<Tick, int>> shape(const std::vector<SequencedNote>& notes, Tick origin = 0)
{
    std::vector<std::pair<Tick, int>> result;
    result.reserve(notes.size());

    for (const SequencedNote& note : notes)
        result.emplace_back(note.startTick - origin, note.key);

    std::sort(result.begin(), result.end());
    return result;
}

std::vector<SequencedNote> notesNear(const std::vector<SequencedNote>& notes, Tick from, Tick to)
{
    std::vector<SequencedNote> result;

    for (const SequencedNote& note : notes)
        if (note.startTick >= from && note.startTick < to)
            result.push_back(note);

    return result;
}

} // namespace

// ── Channel ownership ─────────────────────────────────────────────────────────

TEST_CASE("a pattern holds the notes of several channels at once")
{
    Project project;
    const EntityId kick  = project.addChannel("Kick").id;
    const EntityId snare = project.addChannel("Snare").id;

    Pattern& pattern = project.addPattern("Beat");
    pattern.events.push_back(noteAt(0, 36, kick));
    pattern.events.push_back(noteAt(480, 38, snare));
    pattern.events.push_back(noteAt(960, 36, kick));

    project::PatternCompileOptions options;
    options.defaultChannel = project.defaultChannel();

    options.channel = kick;
    const auto kickNotes = project::compilePattern(pattern, options);

    options.channel = snare;
    const auto snareNotes = project::compilePattern(pattern, options);

    CHECK(kickNotes.size() == 2);
    REQUIRE(snareNotes.size() == 1);
    CHECK(snareNotes[0].key == 38);
}

TEST_CASE("a note with no channel of its own plays on the first channel")
{
    // Every note written before channels existed, and every note a v1.0 project
    // file contains, arrives untagged. It must not go silent.
    Project project;
    const EntityId first  = project.addChannel("First").id;
    const EntityId second = project.addChannel("Second").id;

    Pattern& pattern = project.addPattern("Legacy");
    pattern.events.push_back(noteAt(0, 60));   // no channel id

    project::PatternCompileOptions options;
    options.defaultChannel = project.defaultChannel();

    options.channel = first;
    CHECK(project::compilePattern(pattern, options).size() == 1);

    options.channel = second;
    CHECK(project::compilePattern(pattern, options).empty());

    CHECK(project.defaultChannel() == first);
}

// ── Polymetric lengths and swing ──────────────────────────────────────────────

TEST_CASE("a channel with a shorter length repeats inside the pattern")
{
    Project project;
    const EntityId hats = project.addChannel("Hats").id;

    Pattern& pattern = project.addPattern("Polymetric");
    pattern.length       = ticksPerQuarterNote * 4;      // one bar
    pattern.stepDivision = ticksPerQuarterNote / 4;

    // Three steps of hi-hat against a sixteen-step bar: the classic polymetric
    // figure, which drifts across the bar rather than lining up with it.
    pattern.channelSettings.push_back({hats, pattern.stepDivision * 3, -1.0});

    for (int step = 0; step < 3; ++step)
        pattern.events.push_back(noteAt(step * pattern.stepDivision, 42, hats, 60));

    project::PatternCompileOptions options;
    options.channel        = hats;
    options.defaultChannel = project.defaultChannel();

    const auto notes = project::compilePattern(pattern, options);

    // 16 steps of bar / 3 steps of cycle: five full cycles plus one step.
    CHECK(notes.size() == 16);
    CHECK(pattern.lengthFor(hats) == pattern.stepDivision * 3);

    for (const SequencedNote& note : notes) {
        CHECK(note.key == 42);
        CHECK(note.startTick < pattern.length);
    }
}

TEST_CASE("swing delays the off-beat steps and undoes cleanly")
{
    Project project;
    const EntityId channel = project.addChannel("Hats").id;

    Pattern& pattern = project.addPattern("Shuffle");
    pattern.stepDivision = ticksPerQuarterNote / 4;   // 240 ticks

    for (int step = 0; step < 4; ++step)
        pattern.events.push_back(noteAt(step * pattern.stepDivision, 42, channel, 60));

    project::PatternCompileOptions options;
    options.channel        = channel;
    options.defaultChannel = project.defaultChannel();

    const auto straight = project::compilePattern(pattern, options);
    REQUIRE(straight.size() == 4);
    CHECK(straight[1].startTick == 240);

    pattern.swing = 0.5;
    const auto swung = project::compilePattern(pattern, options);
    REQUIRE(swung.size() == 4);

    CHECK(swung[0].startTick == 0);            // on-beat steps never move
    CHECK(swung[1].startTick == 240 + 60);     // half of half a step
    CHECK(swung[2].startTick == 480);
    CHECK(swung[3].startTick == 720 + 60);

    // Swing is applied at compile time, never written into the notes, so
    // turning it off restores the original timing exactly.
    pattern.swing = 0.0;
    CHECK(shape(project::compilePattern(pattern, options)) == shape(straight));
}

// ── Phase 8 exit criterion ────────────────────────────────────────────────────

TEST_CASE("one pattern placed twice plays identically at both placements")
{
    // docs/ROADMAP.md Phase 8: one pattern placed multiple times in the
    // arrangement plays identically at each placement, and editing it updates
    // all placements.
    Project project;
    const EntityId channel = project.addChannel("Lead").id;
    const EntityId track   = project.addTrack(project::TrackType::instrument, "Track 1").id;

    Pattern& pattern = project.addPattern("Riff");
    pattern.length = ticksPerQuarterNote * 4;

    for (int step = 0; step < 4; ++step)
        pattern.events.push_back(noteAt(step * ticksPerQuarterNote, 60 + step, channel, 240));

    // Some notes only play sometimes: probability must not be what makes two
    // placements differ.
    pattern.events.push_back(noteAt(ticksPerQuarterNote / 2, 72, channel, 120));
    pattern.events.back().probability = 0.5;

    const auto patternFrames = project.tempoMap().frameForTick(pattern.length);

    project::Clip& first = project.addClip(project::ClipType::pattern, track, pattern.id);
    first.start  = 0;
    first.length = patternFrames;

    project::Clip& second = project.addClip(project::ClipType::pattern, track, pattern.id);
    second.start  = patternFrames * 2;
    second.length = patternFrames;

    const auto arranged = project::compileArrangement(project, channel);

    const auto atFirst  = notesNear(arranged, 0, pattern.length);
    const auto atSecond = notesNear(arranged, pattern.length * 2, pattern.length * 3);

    REQUIRE_FALSE(atFirst.empty());
    CHECK(shape(atFirst, 0) == shape(atSecond, pattern.length * 2));

    // Editing the pattern changes both placements, because a clip references a
    // pattern rather than owning a copy of it.
    pattern.events.push_back(noteAt(ticksPerQuarterNote * 3 + 120, 84, channel, 120));

    const auto edited       = project::compileArrangement(project, channel);
    const auto editedFirst  = notesNear(edited, 0, pattern.length);
    const auto editedSecond = notesNear(edited, pattern.length * 2, pattern.length * 3);

    CHECK(editedFirst.size() == atFirst.size() + 1);
    CHECK(shape(editedFirst, 0) == shape(editedSecond, pattern.length * 2));
}

TEST_CASE("a clip longer than its pattern repeats it, and content stays inside the clip")
{
    Project project;
    const EntityId channel = project.addChannel("Bass").id;
    const EntityId track   = project.addTrack(project::TrackType::instrument, "Track 1").id;

    Pattern& pattern = project.addPattern("One bar");
    pattern.length = ticksPerQuarterNote * 4;
    pattern.events.push_back(noteAt(0, 36, channel, ticksPerQuarterNote * 8));   // runs long

    const auto barFrames = project.tempoMap().frameForTick(pattern.length);

    project::Clip& clip = project.addClip(project::ClipType::pattern, track, pattern.id);
    clip.start  = 0;
    clip.length = barFrames * 3;

    const auto notes = project::compileArrangement(project, channel);

    REQUIRE(notes.size() == 3);
    CHECK(notes[0].startTick == 0);
    CHECK(notes[1].startTick == pattern.length);
    CHECK(notes[2].startTick == pattern.length * 2);

    // A note that outlives its repeat would silence the next one on the same
    // key: its note-off lands after the following note-on.
    for (const SequencedNote& note : notes)
        CHECK(note.endTick() <= pattern.length * 3);
}

TEST_CASE("a muted clip contributes nothing and the arrangement length follows the clips")
{
    Project project;
    const EntityId channel = project.addChannel("Lead").id;
    const EntityId track   = project.addTrack(project::TrackType::instrument, "Track 1").id;

    Pattern& pattern = project.addPattern("Riff");
    pattern.length = ticksPerQuarterNote * 4;
    pattern.events.push_back(noteAt(0, 60, channel, 240));

    const auto barFrames = project.tempoMap().frameForTick(pattern.length);

    project::Clip& audible = project.addClip(project::ClipType::pattern, track, pattern.id);
    audible.start  = 0;
    audible.length = barFrames;

    project::Clip& muted = project.addClip(project::ClipType::pattern, track, pattern.id);
    muted.start  = barFrames;
    muted.length = barFrames;
    muted.muted  = true;

    CHECK(project::compileArrangement(project, channel).size() == 1);
    CHECK(project::arrangementLengthTicks(project) == pattern.length * 2);
}

// ── Step sequencer ────────────────────────────────────────────────────────────

TEST_CASE("a step is a note, and toggling one is undoable")
{
    Project project;
    app::CommandRegistry registry{project};

    const EntityId channel = project.addChannel("Kick").id;
    const EntityId pattern = project.addPattern("Beat").id;

    app::StepSequencerModel grid{project, pattern};
    REQUIRE(grid.isValid());
    CHECK(grid.stepCount() == 16);              // one bar of sixteenths
    CHECK_FALSE(grid.isStepOn(channel, 4));

    REQUIRE(registry.execute(std::make_unique<app::ToggleStepCommand>(pattern, channel, 4, 90, 36)));

    const app::StepSequencerModel afterOn{project, pattern};
    CHECK(afterOn.isStepOn(channel, 4));
    CHECK(afterOn.velocityAt(channel, 4) == 90);
    CHECK(afterOn.tickForStep(4) == ticksPerQuarterNote);

    // The same note is visible to the piano roll: one model, two views.
    const Pattern* stored = project.findPattern(pattern);
    REQUIRE(stored != nullptr);
    REQUIRE(stored->events.size() == 1);
    CHECK(stored->events[0].key == 36);
    CHECK(stored->events[0].channelId == channel);

    REQUIRE(registry.execute(std::make_unique<app::ToggleStepCommand>(pattern, channel, 4)));
    CHECK_FALSE(app::StepSequencerModel{project, pattern}.isStepOn(channel, 4));

    REQUIRE(registry.undo());
    CHECK(app::StepSequencerModel{project, pattern}.isStepOn(channel, 4));
    CHECK(app::StepSequencerModel{project, pattern}.velocityAt(channel, 4) == 90);

    REQUIRE(registry.undo());
    CHECK_FALSE(app::StepSequencerModel{project, pattern}.isStepOn(channel, 4));
}

TEST_CASE("the step grid admits what it cannot show")
{
    Project project;
    const EntityId channel = project.addChannel("Keys").id;
    Pattern& pattern = project.addPattern("Loose");

    pattern.events.push_back(noteAt(0, 60, channel));
    pattern.events.push_back(noteAt(37, 62, channel));    // nowhere near a step

    const app::StepSequencerModel grid{project, pattern.id};

    CHECK(grid.isStepOn(channel, 0));
    CHECK(grid.offGridNoteCount(channel) == 1);
}

TEST_CASE("a shorter channel has fewer steps")
{
    Project project;
    const EntityId channel = project.addChannel("Hats").id;
    Pattern& pattern = project.addPattern("Polymetric");
    pattern.channelSettings.push_back({channel, pattern.stepDivision * 6, -1.0});

    const app::StepSequencerModel grid{project, pattern.id};

    CHECK(grid.stepCount() == 16);
    CHECK(grid.stepCount(channel) == 6);
}

// ── Commands ──────────────────────────────────────────────────────────────────

TEST_CASE("a pattern keeps its identity across undo and redo")
{
    // A clip references a pattern by id. If redo minted a new id, the clip
    // would survive pointing at nothing — a silent hole in the arrangement.
    Project project;
    app::CommandRegistry registry{project};

    const EntityId track = project.addTrack(project::TrackType::instrument, "Track 1").id;

    auto add = std::make_unique<app::AddPatternCommand>("Verse");
    app::AddPatternCommand* addPointer = add.get();
    REQUIRE(registry.execute(std::move(add)));

    const EntityId pattern = addPointer->createdPattern();
    REQUIRE(pattern.isValid());

    REQUIRE(registry.execute(std::make_unique<app::AddPatternClipCommand>(track, pattern, 0)));
    REQUIRE(project.clips().size() == 1);

    REQUIRE(registry.undo());   // the clip
    REQUIRE(registry.undo());   // the pattern
    CHECK(project.patterns().empty());

    REQUIRE(registry.redo());
    REQUIRE(registry.redo());

    REQUIRE(project.patterns().size() == 1);
    REQUIRE(project.clips().size() == 1);
    CHECK(project.patterns()[0].id == pattern);
    CHECK(project.clips()[0].source == pattern);
    CHECK(project.findPattern(project.clips()[0].source) != nullptr);
}

TEST_CASE("deleting a pattern takes its placements with it, and undo brings them back")
{
    Project project;
    app::CommandRegistry registry{project};

    const EntityId track   = project.addTrack(project::TrackType::instrument, "Track 1").id;
    const EntityId pattern = project.addPattern("Verse").id;

    REQUIRE(registry.execute(std::make_unique<app::AddPatternClipCommand>(track, pattern, 0)));
    REQUIRE(registry.execute(std::make_unique<app::AddPatternClipCommand>(track, pattern, 96000)));
    REQUIRE(project.clips().size() == 2);

    REQUIRE(registry.execute(std::make_unique<app::DeletePatternCommand>(pattern)));
    CHECK(project.patterns().empty());
    CHECK(project.clips().empty());

    REQUIRE(registry.undo());
    CHECK(project.patterns().size() == 1);
    CHECK(project.clips().size() == 2);
}

TEST_CASE("deleting a channel removes its notes and undo restores them in place")
{
    Project project;
    app::CommandRegistry registry{project};

    const EntityId kick  = project.addChannel("Kick").id;
    const EntityId snare = project.addChannel("Snare").id;

    Pattern& pattern = project.addPattern("Beat");
    pattern.events.push_back(noteAt(0, 36, kick));
    pattern.events.push_back(noteAt(480, 38, snare));
    pattern.events.push_back(noteAt(960, 36, kick));

    REQUIRE(registry.execute(std::make_unique<app::DeleteChannelCommand>(kick)));

    const Pattern* stored = project.findPattern(pattern.id);
    REQUIRE(stored != nullptr);
    REQUIRE(stored->events.size() == 1);
    CHECK(stored->events[0].key == 38);
    CHECK(project.channels().size() == 1);

    REQUIRE(registry.undo());

    stored = project.findPattern(pattern.id);
    REQUIRE(stored != nullptr);
    REQUIRE(stored->events.size() == 3);
    CHECK(stored->events[0].key == 36);
    CHECK(stored->events[1].key == 38);
    CHECK(stored->events[2].key == 36);

    REQUIRE(project.channels().size() == 2);
    CHECK(project.channels()[0].id == kick);    // restored where it was, not appended
}

TEST_CASE("a fader drag is one undo")
{
    Project project;
    app::CommandRegistry registry{project};

    const EntityId channel = project.addChannel("Lead").id;

    using Set = app::SetChannelValueCommand;

    REQUIRE(registry.execute(std::make_unique<Set>(channel, Set::Property::volume, 0.9)));
    for (double value : {0.8, 0.7, 0.6})
        REQUIRE(registry.executeMerging(std::make_unique<Set>(channel, Set::Property::volume, value)));

    CHECK(registry.undoDepth() == 1);
    CHECK(project.findChannel(channel)->volume == doctest::Approx(0.6));

    REQUIRE(registry.undo());
    CHECK(project.findChannel(channel)->volume == doctest::Approx(1.0));
}

TEST_CASE("a per-channel length makes the pattern polymetric and reverts cleanly")
{
    Project project;
    app::CommandRegistry registry{project};

    const EntityId channel = project.addChannel("Hats").id;
    const EntityId pattern = project.addPattern("Beat").id;

    REQUIRE(registry.execute(std::make_unique<app::SetPatternLengthCommand>(
        pattern, ticksPerQuarterNote, channel)));

    CHECK(project.findPattern(pattern)->lengthFor(channel) == ticksPerQuarterNote);

    REQUIRE(registry.undo());

    const Pattern* stored = project.findPattern(pattern);
    REQUIRE(stored != nullptr);
    CHECK(stored->lengthFor(channel) == stored->length);
    CHECK(stored->channelSettings.empty());   // the override is gone, not left at zero
}

// ── Project -> graph ──────────────────────────────────────────────────────────

TEST_CASE("every channel becomes an instrument and a strip in the graph")
{
    Project project;
    const EntityId lead = project.addChannel("Lead").id;
    const EntityId bass = project.addChannel("Bass").id;

    Pattern& pattern = project.addPattern("Riff");
    pattern.events.push_back(noteAt(0, 60, lead, ticksPerQuarterNote));
    pattern.events.push_back(noteAt(0, 36, bass, ticksPerQuarterNote));

    const engine::TempoMap tempoMap{120.0, 48000.0};

    project::GraphCompileOptions options;
    options.activePattern = pattern.id;
    options.sampleRate    = 48000.0;
    options.maxBlockSize  = 256;

    const auto compiled = project::compileProjectGraph(project, tempoMap, options);

    REQUIRE(compiled.graph != nullptr);
    CHECK(compiled.error.empty());
    CHECK(compiled.channelOrder.size() == 2);
    CHECK(compiled.instrumentNodes.size() == 2);
    CHECK(compiled.stripNodes.size() == 2);
    CHECK(compiled.lengthTicks == pattern.length);

    // Instrument and strip per channel, the master mixer node's summing point
    // and strip, and the master gain.
    CHECK(compiled.graph->nodeCount() == 7);
    CHECK(compiled.mixerOrder.size() == 1);          // a new project has a master
    CHECK(compiled.compensationNodes == 0);          // nothing here reports latency

    // Each instrument got only its own channel's note.
    CHECK(compiled.instrumentNodes[0]->sequence().noteCount() == 1);
    CHECK(compiled.instrumentNodes[1]->sequence().noteCount() == 1);
    CHECK(compiled.instrumentNodes[0]->sequence().notes()[0].key == 60);
    CHECK(compiled.instrumentNodes[1]->sequence().notes()[0].key == 36);
}

TEST_CASE("a project with no channels still compiles and renders silence")
{
    const Project project;
    const engine::TempoMap tempoMap{120.0, 48000.0};

    project::GraphCompileOptions options;
    options.maxBlockSize = 128;

    const auto compiled = project::compileProjectGraph(project, tempoMap, options);
    REQUIRE(compiled.graph != nullptr);

    engine::AudioBufferPool output;
    output.allocate(1, 2, 128);
    compiled.graph->process(output.buffer(0), 128, 0);

    CHECK(output.buffer(0).peak() == doctest::Approx(0.0));
}

TEST_CASE("mute silences a channel and solo silences everything else")
{
    Project project;

    // Ids, not references: `addChannel` returns a reference into a vector, and
    // the next add can move it.
    const EntityId lead = project.addChannel("Lead").id;
    const EntityId bass = project.addChannel("Bass").id;

    const engine::TempoMap tempoMap{120.0, 48000.0};

    project::GraphCompileOptions options;
    options.maxBlockSize = 128;

    project.findChannelForEdit(lead)->muted = true;
    {
        const auto compiled = project::compileProjectGraph(project, tempoMap, options);
        REQUIRE(compiled.stripNodes.size() == 2);
        CHECK(compiled.stripNodes[0]->muted());
        CHECK_FALSE(compiled.stripNodes[1]->muted());
    }

    project.findChannelForEdit(lead)->muted  = false;
    project.findChannelForEdit(bass)->soloed = true;
    {
        const auto compiled = project::compileProjectGraph(project, tempoMap, options);
        REQUIRE(compiled.stripNodes.size() == 2);
        CHECK(compiled.stripNodes[0]->muted());        // unsoloed, therefore silent
        CHECK_FALSE(compiled.stripNodes[1]->muted());
    }
}

TEST_CASE("song mode plays the arrangement, pattern mode plays one pattern")
{
    Project project;
    const EntityId channel = project.addChannel("Lead").id;
    const EntityId track   = project.addTrack(project::TrackType::instrument, "Track 1").id;

    Pattern& pattern = project.addPattern("Riff");
    pattern.length = ticksPerQuarterNote * 4;
    pattern.events.push_back(noteAt(0, 60, channel, 240));

    const auto barFrames = project.tempoMap().frameForTick(pattern.length);

    for (int index = 0; index < 3; ++index) {
        project::Clip& clip = project.addClip(project::ClipType::pattern, track, pattern.id);
        clip.start  = barFrames * index;
        clip.length = barFrames;
    }

    const engine::TempoMap tempoMap{120.0, 48000.0};

    project::GraphCompileOptions options;
    options.activePattern = pattern.id;
    options.maxBlockSize  = 128;

    options.mode = project::PlaybackMode::pattern;
    const auto patternMode = project::compileProjectGraph(project, tempoMap, options);
    REQUIRE(patternMode.instrumentNodes.size() == 1);
    CHECK(patternMode.instrumentNodes[0]->sequence().noteCount() == 1);
    CHECK(patternMode.lengthTicks == pattern.length);

    options.mode = project::PlaybackMode::song;
    const auto songMode = project::compileProjectGraph(project, tempoMap, options);
    REQUIRE(songMode.instrumentNodes.size() == 1);
    CHECK(songMode.instrumentNodes[0]->sequence().noteCount() == 3);
    CHECK(songMode.lengthTicks == pattern.length * 3);
}

TEST_CASE("a channel with an unhostable instrument keeps its place and renders silence")
{
    // Phase 13 has not happened: a project that names a VST3 must still open,
    // still save, and still show the channel. Dropping it would lose the user's
    // work the next time they saved.
    Project project;
    const EntityId channel = project.addChannel("Plugin").id;
    project.findChannelForEdit(channel)->instrument = {plugins::Format::vst3, "com.acme.synth"};

    Pattern& pattern = project.addPattern("Riff");
    pattern.events.push_back(noteAt(0, 60, channel, ticksPerQuarterNote));

    const engine::TempoMap tempoMap{120.0, 48000.0};

    project::GraphCompileOptions options;
    options.activePattern = pattern.id;
    options.maxBlockSize  = 128;

    const auto compiled = project::compileProjectGraph(project, tempoMap, options);
    REQUIRE(compiled.graph != nullptr);
    CHECK(compiled.channelOrder.size() == 1);

    engine::AudioBufferPool output;
    output.allocate(1, 2, 128);
    compiled.graph->process(output.buffer(0), 128, 0);

    CHECK(output.buffer(0).peak() == doctest::Approx(0.0));
    CHECK_FALSE(output.buffer(0).hasNonFiniteSamples());
}

// ── Channel strip ─────────────────────────────────────────────────────────────

TEST_CASE("pan is constant power and mute is not a zero volume")
{
    engine::dsp::ChannelStripNode strip;
    strip.prepare(48000.0, 64);

    engine::AudioBufferPool pool;
    pool.allocate(2, 2, 64);

    const auto render = [&](float pan, bool muted) {
        strip.setPan(pan);
        strip.setMuted(muted);
        strip.prepare(48000.0, 64);   // start at the target rather than gliding

        const auto input = pool.buffer(1);
        for (std::size_t channel = 0; channel < 2; ++channel)
            for (engine::FrameCount frame = 0; frame < 64; ++frame)
                input.channel(channel)[frame] = 1.0f;

        const auto output = pool.buffer(0);
        output.clear();

        engine::ProcessContext context;
        context.output     = output;
        context.inputs     = &input;
        context.inputCount = 1;
        context.frameCount = 64;
        context.sampleRate = 48000.0;

        strip.process(context);

        return std::pair{output.channel(0)[63], output.channel(1)[63]};
    };

    const auto [centreLeft, centreRight] = render(0.0f, false);
    CHECK(centreLeft == doctest::Approx(centreRight));
    CHECK(centreLeft == doctest::Approx(0.7071).epsilon(0.01));

    const auto [leftLeft, leftRight] = render(-1.0f, false);
    CHECK(leftLeft == doctest::Approx(1.0).epsilon(0.01));
    CHECK(leftRight == doctest::Approx(0.0).epsilon(0.01));

    // Constant power: the summed energy is the same wherever it sits.
    CHECK(centreLeft * centreLeft + centreRight * centreRight
          == doctest::Approx(leftLeft * leftLeft + leftRight * leftRight).epsilon(0.01));

    const auto [mutedLeft, mutedRight] = render(0.0f, true);
    CHECK(mutedLeft == doctest::Approx(0.0));
    CHECK(mutedRight == doctest::Approx(0.0));

    strip.setMuted(false);
    CHECK(strip.volume() == doctest::Approx(1.0));   // unmuting restores the level
}

// ── Piano Roll channel filter ─────────────────────────────────────────────────

TEST_CASE("the piano roll edits one channel and shows the rest as ghosts")
{
    Project project;
    const EntityId lead = project.addChannel("Lead").id;
    const EntityId bass = project.addChannel("Bass").id;

    Pattern pattern;
    pattern.events.push_back(noteAt(0, 60, lead, 480));
    pattern.events.push_back(noteAt(0, 48, bass, 480));

    app::PianoRollModel model;

    app::PianoRollModel::Viewport viewport;
    viewport.firstTick    = 0;
    viewport.visibleTicks = ticksPerQuarterNote * 4;
    viewport.lowestKey    = 36;
    viewport.visibleKeys  = 36;
    viewport.width        = 800.0;
    viewport.height       = 600.0;
    model.setViewport(viewport);

    std::vector<app::PianoRollModel::VisibleNote> visible;

    // No filter: everything is editable, which is what a one-channel project
    // wants and what every test written before channels existed assumes.
    model.collectVisibleNotes(pattern, visible);
    REQUIRE(visible.size() == 2);
    CHECK_FALSE(visible[0].ghost);
    CHECK_FALSE(visible[1].ghost);

    model.setChannelFilter(lead, project.defaultChannel());
    model.collectVisibleNotes(pattern, visible);

    REQUIRE(visible.size() == 2);      // both drawn
    CHECK_FALSE(visible[0].ghost);
    CHECK(visible[1].ghost);           // the bass note is context, not a target

    const double leadY = model.keyToY(60) + 1.0;
    const double bassY = model.keyToY(48) + 1.0;

    CHECK(model.noteAtPoint(pattern, 10.0, leadY) == 0);
    CHECK(model.noteAtPoint(pattern, 10.0, bassY) == app::PianoRollModel::noNote);

    // Box selection cannot pick up a ghost either, or a drag would silently
    // move notes the user cannot see themselves editing.
    std::vector<std::size_t> selected;
    model.notesInRectangle(pattern, 0.0, 0.0, 800.0, 600.0, selected);

    REQUIRE(selected.size() == 1);
    CHECK(selected[0] == 0);
}
