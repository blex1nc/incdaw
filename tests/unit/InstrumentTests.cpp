#include "doctest.h"

#include "engine/core/AudioBufferPool.h"
#include "engine/core/RealtimeGuard.h"
#include "engine/dsp/GainNode.h"
#include "engine/graph/RenderGraph.h"
#include "engine/instrument/InstrumentNode.h"
#include "engine/instrument/SimpleSynth.h"
#include "engine/midi/NoteSequence.h"
#include "project/PatternCompiler.h"

#include <cmath>
#include <memory>
#include <vector>

using namespace incdaw;
using namespace incdaw::engine;

namespace {

/// Records the frame at which each message was applied, so the base class's
/// block-splitting contract can be asserted directly.
class TimingProbeInstrument final : public Instrument {
public:
    struct Applied {
        FrameCount atFrame = 0;
        int        note    = 0;
    };

    void prepare(SampleRate, FrameCount) override { rendered_ = 0; applied.clear(); }
    void allNotesOff() noexcept override {}
    [[nodiscard]] const char* name() const noexcept override { return "Probe"; }
    [[nodiscard]] int activeVoiceCount() const noexcept override { return 0; }

    std::vector<Applied> applied;

protected:
    void handleMessage(const MidiMessage& message) noexcept override
    {
        applied.push_back({rendered_, message.noteNumber()});
    }

    void renderRange(const AudioBufferView&, FrameCount frameCount) noexcept override
    {
        rendered_ += frameCount;
    }

private:
    FrameCount rendered_ = 0;
};

Sample renderSynth(SimpleSynth& synth, const MidiBuffer& midi, AudioBufferPool& pool, FrameCount frames)
{
    const auto output = pool.buffer(0).subBlock(0, frames);
    output.clear();
    synth.processBlock(output, midi);
    return output.peak();
}

} // namespace

// ── Instrument base: sample-accurate splitting ────────────────────────────────

TEST_CASE("messages are applied on the exact frame they carry")
{
    // This is the contract the base class exists to guarantee. Every instrument
    // inherits it, so no instrument author can quietly get it wrong.
    TimingProbeInstrument probe;
    probe.prepare(48000.0, 512);

    AudioBufferPool pool;
    pool.allocate(1, 1, 512);

    MidiBuffer midi;
    midi.insert(MidiMessage::noteOn(0, 60, 100, 0));
    midi.insert(MidiMessage::noteOn(0, 61, 100, 37));
    midi.insert(MidiMessage::noteOn(0, 62, 100, 200));
    midi.insert(MidiMessage::noteOn(0, 63, 100, 511));

    probe.processBlock(pool.buffer(0), midi);

    REQUIRE(probe.applied.size() == 4);
    CHECK(probe.applied[0].atFrame == 0);
    CHECK(probe.applied[1].atFrame == 37);
    CHECK(probe.applied[2].atFrame == 200);
    CHECK(probe.applied[3].atFrame == 511);
}

TEST_CASE("several messages on the same frame all land there")
{
    TimingProbeInstrument probe;
    probe.prepare(48000.0, 256);

    AudioBufferPool pool;
    pool.allocate(1, 1, 256);

    MidiBuffer midi;
    for (int note = 60; note < 64; ++note)
        midi.insert(MidiMessage::noteOn(0, note, 100, 64));

    probe.processBlock(pool.buffer(0), midi);

    REQUIRE(probe.applied.size() == 4);
    for (const auto& applied : probe.applied)
        CHECK(applied.atFrame == 64);
}

TEST_CASE("a block with no messages is rendered in one pass")
{
    TimingProbeInstrument probe;
    probe.prepare(48000.0, 128);

    AudioBufferPool pool;
    pool.allocate(1, 1, 128);

    probe.processBlock(pool.buffer(0), MidiBuffer{});
    CHECK(probe.applied.empty());
}

// ── SimpleSynth ───────────────────────────────────────────────────────────────

