#include "engine/instrument/PianoInstrument.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace incdaw::engine {
namespace {

constexpr double twoPi  = 2.0 * std::numbers::pi;

/// ln(1000): decay coefficients are expressed as "−60 dB in T seconds", which
/// is how instrument decay times are quoted everywhere else.
constexpr double ln1000 = 6.907755278982137;

/// Middle C, the reference the register-dependent laws are written around.
constexpr double referenceHz = 261.6255653005986;

double frequencyForKey(double key) noexcept
{
    return 440.0 * std::pow(2.0, (key - 69.0) / 12.0);
}

/// Sine by table, for the FM voice only.
///
/// The string model never calls this: its partials advance by rotation, which
/// is exact and cheaper. FM needs a real phase lookup because the carrier's
/// phase is modulated, and a table keeps that off the transcendental path in
/// the render loop.
constexpr std::size_t sineTableSize = 4096;

std::array<double, sineTableSize + 1> makeSineTable() noexcept
{
    std::array<double, sineTableSize + 1> table{};
    for (std::size_t index = 0; index <= sineTableSize; ++index)
        table[index] = std::sin(twoPi * static_cast<double>(index)
                                / static_cast<double>(sineTableSize));

    return table;
}

/// Built at static initialisation, before any audio callback can run: the
/// render path must not be the first caller of anything that initialises.
const std::array<double, sineTableSize + 1> sineTable = makeSineTable();

/// `phase` is in cycles and may be anywhere on the real line — FM adds an
/// unbounded modulation term to it.
double tableSine(double phase) noexcept
{
    const double wrapped  = phase - std::floor(phase);
    const double position = wrapped * static_cast<double>(sineTableSize);
    const auto   index    = static_cast<std::size_t>(position);
    const double fraction = position - static_cast<double>(index);

    return sineTable[index] + (sineTable[index + 1] - sineTable[index]) * fraction;
}

/// What makes one piano a different instrument from another, rather than the
/// same instrument with the tone control moved.
struct PianoModelSpec {
    double slope;         ///< spectral roll-off exponent: a_n ∝ n^-slope
    double inharmonicity; ///< B at middle C; the stiffness of the string
    double strikePoint;   ///< hammer position along the string, in string lengths
    double decayScale;    ///< multiplier on the natural decay time
    double hfDecay;       ///< how much faster each higher partial dies
    double tailMix;       ///< share of the partial that lives in the long tail
    double tailRatio;     ///< how much longer that tail is
    double hammerLevel;   ///< the thump, before the user's own control

    bool   fm;            ///< the electric: an FM tine instead of a string
    double fmRatio;       ///< modulator : carrier
    double fmIndex;       ///< modulation depth at the strike
    double fmIndexTime;   ///< seconds for that depth to fall 60 dB
};

const PianoModelSpec& specFor(PianoModel model) noexcept
{
    static const std::array<PianoModelSpec, static_cast<std::size_t>(pianoModelCount)> specs = {{
        // grand: long tail, modest stiffness, hammer at 1/8 of the string
        {1.10, 0.00013, 0.125, 1.00, 0.55, 0.22, 3.2, 0.50, false, 0.0, 0.0, 0.0},
        // bright grand: harder hammer, slower high-partial decay
        {0.85, 0.00016, 0.115, 0.95, 0.42, 0.20, 3.0, 0.75, false, 0.0, 0.0, 0.0},
        // upright: short strings, so much more inharmonic and much drier
        {1.25, 0.00042, 0.145, 0.72, 0.75, 0.18, 2.6, 0.85, false, 0.0, 0.0, 0.0},
        // mellow: felt-softened, dark, long
        {1.60, 0.00011, 0.135, 1.10, 0.85, 0.28, 3.6, 0.30, false, 0.0, 0.0, 0.0},
        // electric: no string at all — a tine, struck, with the bark in front
        {1.00, 0.00000, 0.125, 1.40, 0.00, 0.00, 0.0, 0.55, true,  2.0, 4.0, 0.12},
    }};

    const int raw = static_cast<int>(model);
    const auto index = raw >= 0 && raw < pianoModelCount ? static_cast<std::size_t>(raw)
                                                         : std::size_t{0};

    return specs[index];
}

/// Equal-power pan, −1 left … +1 right.
void panGains(double pan, double& left, double& right) noexcept
{
    const double clamped = std::clamp(pan, -1.0, 1.0);
    const double angle   = (clamped + 1.0) * 0.25 * std::numbers::pi;

    left  = std::cos(angle);
    right = std::sin(angle);
}

/// A per-sample multiplier that falls 60 dB in `seconds`.
double decayCoefficient(double seconds, SampleRate sampleRate) noexcept
{
    const double frames = std::max(1.0, seconds * sampleRate);
    return std::exp(-ln1000 / frames);
}

constexpr std::size_t slot(int index) noexcept { return static_cast<std::size_t>(index); }

} // namespace

