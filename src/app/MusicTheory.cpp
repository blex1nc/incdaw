#include "app/MusicTheory.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>

namespace incdaw::app::music {
namespace {

constexpr int pitchClassCount = 12;
constexpr int minimumKey      = 0;
constexpr int maximumKey      = 127;

[[nodiscard]] int wrappedPitchClass(int value) noexcept
{
    return ((value % pitchClassCount) + pitchClassCount) % pitchClassCount;
}

/// Sorted unique pitch classes of a key set.
[[nodiscard]] std::vector<int> pitchClassSet(const std::vector<int>& keys)
{
    std::vector<int> classes;
    classes.reserve(keys.size());
    for (const int key : keys)
        classes.push_back(wrappedPitchClass(key));

    std::sort(classes.begin(), classes.end());
    classes.erase(std::unique(classes.begin(), classes.end()), classes.end());
    return classes;
}

/// Total movement between two sorted voicings. Equal sizes pair off in order;
/// unequal sizes charge every note its distance to the nearest counterpart in
/// both directions, so a dropped or doubled note still costs what it moved.
[[nodiscard]] int movementCost(const std::vector<int>& from, const std::vector<int>& to)
{
    if (from.empty() || to.empty())
        return std::numeric_limits<int>::max();

    if (from.size() == to.size()) {
        int cost = 0;
        for (std::size_t index = 0; index < from.size(); ++index)
            cost += std::abs(from[index] - to[index]);
        return cost;
    }

    const auto nearest = [](int key, const std::vector<int>& candidates) {
        int best = std::numeric_limits<int>::max();
        for (const int candidate : candidates)
            best = std::min(best, std::abs(key - candidate));
        return best;
    };

    int cost = 0;
    for (const int key : from)
        cost += nearest(key, to);
    for (const int key : to)
        cost += nearest(key, from);
    return cost;
}

} // namespace

// ── Note names ────────────────────────────────────────────────────────────────

const char* pitchClassName(int pitchClass) noexcept
{
    static constexpr std::array<const char*, 12> names = {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B",
    };
    return names[static_cast<std::size_t>(wrappedPitchClass(pitchClass))];
}

std::string noteName(int key)
{
    // Middle C (60) is C4, so octave = key/12 - 1.
    const int octave = key / pitchClassCount - 1;
    return std::string(pitchClassName(key)) + std::to_string(octave);
}

const char* intervalShortName(int semitones) noexcept
{
    static constexpr std::array<const char*, 13> names = {
        "P1", "m2", "M2", "m3", "M3", "P4", "TT", "P5", "m6", "M6", "m7", "M7", "P8",
    };

    semitones = std::abs(semitones);
    if (semitones == 0)
        return names[0];

    const int reduced = wrappedPitchClass(semitones);
    return reduced == 0 ? names[12] : names[static_cast<std::size_t>(reduced)];
}

// ── Chord dictionary ──────────────────────────────────────────────────────────

std::vector<int> ChordType::pitchClasses() const
{
    std::vector<int> classes;
    classes.reserve(size);
    for (std::size_t index = 0; index < size; ++index)
        classes.push_back(wrappedPitchClass(intervals[index]));
    return classes;
}

const std::vector<ChordType>& chordDictionary()
{
    // Simplest first: on a pitch-class tie (Cm6 and Am7b5 are the same set)
    // the earlier entry names the chord.
    static const std::vector<ChordType> dictionary = {
        { "",      { 0, 4, 7, 0, 0 },     3 },
        { "m",     { 0, 3, 7, 0, 0 },     3 },
        { "dim",   { 0, 3, 6, 0, 0 },     3 },
        { "aug",   { 0, 4, 8, 0, 0 },     3 },
        { "sus2",  { 0, 2, 7, 0, 0 },     3 },
        { "sus4",  { 0, 5, 7, 0, 0 },     3 },
        { "5",     { 0, 7, 0, 0, 0 },     2 },
        { "6",     { 0, 4, 7, 9, 0 },     4 },
        { "m6",    { 0, 3, 7, 9, 0 },     4 },
        { "7",     { 0, 4, 7, 10, 0 },    4 },
        { "maj7",  { 0, 4, 7, 11, 0 },    4 },
        { "m7",    { 0, 3, 7, 10, 0 },    4 },
        { "mMaj7", { 0, 3, 7, 11, 0 },    4 },
        { "dim7",  { 0, 3, 6, 9, 0 },     4 },
        { "m7b5",  { 0, 3, 6, 10, 0 },    4 },
        { "7sus4", { 0, 5, 7, 10, 0 },    4 },
        { "add9",  { 0, 4, 7, 14, 0 },    4 },
        { "madd9", { 0, 3, 7, 14, 0 },    4 },
        { "9",     { 0, 4, 7, 10, 14 },   5 },
        { "maj9",  { 0, 4, 7, 11, 14 },   5 },
        { "m9",    { 0, 3, 7, 10, 14 },   5 },
    };
    return dictionary;
}

const ChordType* findChordType(const std::string& suffix) noexcept
{
    for (const ChordType& type : chordDictionary())
        if (suffix == type.suffix)
            return &type;
    return nullptr;
}

// ── Detection ─────────────────────────────────────────────────────────────────

ChordDetection detectChord(const std::vector<int>& keys)
{
    ChordDetection detection;
    if (keys.empty())
        return detection;

    std::vector<int> sorted = keys;
    std::sort(sorted.begin(), sorted.end());
    sorted.erase(std::unique(sorted.begin(), sorted.end()), sorted.end());

    detection.bassKey = sorted.front();

    if (sorted.size() == 1) {
        detection.display = noteName(sorted.front());
        return detection;
    }

    if (sorted.size() == 2) {
        detection.display = noteName(sorted.front()) + "–" + noteName(sorted.back()) + " ("
                          + intervalShortName(sorted.back() - sorted.front()) + ")";
        return detection;
    }

    const std::vector<int> classes = pitchClassSet(sorted);
    const int bassClass            = wrappedPitchClass(detection.bassKey);

    // Try every present pitch class as the root; the bass-rooted reading wins,
    // then dictionary order.
    const ChordType* bestType  = nullptr;
    int              bestRoot  = -1;
    int              bestRank  = std::numeric_limits<int>::max();

    for (const int root : classes) {
        std::vector<int> relative;
        relative.reserve(classes.size());
        for (const int pitchClass : classes)
            relative.push_back(wrappedPitchClass(pitchClass - root));
        std::sort(relative.begin(), relative.end());

        const std::vector<ChordType>& dictionary = chordDictionary();
        for (std::size_t index = 0; index < dictionary.size(); ++index) {
            std::vector<int> candidate = dictionary[index].pitchClasses();
            std::sort(candidate.begin(), candidate.end());
            if (candidate != relative)
                continue;

            const int rank = (root == bassClass ? 0 : 1000) + static_cast<int>(index);
            if (rank < bestRank) {
                bestRank = rank;
                bestType = &dictionary[index];
                bestRoot = root;
            }
        }
    }

    if (bestType == nullptr)
        return detection;

    detection.matched        = true;
    detection.rootPitchClass = bestRoot;
    detection.type           = bestType;
    detection.inverted       = bassClass != bestRoot;
    detection.display        = std::string(pitchClassName(bestRoot)) + bestType->suffix;
    if (detection.inverted)
        detection.display += std::string("/") + pitchClassName(bassClass);

    return detection;
}

// ── Stamping ──────────────────────────────────────────────────────────────────

std::vector<int> stampChord(int rootKey, const ChordType& type, StampVoicing voicing)
{
    std::vector<int> keys;
    keys.reserve(type.size);

    for (std::size_t index = 0; index < type.size; ++index) {
        const int interval = type.intervals[index];
        int key            = rootKey;

        if (interval > 0) {
            if (voicing == StampVoicing::bottomUp) {
                key = rootKey + interval;
            } else {
                // The root stays on top; every other tone drops below it by
                // its octave complement, so the clicked key still names the
                // chord.
                const int reduced = wrappedPitchClass(interval);
                key = reduced == 0 ? rootKey - pitchClassCount
                                   : rootKey - (pitchClassCount - reduced);
            }
        }

        if (key >= minimumKey && key <= maximumKey)
            keys.push_back(key);
    }

    std::sort(keys.begin(), keys.end());
    keys.erase(std::unique(keys.begin(), keys.end()), keys.end());
    return keys;
}

// ── Voice-leading ─────────────────────────────────────────────────────────────

std::vector<int> voiceLead(const std::vector<int>& previousKeys,
                           const std::vector<int>& pitchClasses)
{
    if (pitchClasses.empty())
        return {};

    // A compact ascending voicing rooted near middle C to invert and shift.
    std::vector<int> base;
    base.reserve(pitchClasses.size());
    int previous = 60 + wrappedPitchClass(pitchClasses.front());
    base.push_back(previous);
    for (std::size_t index = 1; index < pitchClasses.size(); ++index) {
        int key = previous + wrappedPitchClass(pitchClasses[index] - previous);
        if (key <= previous)
            key += pitchClassCount;
        base.push_back(key);
        previous = key;
    }

    if (previousKeys.empty())
        return base;

    std::vector<int> reference = previousKeys;
    std::sort(reference.begin(), reference.end());

    std::vector<int> best     = base;
    int              bestCost = std::numeric_limits<int>::max();

    std::vector<int> inversion = base;
    for (std::size_t rotation = 0; rotation < inversion.size(); ++rotation) {
        if (rotation > 0) {
            // Next inversion: the lowest note leaps up an octave.
            inversion.front() += pitchClassCount;
            std::sort(inversion.begin(), inversion.end());
        }

        for (int octave = -3; octave <= 3; ++octave) {
            std::vector<int> candidate = inversion;
            bool inRange = true;
            for (int& key : candidate) {
                key += octave * pitchClassCount;
                if (key < minimumKey || key > maximumKey)
                    inRange = false;
            }
            if (!inRange)
                continue;

            const int cost = movementCost(reference, candidate);
            if (cost < bestCost) {
                bestCost = cost;
                best     = candidate;
            }
        }
    }

    return best;
}

// ── Scales ────────────────────────────────────────────────────────────────────

const std::array<int, 7>& scaleIntervals(Scale scale) noexcept
{
    static constexpr std::array<int, 7> major         = { 0, 2, 4, 5, 7, 9, 11 };
    static constexpr std::array<int, 7> naturalMinor  = { 0, 2, 3, 5, 7, 8, 10 };
    static constexpr std::array<int, 7> harmonicMinor = { 0, 2, 3, 5, 7, 8, 11 };

    switch (scale) {
        case Scale::naturalMinor:  return naturalMinor;
        case Scale::harmonicMinor: return harmonicMinor;
        case Scale::major:         break;
    }
    return major;
}

std::vector<int> diatonicChordPitchClasses(int keyRootPitchClass, Scale scale,
                                           int degree, std::size_t noteCount)
{
    const std::array<int, 7>& steps = scaleIntervals(scale);
    noteCount                       = std::clamp<std::size_t>(noteCount, 1, 4);

    std::vector<int> classes;
    classes.reserve(noteCount);

    // Stacked thirds: every second scale degree above the starting one.
    for (std::size_t stack = 0; stack < noteCount; ++stack) {
        const std::size_t index =
            (static_cast<std::size_t>(((degree % 7) + 7) % 7) + stack * 2) % steps.size();
        classes.push_back(wrappedPitchClass(keyRootPitchClass + steps[index]));
    }
    return classes;
}

int degreeOf(int keyRootPitchClass, Scale scale, int pitchClass) noexcept
{
    const std::array<int, 7>& steps = scaleIntervals(scale);
    const int relative              = wrappedPitchClass(pitchClass - keyRootPitchClass);

    for (std::size_t index = 0; index < steps.size(); ++index)
        if (steps[index] == relative)
            return static_cast<int>(index);
    return -1;
}

int nearestDegree(int keyRootPitchClass, Scale scale, int pitchClass) noexcept
{
    const int exact = degreeOf(keyRootPitchClass, scale, pitchClass);
    if (exact >= 0)
        return exact;

    const std::array<int, 7>& steps = scaleIntervals(scale);
    const int relative              = wrappedPitchClass(pitchClass - keyRootPitchClass);

    int bestDegree   = 0;
    int bestDistance = std::numeric_limits<int>::max();

    for (std::size_t index = 0; index < steps.size(); ++index) {
        // Distance on the pitch-class circle.
        const int direct   = std::abs(steps[index] - relative);
        const int distance = std::min(direct, pitchClassCount - direct);
        if (distance < bestDistance) {
            bestDistance = distance;
            bestDegree   = static_cast<int>(index);
        }
    }
    return bestDegree;
}

} // namespace incdaw::app::music
