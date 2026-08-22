// The theme palette — the file behind Settings ▸ Appearance.
//
// A theme is user-writable, hand-editable and shareable, which makes it the
// most abusable file INCDAW reads: a typo in a colour, a key from a newer
// build, a missing brace, a scheme mailed from another machine. None of those
// may cost anyone their window. Every test below breaks the file in a
// different way and asserts that what comes back is still a complete,
// drawable palette.

#include "doctest.h"

#include "ui/ThemePalette.h"

#include <filesystem>
#include <fstream>
#include <set>
#include <string>

using namespace incdaw::ui::theme;

namespace {

std::filesystem::path scratchFile(const char* name)
{
    const std::filesystem::path directory =
        std::filesystem::temp_directory_path() / "incdaw-theme-tests";

    std::error_code failed;
    std::filesystem::create_directories(directory, failed);

    return directory / name;
}

void writeText(const std::filesystem::path& file, const std::string& text)
{
    std::ofstream stream(file, std::ios::binary | std::ios::trunc);
    stream << text;
}

} // namespace

TEST_CASE("every role has a unique key, a label and a group")
{
    std::set<std::string> keys;
    std::set<std::string> labels;

    for (const Ink which : allInks()) {
        const std::string key = inkKey(which);

        CHECK(!key.empty());
        CHECK(std::string(inkLabel(which)).size() > 0);
        CHECK(std::string(inkGroup(which)).size() > 0);

        CHECK(keys.insert(key).second);
        CHECK(labels.insert(inkLabel(which)).second);
    }

    CHECK(keys.size() == inkCount);
}

TEST_CASE("keys round-trip back to their role")
{
    for (const Ink which : allInks()) {
        Ink parsed{};
        REQUIRE(inkFromKey(inkKey(which), parsed));
        CHECK(parsed == which);
    }

    Ink ignored{};
    CHECK_FALSE(inkFromKey("aRoleFromAFutureBuild", ignored));
    CHECK_FALSE(inkFromKey("", ignored));
}

TEST_CASE("hex text carries the alpha byte")
{
    CHECK(toHex(0xFF2E8FFF) == "#FF2E8FFF");
    CHECK(toHex(0x12FFFFFF) == "#12FFFFFF");

    std::uint32_t value = 0;

    REQUIRE(fromHex("#12FFFFFF", value));
    CHECK(value == 0x12FFFFFF);

    REQUIRE(fromHex("12ffffff", value));
    CHECK(value == 0x12FFFFFF);

    // Six digits is what a person types. It means opaque, not invisible.
    REQUIRE(fromHex("#2E8FFF", value));
    CHECK(value == 0xFF2E8FFF);

    REQUIRE(fromHex("  #2E8FFF  ", value));
    CHECK(value == 0xFF2E8FFF);

    const std::uint32_t untouched = 0xDEADBEEF;
    value                         = untouched;

    CHECK_FALSE(fromHex("", value));
    CHECK_FALSE(fromHex("#12345", value));
    CHECK_FALSE(fromHex("#GGGGGG", value));
    CHECK_FALSE(fromHex("rebeccapurple", value));
    CHECK(value == untouched);
}

TEST_CASE("a palette survives a round trip through its own file format")
{
    ThemePalette original = defaultPalette();
    original.name         = "Neon";
    original.setColour(Ink::accent, 0xFF00FF88);
    original.setColour(Ink::highlight, 0x22FFFFFF);

    const ThemePalette restored = ThemePalette::fromJson(original.toJson());

    CHECK(restored.name == "Neon");
    CHECK(restored.colour(Ink::accent) == 0xFF00FF88);
    CHECK(restored.colour(Ink::highlight) == 0x22FFFFFF);

    for (const Ink which : allInks())
        CHECK(restored.colour(which) == original.colour(which));

    // Deterministic output, like every other INCDAW format.
    CHECK(restored.toJson() == original.toJson());
}

TEST_CASE("the written file says what it is and which version it is")
{
    const std::string text = defaultPalette().toJson();

    CHECK(text.find("incdaw-theme") != std::string::npos);
    CHECK(text.find("\"version\"") != std::string::npos);
    CHECK(ThemePalette::currentVersion == 1);
}

TEST_CASE("a broken file yields the default palette rather than an error")
{
    const ThemePalette fallback = defaultPalette();

    for (const char* broken : {"", "{", "null", "[1,2,3]", "not json at all"}) {
        const ThemePalette parsed = ThemePalette::fromJson(broken);

        CHECK(parsed.name == fallback.name);
        for (const Ink which : allInks())
            CHECK(parsed.colour(which) == fallback.colour(which));
    }
}

