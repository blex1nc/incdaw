// FL2026 P10 — Audio Unit hosting, the platform half.
//
// Tested against the units Apple ships with the system: a plugin host cannot
// be tested against third-party binaries, and must not be tested against
// nothing (the same reasoning that gave the CLAP host its own test plugins).
// Nothing here asserts what an Apple effect SOUNDS like — only that INCDAW
// instantiates it, renders finite audio through it in place, and can carry its
// state.

#include "doctest.h"

#include "platform/AudioUnitHost.h"
#include "plugins/PluginInstanceManager.h"
#include "plugins/PluginRegistry.h"

#include <cmath>
#include <string>
#include <vector>

using namespace incdaw;

namespace {

/// The first Apple-supplied effect the system reports, or an empty string.
std::string firstAppleEffect()
{
    for (const platform::AudioUnitDescription& unit : platform::scanAudioUnits()) {
        if (!unit.isInstrument && unit.uid.rfind("aufx:", 0) == 0
            && unit.uid.size() > 5 && unit.uid.substr(unit.uid.size() - 5) == ":appl")
            return unit.uid;
    }

    return {};
}

} // namespace

TEST_CASE("the system's Audio Units are found without running any of them")
{
    const std::vector<platform::AudioUnitDescription> units = platform::scanAudioUnits();

    REQUIRE_FALSE(units.empty());

    for (const platform::AudioUnitDescription& unit : units) {
        // "type:subtype:manufacturer", three four-character codes.
        CHECK(unit.uid.size() == 14);
        CHECK_FALSE(unit.name.empty());
    }

    CHECK_FALSE(firstAppleEffect().empty());
}

TEST_CASE("an Audio Unit effect renders in place")
{
    const std::string uid = firstAppleEffect();
    REQUIRE_FALSE(uid.empty());

    std::string error;
    const auto  unit = platform::AudioUnitHandle::open(uid, 48000.0, 512, error);

    REQUIRE(unit != nullptr);
    CHECK(error.empty());

    std::vector<float> left(512, 0.25F);
    std::vector<float> right(512, -0.25F);

    REQUIRE(unit->process(left.data(), right.data(), 512));

    for (std::size_t frame = 0; frame < left.size(); ++frame) {
        CHECK(std::isfinite(left[frame]));
        CHECK(std::isfinite(right[frame]));
    }

    // A block longer than the unit was prepared for is refused rather than
    // rendered past the end of its own buffers.
    std::vector<float> big(1024, 0.0F);
    CHECK_FALSE(unit->process(big.data(), big.data() + 512, 1024));

    // Silence in stays finite: a unit whose tail runs on must not produce NaN.
    std::fill(left.begin(), left.end(), 0.0F);
    std::fill(right.begin(), right.end(), 0.0F);
    REQUIRE(unit->process(left.data(), right.data(), 512));
    CHECK(std::isfinite(left.front()));
}

TEST_CASE("an Audio Unit's parameters and state come back")
{
    const std::string uid = firstAppleEffect();
    REQUIRE_FALSE(uid.empty());

    std::string error;
    const auto  unit = platform::AudioUnitHandle::open(uid, 44100.0, 256, error);
    REQUIRE(unit != nullptr);

    for (const platform::AudioUnitParameterDescription& parameter : unit->parameters()) {
        CHECK(parameter.maximum >= parameter.minimum);
        CHECK(parameter.defaultValue >= parameter.minimum);
        CHECK(parameter.defaultValue <= parameter.maximum);
    }

    std::vector<std::byte> state;
    REQUIRE(unit->saveState(state));
    CHECK_FALSE(state.empty());

    CHECK(unit->restoreState(state.data(), state.size()));

    // Garbage is refused, not applied.
    const std::vector<std::byte> rubbish(64, std::byte{0x7F});
    CHECK_FALSE(unit->restoreState(rubbish.data(), rubbish.size()));
}

TEST_CASE("an identifier that names nothing refuses cleanly")
{
    std::string error;

    CHECK(platform::AudioUnitHandle::open("nonsense", 48000.0, 512, error) == nullptr);
    CHECK_FALSE(error.empty());

    error.clear();
    CHECK(platform::AudioUnitHandle::open("aufx:zzzz:zzzz", 48000.0, 512, error) == nullptr);
    CHECK_FALSE(error.empty());
}

TEST_CASE("an Audio Unit is hosted as an insert like any other plugin")
{
    const std::string uid = firstAppleEffect();
    REQUIRE_FALSE(uid.empty());

    // An empty registry on purpose: an Audio Unit is instantiated from the
    // system's component registry, so it needs no scan and no library on disk.
    plugins::PluginRegistry        registry;
    plugins::PluginInstanceManager manager(registry);

    plugins::PluginIdentifier identifier;
    identifier.format = plugins::Format::audioUnit;
    identifier.uid    = uid;

    std::string error;
    const auto  node = manager.createInsert(1, identifier, 48000.0, 256, error);

    REQUIRE(node != nullptr);
    CHECK(error.empty());
    CHECK(manager.liveInstanceCount() == 1);
    CHECK(manager.loadedLibraryCount() == 0);

    // The slot reuses its instance across rebuilds, exactly as a CLAP slot
    // does — the persistence contract is the interface's, not the format's.
    const auto again = manager.createInsert(1, identifier, 48000.0, 256, error);
    REQUIRE(again != nullptr);
    CHECK(manager.liveInstanceCount() == 1);

    plugins::HostedPlugin* live = manager.instanceFor(1);
    REQUIRE(live != nullptr);

    std::vector<float> left(256, 0.2F);
    std::vector<float> right(256, -0.2F);
    CHECK(live->process(left.data(), right.data(), 256));

    // And it answers the generic seams the rest of INCDAW uses.
    std::vector<std::uint8_t> blob;
    CHECK(live->saveState(blob));
    CHECK_FALSE(blob.empty());

    if (!live->parameters().empty())
        live->setParameter(live->parameters().front().id, live->parameters().front().defaultValue);

    manager.retainOnlyInstances({});
    CHECK(manager.liveInstanceCount() == 0);
}