TEST_CASE("a note produces sound and releasing it silences the voice")
{
    constexpr SampleRate rate  = 48000.0;
    constexpr FrameCount block = 512;

    SimpleSynth synth;
    synth.prepare(rate, block);
    synth.setReleaseSeconds(0.02);

    AudioBufferPool pool;
    pool.allocate(1, 1, block);

    CHECK(synth.activeVoiceCount() == 0);

    MidiBuffer midi;
    midi.insert(MidiMessage::noteOn(0, 60, 100, 0));
    CHECK(renderSynth(synth, midi, pool, block) > 0.0f);
    CHECK(synth.activeVoiceCount() == 1);

    midi.clear();
    midi.insert(MidiMessage::noteOff(0, 60, 64, 0));
    (void)renderSynth(synth, midi, pool, block);

    // Release is 20 ms; a few blocks of 512 frames is more than enough.
    midi.clear();
    for (int block_ = 0; block_ < 6; ++block_)
        (void)renderSynth(synth, midi, pool, block);

    CHECK(synth.activeVoiceCount() == 0);
    CHECK(renderSynth(synth, midi, pool, block) == doctest::Approx(0.0f));
}

TEST_CASE("the synth is polyphonic")
{
    SimpleSynth synth;
    synth.prepare(48000.0, 256);

    AudioBufferPool pool;
    pool.allocate(1, 1, 256);

    MidiBuffer midi;
    for (const int note : {60, 64, 67, 72})
        midi.insert(MidiMessage::noteOn(0, note, 100, 0));

    (void)renderSynth(synth, midi, pool, 256);
    CHECK(synth.activeVoiceCount() == 4);
}

TEST_CASE("retriggering a key releases the old voice instead of stacking a second")
{
    // Stacking would double the level of that note, which is audible as an
    // unexplained jump in loudness when a part repeats a key quickly.
    SimpleSynth synth;
    synth.prepare(48000.0, 256);

    AudioBufferPool pool;
    pool.allocate(1, 1, 256);

    MidiBuffer midi;
    midi.insert(MidiMessage::noteOn(0, 60, 100, 0));
    (void)renderSynth(synth, midi, pool, 256);

    midi.clear();
    midi.insert(MidiMessage::noteOn(0, 60, 100, 0));
    (void)renderSynth(synth, midi, pool, 256);

    // One sounding plus one releasing, never two sounding on the same key.
    CHECK(synth.activeVoiceCount() <= 2);
}

TEST_CASE("voice count is bounded and exceeding it does not misbehave")
{
    SimpleSynth synth;
    synth.prepare(48000.0, 256);

    AudioBufferPool pool;
    pool.allocate(1, 1, 256);

    MidiBuffer midi;
    for (int note = 0; note < 100; ++note)
        midi.insert(MidiMessage::noteOn(0, note, 100, 0));

    const Sample peak = renderSynth(synth, midi, pool, 256);

    CHECK(synth.activeVoiceCount() <= SimpleSynth::maxVoices);
    CHECK(std::isfinite(peak));
}

TEST_CASE("all-notes-off silences everything, from the API and from CC 123")
{
    SimpleSynth synth;
    synth.prepare(48000.0, 256);

    AudioBufferPool pool;
    pool.allocate(1, 1, 256);

    MidiBuffer midi;
    for (const int note : {60, 64, 67})
        midi.insert(MidiMessage::noteOn(0, note, 100, 0));
    (void)renderSynth(synth, midi, pool, 256);
    REQUIRE(synth.activeVoiceCount() == 3);

    // A controller sending All Notes Off expects silence; ignoring it is a
    // classic source of stuck notes.
    midi.clear();
    midi.insert(MidiMessage::controlChange(0, 123, 0, 0));
    (void)renderSynth(synth, midi, pool, 256);
    CHECK(synth.activeVoiceCount() == 0);

    midi.clear();
    midi.insert(MidiMessage::noteOn(0, 60, 100, 0));
    (void)renderSynth(synth, midi, pool, 256);
    synth.allNotesOff();
    CHECK(synth.activeVoiceCount() == 0);
}

TEST_CASE("velocity scales the output level")
{
    AudioBufferPool pool;
    pool.allocate(1, 1, 1024);

    const auto peakForVelocity = [&pool](int velocity) {
        SimpleSynth synth;
        synth.prepare(48000.0, 1024);
        synth.setAttackSeconds(0.001);

        MidiBuffer midi;
        midi.insert(MidiMessage::noteOn(0, 60, velocity, 0));
        return renderSynth(synth, midi, pool, 1024);
    };

    CHECK(peakForVelocity(127) > peakForVelocity(64));
    CHECK(peakForVelocity(64) > peakForVelocity(20));
}

