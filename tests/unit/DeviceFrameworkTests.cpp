// Plugin archive, Wave 0 — the contract freeze (docs/plugin-archive/
// 00-CONTRACTS.md, AGENT-1-FRAMEWORK.md).
//
// Three registries and one vocabulary, all frozen so three agents can build
// on them in parallel. The claims: the effect catalogue still holds exactly
// the seventeen devices the registrars replaced, with no duplicate uid; the
// instrument registry builds the three core instruments through an
// AssetResolver (and refuses an unknown uid cleanly); the spec catalogue is
// empty today and says so; and the DeviceUiSpec vocabulary can express the
// whole of the existing Tone panel as pure data — the existence proof Wave
// 1 renders, checked here before sixty panels are written against it.

#include "doctest.h"

#include "app/DevicePreset.h"
#include "app/devices/DeviceUiCatalogue.h"
#include "app/devices/DeviceUiSpec.h"
#include "engine/dsp/effects/EffectRegistry.h"
#include "engine/dsp/effects/SpaceEffects.h"
#include "engine/dsp/effects/ToneEffects.h"
#include "engine/instrument/BuiltinInstruments.h"
#include "engine/instrument/Sampler.h"
#include "plugins/PluginIdentifier.h"
#include "project/InstrumentFactory.h"

#include <algorithm>
#include <set>
#include <string>
#include <vector>

using namespace incdaw;

namespace {

/// A resolver over one synthetic asset: a short sine, never a file.
class FixtureAssets final : public project::AssetResolver {
public:
    explicit FixtureAssets(project::EntityId known) : known_(known)
    {
        auto data          = std::make_shared<engine::AudioFileData>();
        data->sampleRate   = 48000.0;
        data->channelCount = 1;
        data->frameCount   = 480;
        data->channels.assign(1, std::vector<engine::Sample>(480, engine::Sample{0.25f}));
        audio_ = std::move(data);
    }

    const std::string* assetPath(project::EntityId asset) const override
    {
        return asset == known_ ? &path_ : nullptr;
    }

    std::shared_ptr<const engine::AudioFileData> loadAsset(project::EntityId asset) override
    {
        ++loads;
        if (asset == known_)
            return audio_;

        warnings.push_back("missing");
        return nullptr;
    }

    std::shared_ptr<engine::SamplerZoneStream> streamAsset(project::EntityId) override
    {
        ++streamRequests;
        return nullptr;   // the fixture never streams; the factory must preload
    }

    void warn(std::string message) override { warnings.push_back(std::move(message)); }

    int streamRequests = 0;
    int loads          = 0;
    std::vector<std::string> warnings;

private:
    project::EntityId                            known_;
    std::string                                  path_ = "<fixture>";
    std::shared_ptr<const engine::AudioFileData> audio_;
};

} // namespace

// ── Effects ──────────────────────────────────────────────────────────────────

TEST_CASE("the effect catalogue is the seventeen devices the registrars took over, once each")
{
    const std::set<std::string> expected = {
        "incdaw.utility",   "incdaw.filter",    "incdaw.eq",        "incdaw.tone",
        "incdaw.saturator", "incdaw.compressor", "incdaw.limiter",  "incdaw.gate",
        "incdaw.delay",     "incdaw.reverb",    "incdaw.analyzer",  "incdaw.limiterla",
        "incdaw.loudness",  "incdaw.chorus",    "incdaw.flanger",   "incdaw.phaser",
        "incdaw.transientsplit",
    };

    std::set<std::string> actual;
    for (const engine::dsp::BuiltinEffectInfo& info : engine::dsp::builtinEffects()) {
        CAPTURE(info.uid);
        CHECK(actual.insert(info.uid).second);   // no uid twice
        if (info.parameterCount > 0)             // the two meters have no parameters
            CHECK(info.parameters != nullptr);
    }

    CHECK(actual == expected);
    CHECK(engine::dsp::builtinEffects().size() == expected.size());
}

TEST_CASE("every catalogue row builds, and the row's table is the effect's own")
{
    for (const engine::dsp::BuiltinEffectInfo& info : engine::dsp::builtinEffects()) {
        CAPTURE(info.uid);
        auto node = engine::dsp::makeBuiltinEffect(info.uid, 48000.0);
        REQUIRE(node != nullptr);

        // The probe trick: the catalogue borrowed its table from an instance,
        // so a freshly built one must agree with it pointer for pointer.
        auto* effect = dynamic_cast<engine::dsp::BuiltinEffect*>(node.get());
        REQUIRE(effect != nullptr);
        CHECK(effect->parameters() == info.parameters);
        CHECK(effect->parameterCount() == info.parameterCount);
    }

    CHECK(engine::dsp::makeBuiltinEffect("incdaw.nosuch", 48000.0) == nullptr);
}

