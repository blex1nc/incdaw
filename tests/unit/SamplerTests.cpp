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

TEST_CASE("a sustain loop cycles forever and survives release")
{
    Sampler sampler;
    sampler.prepare(48000.0, blockSize);
    makeEnvelopeTransparent(sampler);

    SamplerZone zone = zoneOf(rampSample(4096, 0.001f));
    zone.loopStart   = 100;
    zone.loopEnd     = 200;
    sampler.setZones({zone});

    MidiBuffer midi;
    midi.insert(MidiMessage::noteOn(0, 60, 127, 0));

    // 8 blocks = 2048 frames, far past the loop end at 200: the voice must
    // still be sounding, and the values must cycle 100..199.
    const auto rendered = render(sampler, midi, 8);
    CHECK(sampler.activeVoiceCount() == 1);

    // Frame 200 wrapped to source frame 100; frame 250 reads source 150.
    CHECK(rendered.left[200] == doctest::Approx(0.100));
    CHECK(rendered.left[250] == doctest::Approx(0.150));
    CHECK(rendered.left[2000] == doctest::Approx(
        static_cast<double>(100 + (2000 - 100) % 100) * 0.001));

    // Release ends the voice through the envelope, not the loop.
    sampler.setReleaseSeconds(32.0 / 48000.0);

    MidiBuffer off;
    off.insert(MidiMessage::noteOff(0, 60, 64, 0));
    (void)render(sampler, off, 1);
    CHECK(sampler.activeVoiceCount() == 0);
}

TEST_CASE("the loop crossfade blends the seam toward the pre-loop material")
{
    Sampler sampler;
    sampler.prepare(48000.0, blockSize);
    makeEnvelopeTransparent(sampler);

    SamplerZone zone   = zoneOf(rampSample(4096, 0.001f));
    zone.loopStart     = 100;
    zone.loopEnd       = 200;
    zone.loopCrossfade = 40;
    sampler.setZones({zone});

    MidiBuffer midi;
    midi.insert(MidiMessage::noteOn(0, 60, 127, 0));

    const auto rendered = render(sampler, midi, 2);

    // Before the crossfade region: verbatim.
    CHECK(rendered.left[150] == doctest::Approx(0.150));

    // Halfway through the fade at source frame 180: half of 180, half of 80.
    CHECK(rendered.left[180] == doctest::Approx(0.180 * 0.5 + 0.080 * 0.5).epsilon(0.02));

    // After the wrap the values continue from the loop start region, and the
    // seam is continuous: frame 199 blends almost entirely into 99.
    CHECK(rendered.left[199] == doctest::Approx(0.099).epsilon(0.02));
    CHECK(rendered.left[200] == doctest::Approx(0.100));
}