TEST_CASE("every waveform produces finite, bounded output")
{
    for (const auto waveform : {SimpleSynth::Waveform::sine, SimpleSynth::Waveform::sawtooth,
                                SimpleSynth::Waveform::square, SimpleSynth::Waveform::triangle}) {
        SimpleSynth synth;
        synth.prepare(48000.0, 512);
        synth.setWaveform(waveform);

        AudioBufferPool pool;
        pool.allocate(1, 1, 512);

        MidiBuffer midi;
        midi.insert(MidiMessage::noteOn(0, 84, 127, 0));   // high note: worst case for aliasing

        for (int block = 0; block < 20; ++block) {
            const auto output = pool.buffer(0);
            output.clear();
            synth.processBlock(output, block == 0 ? midi : MidiBuffer{});

            CHECK_FALSE(output.hasNonFiniteSamples());
            CHECK(output.peak() < 4.0f);
        }
    }
}

TEST_CASE("the sawtooth is band-limited")
{
    // A naive ramp aliases audibly on high notes. PolyBLEP subtracts a
    // correction around the discontinuity; without it the sample immediately
    // after the wrap sits at the full -1 extreme. Detecting that correction is
    // what distinguishes a band-limited saw from a placeholder one.
    constexpr SampleRate rate = 48000.0;

    SimpleSynth synth;
    synth.prepare(rate, 4096);
    synth.setWaveform(SimpleSynth::Waveform::sawtooth);
    synth.setAttackSeconds(0.0001);
    synth.setSustainLevel(1.0);
    synth.setDecaySeconds(0.0001);

    AudioBufferPool pool;
    pool.allocate(1, 1, 4096);

    MidiBuffer midi;
    midi.insert(MidiMessage::noteOn(0, 96, 127, 0));   // ~2093 Hz

    const auto output = pool.buffer(0);
    output.clear();
    synth.processBlock(output, midi);

    // Find the largest single-sample step. A naive saw of this amplitude steps
    // by the full peak-to-peak swing at every wrap; a band-limited one does not.
    Sample largestStep = 0.0f;
    Sample peak        = 0.0f;

    for (FrameCount frame = 2049; frame < 4096; ++frame) {
        largestStep = std::max(largestStep, std::abs(output.channel(0)[frame] - output.channel(0)[frame - 1]));
        peak        = std::max(peak, std::abs(output.channel(0)[frame]));
    }

    REQUIRE(peak > 0.0f);
    CHECK(largestStep < peak * 1.9f);
}

TEST_CASE("rendering the synth allocates nothing")
{
    SimpleSynth synth;
    synth.prepare(48000.0, 256);

    AudioBufferPool pool;
    pool.allocate(1, 2, 256);

    MidiBuffer midi;
    for (const int note : {48, 55, 60, 64, 67, 72})
        midi.insert(MidiMessage::noteOn(0, note, 100, 0));

    rt::resetViolations();
    {
        const rt::ScopedRealtimeContext scope;
        for (int block = 0; block < 500; ++block) {
            const auto output = pool.buffer(0);
            output.clear();
            synth.processBlock(output, block == 0 ? midi : MidiBuffer{});
        }
    }

    CHECK(rt::allocationViolations() == 0);
    CHECK(rt::deallocationViolations() == 0);
}

// ── NoteSequence ──────────────────────────────────────────────────────────────

TEST_CASE("the sequence emits a note-on and a note-off at the right frames")
{
    const TempoMap map{120.0, 48000.0};   // a beat is 24000 frames

    NoteSequence sequence;
    sequence.setNotes({{0, ticksPerQuarterNote, 0, 60, 100}});

    MidiBuffer midi;
    sequence.collectForRange(midi, 0, 512, map);

    REQUIRE(midi.size() == 1);
    CHECK(midi[0].isNoteOn());
    CHECK(midi[0].frameOffset == 0);

    midi.clear();
    sequence.collectForRange(midi, 24000 - 256, 512, map);

    REQUIRE(midi.size() == 1);
    CHECK(midi[0].isNoteOff());
    CHECK(midi[0].frameOffset == 256);
}

TEST_CASE("degenerate notes are dropped rather than producing an orphan note-off")
{
    NoteSequence sequence;
    sequence.setNotes({{0, 0, 0, 60, 100},        // no length
                       {-100, 480, 0, 61, 100},   // before the start
                       {0, 480, 0, 62, 0},        // silent
                       {0, 480, 0, 63, 100}});    // the only valid one

    CHECK(sequence.noteCount() == 1);
    CHECK(sequence.notes()[0].key == 63);
}

