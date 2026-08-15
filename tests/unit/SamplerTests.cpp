// Phase 14 — the sampler: zones, mapping, repitch, envelope.
//
// The samples are synthetic and deterministic — a ramp whose value IS its
// frame index, and constants that name their zone — so every assertion reads
// straight off the output: pitch is which source frame arrived when, mapping
// is which constant is audible, layering is their sum.

#include "doctest.h"

#include "engine/core/AudioBufferPool.h"
#include "engine/instrument/Sampler.h"
#include "engine/midi/MidiBuffer.h"

#include <cmath>
#include <memory>
#include <vector>

using namespace incdaw;
using namespace incdaw::engine;

namespace {

constexpr FrameCount blockSize = 256;

/// A mono sample whose frame f holds the value f * scale.
std::shared_ptr<const AudioFileData> rampSample(FrameCount frames, Sample scale = 1.0f,
                                                SampleRate rate = 48000.0)
{
    auto data          = std::make_shared<AudioFileData>();
    data->sampleRate   = rate;
    data->channelCount = 1;
    data->frameCount   = frames;
    data->channels.resize(1);
    data->channels[0].resize(static_cast<std::size_t>(frames));

    for (FrameCount frame = 0; frame < frames; ++frame)
        data->channels[0][static_cast<std::size_t>(frame)] =
            static_cast<Sample>(frame) * scale;

    return data;
}

/// A constant-valued sample: whoever hears `value` knows which zone played.
std::shared_ptr<const AudioFileData> constantSample(Sample value, FrameCount frames = 4096)
{
    auto data          = std::make_shared<AudioFileData>();
    data->sampleRate   = 48000.0;
    data->channelCount = 1;
    data->frameCount   = frames;
    data->channels.resize(1);
    data->channels[0].assign(static_cast<std::size_t>(frames), value);
    return data;
}

/// A stereo sample with distinct constant channels.
std::shared_ptr<const AudioFileData> stereoSample(Sample left, Sample right,
                                                  FrameCount frames = 4096)
{
    auto data          = std::make_shared<AudioFileData>();
    data->sampleRate   = 48000.0;
    data->channelCount = 2;
    data->frameCount   = frames;
    data->channels.resize(2);
    data->channels[0].assign(static_cast<std::size_t>(frames), left);
    data->channels[1].assign(static_cast<std::size_t>(frames), right);
    return data;
}

/// An instant-on, full-sustain envelope, so amplitude assertions are exact.
void makeEnvelopeTransparent(Sampler& sampler)
{
    sampler.setAttackSeconds(0.0);
    sampler.setDecaySeconds(0.0);
    sampler.setSustainLevel(1.0);
    sampler.setReleaseSeconds(0.0);
}

struct Rendered {
    AudioBufferPool     pool;
    std::vector<Sample> left;
    std::vector<Sample> right;
};

/// Renders `blocks` blocks, feeding `midi` into the first only.
Rendered render(Sampler& sampler, const MidiBuffer& firstBlock, int blocks)
{
    Rendered result;
    result.pool.allocate(1, 2, blockSize);

    MidiBuffer empty;

    for (int block = 0; block < blocks; ++block) {
        const auto output = result.pool.buffer(0);
        output.clear();

        sampler.processBlock(output, block == 0 ? firstBlock : empty);

        result.left.insert(result.left.end(), output.channel(0),
                           output.channel(0) + blockSize);
        result.right.insert(result.right.end(), output.channel(1),
                            output.channel(1) + blockSize);
    }

    return result;
}

SamplerZone zoneOf(std::shared_ptr<const AudioFileData> sample)
{
    SamplerZone zone;
    zone.sample = std::move(sample);
    return zone;
}

} // namespace

TEST_CASE("at the root key the sample plays back verbatim")
{
    Sampler sampler;
    sampler.prepare(48000.0, blockSize);
    makeEnvelopeTransparent(sampler);
    sampler.setZones({zoneOf(rampSample(1024, 0.001f))});

    MidiBuffer midi;
    midi.insert(MidiMessage::noteOn(0, 60, 127, 0));

    const auto rendered = render(sampler, midi, 1);

    for (FrameCount frame = 0; frame < blockSize; ++frame)
        REQUIRE(rendered.left[static_cast<std::size_t>(frame)]
                == doctest::Approx(static_cast<double>(frame) * 0.001).epsilon(0.001));
}

