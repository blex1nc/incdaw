#pragma once

#include <string>

namespace incdaw::plugins {

/// Plugin formats INCDAW hosts.
///
/// VST2 is deliberately absent: Steinberg ended VST2 SDK licensing in 2018 and
/// there is no lawful route for a new closed-source product to support it. See
/// docs/DECISIONS.md D-007.
enum class Format {
    clap,       ///< implemented first — cleanest ABI, MIT licensed
    audioUnit,  ///< native to macOS
    vst3,       ///< widest availability; SDK 3.8.x is MIT licensed
    builtin,    ///< INCDAW's own instruments and effects — never scanned, always present
};

[[nodiscard]] const char* formatName(Format format) noexcept;

// NOTE: deliberately not named `toString`. An unqualified `toString` free
// function beside its own type is found by ADL from anywhere, and hijacks the
// stringification customisation points used by test and logging frameworks.

/// Identifies one plugin, stably, across rescans and machines.
///
/// The file path alone is not an identity: users move plugin folders, and a
/// single bundle can expose several plugins. The pair (format, uid) is what a
/// project file stores, so that a moved plugin is still found and a replaced
/// one is still recognised.
struct PluginIdentifier {
    Format      format = Format::clap;
    std::string uid;    ///< format-native unique id (CLAP id, AU subtype/manu, VST3 CID)

    [[nodiscard]] bool isValid() const noexcept { return !uid.empty(); }

    [[nodiscard]] friend bool operator==(const PluginIdentifier&, const PluginIdentifier&) = default;

    /// Round-trippable form written into project files, e.g. "clap:com.acme.reverb".
    [[nodiscard]] std::string toString() const;
    [[nodiscard]] static bool  fromString(const std::string& text, PluginIdentifier& out);
};

/// INCDAW's own instruments, addressed like plugins so a channel's instrument
/// field has one identity scheme rather than two. An empty identifier still
/// means "no instrument chosen", which the compiler answers with the default
/// synth — these names are for a channel that has *chosen* a builtin.
[[nodiscard]] PluginIdentifier builtinSampler();
[[nodiscard]] PluginIdentifier builtinSimpleSynth();
[[nodiscard]] PluginIdentifier builtinPiano();

} // namespace incdaw::plugins
