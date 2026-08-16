#pragma once

#include <array>
#include <cstddef>
#include <string>
#include <vector>

namespace incdaw::app::music {

/// Chord analysis and generation for the Piano Roll's chord tools.
///
/// Functional reference: FL Studio 2026's Chord Panel (note/chord detection),
/// Chord Stamp (top-down / bottom-up voice-leading) and Chord Progression tool
/// with chord nudge (docs/FL2026_GAP.md P1). Everything here is a pure
/// function over MIDI key numbers — no model access, no UI, fully testable.

// ── Note names ────────────────────────────────────────────────────────────────

/// 0..11 → "C".."B", spelled with sharps.
[[nodiscard]] const char* pitchClassName(int pitchClass) noexcept;

/// MIDI key → "C4" style. Octave convention: middle C (key 60) is C4.
[[nodiscard]] std::string noteName(int key);

/// Interval in semitones → "P1".."P8" short name; larger intervals reduce
/// modulo the octave with the octave count appended ("P5+8ve" stays "P5").
[[nodiscard]] const char* intervalShortName(int semitones) noexcept;

// ── Chord dictionary ──────────────────────────────────────────────────────────

/// One chord quality: a name suffix and its intervals from the root.
///
/// Intervals are ascending semitones and may exceed 12 (an add9 really is a
/// ninth above the root); detection reduces them to pitch classes, voicing
/// keeps them literal.
struct ChordType {
    const char*        suffix;  ///< appended to the root name: "" = major, "m", "7"…
    std::array<int, 5> intervals;
    std::size_t        size;

    [[nodiscard]] std::vector<int> pitchClasses() const;
};

/// Every quality detection and stamping know, simplest first — the order is
/// the tie-break when one pitch-class set names two chords (Cm6 vs Am7b5).
[[nodiscard]] const std::vector<ChordType>& chordDictionary();

/// Lookup by suffix ("", "m", "maj7", …), or nullptr.
[[nodiscard]] const ChordType* findChordType(const std::string& suffix) noexcept;

// ── Detection (Chord Panel) ───────────────────────────────────────────────────

struct ChordDetection {
    bool        matched        = false;
    int         rootPitchClass = -1;
    const ChordType* type      = nullptr;
    int         bassKey        = -1;
    bool        inverted       = false;   ///< bass is not the root

    /// What the panel shows: "Cmaj7", "Am/C", a note name for one key,
    /// "C4–G4 (P5)" for two, or "" when nothing matches.
    std::string display;
};

/// Identifies the chord formed by `keys` (any order, duplicates allowed).
[[nodiscard]] ChordDetection detectChord(const std::vector<int>& keys);

// ── Stamping (Chord Stamp) ────────────────────────────────────────────────────

enum class StampVoicing {
    bottomUp,  ///< the chosen key is the root and the lowest note
    topDown,   ///< the chosen key is the root and the highest note
};

/// Keys of `type` rooted at `rootKey`, voiced per `voicing`, sorted ascending.
/// Keys that would leave 0..127 are dropped rather than folded, so a stamp at
/// the keyboard edge stays a recognisable chord fragment.
[[nodiscard]] std::vector<int> stampChord(int rootKey, const ChordType& type,
                                          StampVoicing voicing);

// ── Voice-leading ─────────────────────────────────────────────────────────────

/// The voicing of the chord spelled by `pitchClasses` (root first) that moves
/// least from `previousKeys`: inversions and octave placements are searched
/// and the candidate with the smallest total key movement wins. This is what
/// makes a stamped progression connect smoothly instead of jumping between
/// root positions.
[[nodiscard]] std::vector<int> voiceLead(const std::vector<int>& previousKeys,
                                         const std::vector<int>& pitchClasses);

// ── Scales and diatonic chords ────────────────────────────────────────────────

enum class Scale { major, naturalMinor, harmonicMinor };

[[nodiscard]] const std::array<int, 7>& scaleIntervals(Scale scale) noexcept;

/// Pitch classes (root first) of the chord stacked in thirds on `degree`
/// (0-based) of the key. `noteCount` 3 is the triad, 4 adds the seventh;
/// 1 and 2 give the degree alone and the degree plus its third.
[[nodiscard]] std::vector<int> diatonicChordPitchClasses(int keyRootPitchClass, Scale scale,
                                                         int degree, std::size_t noteCount);

/// The degree whose pitch class equals `pitchClass`, or -1 when chromatic.
[[nodiscard]] int degreeOf(int keyRootPitchClass, Scale scale, int pitchClass) noexcept;

/// The degree nearest to `pitchClass` (never -1; ties resolve to the lower
/// degree). Lets the nudge tool start from a chromatic chord.
[[nodiscard]] int nearestDegree(int keyRootPitchClass, Scale scale, int pitchClass) noexcept;

} // namespace incdaw::app::music
