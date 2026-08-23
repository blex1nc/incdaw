// C10/C11 — the spectral floor: an inverse transform, an STFT that
// reconstructs exactly, and the two tools built on it.
//
// The null test below is the one that matters. A spectral pass that changes no
// bin must return the input; when it does not, the error is a faint tremolo at
// the hop rate, which sounds like "the algorithm" rather than like a bug and
// is therefore never reported. Everything else here is built on top of that
// one property holding.

#include "doctest.h"

#include "engine/dsp/Fft.h"
#include "engine/dsp/Stft.h"

#include <cmath>
#include <numeric>
#include <random>
#include <vector>

using namespace incdaw;
using namespace incdaw::engine;

namespace {

std::vector<Sample> sine(std::size_t frames, double frequency, SampleRate rate,
                         float amplitude = 0.5f)
{
    std::vector<Sample> signal(frames);
    for (std::size_t index = 0; index < frames; ++index)
        signal[index] = amplitude
                      * static_cast<float>(std::sin(2.0 * M_PI * frequency
                                                    * static_cast<double>(index) / rate));

    return signal;
}

std::vector<Sample> whiteNoise(std::size_t frames, float amplitude, unsigned seed)
{
    std::mt19937                          engine{seed};
    std::uniform_real_distribution<float> spread{-amplitude, amplitude};

    std::vector<Sample> signal(frames);
    for (Sample& value : signal)
        value = spread(engine);

    return signal;
}

double rms(const std::vector<Sample>& signal, std::size_t from = 0, std::size_t to = 0)
{
    const std::size_t last = to == 0 ? signal.size() : to;
    if (last <= from)
        return 0.0;

    double sum = 0.0;
    for (std::size_t index = from; index < last; ++index)
        sum += static_cast<double>(signal[index]) * static_cast<double>(signal[index]);

    return std::sqrt(sum / static_cast<double>(last - from));
}

} // namespace

// ── The transform ────────────────────────────────────────────────────────────

TEST_CASE("the inverse transform undoes the forward one")
{
    dsp::Fft fft;
    fft.setSize(64);

    std::vector<float> real(64), imaginary(64, 0.0f);
    for (std::size_t index = 0; index < 64; ++index)
        real[index] = static_cast<float>(std::sin(static_cast<double>(index) * 0.37))
                    + 0.25f * static_cast<float>(index % 7);

    const std::vector<float> original = real;

    fft.forward(real.data(), imaginary.data());
    fft.inverse(real.data(), imaginary.data());

    for (std::size_t index = 0; index < 64; ++index) {
        CHECK(real[index] == doctest::Approx(original[index]).epsilon(1e-5));
        CHECK(imaginary[index] == doctest::Approx(0.0).epsilon(1e-5));
    }
}

TEST_CASE("a forward transform puts a sine in the bin it belongs to")
{
    // The reference is arithmetic, not the class under test: a 64-point
    // transform of exactly four cycles has all its energy in bin 4.
    dsp::Fft fft;
    fft.setSize(64);

    std::vector<float> real(64), imaginary(64, 0.0f);
    for (std::size_t index = 0; index < 64; ++index)
        real[index] = static_cast<float>(std::cos(2.0 * M_PI * 4.0
                                                  * static_cast<double>(index) / 64.0));

    fft.forward(real.data(), imaginary.data());

    for (std::size_t bin = 0; bin <= 32; ++bin) {
        const double magnitude = std::hypot(static_cast<double>(real[bin]),
                                            static_cast<double>(imaginary[bin]));

        if (bin == 4)
            CHECK(magnitude == doctest::Approx(32.0).epsilon(0.01));
        else
            CHECK(magnitude < 0.01);
    }
}

// ── The null test ────────────────────────────────────────────────────────────

TEST_CASE("an STFT pass that changes nothing returns the signal")
{
    dsp::Stft stft;

    // Something with content across the spectrum and no periodicity that
    // could hide a hop-rate error.
    std::vector<Sample> input = sine(20000, 440.0, 48000.0, 0.4f);
    const auto          noise = whiteNoise(20000, 0.1f, 7);
    for (std::size_t index = 0; index < input.size(); ++index)
        input[index] += noise[index];

    const auto output = stft.process(input, [](std::size_t, float*, float*) {});

    REQUIRE(output.size() == input.size());

    double worst = 0.0;
    for (std::size_t index = 0; index < input.size(); ++index)
        worst = std::max(worst, std::abs(static_cast<double>(output[index] - input[index])));

    // Exact to float arithmetic. Anything larger is a windowing or
    // normalisation error, and the audible symptom is a tremolo at the hop
    // rate that nobody attributes to the transform.
    CHECK(worst < 1.0e-4);
}

