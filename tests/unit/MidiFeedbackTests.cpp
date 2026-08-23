// C3 — controller feedback: the mapping system read backwards.
//
// The unit tests below pin the flush rules; the end-to-end one is the point of
// the feature. A motorised fader mapped to the master volume has to follow an
// AUTOMATION LANE — something the fader never touched and the mapping node
// never sees — which is why the tap sits on the applier every writer shares
// rather than on the node that happens to receive MIDI.

#include "doctest.h"

#include "app/CommandRegistry.h"
#include "app/commands/MidiMappingCommands.h"
#include "engine/core/AudioBufferPool.h"
#include "engine/midi/MidiFeedback.h"
#include "engine/midi/MidiOutput.h"
#include "engine/transport/TempoMap.h"
#include "project/Model.h"
#include "project/ParameterRegistry.h"
#include "project/ProjectGraphCompiler.h"

#include <memory>
#include <string>
#include <vector>

using namespace incdaw;
using namespace incdaw::engine;

namespace {

/// A destination that records instead of playing.
class RecordingMidiDevice final : public platform::MidiDevice {
public:
    std::vector<platform::MidiDeviceInfo> enumerateInputs()  const override { return {}; }
    std::vector<platform::MidiDeviceInfo> enumerateOutputs() const override { return {}; }

    bool open(const std::vector<std::string>&, platform::MidiInputCallback&, std::string&) override
    {
        return true;
    }

    void close() override {}
    [[nodiscard]] bool isOpen() const noexcept override { return true; }

    bool selectOutput(const std::string&, std::string&) override { return true; }
    [[nodiscard]] std::string selectedOutput() const override { return "test"; }

    void sendMessage(const platform::TimestampedMidiMessage& message) noexcept override
    {
        sent.push_back(message);
    }

    std::vector<platform::TimestampedMidiMessage> sent;
};

/// A feedback map wired to a recording device, drained synchronously.
struct Surface {
    RecordingMidiDevice device;
    MidiOutput          output;
    MidiFeedback        feedback;

    Surface() { output.setDevice(&device); }

    /// Flushes and drains, and answers with what the hardware would have seen.
    std::vector<platform::TimestampedMidiMessage> pump()
    {
        device.sent.clear();
        (void)feedback.flush(output);
        (void)output.drainPending();
        return device.sent;
    }

    void bind(int channel, int controller, float minValue = 0.0f, float maxValue = 1.0f)
    {
        MidiFeedback::Binding binding;
        binding.midiChannel = channel;
        binding.controller  = controller;
        binding.minValue    = minValue;
        binding.maxValue    = maxValue;
        binding.active      = true;

        std::vector<MidiFeedback::Binding> bindings{binding};
        feedback.setBindings(std::move(bindings));
    }
};

project::AutomationPoint point(Tick tick, double value)
{
    return {tick, value, project::AutomationCurve::linear, 0.0};
}

} // namespace

// ── The flush rules ──────────────────────────────────────────────────────────

TEST_CASE("a parameter's value is sent as the controller it is mapped to")
{
    Surface surface;
    surface.bind(2, 7);

    surface.feedback.observe(0, 1.0f);

    const auto sent = surface.pump();
    REQUIRE(sent.size() == 1);
    CHECK(sent[0].status == 0xB2);   // control change, channel 3 (index 2)
    CHECK(sent[0].data1 == 7);
    CHECK(sent[0].data2 == 127);
}

TEST_CASE("only movement is sent")
{
    Surface surface;
    surface.bind(0, 7);

    surface.feedback.observe(0, 0.5f);
    CHECK(surface.pump().size() == 1);

    // The same value again. A surface that is told its own position thirty
    // times a second is a surface with no bandwidth left for anything else.
    CHECK(surface.pump().empty());

    // A move too small to change the 7-bit value. A control surface has 128
    // positions; reporting the ones between them is reporting noise.
    surface.feedback.observe(0, 0.503f);
    CHECK(surface.pump().empty());

    surface.feedback.observe(0, 0.6f);
    CHECK(surface.pump().size() == 1);
}

