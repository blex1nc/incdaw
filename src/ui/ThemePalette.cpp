#include "ui/ThemePalette.h"

#include "project/Json.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <system_error>

namespace incdaw::ui::theme {

using project::Json;

namespace {

/// One row per role, in enum order. The key is format; the label and the group
/// are prose the editor shows.
struct RoleInfo {
    Ink         ink;
    const char* key;
    const char* label;
    const char* group;
};

constexpr const char* groupSurfaces  = "Surfaces";
constexpr const char* groupRows      = "Rows and grid";
constexpr const char* groupEdges     = "Edges";
constexpr const char* groupText      = "Text";
constexpr const char* groupAccents   = "Accents";
constexpr const char* groupMaterial  = "Material";
constexpr const char* groupReadout   = "Readout";
constexpr const char* groupMeters    = "Meters";
constexpr const char* groupSelection = "Selection";

constexpr RoleInfo roles[inkCount] = {
    {Ink::windowBackground, "windowBackground", "Window background",   groupSurfaces},
    {Ink::chromeTop,        "chromeTop",        "Chrome, top",         groupSurfaces},
    {Ink::chromeBottom,     "chromeBottom",     "Chrome, bottom",      groupSurfaces},
    {Ink::panel,            "panel",            "Panel",               groupSurfaces},
    {Ink::panelRaised,      "panelRaised",      "Panel, raised",       groupSurfaces},
    {Ink::panelRaisedTop,   "panelRaisedTop",   "Panel, raised top",   groupSurfaces},
    {Ink::panelSunken,      "panelSunken",      "Panel, sunken",       groupSurfaces},

    {Ink::rowEven,          "rowEven",          "Row, even",           groupRows},
    {Ink::rowOdd,           "rowOdd",           "Row, odd",            groupRows},
    {Ink::rowSelected,      "rowSelected",      "Row, selected",       groupRows},
    {Ink::gridLine,         "gridLine",         "Grid line",           groupRows},
    {Ink::gridLineStrong,   "gridLineStrong",   "Bar line",            groupRows},

    {Ink::separator,        "separator",        "Separator",           groupEdges},
    {Ink::highlight,        "highlight",        "Bevel highlight",     groupEdges},
    {Ink::shadow,           "shadow",           "Bevel shadow",        groupEdges},

    {Ink::textPrimary,      "textPrimary",      "Text",                groupText},
    {Ink::textSecondary,    "textSecondary",    "Text, secondary",     groupText},
    {Ink::textDim,          "textDim",          "Text, dim",           groupText},
    {Ink::textOnAccent,     "textOnAccent",     "Text on accent",      groupText},

    {Ink::accent,           "accent",           "Accent",              groupAccents},
    {Ink::accentDim,        "accentDim",        "Accent, dim",         groupAccents},
    {Ink::record,           "record",           "Record",              groupAccents},
    {Ink::solo,             "solo",             "Solo",                groupAccents},
    {Ink::mute,             "mute",             "Mute",                groupAccents},

    {Ink::midi,             "midi",             "Pattern and MIDI",    groupMaterial},
    {Ink::audio,            "audio",            "Audio",               groupMaterial},
    {Ink::automation,       "automation",       "Automation",          groupMaterial},
    {Ink::playhead,         "playhead",         "Playhead",            groupMaterial},

    {Ink::lcdBackground,    "lcdBackground",    "Display background",  groupReadout},
    {Ink::lcdBezel,         "lcdBezel",         "Display bezel",       groupReadout},
    {Ink::lcdText,          "lcdText",          "Display text",        groupReadout},
    {Ink::lcdTextDim,       "lcdTextDim",       "Display text, dim",   groupReadout},

    {Ink::meterLow,         "meterLow",         "Meter, low",          groupMeters},
    {Ink::meterMid,         "meterMid",         "Meter, mid",          groupMeters},
    {Ink::meterHigh,        "meterHigh",        "Meter, high",         groupMeters},

    {Ink::selectionFill,    "selectionFill",    "Selection fill",      groupSelection},
    {Ink::selectionStroke,  "selectionStroke",  "Selection stroke",    groupSelection},
};

constexpr std::size_t index(Ink which) { return static_cast<std::size_t>(which); }

/// Midnight. These are the values the shell drew before this file existed, to
/// the byte: turning themes on must not change how INCDAW looks on first launch.
constexpr std::uint32_t midnight[inkCount] = {
    0xFF0F1115, 0xFF2C313A, 0xFF1B1F26, 0xFF181B21, 0xFF242931, 0xFF2E343E, 0xFF0B0D11,
    0xFF1C2027, 0xFF181C22, 0xFF2A313C, 0xFF22262E, 0xFF333944,
    0xFF07080B, 0x12FFFFFF, 0x73000000,
    0xFFE9ECF1, 0xFF99A2B0, 0xFF636C7A, 0xFF0B0D11,
    0xFF2E8FFF, 0xFF1C5392, 0xFFFF453A, 0xFFFFD426, 0xFFFF6B5E,
    0xFF46D97F, 0xFF3AA9FF, 0xFFC07BFF, 0xFFFFC24A,
    0xFF0A0E12, 0xFF3A414C, 0xFFFFC96B, 0xFF6D7886,
    0xFF35D06E, 0xFFFFD426, 0xFFFF453A,
    0x382E8FFF, 0xD959ADFF,
};

/// Slate. Neutral graphite with a teal accent — the same density with the blue
/// cast taken out, for anyone who reads a blue-grey shell as cold.
constexpr std::uint32_t slate[inkCount] = {
    0xFF121212, 0xFF343434, 0xFF212121, 0xFF1C1C1C, 0xFF2A2A2A, 0xFF353535, 0xFF0E0E0E,
    0xFF202020, 0xFF1B1B1B, 0xFF323232, 0xFF272727, 0xFF3A3A3A,
    0xFF080808, 0x14FFFFFF, 0x73000000,
    0xFFEDEDED, 0xFFA6A6A6, 0xFF6E6E6E, 0xFF0E0E0E,
    0xFF23C4A8, 0xFF14705F, 0xFFE5484D, 0xFFF5C518, 0xFFE87A6E,
    0xFF5AD08A, 0xFF48B0E0, 0xFFB98CF0, 0xFFF0B429,
    0xFF0D0D0D, 0xFF404040, 0xFF7FE8D2, 0xFF6A7A76,
    0xFF3FC97A, 0xFFF5C518, 0xFFE5484D,
    0x3823C4A8, 0xD95FDCC6,
};

/// Ember. A warm dark room: brown chrome, amber accent. Long sessions are the
/// use case, and warm greys glare less under a lamp than blue ones.
constexpr std::uint32_t ember[inkCount] = {
    0xFF14100D, 0xFF38302A, 0xFF231D19, 0xFF1E1815, 0xFF2C2520, 0xFF3A312A, 0xFF0F0B09,
    0xFF221C18, 0xFF1D1714, 0xFF3A2E24, 0xFF2B2320, 0xFF3F352E,
    0xFF0A0706, 0x12FFF0E0, 0x7A000000,
    0xFFF3E9DE, 0xFFB5A292, 0xFF7C6C5F, 0xFF14100D,
    0xFFFF8C3A, 0xFF8A4A18, 0xFFFF4B3E, 0xFFFFC93A, 0xFFFF8A72,
    0xFF8FD26A, 0xFF56B8D6, 0xFFD98AE8, 0xFFFFD166,
    0xFF0F0B08, 0xFF473A31, 0xFFFFB35C, 0xFF7E6A58,
    0xFF7FC96A, 0xFFFFC93A, 0xFFFF4B3E,
    0x38FF8C3A, 0xD9FFB273,
};

/// Daylight. The one light scheme, for a bright room and for screenshots. The
/// bevel roles invert with it — a light surface is lit from a white edge and
/// shadowed by a much weaker black than a dark one.
constexpr std::uint32_t daylight[inkCount] = {
    0xFFE7E9EE, 0xFFFAFBFC, 0xFFDDE1E8, 0xFFF2F4F7, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFE0E3E9,
    0xFFF7F8FA, 0xFFEDEFF3, 0xFFD5E4FA, 0xFFD7DBE2, 0xFFB9BFC9,
    0xFFAFB5BF, 0x99FFFFFF, 0x26000000,
    0xFF16191F, 0xFF5A626E, 0xFF8B93A0, 0xFFFFFFFF,
    0xFF1A6FD4, 0xFF9EC2EE, 0xFFD22B21, 0xFFC79600, 0xFFD1604F,
    0xFF1F9A55, 0xFF1370B8, 0xFF7A3FBF, 0xFFD98200,
    0xFF20242B, 0xFFB4BAC4, 0xFFFFC96B, 0xFF8A93A0,
    0xFF23A85C, 0xFFDDA300, 0xFFD22B21,
    0x381A6FD4, 0xD91A6FD4,
};

struct Builtin {
    const char*            name;
    const std::uint32_t (*colours)[inkCount];
};

constexpr Builtin builtins[] = {
    {"Midnight", &midnight},
    {"Slate",    &slate},
    {"Ember",    &ember},
    {"Daylight", &daylight},
};

constexpr std::size_t builtinTotal = sizeof(builtins) / sizeof(builtins[0]);

int hexDigit(char character)
{
    if (character >= '0' && character <= '9') return character - '0';
    if (character >= 'a' && character <= 'f') return character - 'a' + 10;
    if (character >= 'A' && character <= 'F') return character - 'A' + 10;
    return -1;
}

} // namespace

// ── Roles ────────────────────────────────────────────────────────────────────

const char* inkKey(Ink which)   { return roles[index(which)].key; }
const char* inkLabel(Ink which) { return roles[index(which)].label; }
const char* inkGroup(Ink which) { return roles[index(which)].group; }

bool inkFromKey(std::string_view key, Ink& out)
{
    for (const RoleInfo& role : roles) {
        if (key == role.key) {
            out = role.ink;
            return true;
        }
    }

    return false;
}

bool namesMatch(std::string_view left, std::string_view right)
{
    if (left.size() != right.size())
        return false;

    for (std::size_t slot = 0; slot < left.size(); ++slot) {
        const unsigned char a = static_cast<unsigned char>(left[slot]);
        const unsigned char b = static_cast<unsigned char>(right[slot]);
        if (std::tolower(a) != std::tolower(b))
            return false;
    }

    return true;
}

const std::array<Ink, inkCount>& allInks()
{
    static const std::array<Ink, inkCount> list = [] {
        std::array<Ink, inkCount> result{};
        for (std::size_t slot = 0; slot < inkCount; ++slot)
            result[slot] = roles[slot].ink;
        return result;
    }();

    return list;
}

// ── Colour text ──────────────────────────────────────────────────────────────

std::string toHex(std::uint32_t argb)
{
    char buffer[10];
    std::snprintf(buffer, sizeof(buffer), "#%08X", argb);
    return buffer;
}

bool fromHex(std::string_view text, std::uint32_t& out)
{
    std::string_view digits = text;

    while (!digits.empty() && (digits.front() == ' ' || digits.front() == '\t'))
        digits.remove_prefix(1);
    while (!digits.empty() && (digits.back() == ' ' || digits.back() == '\t'))
        digits.remove_suffix(1);

    if (!digits.empty() && digits.front() == '#')
        digits.remove_prefix(1);

    if (digits.size() != 6 && digits.size() != 8)
        return false;

    std::uint32_t value = 0;
    for (const char character : digits) {
        const int digit = hexDigit(character);
        if (digit < 0)
            return false;

        value = (value << 4) | static_cast<std::uint32_t>(digit);
    }

    // Six digits is a colour someone typed by hand; it means opaque, not
    // invisible. Only the two bevel roles ever want a partial alpha, and those
    // are written back out with all eight.
    out = digits.size() == 6 ? (0xFF000000u | value) : value;
    return true;
}

// ── Palette ──────────────────────────────────────────────────────────────────

std::array<std::uint32_t, inkCount> defaultColours()
{
    std::array<std::uint32_t, inkCount> result{};
    for (std::size_t slot = 0; slot < inkCount; ++slot)
        result[slot] = midnight[slot];

    return result;
}

std::string ThemePalette::toJson() const
{
    Json root = Json::object();
    root.set("format", "incdaw-theme");
    root.set("version", currentVersion);
    root.set("name", name);

    Json coloursObject = Json::object();
    for (const Ink which : allInks())
        coloursObject.set(inkKey(which), toHex(colour(which)));
    root.set("colours", std::move(coloursObject));

    return root.dump();
}

ThemePalette ThemePalette::fromJson(const std::string& text)
{
    ThemePalette palette;

    Json        root;
    std::string error;
    if (!Json::parse(text, root, error) || !root.isObject())
        return palette;

    if (const std::string named = root["name"].asString(); !named.empty())
        palette.name = named;

    // Both spellings are read. The file is written with "colours" and the
    // project is written in British English throughout, but a theme is a file
    // people hand-edit and share, and rejecting a hand-typed "colors" would be
    // a parse failure over a vowel.
    const Json& coloursObject = root["colours"].isObject() ? root["colours"] : root["colors"];
    if (!coloursObject.isObject())
        return palette;

    for (const std::pair<std::string, Json>& member : coloursObject.members()) {
        Ink which{};
        if (!inkFromKey(member.first, which))
            continue;   // a role this build does not have: newer file, not an error

        std::uint32_t value = 0;
        if (fromHex(member.second.asString(), value))
            palette.setColour(which, value);
    }

    return palette;
}

bool ThemePalette::save(const std::filesystem::path& file) const
{
    if (file.empty())
        return false;

    std::error_code code;
    if (const std::filesystem::path parent = file.parent_path(); !parent.empty())
        std::filesystem::create_directories(parent, code);

    const std::filesystem::path temporary = std::filesystem::path(file).concat(".tmp");

    {
        std::ofstream stream(temporary, std::ios::binary | std::ios::trunc);
        if (!stream)
            return false;

        stream << toJson() << '\n';
        if (!stream)
            return false;
    }

    std::filesystem::rename(temporary, file, code);
    if (code) {
        std::filesystem::remove(temporary, code);
        return false;
    }

    return true;
}

ThemePalette ThemePalette::load(const std::filesystem::path& file)
{
    std::ifstream stream(file, std::ios::binary);
    if (!stream)
        return {};

    std::ostringstream text;
    text << stream.rdbuf();

    ThemePalette palette = fromJson(text.str());

    // The file name wins over the name inside the file. A theme is a file, and
    // a theme renamed or copied in Finder — which is exactly how one gets from
    // one machine to another — is the name the user now means. The name inside
    // is only a fallback for a path with no stem at all.
    if (const std::string stem = file.stem().string(); !stem.empty())
        palette.name = stem;

    return palette;
}

// ── Built-in schemes ─────────────────────────────────────────────────────────

std::size_t builtinCount() { return builtinTotal; }

const char* builtinName(std::size_t slot)
{
    return slot < builtinTotal ? builtins[slot].name : "";
}

ThemePalette builtinPalette(std::size_t which)
{
    ThemePalette palette;
    if (which >= builtinTotal)
        return palette;

    palette.name = builtins[which].name;
    for (std::size_t slot = 0; slot < inkCount; ++slot)
        palette.colours[slot] = (*builtins[which].colours)[slot];

    return palette;
}

bool isBuiltinName(std::string_view name)
{
    for (const Builtin& entry : builtins)
        if (namesMatch(name, entry.name))
            return true;

    return false;
}

ThemePalette defaultPalette() { return builtinPalette(0); }

} // namespace incdaw::ui::theme
