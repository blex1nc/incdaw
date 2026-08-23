#include "project/instruments/CoreInstrumentFactory.h"

#include "engine/instrument/PianoInstrument.h"
#include "engine/instrument/Sampler.h"
#include "engine/instrument/SimpleSynth.h"

namespace incdaw::project {

namespace {

std::unique_ptr<engine::Instrument> makeSampler(const InstrumentBuildContext& context)
{
    auto sampler = std::make_unique<engine::Sampler>();

    std::vector<engine::SamplerZone> zones;
    for (const ChannelSamplerZone& spec : context.channel.samplerZones) {
        // The streaming decision mirrors the clip policy: long files stream
        // when a streamer exists. Only forward, unlooped zones qualify — a
        // loop has to be resident to be seamless, so looped and reversed
        // zones preload whole regardless of size. Whether the *file* may
        // stream is the resolver's call.
        std::shared_ptr<engine::SamplerZoneStream> zoneStream;

        if (!spec.reverse && spec.loopEnd <= spec.loopStart)
            zoneStream = context.assets.streamAsset(spec.asset);

        engine::SamplerZone zone;

        if (zoneStream != nullptr) {
            zone.sample = zoneStream->head();
            zone.stream = std::move(zoneStream);
        } else {
            auto audio = context.assets.loadAsset(spec.asset);
            if (audio == nullptr)
                continue;   // warned by the resolver; the zone is absent, not wrong

            zone.sample = std::move(audio);
        }

        zone.rootKey       = spec.rootKey;
        zone.keyLow        = spec.keyLow;
        zone.keyHigh       = spec.keyHigh;
        zone.velocityLow   = spec.velocityLow;
        zone.velocityHigh  = spec.velocityHigh;
        zone.start         = spec.start;
        zone.end           = spec.end;
        zone.loopStart     = spec.loopStart;
        zone.loopEnd       = spec.loopEnd;
        zone.loopCrossfade = spec.loopCrossfade;
        zone.reverse       = spec.reverse;
        zone.gain          = spec.gain;
        zones.push_back(std::move(zone));
    }

    // A sampler with no loadable zone still compiles — a channel whose sample
    // is missing should be silent with a warning, not absent.
    sampler->setZones(std::move(zones));
    return sampler;
}

} // namespace

void registerCoreInstruments(std::vector<BuiltinInstrumentEntry>& rows)
{
    // The uids are the ones plugins::builtinSimpleSynth() & co. hand out and
    // engine::builtinInstruments() lists; a literal here keeps the row a
    // compile-time constant, and the registry test holds the three in step.
    rows.push_back({"incdaw.simplesynth",
                    [](const InstrumentBuildContext&) -> std::unique_ptr<engine::Instrument> {
                        return std::make_unique<engine::SimpleSynth>();
                    }});

    rows.push_back({"incdaw.piano",
                    [](const InstrumentBuildContext&) -> std::unique_ptr<engine::Instrument> {
                        return std::make_unique<engine::PianoInstrument>();
                    }});

    rows.push_back({"incdaw.sampler", &makeSampler});
}

} // namespace incdaw::project