TEST_CASE("an octave above the root reads the source twice as fast")
{
    Sampler sampler;
    sampler.prepare(48000.0, blockSize);
    makeEnvelopeTransparent(sampler);
    sampler.setZones({zoneOf(rampSample(4096, 0.0001f))});

    MidiBuffer midi;
    midi.insert(MidiMessage::noteOn(0, 72, 127, 0));   // root is 60

    const auto rendered = render(sampler, midi, 1);

    // Linear interpolation lands exactly on integers at rate 2.
    for (FrameCount frame = 1; frame < blockSize; ++frame)
        REQUIRE(rendered.left[static_cast<std::size_t>(frame)]
                == doctest::Approx(static_cast<double>(frame) * 2.0 * 0.0001).epsilon(0.001));
}

TEST_CASE("keys and velocities outside a zone stay silent")
{
    Sampler sampler;
    sampler.prepare(48000.0, blockSize);
    makeEnvelopeTransparent(sampler);

    SamplerZone zone  = zoneOf(constantSample(0.5f));
    zone.keyLow       = 60;
    zone.keyHigh      = 72;
    zone.velocityLow  = 64;
    zone.velocityHigh = 127;
    sampler.setZones({zone});

    MidiBuffer wrongKey;
    wrongKey.insert(MidiMessage::noteOn(0, 40, 100, 0));
    (void)render(sampler, wrongKey, 1);
    CHECK(sampler.activeVoiceCount() == 0);

    MidiBuffer softVelocity;
    softVelocity.insert(MidiMessage::noteOn(0, 66, 20, 0));
    (void)render(sampler, softVelocity, 1);
    CHECK(sampler.activeVoiceCount() == 0);

    MidiBuffer inRange;
    inRange.insert(MidiMessage::noteOn(0, 66, 100, 0));
    (void)render(sampler, inRange, 1);
    CHECK(sampler.activeVoiceCount() == 1);
}

TEST_CASE("velocity picks the layer; overlapping zones sum")
{
    Sampler sampler;
    sampler.prepare(48000.0, blockSize);
    makeEnvelopeTransparent(sampler);

    SamplerZone soft  = zoneOf(constantSample(0.25f));
    soft.velocityLow  = 1;
    soft.velocityHigh = 64;

    SamplerZone loud  = zoneOf(constantSample(0.5f));
    loud.velocityLow  = 65;
    loud.velocityHigh = 127;

    SamplerZone always = zoneOf(constantSample(0.125f));

    sampler.setZones({soft, loud, always});

    // Velocity 127: the loud layer plus the always layer, never the soft one.
    MidiBuffer midi;
    midi.insert(MidiMessage::noteOn(0, 60, 127, 0));

    const auto rendered = render(sampler, midi, 1);
    CHECK(sampler.activeVoiceCount() == 2);
    CHECK(rendered.left[10] == doctest::Approx(0.5 + 0.125));
}

TEST_CASE("velocity scales level")
{
    Sampler sampler;
    sampler.prepare(48000.0, blockSize);
    makeEnvelopeTransparent(sampler);
    sampler.setZones({zoneOf(constantSample(1.0f))});

    MidiBuffer midi;
    midi.insert(MidiMessage::noteOn(0, 60, 64, 0));

    const auto rendered = render(sampler, midi, 1);
    CHECK(rendered.left[10] == doctest::Approx(64.0 / 127.0));
}

TEST_CASE("reverse plays the slice backwards")
{
    Sampler sampler;
    sampler.prepare(48000.0, blockSize);
    makeEnvelopeTransparent(sampler);

    SamplerZone zone = zoneOf(rampSample(1024, 0.001f));
    zone.reverse     = true;
    sampler.setZones({zone});

    MidiBuffer midi;
    midi.insert(MidiMessage::noteOn(0, 60, 127, 0));

    const auto rendered = render(sampler, midi, 1);

    // Frame 0 is the LAST source frame, then downwards.
    for (FrameCount frame = 0; frame < blockSize; ++frame)
        REQUIRE(rendered.left[static_cast<std::size_t>(frame)]
                == doctest::Approx(static_cast<double>(1023 - frame) * 0.001).epsilon(0.001));
}