TEST_CASE("a value that came from the hardware is not sent back to it")
{
    Surface surface;
    surface.bind(0, 7);

    // What the mapping node does when a CC arrives: the applier writes the
    // value, and the slot is marked as already sent.
    surface.feedback.observe(0, 100.0f / 127.0f);
    surface.feedback.suppress(0, 100);

    CHECK(surface.pump().empty());
    CHECK(surface.feedback.lastSentAt(0) == 100);

    // And the surface is not left stale: the next move from anywhere else is
    // sent normally.
    surface.feedback.observe(0, 0.0f);
    REQUIRE(surface.pump().size() == 1);
    CHECK(surface.pump().empty());
}

TEST_CASE("an inverted mapping reads back inverted")
{
    Surface surface;
    surface.bind(0, 7, 1.0f, 0.0f);   // 0 on the knob is 1.0 on the parameter

    surface.feedback.observe(0, 1.0f);
    auto sent = surface.pump();
    REQUIRE(sent.size() == 1);
    CHECK(sent[0].data2 == 0);

    surface.feedback.observe(0, 0.0f);
    sent = surface.pump();
    REQUIRE(sent.size() == 1);
    CHECK(sent[0].data2 == 127);
}

TEST_CASE("a value outside the mapped range clamps rather than wrapping")
{
    Surface surface;
    surface.bind(0, 7, 0.25f, 0.75f);

    surface.feedback.observe(0, 10.0f);
    auto sent = surface.pump();
    REQUIRE(sent.size() == 1);
    CHECK(sent[0].data2 == 127);

    surface.feedback.observe(0, -10.0f);
    sent = surface.pump();
    REQUIRE(sent.size() == 1);
    CHECK(sent[0].data2 == 0);
}

TEST_CASE("rebinding writes the whole surface again")
{
    Surface surface;
    surface.bind(0, 7);

    surface.feedback.observe(0, 0.5f);
    REQUIRE(surface.pump().size() == 1);
    REQUIRE(surface.pump().empty());

    // A project load, or a mapping edit. The surface is holding whatever the
    // previous project put there, so "nothing changed" is the wrong answer.
    surface.bind(0, 7);
    CHECK(surface.pump().size() == 1);
}

TEST_CASE("feedback can be switched off")
{
    Surface surface;
    surface.bind(0, 7);
    surface.feedback.setEnabled(false);

    surface.feedback.observe(0, 1.0f);
    CHECK(surface.pump().empty());

    surface.feedback.setEnabled(true);
    CHECK(surface.pump().size() == 1);
}

TEST_CASE("a mapping with no range has no position to report")
{
    Surface surface;
    surface.bind(0, 7, 0.5f, 0.5f);

    surface.feedback.observe(0, 0.5f);
    CHECK(surface.pump().empty());
}

TEST_CASE("an inactive slot is silent")
{
    Surface surface;

    MidiFeedback::Binding binding;
    binding.controller = 7;
    binding.active     = false;
    surface.feedback.setBindings({binding});

    surface.feedback.observe(0, 1.0f);
    CHECK(surface.pump().empty());
}

// ── End to end ───────────────────────────────────────────────────────────────