TEST_CASE("the reconstruction holds at the very edges of the region")
{
    dsp::Stft stft;

    // The padding exists so the first and last samples are covered by as many
    // frames as the middle. Without it an edit applied to a selection fades in
    // and out of the audio around it.
    const auto input  = sine(6000, 220.0, 48000.0, 0.8f);
    const auto output = stft.process(input, [](std::size_t, float*, float*) {});

    REQUIRE(output.size() == input.size());

    for (std::size_t index : {std::size_t{0}, std::size_t{1}, std::size_t{5998},
                              std::size_t{5999}})
        CHECK(output[index] == doctest::Approx(input[index]).epsilon(0.001));
}

TEST_CASE("an odd FFT size is rounded down rather than trusted")
{
    // The transform requires a power of two. Accepting 1000 and producing a
    // wrong-sized result would be worse than correcting it.
    const dsp::Stft stft{1000};
    CHECK(stft.fftSize() == 512);
    CHECK(stft.hopSize() == 128);
    CHECK(stft.binCount() == 257);
}

TEST_CASE("bins map onto frequencies")
{
    const dsp::Stft stft{2048};

    CHECK(stft.frequencyOfBin(0, 48000.0) == doctest::Approx(0.0));
    CHECK(stft.frequencyOfBin(1024, 48000.0) == doctest::Approx(24000.0));   // Nyquist
    CHECK(stft.frequencyOfBin(43, 48000.0) == doctest::Approx(1007.8).epsilon(0.01));
}

TEST_CASE("analysis finds the tone it was given")
{
    const dsp::Stft stft{2048};

    const auto input = sine(30000, 1000.0, 48000.0, 0.5f);

    std::vector<float> loudest;
    std::size_t        frames = 0;

    stft.analyse(input, [&](std::size_t, const std::vector<float>& magnitudes) {
        ++frames;
        if (loudest.empty())
            loudest = magnitudes;
        else
            for (std::size_t bin = 0; bin < magnitudes.size(); ++bin)
                loudest[bin] = std::max(loudest[bin], magnitudes[bin]);
    });

    REQUIRE(frames > 0);
    REQUIRE(!loudest.empty());

    std::size_t peak = 0;
    for (std::size_t bin = 1; bin < loudest.size(); ++bin)
        if (loudest[bin] > loudest[peak])
            peak = bin;

    // 1000 Hz at 48 kHz with a 2048-point window is bin 42.67 — 42 or 43.
    CHECK(peak >= 42);
    CHECK(peak <= 43);
}

TEST_CASE("silencing every bin silences the signal")
{
    dsp::Stft stft;

    const auto input = sine(8000, 500.0, 48000.0, 0.9f);

    const auto output = stft.process(input, [&](std::size_t, float* real, float* imaginary) {
        for (std::size_t index = 0; index < stft.fftSize(); ++index) {
            real[index]      = 0.0f;
            imaginary[index] = 0.0f;
        }
    });

    CHECK(rms(output) < 1.0e-6);
}

// ── C10: denoise ─────────────────────────────────────────────────────────────

#include "engine/dsp/Denoise.h"

namespace {

/// Mono audio from a signal vector.
AudioFileData monoOf(std::vector<Sample> samples, SampleRate rate = 48000.0)
{
    AudioFileData data;
    data.sampleRate   = rate;
    data.channelCount = 1;
    data.frameCount   = static_cast<FrameCount>(samples.size());
    data.channels.push_back(std::move(samples));
    return data;
}

} // namespace

TEST_CASE("a profile needs at least a window of silence to learn from")
{
    const auto data = monoOf(whiteNoise(1000, 0.05f, 3));

    // 1000 frames is less than one 2048-point window. A profile built from a
    // fragment describes the fragment, and removes the wrong thing everywhere
    // it is applied.
    const auto profile = dsp::learnNoiseProfile(data, 0, 1000);
    CHECK(profile.isEmpty());
}