TEST_CASE("playback is identical at every block size")
{
    const TempoMap map{120.0, 48000.0};

    NoteSequence sequence;
    std::vector<SequencedNote> notes;
    for (int index = 0; index < 16; ++index)
        notes.push_back({static_cast<Tick>(index) * (ticksPerQuarterNote / 4),
                         ticksPerQuarterNote / 8, 0, 60 + index % 12, 100});
    sequence.setNotes(notes);

    const FramePosition total = map.frameForTick(ticksPerQuarterNote * 4);

    std::vector<std::pair<FramePosition, int>> reference;

    for (const FrameCount blockSize : {32, 64, 128, 256, 512, 1024}) {
        std::vector<std::pair<FramePosition, int>> collected;
        MidiBuffer midi;

        for (FramePosition frame = 0; frame < total; frame += blockSize) {
            midi.clear();
            sequence.collectForRange(midi, frame, blockSize, map);

            for (const MidiMessage& message : midi)
                if (message.isNoteOn())
                    collected.emplace_back(frame + message.frameOffset, message.noteNumber());
        }

        if (reference.empty())
            reference = collected;
        else
            CHECK(collected == reference);
    }

    CHECK(reference.size() == 16);
}

TEST_CASE("collecting from the sequence is realtime-safe")
{
    const TempoMap map{140.0, 48000.0};

    NoteSequence sequence;
    std::vector<SequencedNote> notes;
    for (int index = 0; index < 2000; ++index)
        notes.push_back({static_cast<Tick>(index) * 120, 100, 0, 36 + index % 60, 100});
    sequence.setNotes(notes);

    MidiBuffer midi;

    rt::resetViolations();
    {
        const rt::ScopedRealtimeContext scope;
        for (int block = 0; block < 2000; ++block) {
            midi.clear();
            sequence.collectForRange(midi, block * 128, 128, map);
        }
    }

    CHECK(rt::allocationViolations() == 0);
    CHECK(rt::deallocationViolations() == 0);
}

// ── Pattern compilation ───────────────────────────────────────────────────────

TEST_CASE("patterns compile to sequences, skipping non-notes")
{
    project::Pattern pattern;
    const project::EntityId channel{1};
    auto& events = pattern.contentFor(channel).events;

    project::MidiEvent note;
    note.type = project::MidiEventType::note;
    note.tick = 0;
    note.duration = 480;
    note.key = 60;
    note.value = 100;
    events.push_back(note);

    project::MidiEvent cc;
    cc.type = project::MidiEventType::controlChange;
    events.push_back(cc);

    const auto compiled = project::compilePattern(pattern, channel);

    REQUIRE(compiled.size() == 1);
    CHECK(compiled[0].key == 60);
    CHECK(compiled[0].lengthTicks == 480);
}

TEST_CASE("note probability is deterministic for a seed")
{
    // Playback and offline render must agree, so probability cannot be rolled
    // on the audio thread.
    project::Pattern pattern;
    const project::EntityId channel{1};
    auto& events = pattern.contentFor(channel).events;

    for (int index = 0; index < 200; ++index) {
        project::MidiEvent note;
        note.type        = project::MidiEventType::note;
        note.tick        = static_cast<Tick>(index) * 120;
        note.duration    = 100;
        note.key         = 60;
        note.value       = 100;
        note.probability = 0.5;
        events.push_back(note);
    }

    const auto first  = project::compilePattern(pattern, channel, 4242);
    const auto second = project::compilePattern(pattern, channel, 4242);
    const auto other  = project::compilePattern(pattern, channel, 99);

    // Compared by content, not by count: two seeds can easily drop the same
    // *number* of notes while dropping different ones, and a size comparison
    // would call that a pass.
    const auto starts = [](const std::vector<engine::SequencedNote>& compiled) {
        std::vector<Tick> ticks;
        ticks.reserve(compiled.size());
        for (const engine::SequencedNote& note : compiled)
            ticks.push_back(note.startTick);
        return ticks;
    };

    CHECK(starts(first) == starts(second));        // same seed, same result
    CHECK(starts(first) != starts(other));         // a different seed differs
    CHECK(first.size() < events.size());           // some were skipped
    CHECK(first.size() > 0);
}

// ── Phase 7 exit criterion ────────────────────────────────────────────────────

