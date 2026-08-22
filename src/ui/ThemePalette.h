#pragma once

// INCDAW — the palette as data, so that a theme is a file rather than a build.
//
// `ui/macos/Theme.h` decides what a surface, a pad, a fader or a readout looks
// like; it used to also decide what colour each of those is, in a switch
// statement compiled into the shell. That made the design language coherent and
// the palette immovable: changing one grey meant editing Objective-C++ and
// rebuilding, and a user could not change it at all.
//
// This header separates the two. The roles stay exactly where they were — views
// still ask for "the ground under a grid" and never for a colour — but the
// values behind those roles now live in a plain data structure that can be
// read from a file, edited at runtime and written back out. Nothing here
// touches AppKit, so the palette is testable without a window server.
//
// The file format is versioned from the first line it was ever written
// (CLAUDE.md §2, docs/DECISIONS.md D-039): a theme is persisted state, and
// unversioned persisted state is a migration that cannot be written later.

#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace incdaw::ui::theme {

// ── Roles ────────────────────────────────────────────────────────────────────

/// Named roles rather than named colours: views ask for "the ground under a
/// grid", not "0.06 grey", so the scheme can move without touching them.
///
/// The order is the file order and the editor's order. Appending is safe;
/// reordering is not, because saved themes are keyed by name, not by index —
/// see `inkKey`.
enum class Ink {
    windowBackground,   ///< behind everything
    chromeTop,          ///< control bar gradient, top stop
    chromeBottom,       ///< control bar gradient, bottom stop
    panel,              ///< a pane's own ground
    panelRaised,        ///< a header, strip or button sitting on the panel
    panelRaisedTop,     ///< raised gradient, top stop
    panelSunken,        ///< a well: grid, timeline, meter housing
    rowEven,            ///< list/rack rows
    rowOdd,
    rowSelected,
    gridLine,           ///< beat lines, row separators
    gridLineStrong,     ///< bar lines
    separator,          ///< hard division between panes
    highlight,          ///< the 1px bevel light (white, low alpha)
    shadow,             ///< the 1px bevel dark (black, low alpha)
    textPrimary,
    textSecondary,
    textDim,
    textOnAccent,
    accent,             ///< selection, focus, active tab
    accentDim,
    record,
    solo,
    mute,
    midi,               ///< pattern / MIDI material
    audio,              ///< audio material
    automation,         ///< automation material
    playhead,
    lcdBackground,
    lcdBezel,
    lcdText,
    lcdTextDim,
    meterLow,
    meterMid,
    meterHigh,
    selectionFill,      ///< marquee / range selection wash
    selectionStroke,
};

/// How many roles a palette carries. Kept as a constant rather than as a
/// trailing enumerator so that `switch (Ink)` stays exhaustive without a
/// default case that would hide a forgotten role.
inline constexpr std::size_t inkCount = 37;

/// The stable identifier used as the JSON key. Never changes for a role that
/// already shipped; a renamed key is a theme file that silently loses a colour.
[[nodiscard]] const char* inkKey(Ink which);

/// What the editor calls the role. Free to change — it is prose, not format.
[[nodiscard]] const char* inkLabel(Ink which);

/// The editor's section heading for the role, so that thirty-seven colours read
/// as eight groups rather than as a wall.
[[nodiscard]] const char* inkGroup(Ink which);

/// False when `key` names no role — which is what a theme file from a newer
/// build looks like, and is not an error.
[[nodiscard]] bool inkFromKey(std::string_view key, Ink& out);

/// Every role, in file order.
[[nodiscard]] const std::array<Ink, inkCount>& allInks();

/// Theme names compared the way the folder holding them compares file names:
/// without regard to case. "midnight" and "Midnight" are one theme, because on
/// macOS they are one file.
[[nodiscard]] bool namesMatch(std::string_view left, std::string_view right);

// ── Colour text ──────────────────────────────────────────────────────────────

/// "#AARRGGBB". Written with the alpha byte always present, because two of the
/// roles are hairlines that exist only as low-alpha white and black, and a
/// theme that dropped their alpha would draw them as solid.
[[nodiscard]] std::string toHex(std::uint32_t argb);

/// Accepts "#AARRGGBB", "#RRGGBB" (opaque), and either without the leading
/// hash. Returns false on anything else and leaves `out` untouched.
[[nodiscard]] bool fromHex(std::string_view text, std::uint32_t& out);

// ── Palette ──────────────────────────────────────────────────────────────────

/// The Midnight values — what the shell drew before themes existed.
[[nodiscard]] std::array<std::uint32_t, inkCount> defaultColours();

/// One complete scheme: a name and one 0xAARRGGBB value per role.
///
/// Every field degrades to a default. A theme file is a preference, never a
/// precondition for drawing a window: a corrupt file, a file from a newer
/// build, or a hand-edited one with a typo in a colour all resolve to "use the
/// built-in value for that role" rather than to an error or to a black window.
struct ThemePalette {
    /// Bumped when a role's meaning changes, never when one is added — the
    /// reader tolerates unknown keys and missing keys alike.
    static constexpr int currentVersion = 1;

    std::string                            name    = "Midnight";
    std::array<std::uint32_t, inkCount>    colours = defaultColours();

    [[nodiscard]] std::uint32_t colour(Ink which) const noexcept
    {
        return colours[static_cast<std::size_t>(which)];
    }

    void setColour(Ink which, std::uint32_t argb) noexcept
    {
        colours[static_cast<std::size_t>(which)] = argb;
    }

    [[nodiscard]] std::string toJson() const;

    /// Never fails. Anything unparseable yields the built-in default for the
    /// roles it could not read, and keeps the ones it could.
    [[nodiscard]] static ThemePalette fromJson(const std::string& text);

    /// Writes atomically enough for a preferences file: a failed write leaves
    /// the previous theme in place and returns false.
    [[nodiscard]] bool save(const std::filesystem::path& file) const;

    /// A missing file yields the default palette.
    [[nodiscard]] static ThemePalette load(const std::filesystem::path& file);
};

// ── Built-in schemes ─────────────────────────────────────────────────────────

/// The schemes compiled into the application. They are read-only: editing one
/// in the Appearance tab copies it to a user theme first, so that the ground
/// INCDAW ships with is always one selection away.
[[nodiscard]] std::size_t builtinCount();
[[nodiscard]] ThemePalette builtinPalette(std::size_t index);
[[nodiscard]] const char*  builtinName(std::size_t index);
[[nodiscard]] bool         isBuiltinName(std::string_view name);

/// Midnight — the scheme the shell was designed against (docs/DECISIONS.md D-035).
[[nodiscard]] ThemePalette defaultPalette();

} // namespace incdaw::ui::theme