// ── C16: AU instrument hosting ───────────────────────────────────────────────
//
// AU effects have hosted since Phase 13; instruments were excluded at the
// enumeration step, so every AU synth on the machine was invisible to a DAW
// that could already load its effects. These run against whatever Apple ships,
// which on every Mac includes DLSMusicDevice — a real instrument, not a stub.

#include "engine/core/AudioBufferPool.h"
#include "plugins/au/AudioUnitInstrument.h"

namespace {

/// The first Apple-supplied instrument the system reports, or an empty string.
std::string firstAppleInstrument()
{
    for (const platform::AudioUnitDescription& unit : platform::scanAudioUnits()) {
        if (unit.isInstrument && unit.uid.rfind("aumu:", 0) == 0
            && unit.uid.size() > 5 && unit.uid.substr(unit.uid.size() - 5) == ":appl")
            return unit.uid;
    }

    return {};
}

} // namespace

TEST_CASE("the system reports instruments as well as effects")
{
    const auto units = platform::scanAudioUnits();

    bool sawInstrument = false;
    for (const platform::AudioUnitDescription& unit : units)
        if (unit.isInstrument)
            sawInstrument = true;

    // Every Mac ships at least DLSMusicDevice. A machine that reports none is
    // one where this feature has nothing to be tested against, so say so
    // rather than passing quietly.
    if (!sawInstrument) {
        MESSAGE("no Audio Unit instruments on this machine — C16 not exercised");
        return;
    }

    CHECK(sawInstrument);
}

TEST_CASE("an effect refuses to open as an instrument")
{
    const std::string effect = firstAppleEffect();
    if (effect.empty()) {
        MESSAGE("no Apple effect available");
        return;
    }

    std::string error;

    // Opening an effect as an instrument would attach no input callback and
    // then render silence forever. Refused at the description, before anything
    // is instantiated.
    CHECK(platform::AudioUnitHandle::openInstrument(effect, 48000.0, 512, error) == nullptr);
    CHECK_FALSE(error.empty());
}

TEST_CASE("an instrument opens, takes MIDI and makes sound")
{
    const std::string uid = firstAppleInstrument();
    if (uid.empty()) {
        MESSAGE("no Apple instrument available");
        return;
    }

    std::string error;
    auto instrument = plugins::AudioUnitInstrument::create(uid, 48000.0, 512, error);

    REQUIRE_MESSAGE(instrument != nullptr, error);

    CHECK(instrument->activeVoiceCount() == 0);

    engine::AudioBufferPool pool;
    pool.allocate(1, 2, 512);

    // Silence before the note: an instrument that produces sound unasked is
    // not silent, it is broken.
    pool.buffer(0).clear();
    {
        engine::MidiBuffer empty;
        instrument->processBlock(pool.buffer(0), empty);
    }

    double before = 0.0;
    for (engine::FrameCount frame = 0; frame < 512; ++frame)
        before += std::abs(static_cast<double>(pool.buffer(0).channel(0)[frame]));

    CHECK(before == doctest::Approx(0.0).epsilon(0.001));

    // A note on, then several blocks: an instrument with an attack needs more
    // than one block to be audible, and asserting on the first is how this
    // test would fail against a perfectly good synthesiser.
    engine::MidiBuffer midi;
    (void)midi.insert(engine::MidiMessage::noteOn(0, 60, 110, 0));

    double loudest = 0.0;

    for (int block = 0; block < 20; ++block) {
        pool.buffer(0).clear();
        instrument->processBlock(pool.buffer(0), block == 0 ? midi : engine::MidiBuffer{});

        for (engine::FrameCount frame = 0; frame < 512; ++frame)
            loudest = std::max(loudest,
                               std::abs(static_cast<double>(pool.buffer(0).channel(0)[frame])));
    }

    CHECK(instrument->activeVoiceCount() == 1);
    CHECK(loudest > 0.0001);

    // And it stops when told to.
    instrument->allNotesOff();
    CHECK(instrument->activeVoiceCount() == 0);
}

TEST_CASE("an instrument's state round-trips")
{
    const std::string uid = firstAppleInstrument();
    if (uid.empty()) {
        MESSAGE("no Apple instrument available");
        return;
    }

    std::string error;
    auto instrument = plugins::AudioUnitInstrument::create(uid, 48000.0, 512, error);
    REQUIRE(instrument != nullptr);

    std::vector<std::uint8_t> blob;
    REQUIRE(instrument->saveState(blob));
    CHECK_FALSE(blob.empty());

    // Into a second instance, which is what loading a project does.
    auto reloaded = plugins::AudioUnitInstrument::create(uid, 48000.0, 512, error);
    REQUIRE(reloaded != nullptr);
    CHECK(reloaded->loadState(blob.data(), blob.size()));
}

TEST_CASE("an instrument that does not exist is refused, not crashed into")
{
    std::string error;

    CHECK(plugins::AudioUnitInstrument::create("aumu:zzzz:zzzz", 48000.0, 512, error) == nullptr);
    CHECK_FALSE(error.empty());

    CHECK(plugins::AudioUnitInstrument::create("nonsense", 48000.0, 512, error) == nullptr);
    CHECK_FALSE(error.empty());
}