TEST_CASE("a loop that does not fit its slice does not play as one")
{
    Sampler sampler;
    sampler.prepare(48000.0, blockSize);
    makeEnvelopeTransparent(sampler);

    SamplerZone zone = zoneOf(rampSample(1024, 0.001f));
    zone.end         = 200;
    zone.loopStart   = 150;
    zone.loopEnd     = 400;   // beyond the slice: not what the user drew
    sampler.setZones({zone});

    MidiBuffer midi;
    midi.insert(MidiMessage::noteOn(0, 60, 127, 0));

    const auto rendered = render(sampler, midi, 1);

    // No loop: the voice ends where the slice does.
    CHECK(rendered.left[199] == doctest::Approx(0.199));
    CHECK(rendered.left[200] == doctest::Approx(0.0));
    CHECK(sampler.activeVoiceCount() == 0);
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

// ── Filter and LFO (Phase 14 part 5) ─────────────────────────────────────────

namespace {

/// A mono sample alternating +v/−v every frame: a tone at Nyquist, the
/// highest content a lowpass can be asked to remove.
std::shared_ptr<const AudioFileData> nyquistSample(Sample value, FrameCount frames = 8192)
{
    auto data          = std::make_shared<AudioFileData>();
    data->sampleRate   = 48000.0;
    data->channelCount = 1;
    data->frameCount   = frames;
    data->channels.resize(1);
    data->channels[0].resize(static_cast<std::size_t>(frames));

    for (FrameCount frame = 0; frame < frames; ++frame)
        data->channels[0][static_cast<std::size_t>(frame)] = (frame % 2 == 0) ? value : -value;

    return data;
}

double rmsOf(const std::vector<Sample>& samples, std::size_t from = 0)
{
    double sum = 0.0;
    for (std::size_t index = from; index < samples.size(); ++index)
        sum += static_cast<double>(samples[index]) * static_cast<double>(samples[index]);

    return std::sqrt(sum / static_cast<double>(samples.size() - from));
}

} // namespace

TEST_CASE("the lowpass removes what the source has at the top")
{
    Sampler sampler;
    sampler.prepare(48000.0, blockSize);
    makeEnvelopeTransparent(sampler);
    sampler.setZones({zoneOf(nyquistSample(0.5f))});

    MidiBuffer midi;
    midi.insert(MidiMessage::noteOn(0, 60, 127, 0));

    // Baseline: filter off, the Nyquist tone arrives at full strength.
    const auto open = render(sampler, midi, 4);
    const double openRms = rmsOf(open.left, blockSize);   // skip the attack
    REQUIRE(openRms > 0.3);

    sampler.allNotesOff();
    sampler.setFilterMode(Sampler::FilterMode::lowpass);
    sampler.setFilterCutoffHz(200.0);

    const auto closed = render(sampler, midi, 4);
    const double closedRms = rmsOf(closed.left, blockSize);

    // A 200 Hz lowpass against a 24 kHz tone: the energy must collapse.
    CHECK(closedRms < openRms * 0.05);

    for (const Sample value : closed.left)
        REQUIRE(std::isfinite(value));
}

TEST_CASE("the highpass removes DC and keeps the top")
{
    Sampler sampler;
    sampler.prepare(48000.0, blockSize);
    makeEnvelopeTransparent(sampler);

    SUBCASE("DC through a highpass collapses")
    {
        sampler.setZones({zoneOf(constantSample(0.5f, 8192))});
        sampler.setFilterMode(Sampler::FilterMode::highpass);
        sampler.setFilterCutoffHz(2000.0);

        MidiBuffer midi;
        midi.insert(MidiMessage::noteOn(0, 60, 127, 0));

        const auto rendered = render(sampler, midi, 4);

        // After the filter settles, the constant is gone.
        CHECK(rmsOf(rendered.left, blockSize * 2) < 0.01);
    }

    SUBCASE("the Nyquist tone passes a low highpass nearly unchanged")
    {
        sampler.setZones({zoneOf(nyquistSample(0.5f))});
        sampler.setFilterMode(Sampler::FilterMode::highpass);
        sampler.setFilterCutoffHz(30.0);

        MidiBuffer midi;
        midi.insert(MidiMessage::noteOn(0, 60, 127, 0));

        const auto rendered = render(sampler, midi, 4);
        CHECK(rmsOf(rendered.left, blockSize) > 0.3);
    }
}

TEST_CASE("LFO depth zero is bit-identical to no LFO at all")
{
    MidiBuffer midi;
    midi.insert(MidiMessage::noteOn(0, 60, 127, 0));

    const auto renderWith = [&](double pitchDepth) {
        Sampler sampler;
        sampler.prepare(48000.0, blockSize);
        makeEnvelopeTransparent(sampler);
        sampler.setZones({zoneOf(rampSample(65536, 0.0001f))});
        sampler.setLfoRateHz(6.0);
        sampler.setLfoToPitchSemitones(pitchDepth);
        return render(sampler, midi, 8);
    };

    const auto without = renderWith(0.0);
    const auto with    = renderWith(1.0);
    const auto again   = renderWith(0.0);

    REQUIRE(without.left.size() == again.left.size());
    for (std::size_t index = 0; index < without.left.size(); ++index)
        REQUIRE(without.left[index] == again.left[index]);

    // With depth, the read position wanders: the output must differ.
    bool differs = false;
    for (std::size_t index = 0; index < with.left.size(); ++index)
        differs = differs || with.left[index] != without.left[index];

    CHECK(differs);

    for (const Sample value : with.left)
        REQUIRE(std::isfinite(value));
}

TEST_CASE("extreme resonance stays finite")
{
    Sampler sampler;
    sampler.prepare(48000.0, blockSize);
    makeEnvelopeTransparent(sampler);
    sampler.setZones({zoneOf(nyquistSample(0.5f))});
    sampler.setFilterMode(Sampler::FilterMode::bandpass);
    sampler.setFilterCutoffHz(1000.0);
    sampler.setFilterResonance(50.0);
    sampler.setLfoRateHz(20.0);
    sampler.setLfoToCutoffOctaves(4.0);

    MidiBuffer midi;
    midi.insert(MidiMessage::noteOn(0, 60, 127, 0));

    const auto rendered = render(sampler, midi, 8);

    for (const Sample value : rendered.left)
        REQUIRE(std::isfinite(value));
}