TEST_CASE("unknown keys are ignored and missing keys keep their default")
{
    const ThemePalette parsed = ThemePalette::fromJson(
        R"({"format":"incdaw-theme","version":99,"name":"Partial",
            "colours":{"accent":"#FF112233","aRoleFromTheFuture":"#FFFFFFFF"}})");

    CHECK(parsed.name == "Partial");
    CHECK(parsed.colour(Ink::accent) == 0xFF112233);
    CHECK(parsed.colour(Ink::panel) == defaultPalette().colour(Ink::panel));
}

TEST_CASE("a colour that does not parse keeps the default rather than going black")
{
    const ThemePalette parsed = ThemePalette::fromJson(
        R"({"name":"Typo","colours":{"panel":"#ZZZZZZ","accent":"#FF010203"}})");

    CHECK(parsed.colour(Ink::panel) == defaultPalette().colour(Ink::panel));
    CHECK(parsed.colour(Ink::accent) == 0xFF010203);
}

TEST_CASE("a hand-typed American spelling is read, not rejected")
{
    const ThemePalette parsed =
        ThemePalette::fromJson(R"({"name":"US","colors":{"accent":"#FF445566"}})");

    CHECK(parsed.colour(Ink::accent) == 0xFF445566);
}

TEST_CASE("the built-ins are complete, named and distinct")
{
    REQUIRE(builtinCount() >= 2);

    std::set<std::string> names;
    for (std::size_t slot = 0; slot < builtinCount(); ++slot) {
        const ThemePalette palette = builtinPalette(slot);

        CHECK(palette.name == builtinName(slot));
        CHECK(isBuiltinName(palette.name));
        CHECK(names.insert(palette.name).second);

        // Every role filled: a built-in with a transparent surface would be a
        // window nobody can read, and a zero here is what that looks like.
        for (const Ink which : allInks())
            CHECK((palette.colour(which) >> 24) != 0u);
    }

    CHECK_FALSE(isBuiltinName("Neon"));
    CHECK_FALSE(isBuiltinName(""));
}

TEST_CASE("the default palette is the scheme the shell was designed against")
{
    const ThemePalette midnight = defaultPalette();

    CHECK(midnight.name == "Midnight");
    CHECK(midnight.colour(Ink::windowBackground) == 0xFF0F1115);
    CHECK(midnight.colour(Ink::accent) == 0xFF2E8FFF);
    CHECK(midnight.colour(Ink::highlight) == 0x12FFFFFF);
    CHECK(midnight.colour(Ink::shadow) == 0x73000000);
    CHECK(midnight.colours == defaultColours());
}

TEST_CASE("an index past the end of the built-ins is the default, not a crash")
{
    const ThemePalette past = builtinPalette(builtinCount() + 5);

    CHECK(past.colours == defaultColours());
    CHECK(std::string(builtinName(builtinCount() + 5)).empty());
}

TEST_CASE("saving and loading a theme file preserves every colour")
{
    const std::filesystem::path file = scratchFile("Round Trip.json");

    ThemePalette written = builtinPalette(1);
    written.name         = "Round Trip";
    written.setColour(Ink::playhead, 0xFF00CCFF);

    REQUIRE(written.save(file));

    const ThemePalette read = ThemePalette::load(file);
    CHECK(read.name == "Round Trip");
    for (const Ink which : allInks())
        CHECK(read.colour(which) == written.colour(which));
}

TEST_CASE("the file name is the theme's name, whatever the file says inside")
{
    const std::filesystem::path file = scratchFile("Renamed.json");

    ThemePalette palette = defaultPalette();
    palette.name         = "Midnight";
    REQUIRE(palette.save(file));

    CHECK(ThemePalette::load(file).name == "Renamed");
}

TEST_CASE("a missing file loads as the default palette")
{
    const ThemePalette missing =
        ThemePalette::load(scratchFile("nothing-was-ever-written-here.json"));

    CHECK(missing.colours == defaultColours());
}

TEST_CASE("a truncated file on disk still yields a drawable palette")
{
    const std::filesystem::path file = scratchFile("truncated.json");
    writeText(file, R"({"format":"incdaw-theme","colours":{"accent":"#FF11)");

    const ThemePalette parsed = ThemePalette::load(file);

    // The name comes from the file when the contents could not supply one.
    CHECK(parsed.name == "truncated");
    CHECK(parsed.colours == defaultColours());
}

TEST_CASE("saving to a path that cannot be written fails without throwing")
{
    ThemePalette palette = defaultPalette();

    CHECK_FALSE(palette.save({}));
    CHECK_FALSE(palette.save("/this/path/does/not/exist/and/cannot/be/made/x.json"));
}
