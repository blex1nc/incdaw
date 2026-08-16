// Phase 16 — MIDI hardware and controller linking.
//
// The exit criterion (docs/ROADMAP.md): a hardware control mapped by learn
// mode drives a mixer, instrument, and plugin parameter, and the mapping
// survives save/load. The learn gesture's tail end is AddMidiMappingCommand;
// what it produces is tested here end to end: a CC arriving in a block's
// live MIDI moves the actual parameter through a compiled graph — before AND
// after the project has been through disk.

#include "doctest.h"

#include "app/CommandRegistry.h"
#include "app/commands/MidiMappingCommands.h"
#include "engine/core/AudioBufferPool.h"
#include "engine/dsp/effects/UtilityEffects.h"
#include "engine/instrument/Sampler.h"
#include "engine/midi/MidiMapNode.h"
#include "engine/transport/TempoMap.h"
#include "project/Model.h"
#include "project/ParameterRegistry.h"
#include "project/ProjectFile.h"
#include "project/ProjectGraphCompiler.h"

#include <filesystem>
#include <memory>
#include <string>

using namespace incdaw;

namespace fs = std::filesystem;

namespace {

struct ScratchDirectory {
    explicit ScratchDirectory(const std::string& name)
        : path(fs::temp_directory_path()
               / ("incdaw-midimap-" + name + "-" + std::to_string(nextSerial())))
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

engine::MidiMessage controlChange(int channel, int controller, int value)
{
    return engine::MidiMessage::controlChange(channel, controller, value, 0);
}

} // namespace

TEST_CASE("the MidiMapNode applies matching control changes")
{
    engine::MidiMapNode node;

    double received = -1.0;

    engine::MidiMapNode::Binding binding;
    binding.controller = 21;
    binding.apply      = [&received](float value) { received = static_cast<double>(value); };
    node.addBinding(std::move(binding));

    engine::AudioBufferPool pool;
    pool.allocate(1, 2, 64);

    engine::MidiBuffer midi;
    midi.insert(controlChange(0, 21, 127));

    engine::ProcessContext context;
    context.output     = pool.buffer(0);
    context.frameCount = 64;
    context.sampleRate = 48000.0;
    context.liveMidi   = &midi;

    node.process(context);
    CHECK(received == doctest::Approx(1.0));

    SUBCASE("a different controller does not fire")
    {
        received = -1.0;
        engine::MidiBuffer other;
        other.insert(controlChange(0, 22, 127));
        context.liveMidi = &other;

        node.process(context);
        CHECK(received == doctest::Approx(-1.0));
    }

    SUBCASE("an inverted range maps 127 to the minimum")
    {
        engine::MidiMapNode inverted;

        engine::MidiMapNode::Binding flip;
        flip.controller = 21;
        flip.minValue   = 1.0f;
        flip.maxValue   = 0.0f;
        flip.apply      = [&received](float value) { received = static_cast<double>(value); };
        inverted.addBinding(std::move(flip));

        inverted.process(context);
        CHECK(received == doctest::Approx(0.0));
    }
}

TEST_CASE("mapping commands are undoable and re-learn replaces")
{
    project::Project project;
    app::CommandRegistry registry{project};

    REQUIRE(registry.execute(std::make_unique<app::AddMidiMappingCommand>(
        -1, 74, "volume", project.masterMixerNode())));
    REQUIRE(project.midiMappings().size() == 1);
    const project::EntityId first = project.midiMappings()[0].id;

    // The same CC cannot be mapped twice — the shell removes first, and the
    // command itself refuses a silent double-bind.
    CHECK(!registry.execute(std::make_unique<app::AddMidiMappingCommand>(
        -1, 74, "pan", project.masterMixerNode())));
    CHECK(project.midiMappings().size() == 1);

    REQUIRE(registry.execute(std::make_unique<app::RemoveMidiMappingCommand>(first)));
    CHECK(project.midiMappings().empty());

    REQUIRE(registry.execute(std::make_unique<app::AddMidiMappingCommand>(
        -1, 74, "pan", project.masterMixerNode())));
    CHECK(project.midiMappings()[0].parameterKey == "pan");

    // Undo unwinds in order: the new mapping goes, the removal comes back.
    REQUIRE(registry.undo());
    CHECK(project.midiMappings().empty());
    REQUIRE(registry.undo());
    REQUIRE(project.midiMappings().size() == 1);
    CHECK(project.midiMappings()[0].id == first);
    CHECK(project.midiMappings()[0].parameterKey == "volume");
}

TEST_CASE("the v1.4 fixture still loads")
{
    const fs::path fixture = fs::path{INCDAW_FIXTURE_DIR} / "v1.4" / "Fixture.incdaw";
    REQUIRE(fs::exists(fixture));

    project::Project project;
    const auto result = project::ProjectFile::load(project, fixture);

    REQUIRE(result.succeeded);
    CHECK(result.migrated);   // 1.4 -> 1.5 (additive) since instrument parameters
    CHECK(result.migratedFrom == "1.4");

    REQUIRE(project.midiMappings().size() == 1);
    const project::MidiMapping& mapping = project.midiMappings()[0];
    CHECK(mapping.controller == 74);
    CHECK(mapping.midiChannel == -1);
    CHECK(mapping.parameterKey == "volume");
    CHECK(mapping.targetEntity == project.masterMixerNode());
}

TEST_CASE("the v1.3 fixture migrates to 1.4 with no mappings")
{
    const fs::path fixture = fs::path{INCDAW_FIXTURE_DIR} / "v1.3" / "Fixture.incdaw";

    project::Project project;
    const auto result = project::ProjectFile::load(project, fixture);

    REQUIRE(result.succeeded);
    CHECK(result.migrated);
    CHECK(result.migratedFrom == "1.3");
    CHECK(project.midiMappings().empty());
}

TEST_CASE("EXIT CRITERION: mapped controls drive mixer, instrument and plugin "
          "parameters, surviving save/load")
{
    ScratchDirectory scratch{"exit"};

    // The project: a sampler channel (instrument target), a utility insert on
    // the master (the plugin-parameter target, driven through the identical
    // SinkApplier machinery hosted plugins use), and the master strip itself
    // (the mixer target).
    project::Project project;

    auto& channel        = project.addChannel("Keys");
    channel.instrument   = plugins::builtinSampler();
    const auto channelId = channel.id;

    project::MixerNode* master = project.findMixerNode(project.masterMixerNode());
    project::PluginSlot slot;
    slot.id     = project.ids().next();
    slot.plugin = {plugins::Format::builtin, "incdaw.utility"};
    master->inserts.push_back(slot);
    const auto slotId = slot.id;

    project.addPattern("P");

    // The three mappings the criterion names — exactly what learn mode's
    // command writes.
    app::CommandRegistry registry{project};
    REQUIRE(registry.execute(std::make_unique<app::AddMidiMappingCommand>(
        -1, 1, "volume", project.masterMixerNode())));
    REQUIRE(registry.execute(std::make_unique<app::AddMidiMappingCommand>(
        -1, 2,
        project::ParameterRegistry::pluginParameterKey(
            "incdaw.sampler", static_cast<std::uint32_t>(engine::SamplerParam::filterCutoffHz)),
        channelId)));
    REQUIRE(registry.execute(std::make_unique<app::AddMidiMappingCommand>(
        -1, 3,
        project::ParameterRegistry::pluginParameterKey(
            "incdaw.utility", engine::dsp::UtilityEffect::gainDb),
        slotId)));

    project::ParameterRegistry parameters = project::ParameterRegistry::withBuiltins();
    parameters.registerBuiltinEffects();
    parameters.registerBuiltinInstruments();

    const engine::TempoMap map{120.0, 48000.0};

    const auto drive = [&](const project::Project& compiledProject) {
        project::GraphCompileOptions options;
        options.maxBlockSize = 128;
        options.parameters   = &parameters;

        const auto compiled = project::compileProjectGraph(compiledProject, map, options);
        REQUIRE(compiled);

        engine::dsp::MixerStripNode* strip =
            compiled.stripFor(compiledProject.masterMixerNode());
        REQUIRE(strip != nullptr);

        auto* instrumentNode = compiled.instrumentFor(
            compiledProject.channels().front().id);
        REQUIRE(instrumentNode != nullptr);
        auto* sampler = dynamic_cast<engine::Sampler*>(instrumentNode->instrument());
        REQUIRE(sampler != nullptr);

        engine::StateIO* utilityState =
            compiled.insertStateFor(compiledProject.mixerNodes()
                                        .front()
                                        .inserts.front()
                                        .id);
        REQUIRE(utilityState != nullptr);

        const float  gainBefore   = strip->gain();
        const double cutoffBefore = sampler->filterCutoffHz();

        // The hardware: three knobs turn, all in one block's live MIDI.
        engine::MidiBuffer midi;
        midi.insert(controlChange(0, 1, 0));      // volume to the bottom
        midi.insert(controlChange(0, 2, 0));      // cutoff to 20 Hz
        midi.insert(controlChange(0, 3, 127));    // utility gain to +24 dB

        engine::AudioBufferPool pool;
        pool.allocate(1, 2, 128);
        compiled.graph->process(pool.buffer(0), 128, 0, &midi);

        CHECK(strip->gain() < gainBefore);
        CHECK(strip->gain() == doctest::Approx(0.0));

        CHECK(sampler->filterCutoffHz() < cutoffBefore);
        CHECK(sampler->filterCutoffHz() == doctest::Approx(20.0));

        // The utility saw +24 dB through its sink; its state blob proves it.
        std::vector<std::uint8_t> blob;
        REQUIRE(utilityState->saveState(blob));

        engine::dsp::UtilityEffect probe;
        REQUIRE(probe.loadState(blob.data(), blob.size()));
        CHECK(probe.value(engine::dsp::UtilityEffect::gainDb) == doctest::Approx(24.0));
    };

    drive(project);

    // And the criterion's last clause: the mapping survives save/load.
    const fs::path packagePath = scratch.path / "Song.incdaw";
    REQUIRE(project::ProjectFile::save(project, packagePath));

    project::Project loaded;
    REQUIRE(project::ProjectFile::load(loaded, packagePath));
    REQUIRE(loaded.midiMappings().size() == 3);

    drive(loaded);
}