TEST_CASE("a mapped fader follows an automation lane it never touched")
{
    project::Project project;
    const auto channel = project.addChannel("Channel 1").id;
    const auto pattern = project.addPattern("Pattern 1").id;
    (void)channel;

    // The mapping: CC 7 on the master volume, exactly what learn mode writes.
    app::CommandRegistry registry{project};
    REQUIRE(registry.execute(std::make_unique<app::AddMidiMappingCommand>(
        -1, 7, "volume", project.masterMixerNode())));

    // The lane: the same parameter, ramping over four beats. Nothing here
    // knows a controller exists.
    project::AutomationLane& lane =
        project.addAutomationLane(project.masterMixerNode(), "volume");
    lane.points.push_back(point(0, 0.0));
    lane.points.push_back(point(ticksPerQuarterNote * 4, 1.0));

    Surface surface;

    engine::TempoMap tempo{120.0, 48000.0};

    project::GraphCompileOptions options;
    options.pattern      = pattern;
    options.midiFeedback = &surface.feedback;

    auto compiled = project::compileProjectGraph(project, tempo, options);
    REQUIRE(compiled);
    REQUIRE(surface.feedback.bindingCount() == 1);

    AudioBufferPool pool;
    pool.allocate(1, 2, 64);

    const auto renderAt = [&](Tick tick) {
        pool.buffer(0).clear();
        compiled.graph->process(pool.buffer(0), 64, tempo.frameForTick(tick));
    };

    renderAt(0);
    auto sent = surface.pump();
    REQUIRE(sent.size() == 1);
    CHECK(sent[0].data1 == 7);
    CHECK(sent[0].data2 == 0);

    // Halfway up the ramp. The fader has to move, and nothing sent it a MIDI
    // message to make that happen — this is the whole feature.
    renderAt(ticksPerQuarterNote * 2);
    sent = surface.pump();
    REQUIRE(sent.size() == 1);
    CHECK(sent[0].data2 > 55);
    CHECK(sent[0].data2 < 72);

    renderAt(ticksPerQuarterNote * 4);
    sent = surface.pump();
    REQUIRE(sent.size() == 1);
    CHECK(sent[0].data2 == 127);
}

TEST_CASE("turning the mapped knob does not make the surface answer itself")
{
    project::Project project;
    const auto pattern = project.addPattern("Pattern 1").id;
    (void)project.addChannel("Channel 1");

    app::CommandRegistry registry{project};
    REQUIRE(registry.execute(std::make_unique<app::AddMidiMappingCommand>(
        -1, 7, "volume", project.masterMixerNode())));

    Surface surface;

    engine::TempoMap tempo{120.0, 48000.0};

    project::GraphCompileOptions options;
    options.pattern      = pattern;
    options.midiFeedback = &surface.feedback;

    auto compiled = project::compileProjectGraph(project, tempo, options);
    REQUIRE(compiled);

    AudioBufferPool pool;
    pool.allocate(1, 2, 64);

    MidiBuffer midi;
    (void)midi.insert(MidiMessage::controlChange(0, 7, 90));

    pool.buffer(0).clear();
    compiled.graph->process(pool.buffer(0), 64, 0, &midi);

    // The knob moved the parameter; the parameter must not move the knob.
    // Without this a motorised fader answers its own message, which the
    // hardware answers in turn, and it hunts around the position forever.
    CHECK(surface.pump().empty());
    CHECK(surface.feedback.lastSentAt(0) == 90);
}

TEST_CASE("without a feedback map, mappings stay one-way")
{
    project::Project project;
    const auto pattern = project.addPattern("Pattern 1").id;
    (void)project.addChannel("Channel 1");

    app::CommandRegistry registry{project};
    REQUIRE(registry.execute(std::make_unique<app::AddMidiMappingCommand>(
        -1, 7, "volume", project.masterMixerNode())));

    engine::TempoMap tempo{120.0, 48000.0};

    project::GraphCompileOptions options;
    options.pattern = pattern;   // midiFeedback deliberately left null

    auto compiled = project::compileProjectGraph(project, tempo, options);
    REQUIRE(compiled);

    AudioBufferPool pool;
    pool.allocate(1, 2, 64);

    MidiBuffer midi;
    (void)midi.insert(MidiMessage::controlChange(0, 7, 64));

    pool.buffer(0).clear();
    compiled.graph->process(pool.buffer(0), 64, 0, &midi);

    engine::dsp::MixerStripNode* master = compiled.stripFor(project.masterMixerNode());
    REQUIRE(master != nullptr);
    CHECK(master->gain() == doctest::Approx(0.5).epsilon(0.02));
}