TEST_CASE("a profile carries the parameters it was learned with")
{
    const auto data    = monoOf(whiteNoise(20000, 0.05f, 5));
    const auto profile = dsp::learnNoiseProfile(data, 0, 20000);

    REQUIRE_FALSE(profile.isEmpty());
    CHECK(profile.fftSize == dsp::Stft::defaultFftSize);
    CHECK(profile.sampleRate == doctest::Approx(48000.0));
    REQUIRE(profile.channels.size() == 1);
    CHECK(profile.channels[0].size() == dsp::Stft::defaultFftSize / 2 + 1);
}

TEST_CASE("a profile from another sample rate is refused, not misapplied")
{
    auto data = monoOf(whiteNoise(20000, 0.05f, 5));

    auto profile       = dsp::learnNoiseProfile(data, 0, 20000);
    profile.sampleRate = 44100.0;

    // Its bins are other frequencies. Applying it would remove a hum that is
    // not there and leave the one that is.
    CHECK_FALSE(dsp::denoise(data, 0, 20000, profile, 1.0));
}

TEST_CASE("denoising by nothing leaves the file untouched")
{
    auto       data     = monoOf(whiteNoise(20000, 0.05f, 11));
    const auto original = data.channels[0];
    const auto profile  = dsp::learnNoiseProfile(data, 0, 20000);

    REQUIRE(dsp::denoise(data, 0, 20000, profile, 0.0));

    // Bit-identical, not "close": a user who dials the amount to zero gets
    // their file back rather than a re-rendered copy of it.
    CHECK(data.channels[0] == original);
}

TEST_CASE("learned noise is substantially removed")
{
    const auto noise = whiteNoise(60000, 0.08f, 13);
    auto       data  = monoOf(noise);

    const auto profile = dsp::learnNoiseProfile(data, 0, 30000);
    REQUIRE_FALSE(profile.isEmpty());

    const double before = rms(data.channels[0], 30000, 60000);

    REQUIRE(dsp::denoise(data, 30000, 60000, profile, 1.0));

    const double after = rms(data.channels[0], 30000, 60000);

    // Not silence — a spectral floor is deliberately left, because subtracting
    // all the way produces isolated warbling bins that sound worse than the
    // hiss. Halved is the claim, and it holds with room to spare.
    CHECK(after < before * 0.5);
    CHECK(after > 0.0);
}

TEST_CASE("the signal survives the noise being taken out of it")
{
    const auto tone  = sine(90000, 1000.0, 48000.0, 0.35f);
    const auto noise = whiteNoise(90000, 0.15f, 17);

    std::vector<Sample> mixed(tone.size());
    for (std::size_t index = 0; index < mixed.size(); ++index)
        mixed[index] = tone[index] + noise[index];

    auto data = monoOf(mixed);

    // The first half second is "silence" — noise only, which is what the user
    // selects and says: this is the room.
    auto noiseOnly = monoOf(std::vector<Sample>(noise.begin(), noise.begin() + 40000));
    const auto profile = dsp::learnNoiseProfile(noiseOnly, 0, 40000);
    REQUIRE_FALSE(profile.isEmpty());

    REQUIRE(dsp::denoise(data, 40000, 90000, profile, 1.0));

    // The tone is still there and still the right size. A denoiser that took
    // the signal with the noise would pass a "the noise went away" test and
    // fail this one, which is why both are here.
    const double toneRms  = rms(tone, 40000, 90000);
    const double afterRms = rms(data.channels[0], 40000, 90000);

    CHECK(afterRms > toneRms * 0.7);
    CHECK(afterRms < toneRms * 1.3);

    // And the mix was louder than the tone alone before the pass.
    CHECK(rms(mixed, 40000, 90000) > toneRms * 1.02);
}

TEST_CASE("denoise applies per channel rather than averaging the floors")
{
    // Two channels with different noise levels — two microphones, which is the
    // ordinary case. Averaging the floors subtracts each one's hum from the
    // other's.
    AudioFileData data;
    data.sampleRate   = 48000.0;
    data.channelCount = 2;
    data.frameCount   = 40000;
    data.channels.push_back(whiteNoise(40000, 0.02f, 21));
    data.channels.push_back(whiteNoise(40000, 0.20f, 22));

    const auto profile = dsp::learnNoiseProfile(data, 0, 20000);
    REQUIRE(profile.channels.size() == 2);

    // The loud channel's profile is much louder than the quiet one's.
    const double quiet = std::accumulate(profile.channels[0].begin(), profile.channels[0].end(), 0.0);
    const double loud  = std::accumulate(profile.channels[1].begin(), profile.channels[1].end(), 0.0);

    CHECK(loud > quiet * 5.0);

    const double quietBefore = rms(data.channels[0], 20000, 40000);
    const double loudBefore  = rms(data.channels[1], 20000, 40000);

    REQUIRE(dsp::denoise(data, 20000, 40000, profile, 1.0));

    CHECK(rms(data.channels[0], 20000, 40000) < quietBefore * 0.6);
    CHECK(rms(data.channels[1], 20000, 40000) < loudBefore * 0.6);
}