const char* pianoModelName(int model) noexcept
{
    switch (model) {
        case 1:  return "Bright Grand";
        case 2:  return "Upright";
        case 3:  return "Mellow";
        case 4:  return "Electric";
        default: return "Grand";
    }
}

// ── Lifecycle ─────────────────────────────────────────────────────────────────

void PianoInstrument::prepare(SampleRate sampleRate, FrameCount maxBlockSize)
{
    (void)maxBlockSize;   // every buffer is a fixed member; nothing scales with the block
    sampleRate_ = sampleRate > 0.0 ? sampleRate : 48000.0;
    allNotesOff();
}

void PianoInstrument::allNotesOff() noexcept
{
    for (Voice& voice : voices_) {
        voice.active = false;
        voice.held   = false;
        voice.key    = -1;
        voice.damper = 0.0;
        voice.partialCount = 0;
        voice.noiseLevel   = 0.0;
        voice.fmLevel      = 0.0;
    }

    pedalDown_ = false;
}

int PianoInstrument::activeVoiceCount() const noexcept
{
    int count = 0;
    for (const Voice& voice : voices_)
        if (voice.active)
            ++count;

    return count;
}

// ── Parameters ────────────────────────────────────────────────────────────────

void PianoInstrument::setParameter(std::uint32_t parameterId, double plainValue) noexcept
{
    switch (static_cast<PianoParam>(parameterId)) {
        case PianoParam::model: {
            const auto index = static_cast<int>(std::lround(plainValue));
            setModel(static_cast<PianoModel>(std::clamp(index, 0, pianoModelCount - 1)));
            break;
        }
        case PianoParam::tone:         setTone(std::clamp(plainValue, -1.0, 1.0)); break;
        case PianoParam::hardness:     setHardness(std::clamp(plainValue, 0.0, 1.0)); break;
        case PianoParam::decay:        setDecay(std::clamp(plainValue, 0.1, 4.0)); break;
        case PianoParam::release:      setRelease(std::clamp(plainValue, 0.1, 4.0)); break;
        case PianoParam::stretchCents: setStretchCents(std::clamp(plainValue, 0.0, 50.0)); break;
        case PianoParam::hammerNoise:  setHammerNoise(std::clamp(plainValue, 0.0, 1.0)); break;
        case PianoParam::pedalTail:    setPedalTail(std::clamp(plainValue, 0.0, 1.0)); break;
        case PianoParam::stereoSpread: setStereoSpread(std::clamp(plainValue, 0.0, 1.0)); break;
        case PianoParam::gain:         setGain(std::clamp(plainValue, 0.0, 1.0)); break;
    }
}

// ── Voice allocation ──────────────────────────────────────────────────────────

double PianoInstrument::voicePeak(const Voice& voice) const noexcept
{
    double sum = voice.noiseLevel + voice.fmLevel;

    for (int index = 0; index < voice.partialCount; ++index) {
        const Partial& partial = voice.partials[slot(index)];
        sum += partial.fastLevel + partial.slowLevel;
    }

    return sum * voice.damper;
}

int PianoInstrument::findVoiceToSteal() const noexcept
{
    for (int index = 0; index < maxVoices; ++index)
        if (!voices_[slot(index)].active)
            return index;

    // Everything is sounding. Take the quietest voice that is no longer held:
    // a note whose key is up and whose damper is falling is the one the player
    // has already let go of.
    int    best  = -1;
    double quiet = 1e30;

    for (int index = 0; index < maxVoices; ++index) {
        const Voice& voice = voices_[slot(index)];
        if (voice.held)
            continue;

        const double peak = voicePeak(voice);
        if (peak < quiet) {
            quiet = peak;
            best  = index;
        }
    }

    if (best >= 0)
        return best;

    // Every voice is held. The oldest is the one furthest into its decay.
    std::uint64_t oldest = ~0ull;
    best = 0;

    for (int index = 0; index < maxVoices; ++index) {
        if (voices_[slot(index)].startedAt < oldest) {
            oldest = voices_[slot(index)].startedAt;
            best   = index;
        }
    }

    return best;
}