TEST_CASE("a family registrar is self-contained: one call, its own rows, nothing else")
{
    std::vector<engine::dsp::EffectCatalogueEntry> rows;
    engine::dsp::registerSpaceEffects(rows);

    REQUIRE(rows.size() == 2);
    CHECK(std::string(rows[0].info.uid) == "incdaw.delay");
    CHECK(std::string(rows[1].info.uid) == "incdaw.reverb");
    CHECK(rows[1].make(44100.0) != nullptr);
}

// ── Instruments ──────────────────────────────────────────────────────────────

TEST_CASE("the instrument registry and the parameter catalogue name the same builtins")
{
    std::set<std::string> registered;
    for (const project::BuiltinInstrumentEntry& entry : project::builtinInstrumentEntries()) {
        CAPTURE(entry.uid);
        CHECK(registered.insert(entry.uid).second);
        CHECK(static_cast<bool>(entry.make));
        CHECK(engine::findBuiltinInstrument(entry.uid) != nullptr);
    }

    std::set<std::string> catalogued;
    for (const engine::BuiltinInstrumentInfo& info : engine::builtinInstruments())
        catalogued.insert(info.uid);

    CHECK(registered == catalogued);
    CHECK(registered.count(plugins::builtinSimpleSynth().uid) == 1);
    CHECK(registered.count(plugins::builtinPiano().uid) == 1);
    CHECK(registered.count(plugins::builtinSampler().uid) == 1);
}

TEST_CASE("the registry builds each core instrument and refuses an unknown uid")
{
    project::Channel channel;
    FixtureAssets    assets{project::EntityId{}};

    channel.instrument = plugins::builtinSimpleSynth();
    auto synth = project::makeBuiltinInstrument({channel, 48000.0, assets});
    REQUIRE(synth != nullptr);
    CHECK(std::string(synth->name()) == "Reference Synth");

    channel.instrument = plugins::builtinPiano();
    auto piano = project::makeBuiltinInstrument({channel, 48000.0, assets});
    REQUIRE(piano != nullptr);
    CHECK(std::string(piano->name()) == "INCDAW Piano");

    channel.instrument = {plugins::Format::builtin, "incdaw.nosuch"};
    CHECK(project::makeBuiltinInstrument({channel, 48000.0, assets}) == nullptr);
    CHECK(project::findBuiltinInstrumentEntry("incdaw.nosuch") == nullptr);
}

TEST_CASE("the sampler factory resolves its zones through the AssetResolver")
{
    const project::EntityId known{7};
    const project::EntityId missing{8};
    FixtureAssets           assets{known};

    project::Channel channel;
    channel.instrument = plugins::builtinSampler();

    project::ChannelSamplerZone resident;
    resident.asset   = known;
    resident.rootKey = 60;

    project::ChannelSamplerZone looped = resident;   // a loop never asks to stream
    looped.loopStart = 0;
    looped.loopEnd   = 100;

    project::ChannelSamplerZone absent = resident;
    absent.asset = missing;

    channel.samplerZones = {resident, looped, absent};

    auto instrument = project::makeBuiltinInstrument({channel, 48000.0, assets});
    REQUIRE(instrument != nullptr);

    auto* sampler = dynamic_cast<engine::Sampler*>(instrument.get());
    REQUIRE(sampler != nullptr);

    // Two zones load; the missing one is skipped with the resolver's warning,
    // and the sampler still exists — silent with a warning, not absent.
    CHECK(sampler->zoneCount() == 2);
    CHECK(assets.loads == 3);
    CHECK(assets.warnings.size() == 1);

    // Only the forward, unlooped zones asked whether they may stream.
    CHECK(assets.streamRequests == 2);
}

// ── UI specs ─────────────────────────────────────────────────────────────────

TEST_CASE("no device has a UI spec yet, and a missing spec is a nullptr, not an error")
{
    CHECK(app::deviceUiSpecs().empty());
    CHECK(app::deviceUiSpec("incdaw.tone") == nullptr);
    CHECK(app::deviceUiSpec("") == nullptr);
}