TEST_CASE("denoise leaves the audio outside the region alone")
{
    const auto noise = whiteNoise(80000, 0.08f, 29);
    auto       data  = monoOf(noise);

    const auto profile = dsp::learnNoiseProfile(data, 0, 30000);

    const std::vector<Sample> untouched(data.channels[0].begin() + 60000,
                                        data.channels[0].end());

    REQUIRE(dsp::denoise(data, 30000, 60000, profile, 1.0));

    const std::vector<Sample> after(data.channels[0].begin() + 60000, data.channels[0].end());

    // Not "nearly": a selection edit that bleeds past its edges is one the
    // user cannot reason about.
    CHECK(after == untouched);
}

TEST_CASE("an empty profile and an empty region are refused")
{
    auto data = monoOf(whiteNoise(20000, 0.05f, 31));

    const dsp::NoiseProfile empty;
    CHECK_FALSE(dsp::denoise(data, 0, 20000, empty, 1.0));

    const auto profile = dsp::learnNoiseProfile(data, 0, 20000);
    CHECK_FALSE(dsp::denoise(data, 5000, 5000, profile, 1.0));
    CHECK_FALSE(dsp::denoise(data, 9000, 1000, profile, 1.0));
}

// ── C11: the spectral view and spectral editing ──────────────────────────────

#include "engine/dsp/Spectrogram.h"

TEST_CASE("a spectrogram puts a tone in the row it belongs to")
{
    const auto data = monoOf(sine(60000, 2000.0, 48000.0, 0.6f));

    const auto picture = dsp::buildSpectrogram(data, 0, 60000);

    REQUIRE_FALSE(picture.isEmpty());
    CHECK(picture.sampleRate == doctest::Approx(48000.0));
    CHECK(picture.binCount == dsp::Stft::defaultFftSize / 2 + 1);

    const std::size_t expected = picture.binOfFrequency(2000.0);

    // The loudest bin of the middle column, which is well clear of the edges.
    const std::size_t column = picture.columns / 2;

    std::size_t peak = 0;
    for (std::size_t bin = 1; bin < picture.binCount; ++bin)
        if (picture.at(column, bin) > picture.at(column, peak))
            peak = bin;

    CHECK(peak >= expected - 1);
    CHECK(peak <= expected + 1);

    // And the picture has range: a spectrogram whose cells are all the same
    // number is a black rectangle.
    CHECK(picture.highestDb > picture.lowestDb + 40.0f);
}

TEST_CASE("a long region is capped in columns, not in coverage")
{
    const auto data = monoOf(sine(480000, 440.0, 48000.0, 0.5f));

    const auto picture = dsp::buildSpectrogram(data, 0, 480000, 64);

    // Ten seconds at a 512-frame hop is nearly a thousand frames; the picture
    // is 64 columns and still describes all ten seconds.
    CHECK(picture.columns == 64);
    CHECK(picture.frameCount == 480000);
    CHECK(picture.startFrame == 0);
}

TEST_CASE("a spectrogram of nothing is empty rather than a crash")
{
    const AudioFileData nothing;
    CHECK(dsp::buildSpectrogram(nothing, 0, 1000).isEmpty());

    const auto data = monoOf(sine(1000, 440.0, 48000.0));
    CHECK(dsp::buildSpectrogram(data, 500, 500).isEmpty());
    CHECK(dsp::buildSpectrogram(data, 0, 1000, 0).isEmpty());
}

TEST_CASE("bins and frequencies round-trip")
{
    const auto data    = monoOf(sine(20000, 440.0, 48000.0));
    const auto picture = dsp::buildSpectrogram(data, 0, 20000);

    REQUIRE_FALSE(picture.isEmpty());

    for (double hertz : {0.0, 1000.0, 5000.0, 20000.0}) {
        const std::size_t bin = picture.binOfFrequency(hertz);
        CHECK(picture.frequencyOfBin(bin) == doctest::Approx(hertz).epsilon(0.02));
    }

    // Above Nyquist clamps rather than running off the end.
    CHECK(picture.binOfFrequency(96000.0) == picture.binCount - 1);
}