void PianoInstrument::startVoice(int index, int channel, int key, int velocity) noexcept
{
    if (index < 0 || index >= maxVoices)
        return;

    Voice& voice = voices_[slot(index)];

    const PianoModel  model = model_.load(std::memory_order_relaxed);
    const PianoModelSpec& spec = specFor(model);

    const double tone       = tone_.load(std::memory_order_relaxed);
    const double hardness   = hardness_.load(std::memory_order_relaxed);
    const double decayScale = decay_.load(std::memory_order_relaxed);
    const double stretch    = stretch_.load(std::memory_order_relaxed);
    const double hammer     = hammer_.load(std::memory_order_relaxed);
    const double pedalTail  = pedalTail_.load(std::memory_order_relaxed);
    const double spread     = spread_.load(std::memory_order_relaxed);

    // Stretch tuning: the bass is tuned flat and the treble sharp, because a
    // stiff string's own partials are stretched and a piano tuned to equal
    // temperament by beat rates ends up this way.
    const double cents = stretch * (static_cast<double>(key) - 60.0) / 36.0;
    const double f0    = frequencyForKey(static_cast<double>(key)) * std::pow(2.0, cents / 1200.0);

    const double v = std::clamp(static_cast<double>(velocity) / 127.0, 0.0, 1.0);

    voice.active   = true;
    voice.held     = true;
    voice.key      = key;
    voice.channel  = channel;
    voice.velocity = v;
    voice.damper   = 1.0;
    voice.damperCoef = 1.0;
    voice.startedAt  = counter_++;
    voice.fm         = spec.fm;
    voice.partialCount = 0;

    panGains(spread * std::clamp((static_cast<double>(key) - 60.0) / 36.0, -1.0, 1.0),
             voice.leftGain, voice.rightGain);

    // The hammer thump. Its own seed per strike keeps the noise deterministic
    // for a given event stream, which is what makes an offline render identical
    // to the realtime one (docs/TESTING.md §7).
    voice.noiseSeed  = static_cast<std::uint32_t>(key & 0x7F) * 2654435761u
                     + static_cast<std::uint32_t>(voice.startedAt & 0xFFFFu) * 40503u + 1u;
    voice.noiseState = 0.0;
    voice.noiseLevel = hammer * spec.hammerLevel * (0.15 + 0.85 * v * v);
    voice.noiseCoef  = decayCoefficient(0.010 + 0.020 * (referenceHz / std::max(20.0, f0)),
                                        sampleRate_);

    const double loudness = std::pow(v, 1.3);

    if (spec.fm) {
        // The electric: one carrier, one modulator, and a modulation index
        // that collapses. That collapse is the bark at the front of the note;
        // what is left behind is close to a sine, which is what a tine is.
        voice.fmCarrierPhase = 0.0;
        voice.fmModPhase     = 0.0;
        voice.fmCarrierStep  = f0 / sampleRate_;
        voice.fmModStep      = f0 * spec.fmRatio / sampleRate_;
        voice.fmIndex        = spec.fmIndex * (0.35 + 0.65 * v) * (1.0 + tone * 0.5)
                             * (1.0 + hardness * (v - 0.5));
        voice.fmIndexCoef    = decayCoefficient(spec.fmIndexTime, sampleRate_);
        voice.fmLevel        = loudness * 0.9;

        const double tineDecay = std::clamp(spec.decayScale * decayScale
                                                * std::pow(referenceHz / f0, 0.45),
                                            0.15, 20.0);
        voice.fmCoef = decayCoefficient(tineDecay, sampleRate_);
        voice.partialCount = 0;
        return;
    }

    voice.fmLevel = 0.0;
    voice.fmIndex = 0.0;

    // A stiff string's partials are stretched, and short strings are stiffer:
    // the treble is markedly more inharmonic than the bass, which is why a
    // piano's top octave sounds the way it does.
    const double stiffness = spec.inharmonicity
                           * std::pow(2.0, (static_cast<double>(key) - 60.0) / 16.0);

    // Velocity opens the spectrum: a harder blow excites more of it. `hardness`
    // is how strongly this instrument does that.
    const double slope = std::clamp(spec.slope - tone * 0.8 - hardness * (v - 0.55) * 1.4,
                                    0.55, 4.0);

    const double nyquistLimit = sampleRate_ * 0.47;

    // The natural decay: bass strings ring for tens of seconds, the top octave
    // for barely one. The exponent is what puts that curve between them.
    const double baseDecay = std::clamp(8.0 * spec.decayScale * decayScale
                                            * std::pow(referenceHz / f0, 0.55),
                                        0.12, 40.0);

    // With the dampers lifted the strings are coupled and the tail is longer.
    // This is decided at the strike, not continuously: a true sympathetic
    // model is not what this is, and the header says so.
    const double tailMix = std::clamp(spec.tailMix + (pedalDown_ ? pedalTail * 0.35 : 0.0),
                                      0.0, 0.85);

    double amplitudeSum = 0.0;
    int    count        = 0;

    for (int n = 1; n <= maxPartials; ++n) {
        const double harmonic  = static_cast<double>(n);
        const double frequency = harmonic * f0 * std::sqrt(1.0 + stiffness * harmonic * harmonic);

        if (frequency >= nyquistLimit)
            break;

        // The hammer cannot excite a partial that has a node where it strikes.
        // Those missing partials are a piano's spectral fingerprint.
        const double comb      = std::abs(std::sin(harmonic * std::numbers::pi * spec.strikePoint));
        const double amplitude = std::pow(harmonic, -slope) * comb;

        if (amplitude < 1.0e-5)
            continue;

        Partial& partial = voice.partials[slot(count)];

        const double omega = twoPi * frequency / sampleRate_;
        partial.stepCos = std::cos(omega);
        partial.stepSin = std::sin(omega);
        partial.cosine  = 1.0;
        partial.sine    = 0.0;

        const double partialDecay = baseDecay / (1.0 + spec.hfDecay * (harmonic - 1.0));

        partial.fastLevel = amplitude * (1.0 - tailMix);
        partial.slowLevel = amplitude * tailMix;
        partial.fastCoef  = decayCoefficient(partialDecay, sampleRate_);
        partial.slowCoef  = decayCoefficient(partialDecay * std::max(1.0, spec.tailRatio),
                                             sampleRate_);

        amplitudeSum += amplitude;
        ++count;
    }

    voice.partialCount = count;

    // Normalise by the SUM of the partial amplitudes, not by their energy.
    // Every partial starts in phase — that alignment is the hammer strike —
    // so at frame zero they add arithmetically, and normalising by energy
    // would let a bright note leave the instrument at roughly three times
    // full scale before the mixer ever saw it. This bounds the strike at
    // `loudness` and lets the tone come back up as the partials decohere.
    const double normalise = amplitudeSum > 0.0 ? loudness / amplitudeSum : 0.0;

    for (int written = 0; written < count; ++written) {
        Partial& partial = voice.partials[slot(written)];
        partial.fastLevel *= normalise;
        partial.slowLevel *= normalise;
    }
}

