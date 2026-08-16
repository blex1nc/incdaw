// Phase 15 — builtin effects ARE inserts: the no-special-casing half of the
// exit criterion. A builtin slot compiles through exactly the machinery a
// hosted plugin uses — chain wiring, state carriers, parameter sinks, the
// automation registry — with the compiler's builtin branch being the single
// point where the two differ.

#include "doctest.h"

#include "engine/dsp/effects/BuiltinEffects.h"
#include "engine/dsp/effects/UtilityEffects.h"
#include "engine/transport/TempoMap.h"
#include "project/Model.h"
#include "project/ParameterRegistry.h"
#include "project/PluginStateFiles.h"
#include "project/ProjectGraphCompiler.h"

#include <memory>
#include <utility>
#include <vector>

using namespace incdaw;

namespace {

project::Project projectWithMasterInsert(const std::string& uid, project::EntityId& slotOut)
{
    project::Project project;

    project::MixerNode* master = project.findMixerNode(project.masterMixerNode());
    REQUIRE(master != nullptr);

    project::PluginSlot slot;
    slot.id     = project.ids().next();
    slot.plugin = {plugins::Format::builtin, uid};
    master->inserts.push_back(slot);

    slotOut = slot.id;
    return project;
}

} // namespace

TEST_CASE("a builtin insert compiles with no plugin host and no warnings")
{
    project::EntityId slotId;
    project::Project  project = projectWithMasterInsert("incdaw.utility", slotId);
    project.addPattern("P");

    const engine::TempoMap map{120.0, 48000.0};

    project::GraphCompileOptions options;   // deliberately NO insertFactory

    const auto compiled = project::compileProjectGraph(project, map, options);

    REQUIRE(compiled);
    CHECK(compiled.warnings.empty());

    // The slot's state carrier is present exactly as a hosted plugin's is —
    // project save reaches a builtin's parameters through the same door.
    engine::StateIO* state = compiled.insertStateFor(slotId);
    REQUIRE(state != nullptr);

    std::vector<std::uint8_t> blob;
    REQUIRE(state->saveState(blob));
    CHECK(!blob.empty());
}

TEST_CASE("builtin effect state round-trips through the blob")
{
    engine::dsp::UtilityEffect saved;
    saved.setParameter(engine::dsp::UtilityEffect::gainDb, -12.5);
    saved.setParameter(engine::dsp::UtilityEffect::pan, 0.25);
    saved.setParameter(engine::dsp::UtilityEffect::mono, 1.0);

    std::vector<std::uint8_t> blob;
    REQUIRE(saved.saveState(blob));

    engine::dsp::UtilityEffect loaded;
    REQUIRE(loaded.loadState(blob.data(), blob.size()));

    CHECK(loaded.value(engine::dsp::UtilityEffect::gainDb) == doctest::Approx(-12.5));
    CHECK(loaded.value(engine::dsp::UtilityEffect::pan) == doctest::Approx(0.25));
    CHECK(loaded.value(engine::dsp::UtilityEffect::mono) == doctest::Approx(1.0));

    // Garbage is refused, and the instance stays usable.
    const std::uint8_t junk[3] = {1, 2, 3};
    CHECK(!loaded.loadState(junk, 3));
    CHECK(loaded.value(engine::dsp::UtilityEffect::gainDb) == doctest::Approx(-12.5));
}

TEST_CASE("an unknown builtin uid warns and passes through")
{
    project::EntityId slotId;
    project::Project  project = projectWithMasterInsert("incdaw.does-not-exist", slotId);
    project.addPattern("P");

    const engine::TempoMap map{120.0, 48000.0};

    project::GraphCompileOptions options;

    const auto compiled = project::compileProjectGraph(project, map, options);

    REQUIRE(compiled);   // never a failed compile
    CHECK(!compiled.warnings.empty());
    CHECK(compiled.insertStateFor(slotId) == nullptr);
}

TEST_CASE("builtin parameters register into the automation registry like a plugin's")
{
    project::ParameterRegistry registry = project::ParameterRegistry::withBuiltins();
    registry.registerBuiltinEffects();

    // The key scheme is the plugin one — no builtin-specific naming.
    const auto* entry = registry.find(project::ParameterRegistry::pluginParameterKey(
        "incdaw.utility", engine::dsp::UtilityEffect::gainDb));
    REQUIRE(entry != nullptr);

    const auto* applier = std::get_if<project::ParameterRegistry::SinkApplier>(&entry->apply);
    REQUIRE(applier != nullptr);

    // Driving the applier at full scale lands on the parameter's own maximum,
    // through the ParameterSink interface and nothing else.
    engine::dsp::UtilityEffect effect;
    (*applier)(effect, 1.0f);
    CHECK(effect.value(engine::dsp::UtilityEffect::gainDb) == doctest::Approx(24.0));

    (*applier)(effect, 0.0f);
    CHECK(effect.value(engine::dsp::UtilityEffect::gainDb) == doctest::Approx(-60.0));

    // Every catalogued effect registered every parameter.
    for (const engine::dsp::BuiltinEffectInfo& info : engine::dsp::builtinEffects())
        for (std::size_t index = 0; index < info.parameterCount; ++index)
            CHECK(registry.find(project::ParameterRegistry::pluginParameterKey(
                      info.uid, info.parameters[index].id))
                  != nullptr);
}

