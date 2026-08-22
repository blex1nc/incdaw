#pragma once

// INCDAW — presets for builtin instruments and effects (A5).
//
// A channel's instrument values already persist as ChannelInstrumentParameter
// and an insert's already ride in its state blob, but neither of those is a
// *preset*: they belong to one channel in one project and cannot be named,
// recalled somewhere else, or shipped as a starting point. A preset is the
// portable form — a uid, a name, and the parameter values that make the sound.
//
// The folder is modelled on ui/ThemeLibrary: a directory the user owns, one
// versioned JSON file per preset, named by the file rather than by anything
// inside it, so a preset can be copied between machines, mailed, or kept in a
// repository without INCDAW running. Factory presets come from the two engine
// catalogues and are read-only, exactly as built-in themes are.
//
// Plain C++ with no AppKit and no engine pointers: the awkward parts — a name
// with a slash in it, a file that vanished between the scan and the load, a
// preset saved by a newer INCDAW — are logic, and logic belongs where it can
// be tested.

#include "engine/dsp/effects/BuiltinEffect.h"

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace incdaw::project {

/// Bumped when the on-disk shape changes. A file claiming a version this
/// build does not know is refused rather than half-read.
inline constexpr int presetFormatVersion = 1;

/// One preset, in memory.
///
/// `values` names a SUBSET of the target's parameters on purpose: a preset
/// that leaves "Mix" alone lets someone audition three reverb characters
/// without losing the blend they set. Applying a preset therefore writes what
/// it lists and touches nothing else.
struct Preset {
    std::string uid;    ///< the catalogue uid this preset belongs to
    std::string name;
    std::vector<engine::dsp::PresetValue> values;

    /// Serialised form. Deterministic, like every other INCDAW JSON.
    [[nodiscard]] std::string toJson() const;

    /// Parses `text`. `name` comes from the CALLER (the file's stem), not the
    /// document, so a copied file renamed on disk reports its new name. A
    /// document whose uid disagrees with `expectedUid`, whose version is from
    /// the future, or which is malformed, is refused with `error` set.
    [[nodiscard]] static bool fromJson(const std::string& text,
                                       std::string_view   expectedUid,
                                       std::string_view   name,
                                       Preset&            out,
                                       std::string&       error);
};

/// The factory presets of a builtin uid — effect or instrument, one lookup
/// for both, because a preset does not care which catalogue its target came
/// from. Empty for an unknown uid.
[[nodiscard]] engine::dsp::FactoryPresetTable factoryPresetsFor(std::string_view uid);

/// Every parameter of `uid` at its catalogue default, or empty for an unknown
/// uid. This is what the synthesised "Default" preset restores.
[[nodiscard]] std::vector<engine::dsp::PresetValue> defaultPresetValues(std::string_view uid);

/// The factory presets plus whatever is in the user's Presets folder.
class PresetLibrary {
public:
    /// `directory` is normally "<Application Support>/INCDAW/Presets". An
    /// empty path is legal and yields the factory presets alone — what a
    /// machine with no writable support directory gets, rather than an
    /// application that refuses to open a panel.
    explicit PresetLibrary(std::filesystem::path directory);

    /// The name of the synthesised entry that puts every parameter back to
    /// its catalogue default. Not a file: derived from the parameter table so
    /// the two cannot disagree.
    static constexpr const char* defaultPresetName = "Default";

    struct Entry {
        std::string           name;
        bool                  factory = false;
        std::filesystem::path file;   ///< empty for factory entries
    };

    [[nodiscard]] const std::filesystem::path& directory() const noexcept { return directory_; }

    /// Where this uid's user presets live: "<directory>/<uid>". Empty when
    /// the library has no directory.
    [[nodiscard]] std::filesystem::path directoryFor(std::string_view uid) const;

    /// "Default", then the factory presets in their declared order, then the
    /// user's sorted by name. A user preset that takes a factory name is
    /// listed under it and wins nothing — `resolve` prefers the factory one,
    /// because what INCDAW ships must stay reachable.
    [[nodiscard]] std::vector<Entry> entries(std::string_view uid) const;

    /// Name → preset. Nothing when the name is unknown, the file is gone, or
    /// the file does not parse as a preset for this uid.
    [[nodiscard]] std::optional<Preset> resolve(std::string_view uid,
                                                std::string_view name) const;

    [[nodiscard]] bool contains(std::string_view uid, std::string_view name) const;

    /// Whether this name belongs to INCDAW rather than to the user.
    [[nodiscard]] static bool isFactoryName(std::string_view uid, std::string_view name);

    /// Where a user preset of this name is written. Empty when the library
    /// has no directory, or when the name sanitises down to nothing.
    [[nodiscard]] std::filesystem::path fileFor(std::string_view uid,
                                                std::string_view name) const;

    /// Writes `preset` under its own name, creating the uid's folder.
    /// Refuses to overwrite a factory preset: one that can be edited is not a
    /// factory preset.
    [[nodiscard]] bool store(const Preset& preset, std::string& error) const;

    /// Copies an existing preset to a free name derived from `preferred`, and
    /// returns the name actually used. Empty on failure, with `error` set.
    [[nodiscard]] std::string duplicate(std::string_view uid,
                                        std::string_view name,
                                        std::string_view preferred,
                                        std::string&     error) const;

    /// Renames a user preset. Refuses to touch a factory one, to land on a
    /// name already taken, or to rename something that is not there.
    [[nodiscard]] bool rename(std::string_view uid,
                              std::string_view from,
                              std::string_view to,
                              std::string&     error) const;

    /// Deletes a user preset. Factory presets cannot be deleted.
    [[nodiscard]] bool remove(std::string_view uid, std::string_view name,
                              std::string& error) const;

    /// Strips what a file name may not carry — separators, dots at the front,
    /// control characters — and trims the result. May return an empty string,
    /// which callers must treat as "no usable name".
    [[nodiscard]] static std::string sanitiseName(std::string_view name);

    /// `preferred`, or "preferred 2", "preferred 3"… until nothing owns it.
    [[nodiscard]] std::string uniqueName(std::string_view uid,
                                         std::string_view preferred) const;

private:
    std::filesystem::path directory_;
};

} // namespace incdaw::project