TEST_CASE("the vocabulary expresses the whole Tone panel as data")
{
    using namespace app::widgets;
    using Eq = engine::dsp::EqEffect;

    const app::DeviceUiRange hz{20.0, 20000.0, app::DeviceSkew::logarithmic, 0.0};

    // Three bipolar gain knobs over a response curve, and the frequencies
    // and Q behind a folded "Advanced" section — TonePanel.mm, as a tree.
    app::DeviceUiSpec spec;
    spec.uid             = "incdaw.tone";
    spec.title           = "Tone";
    spec.preferredWidth  = 468.0;
    spec.preferredHeight = 344.0;
    spec.root            = {
        drawn(app::DeviceWidget::drawableCurve,
              {Eq::lowFreq, Eq::lowGainDb, Eq::midFreq, Eq::midGainDb, Eq::midQ, Eq::highFreq,
               Eq::highGainDb},
              "Response", "eq-response")
            .withTint("accent"),
        grid(3, {knob(Eq::lowGainDb, "BASS").withUnit("dB").asBipolar(),
                 knob(Eq::midGainDb, "MID").withUnit("dB").asBipolar(),
                 knob(Eq::highGainDb, "TREBLE").withUnit("dB").asBipolar()}),
        section("Advanced", {slider(Eq::lowFreq, "Bass Freq").withUnit("Hz").withRange(hz),
                             slider(Eq::midFreq, "Mid Freq").withUnit("Hz").withRange(hz),
                             slider(Eq::midQ, "Mid Q"),
                             slider(Eq::highFreq, "Treble Freq").withUnit("Hz").withRange(hz)})
            .startCollapsed(),
    };

    REQUIRE(spec.root.size() == 3);
    CHECK(spec.customView.empty());

    const app::DeviceUiWidget& curve = spec.root[0];
    CHECK(curve.kind == app::DeviceWidget::drawableCurve);
    CHECK(curve.parameters.size() == Eq::paramCount);
    CHECK(curve.plot == "eq-response");
    CHECK(curve.tint == "accent");

    const app::DeviceUiWidget& knobs = spec.root[1];
    CHECK(knobs.kind == app::DeviceWidget::grid);
    CHECK(knobs.columns == 3);
    REQUIRE(knobs.children.size() == 3);
    for (const app::DeviceUiWidget& knob : knobs.children) {
        CHECK(knob.kind == app::DeviceWidget::knob);
        CHECK(knob.bipolar);
        CHECK(knob.unit == "dB");
        CHECK_FALSE(knob.range.has_value());   // the parameter table's ±dB range applies
    }
    CHECK(knobs.children[2].parameters == std::vector<std::uint32_t>{Eq::highGainDb});

    const app::DeviceUiWidget& advanced = spec.root[2];
    CHECK(advanced.kind == app::DeviceWidget::section);
    CHECK(advanced.collapsed);
    REQUIRE(advanced.children.size() == 4);
    REQUIRE(advanced.children[0].range.has_value());
    CHECK(advanced.children[0].range->skew == app::DeviceSkew::logarithmic);
    CHECK(advanced.children[0].range->min == 20.0);
    CHECK_FALSE(advanced.children[2].range.has_value());   // Q is linear, as the panel draws it

    // Every parameter the effect has is reachable from the tree exactly where
    // the panel puts it: the three gains on knobs, the four others in Advanced.
    std::set<std::uint32_t> editable;
    for (const app::DeviceUiWidget& knob : knobs.children)
        editable.insert(knob.parameters.begin(), knob.parameters.end());
    for (const app::DeviceUiWidget& control : advanced.children)
        editable.insert(control.parameters.begin(), control.parameters.end());
    CHECK(editable.size() == Eq::paramCount);
}

TEST_CASE("a preset is a device's state blob under a name")
{
    app::DevicePreset preset;
    preset.deviceUid = "incdaw.eq";
    preset.name      = "Warm";
    preset.author    = "INCDAW";
    preset.tags      = {"bass", "vocal"};
    preset.state     = {1, 0, 0, 0};

    const app::DevicePreset copy = preset;
    CHECK(copy.deviceUid == "incdaw.eq");
    CHECK(copy.tags.size() == 2);
    CHECK(copy.state == std::vector<std::uint8_t>{1, 0, 0, 0});
}