TEST_CASE("a note reaches the mixer as audio through the graph")
{
    // docs/ROADMAP.md Phase 7: a MIDI note played into a channel produces
    // audible sound through the graph, correctly routed to the mixer.
    constexpr SampleRate rate  = 48000.0;
    constexpr FrameCount block = 512;

    const TempoMap map{120.0, rate};

    auto synth = std::make_unique<SimpleSynth>();
    auto node  = std::make_unique<InstrumentNode>(std::move(synth), map);

    // One note at the very start of the timeline.
    node->sequence().setNotes({{0, ticksPerQuarterNote, 0, 60, 110}});

    GraphBuilder builder;
    const auto instrument = builder.addNode(std::move(node));
    const auto master     = builder.addNode(std::make_unique<dsp::GainNode>(0.5f));

    builder.connect(instrument, master);
    builder.setMaster(master);

    const auto graph = builder.compile(rate, block, 2);
    REQUIRE(graph != nullptr);

    AudioBufferPool output;
    output.allocate(1, 2, block);

    // Silence before the note would be wrong too — render the first block and
    // require actual signal out of the master.
    graph->process(output.buffer(0), block, 0);

    const Sample peak = output.buffer(0).peak();

    CHECK(peak > 0.001f);                       // audible
    CHECK(peak < 1.0f);                         // not clipping
    CHECK_FALSE(output.buffer(0).hasNonFiniteSamples());

    // Both channels carry it: the instrument is mono, mirrored to the bus.
    CHECK(output.buffer(0).channel(1)[100] == doctest::Approx(output.buffer(0).channel(0)[100]));

    // And the master gain is actually in the path.
    CHECK(peak < 0.6f);
}

TEST_CASE("the whole chain is silent when there is nothing to play")
{
    constexpr SampleRate rate  = 48000.0;
    constexpr FrameCount block = 256;

    const TempoMap map{120.0, rate};

    auto node = std::make_unique<InstrumentNode>(std::make_unique<SimpleSynth>(), map);

    GraphBuilder builder;
    const auto instrument = builder.addNode(std::move(node));
    const auto master     = builder.addNode(std::make_unique<dsp::GainNode>(1.0f));
    builder.connect(instrument, master);
    builder.setMaster(master);

    const auto graph = builder.compile(rate, block, 2);
    REQUIRE(graph != nullptr);

    AudioBufferPool output;
    output.allocate(1, 2, block);

    graph->process(output.buffer(0), block, 0);
    CHECK(output.buffer(0).peak() == doctest::Approx(0.0f));
}

TEST_CASE("a transport jump silences sounding voices instead of leaving them hanging")
{
    // Without this, seeking or looping mid-note leaves the note sounding until
    // something happens to release it — the classic stuck-note-after-loop bug.
    constexpr SampleRate rate  = 48000.0;
    constexpr FrameCount block = 256;

    const TempoMap map{120.0, rate};

    auto owned = std::make_unique<SimpleSynth>();
    auto* synth = owned.get();

    InstrumentNode node{std::move(owned), map};
    node.prepare(rate, block);
    node.sequence().setNotes({{0, ticksPerQuarterNote * 4, 0, 60, 100}});   // a long note

    AudioBufferPool pool;
    pool.allocate(1, 1, block);

    ProcessContext context;
    context.output     = pool.buffer(0);
    context.frameCount = block;
    context.sampleRate = rate;

    context.playPosition = 0;
    pool.buffer(0).clear();
    node.process(context);
    REQUIRE(synth->activeVoiceCount() == 1);

    // Continuous: the voice keeps sounding.
    context.playPosition = block;
    pool.buffer(0).clear();
    node.process(context);
    CHECK(synth->activeVoiceCount() == 1);

    // Discontinuous: the transport jumped, so the voice belongs to a position
    // that no longer exists.
    context.playPosition = 500000;
    pool.buffer(0).clear();
    node.process(context);
    CHECK(synth->activeVoiceCount() == 0);
}

TEST_CASE("live input and sequenced notes reach the same instrument")
{
    constexpr SampleRate rate  = 48000.0;
    constexpr FrameCount block = 256;

    const TempoMap map{120.0, rate};

    auto owned = std::make_unique<SimpleSynth>();
    auto* synth = owned.get();

    InstrumentNode node{std::move(owned), map};
    node.prepare(rate, block);
    node.sequence().setNotes({{0, ticksPerQuarterNote, 0, 60, 100}});

    MidiBuffer live;
    live.insert(MidiMessage::noteOn(0, 72, 100, 10));

    AudioBufferPool pool;
    pool.allocate(1, 1, block);

    ProcessContext context;
    context.output       = pool.buffer(0);
    context.frameCount   = block;
    context.sampleRate   = rate;
    context.playPosition = 0;
    context.liveMidi     = &live;

    pool.buffer(0).clear();
    node.process(context);

    // One from the sequence, one from the keyboard.
    CHECK(synth->activeVoiceCount() == 2);
}