TEST_CASE("every catalogued effect constructs, prepares and carries the shared interface")
{
    for (const engine::dsp::BuiltinEffectInfo& info : engine::dsp::builtinEffects()) {
        auto node = engine::dsp::makeBuiltinEffect(info.uid);
        REQUIRE_MESSAGE(node != nullptr, info.uid);

        node->prepare(48000.0, 512);

        CHECK(node->parameterSink() != nullptr);
        CHECK(node->stateIO() != nullptr);
        CHECK(node->latencyFrames() == 0);
    }

    CHECK(engine::dsp::makeBuiltinEffect("incdaw.nope") == nullptr);
}

namespace {

/// Current value of one parameter of a live insert, read the way the panel
/// reads it: StateIO -> blob -> the shared decoder.
double decodedValue(engine::StateIO& state, std::uint32_t parameterId)
{
    std::vector<std::uint8_t> blob;
    REQUIRE(state.saveState(blob));

    std::vector<std::pair<std::uint32_t, double>> decoded;
    REQUIRE(engine::dsp::BuiltinEffect::decodeState(blob.data(), blob.size(), decoded));

    for (const auto& [id, value] : decoded)
        if (id == parameterId)
            return value;

    FAIL("parameter not in decoded state");
    return 0.0;
}

} // namespace

TEST_CASE("the compiled graph exposes an insert's parameter sink, and writes land")
{
    project::EntityId slotId;
    project::Project  project = projectWithMasterInsert("incdaw.utility", slotId);
    project.addPattern("P");

    const engine::TempoMap map{120.0, 48000.0};

    const auto compiled =
        project::compileProjectGraph(project, map, project::GraphCompileOptions{});
    REQUIRE(compiled);

    engine::ParameterSink* sink = compiled.insertSinkFor(slotId);
    REQUIRE(sink != nullptr);
    CHECK(compiled.insertSinkFor(project::EntityId{999999}) == nullptr);

    sink->setParameter(engine::dsp::UtilityEffect::gainDb, -6.0);

    engine::StateIO* state = compiled.insertStateFor(slotId);
    REQUIRE(state != nullptr);
    CHECK(decodedValue(*state, engine::dsp::UtilityEffect::gainDb)
          == doctest::Approx(-6.0));
}

TEST_CASE("builtin insert state survives a rebuild through the carry-over")
{
    project::EntityId slotId;
    project::Project  project = projectWithMasterInsert("incdaw.utility", slotId);
    project.addPattern("P");

    const engine::TempoMap map{120.0, 48000.0};

    const auto first =
        project::compileProjectGraph(project, map, project::GraphCompileOptions{});
    REQUIRE(first);

    // A value automation, a MIDI mapping, or the panel set on the live node —
    // nothing in the model records it.
    first.insertSinkFor(slotId)->setParameter(engine::dsp::UtilityEffect::gainDb, -9.0);

    const auto carried = project::captureBuiltinInsertState(project, first);
    REQUIRE(carried.size() == 1);

    // The rebuild constructs a fresh node, which starts at its defaults —
    // this is the reset the carry-over exists to undo.
    const auto second =
        project::compileProjectGraph(project, map, project::GraphCompileOptions{});
    REQUIRE(second);

    engine::StateIO* fresh = second.insertStateFor(slotId);
    REQUIRE(fresh != nullptr);
    CHECK(decodedValue(*fresh, engine::dsp::UtilityEffect::gainDb) == doctest::Approx(0.0));

    project::restoreBuiltinInsertState(carried, second);

    CHECK(decodedValue(*fresh, engine::dsp::UtilityEffect::gainDb) == doctest::Approx(-9.0));

    // A slot that vanished between capture and restore is skipped, not an error.
    project::Project emptyProject;
    emptyProject.addPattern("P");
    const auto third =
        project::compileProjectGraph(emptyProject, map, project::GraphCompileOptions{});
    project::restoreBuiltinInsertState(carried, third);
}