void PianoInstrument::damp(Voice& voice, double seconds) noexcept
{
    voice.damperCoef = decayCoefficient(std::max(0.005, seconds), sampleRate_);
}

void PianoInstrument::releaseKey(int channel, int key) noexcept
{
    const double release = release_.load(std::memory_order_relaxed);

    for (Voice& voice : voices_) {
        if (!voice.active || voice.key != key || voice.channel != channel)
            continue;

        voice.held = false;

        // The pedal holds the damper off the string; the key being up changes
        // nothing until it is lifted.
        if (pedalDown_)
            continue;

        const double f0 = frequencyForKey(static_cast<double>(key));

        // A bass damper takes far longer to silence its string than a treble
        // one — the mass it has to stop is not the same.
        const double time = release
                          * std::clamp(0.05 + 0.45 * std::sqrt(referenceHz / std::max(20.0, f0)),
                                       0.05, 1.4);
        damp(voice, time);
    }
}

// ── MIDI ──────────────────────────────────────────────────────────────────────

void PianoInstrument::handleMessage(const MidiMessage& message) noexcept
{
    if (message.isNoteOn()) {
        // A re-struck key is one string, not two: the old voice is damped hard
        // rather than left to sum with the new one at double the level.
        for (Voice& voice : voices_) {
            if (voice.active && voice.key == message.noteNumber()
                && voice.channel == message.channel()) {
                voice.held = false;
                damp(voice, 0.02);
            }
        }

        startVoice(findVoiceToSteal(), message.channel(), message.noteNumber(),
                   message.velocity());
        return;
    }

    if (message.isNoteOff()) {
        releaseKey(message.channel(), message.noteNumber());
        return;
    }

    if (!message.isControlChange())
        return;

    // CC 64, the sustain pedal. Half-pedalling is not modelled: the threshold
    // is the standard 64, and the pedal is down or it is not.
    if (message.data1 == 64) {
        const bool down = message.data2 >= 64;
        if (down == pedalDown_)
            return;

        pedalDown_ = down;

        if (!pedalDown_) {
            // The dampers come back down onto every string whose key is up.
            const double release = release_.load(std::memory_order_relaxed);

            for (Voice& voice : voices_) {
                if (!voice.active || voice.held)
                    continue;

                const double f0 = frequencyForKey(static_cast<double>(voice.key));
                damp(voice, release
                                * std::clamp(0.05 + 0.45 * std::sqrt(referenceHz
                                                                     / std::max(20.0, f0)),
                                             0.05, 1.4));
            }
        }
        return;
    }

    // 120 All Sound Off, 123 All Notes Off. A controller that sends either
    // expects silence, and ignoring them is the classic stuck-note bug.
    if (message.data1 == 123 || message.data1 == 120)
        allNotesOff();
}