TEST_CASE("start and end bound the slice, and the voice ends with it")
{
    Sampler sampler;
    sampler.prepare(48000.0, blockSize);
    makeEnvelopeTransparent(sampler);

    SamplerZone zone = zoneOf(rampSample(4096, 0.001f));
    zone.start       = 100;
    zone.end         = 100 + 64;   // a 64-frame slice
    sampler.setZones({zone});

    MidiBuffer midi;
    midi.insert(MidiMessage::noteOn(0, 60, 127, 0));

    const auto rendered = render(sampler, midi, 1);

    CHECK(rendered.left[0] == doctest::Approx(0.100));
    CHECK(rendered.left[63] == doctest::Approx(0.163));
    CHECK(rendered.left[64] == doctest::Approx(0.0));   // past the slice: silence
    CHECK(sampler.activeVoiceCount() == 0);             // and the voice is gone
}

TEST_CASE("note off releases; allNotesOff is immediate")
{
    Sampler sampler;
    sampler.prepare(48000.0, blockSize);
    makeEnvelopeTransparent(sampler);
    sampler.setReleaseSeconds(64.0 / 48000.0);   // 64 frames of release
    sampler.setZones({zoneOf(constantSample(1.0f))});

    MidiBuffer midi;
    midi.insert(MidiMessage::noteOn(0, 60, 127, 0));
    midi.insert(MidiMessage::noteOff(0, 60, 64, blockSize / 2));

    const auto rendered = render(sampler, midi, 2);

    // Full level before the off, a ramp after it, silence once released.
    CHECK(rendered.left[blockSize / 2 - 1] == doctest::Approx(1.0));
    CHECK(rendered.left[blockSize / 2 + 32]
          == doctest::Approx(0.5).epsilon(0.05));
    CHECK(rendered.left[blockSize / 2 + 80] == doctest::Approx(0.0));
    CHECK(sampler.activeVoiceCount() == 0);

    MidiBuffer restart;
    restart.insert(MidiMessage::noteOn(0, 60, 127, 0));
    (void)render(sampler, restart, 1);
    CHECK(sampler.activeVoiceCount() == 1);

    sampler.allNotesOff();
    CHECK(sampler.activeVoiceCount() == 0);
}

TEST_CASE("a stereo sample keeps its channels; a mono one feeds both")
{
    Sampler sampler;
    sampler.prepare(48000.0, blockSize);
    makeEnvelopeTransparent(sampler);
    sampler.setZones({zoneOf(stereoSample(0.25f, 0.75f))});

    MidiBuffer midi;
    midi.insert(MidiMessage::noteOn(0, 60, 127, 0));

    const auto stereo = render(sampler, midi, 1);
    CHECK(stereo.left[10] == doctest::Approx(0.25));
    CHECK(stereo.right[10] == doctest::Approx(0.75));

    sampler.setZones({zoneOf(constantSample(0.5f))});

    MidiBuffer again;
    again.insert(MidiMessage::noteOn(0, 60, 127, 0));

    const auto mono = render(sampler, again, 1);
    CHECK(mono.left[10] == doctest::Approx(0.5));
    CHECK(mono.right[10] == doctest::Approx(0.5));
}

TEST_CASE("a sample at another rate is resampled to the engine's")
{
    Sampler sampler;
    sampler.prepare(48000.0, blockSize);
    makeEnvelopeTransparent(sampler);

    // A 24 kHz ramp at the root key must advance half a source frame per
    // output frame: the sampler resamples by rate, so pitch stays true.
    sampler.setZones({zoneOf(rampSample(4096, 0.001f, 24000.0))});

    MidiBuffer midi;
    midi.insert(MidiMessage::noteOn(0, 60, 127, 0));

    const auto rendered = render(sampler, midi, 1);

    CHECK(rendered.left[100] == doctest::Approx(50.0 * 0.001).epsilon(0.001));
}