TEST_CASE("erasing a band removes the tone in it and leaves the others")
{
    // Three tones, well apart. The middle one is the squeak to be removed.
    std::vector<Sample> mixed(96000, 0.0f);

    const auto lowTone    = sine(96000, 300.0, 48000.0, 0.4f);
    const auto middleTone = sine(96000, 3000.0, 48000.0, 0.4f);
    const auto highTone   = sine(96000, 9000.0, 48000.0, 0.4f);

    for (std::size_t index = 0; index < mixed.size(); ++index)
        mixed[index] = lowTone[index] + middleTone[index] + highTone[index];

    auto data = monoOf(mixed);

    REQUIRE(dsp::spectralErase(data, 0, 96000, 2500.0, 3500.0, 1.0));

    // Measured through the analysis, which is the only way to say "that
    // frequency is gone" rather than "the file got quieter".
    const dsp::Stft stft;

    std::vector<float> loudest(stft.binCount(), 0.0f);
    stft.analyse(data.channels[0], [&](std::size_t frameIndex,
                                       const std::vector<float>& magnitudes) {
        // Skip the padded ends, where the erase's own taper lives.
        if (frameIndex < 8)
            return;

        for (std::size_t bin = 0; bin < loudest.size(); ++bin)
            loudest[bin] = std::max(loudest[bin], magnitudes[bin]);
    });

    const auto binAt = [&](double hertz) {
        return static_cast<std::size_t>(hertz * static_cast<double>(stft.fftSize()) / 48000.0 + 0.5);
    };

    const float low    = loudest[binAt(300.0)];
    const float middle = loudest[binAt(3000.0)];
    const float high   = loudest[binAt(9000.0)];

    CHECK(middle < low * 0.05f);
    CHECK(middle < high * 0.05f);

    // The neighbours are untouched — an erase that took the whole spectrum
    // down would pass "the band is gone" and be useless.
    CHECK(low > 0.1f);
    CHECK(high > 0.1f);
}

TEST_CASE("erasing by nothing, or an empty band, is refused")
{
    auto data = monoOf(sine(30000, 1000.0, 48000.0, 0.5f));
    const auto original = data.channels[0];

    CHECK_FALSE(dsp::spectralErase(data, 0, 30000, 1000.0, 1000.0, 1.0));
    CHECK_FALSE(dsp::spectralErase(data, 0, 30000, 2000.0, 1000.0, 1.0));
    CHECK_FALSE(dsp::spectralErase(data, 0, 30000, 500.0, 1500.0, 0.0));
    CHECK_FALSE(dsp::spectralErase(data, 5000, 5000, 500.0, 1500.0, 1.0));

    CHECK(data.channels[0] == original);
}

TEST_CASE("a partial erase attenuates rather than removing")
{
    const auto tone = sine(60000, 1000.0, 48000.0, 0.5f);

    auto full = monoOf(tone);
    auto half = monoOf(tone);

    REQUIRE(dsp::spectralErase(full, 0, 60000, 800.0, 1200.0, 1.0));
    REQUIRE(dsp::spectralErase(half, 0, 60000, 800.0, 1200.0, 0.5));

    const double fullRms = rms(full.channels[0], 10000, 50000);
    const double halfRms = rms(half.channels[0], 10000, 50000);
    const double toneRms = rms(tone, 10000, 50000);

    CHECK(fullRms < halfRms);
    CHECK(halfRms < toneRms);
    CHECK(halfRms == doctest::Approx(toneRms * 0.5).epsilon(0.15));
}

TEST_CASE("a spectral erase leaves the audio outside the region alone")
{
    const auto tone = sine(120000, 1000.0, 48000.0, 0.5f);
    auto       data = monoOf(tone);

    const std::vector<Sample> before(data.channels[0].begin() + 90000, data.channels[0].end());

    REQUIRE(dsp::spectralErase(data, 30000, 60000, 800.0, 1200.0, 1.0));

    const std::vector<Sample> after(data.channels[0].begin() + 90000, data.channels[0].end());
    CHECK(after == before);
}