// ── Render ────────────────────────────────────────────────────────────────────

void PianoInstrument::renderRange(const AudioBufferView& output, FrameCount frameCount) noexcept
{
    if (frameCount <= 0 || output.channelCount() == 0)
        return;

    const double gain     = gain_.load(std::memory_order_relaxed);
    const bool   isStereo = output.channelCount() >= 2;

    Sample* left  = output.channel(0);
    Sample* right = isStereo ? output.channel(1) : nullptr;

    for (Voice& voice : voices_) {
        if (!voice.active)
            continue;

        const double leftGain  = isStereo ? voice.leftGain * gain : gain;
        const double rightGain = voice.rightGain * gain;

        for (FrameCount frame = 0; frame < frameCount; ++frame) {
            double sample = 0.0;

            if (voice.fm) {
                const double modulation = tableSine(voice.fmModPhase) * voice.fmIndex;
                sample += tableSine(voice.fmCarrierPhase + modulation) * voice.fmLevel;

                voice.fmCarrierPhase += voice.fmCarrierStep;
                voice.fmModPhase     += voice.fmModStep;
                voice.fmIndex        *= voice.fmIndexCoef;
                voice.fmLevel        *= voice.fmCoef;
            }

            for (int index = 0; index < voice.partialCount; ++index) {
                Partial& partial = voice.partials[slot(index)];

                // One rotation of a unit vector: exact in frequency, no
                // transcendental per sample, and no phase to wrap.
                const double nextCosine = partial.cosine * partial.stepCos
                                        - partial.sine * partial.stepSin;
                partial.sine   = partial.cosine * partial.stepSin
                               + partial.sine * partial.stepCos;
                partial.cosine = nextCosine;

                sample += partial.sine * (partial.fastLevel + partial.slowLevel);

                partial.fastLevel *= partial.fastCoef;
                partial.slowLevel *= partial.slowCoef;
            }

            if (voice.noiseLevel > 0.0) {
                voice.noiseSeed = voice.noiseSeed * 1664525u + 1013904223u;
                const double white =
                    static_cast<double>(voice.noiseSeed >> 9) * (1.0 / 4194304.0) - 1.0;

                // One pole of lowpass: a hammer thump is broadband but far from
                // white, and an unfiltered burst reads as a click.
                voice.noiseState += (white - voice.noiseState) * 0.35;
                sample += voice.noiseState * voice.noiseLevel;
                voice.noiseLevel *= voice.noiseCoef;
            }

            sample *= voice.damper;
            voice.damper *= voice.damperCoef;

            left[frame] += static_cast<Sample>(sample * leftGain);
            if (right != nullptr)
                right[frame] += static_cast<Sample>(sample * rightGain);
        }

        // Retire the voice once it is inaudible. The threshold is far below
        // 24-bit resolution, so nothing is cut off that could be heard — and
        // without it the levels would decay into denormals and stay there,
        // costing far more than the voice is worth.
        if (voicePeak(voice) < 1.0e-7) {
            voice.active       = false;
            voice.held         = false;
            voice.key          = -1;
            voice.partialCount = 0;
            voice.noiseLevel   = 0.0;
            voice.fmLevel      = 0.0;
        }
    }

    // A mono destination was written at unity above rather than panned, so
    // there is nothing to fold down. Channels beyond the second mirror the
    // left, the way every other instrument here handles an unusual layout.
    if (!isStereo)
        return;

    for (std::size_t channel = 2; channel < output.channelCount(); ++channel) {
        Sample* destination = output.channel(channel);
        for (FrameCount frame = 0; frame < frameCount; ++frame)
            destination[frame] += left[frame];
    }
}

} // namespace incdaw::engine
