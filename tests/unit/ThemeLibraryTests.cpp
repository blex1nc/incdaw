// The themes folder — what "Duplicate", "Delete" and the theme menu actually do.
//
// The folder belongs to the user, which means everything in it can change
// while INCDAW is not looking: files renamed, deleted, dropped in from another
// machine, or given names the filesystem cannot carry. The library's job is to
// make that ordinary rather than fatal, and to keep one promise above the
// others — the schemes INCDAW ships with can never be edited away.

#include "doctest.h"

#include "ui/ThemeLibrary.h"

#include <filesystem>
#include <fstream>
#include <string>

using namespace incdaw::ui::theme;

namespace {

/// A folder of its own per test case, emptied first: a leftover theme from a
/// previous run would make "the folder starts with only the built-ins" pass or
/// fail depending on history.
std::filesystem::path scratchDirectory(const char* name)
{
    const std::filesystem::path directory =
        std::filesystem::temp_directory_path() / "incdaw-theme-library-tests" / name;

    std::error_code failed;
    std::filesystem::remove_all(directory, failed);
    std::filesystem::create_directories(directory, failed);

    return directory;
}

std::size_t userCount(const ThemeLibrary& library)
{
    std::size_t count = 0;
    for (const ThemeLibrary::Entry& entry : library.entries())
        if (!entry.builtin)
            ++count;

    return count;
}

} // namespace

TEST_CASE("an empty folder still offers every built-in")
{
    const ThemeLibrary library(scratchDirectory("empty"));

    const std::vector<ThemeLibrary::Entry> entries = library.entries();
    REQUIRE(entries.size() == builtinCount());

    for (std::size_t slot = 0; slot < builtinCount(); ++slot) {
        CHECK(entries[slot].builtin);
        CHECK(entries[slot].name == builtinName(slot));
        CHECK(entries[slot].file.empty());
    }
}

TEST_CASE("a library with no folder at all is the built-ins, not a failure")
{
    const ThemeLibrary library{std::filesystem::path{}};

    CHECK(library.entries().size() == builtinCount());
    CHECK(library.resolve("Midnight").name == "Midnight");
    CHECK(library.resolve("Anything Else").colours == defaultColours());
    CHECK(library.fileFor("Neon").empty());

    std::string error;
    ThemePalette palette = defaultPalette();
    palette.name         = "Neon";
    CHECK_FALSE(library.store(palette, error));
    CHECK(!error.empty());
}

TEST_CASE("a stored theme appears after the built-ins, sorted by name")
{
    const ThemeLibrary library(scratchDirectory("sorted"));

    std::string error;
    for (const char* name : {"Zinc", "Amber", "Moss"}) {
        ThemePalette palette = defaultPalette();
        palette.name         = name;
        REQUIRE(library.store(palette, error));
    }

    const std::vector<ThemeLibrary::Entry> entries = library.entries();
    REQUIRE(entries.size() == builtinCount() + 3);

    CHECK(entries[builtinCount() + 0].name == "Amber");
    CHECK(entries[builtinCount() + 1].name == "Moss");
    CHECK(entries[builtinCount() + 2].name == "Zinc");

    for (std::size_t slot = builtinCount(); slot < entries.size(); ++slot) {
        CHECK_FALSE(entries[slot].builtin);
        CHECK_FALSE(entries[slot].file.empty());
    }
}

TEST_CASE("resolving reads the colours back")
{
    const ThemeLibrary library(scratchDirectory("resolve"));

    ThemePalette palette = defaultPalette();
    palette.name         = "Neon";
    palette.setColour(Ink::accent, 0xFF00FF88);

    std::string error;
    REQUIRE(library.store(palette, error));

    const ThemePalette read = library.resolve("Neon");
    CHECK(read.name == "Neon");
    CHECK(read.colour(Ink::accent) == 0xFF00FF88);
}

TEST_CASE("an unknown name resolves to the default rather than to nothing")
{
    const ThemeLibrary library(scratchDirectory("unknown"));

    CHECK(library.resolve("Deleted In Finder").colours == defaultColours());
    CHECK(library.resolve("").colours == defaultColours());
    CHECK_FALSE(library.contains("Deleted In Finder"));
}

TEST_CASE("built-ins win their own names and cannot be written or deleted")
{
    const ThemeLibrary library(scratchDirectory("builtins"));

    ThemePalette impostor = defaultPalette();
    impostor.name         = "Midnight";
    impostor.setColour(Ink::windowBackground, 0xFFFF0000);

    std::string error;
    CHECK_FALSE(library.store(impostor, error));
    CHECK(!error.empty());

    CHECK(library.resolve("Midnight").colour(Ink::windowBackground) == 0xFF0F1115);

    CHECK_FALSE(library.remove("Midnight", error));
    CHECK(!error.empty());

    // Case is not a way around it either: the folder is case-insensitive on
    // macOS, and so is the lookup.
    CHECK(library.resolve("midnight").name == "Midnight");
}

TEST_CASE("duplicating a built-in makes an editable copy under a free name")
{
    const ThemeLibrary library(scratchDirectory("duplicate"));

    std::string error;
    const std::string first = library.duplicate(defaultPalette(), "Midnight Custom", error);
    REQUIRE(first == "Midnight Custom");

    const std::string second = library.duplicate(defaultPalette(), "Midnight Custom", error);
    CHECK(second == "Midnight Custom 2");

    const std::string third = library.duplicate(defaultPalette(), "Midnight Custom", error);
    CHECK(third == "Midnight Custom 3");

    CHECK(userCount(library) == 3);

    // The copy carries its new name, not the name it was copied from.
    CHECK(library.resolve(second).name == "Midnight Custom 2");
}

TEST_CASE("a copy never lands on a built-in's name")
{
    const ThemeLibrary library(scratchDirectory("collision"));

    std::string error;
    const std::string name = library.duplicate(defaultPalette(), "Midnight", error);

    CHECK(name == "Midnight 2");
    CHECK(library.resolve("Midnight").colours == defaultColours());
}

TEST_CASE("deleting removes the file and the entry")
{
    const ThemeLibrary library(scratchDirectory("delete"));

    std::string error;
    const std::string name = library.duplicate(defaultPalette(), "Throwaway", error);
    REQUIRE(!name.empty());
    REQUIRE(library.contains(name));

    REQUIRE(library.remove(name, error));
    CHECK(error.empty());
    CHECK_FALSE(library.contains(name));
    CHECK(userCount(library) == 0);

    // Deleting it twice is a stale menu, not a crash.
    CHECK_FALSE(library.remove(name, error));
}

TEST_CASE("a name the filesystem cannot carry is trimmed, not obeyed")
{
    CHECK(ThemeLibrary::sanitiseName("../../etc/passwd") == "etcpasswd");
    CHECK(ThemeLibrary::sanitiseName("Neon/Blue") == "NeonBlue");
    CHECK(ThemeLibrary::sanitiseName("  Neon  ") == "Neon");
    CHECK(ThemeLibrary::sanitiseName(".hidden") == "hidden");
    CHECK(ThemeLibrary::sanitiseName("").empty());
    CHECK(ThemeLibrary::sanitiseName("///").empty());
    CHECK(ThemeLibrary::sanitiseName("...").empty());
    CHECK(ThemeLibrary::sanitiseName(std::string(400, 'x')).size() == 64);
}

TEST_CASE("a theme with an unusable name is refused rather than written somewhere odd")
{
    const ThemeLibrary library(scratchDirectory("unusable"));

    ThemePalette palette = defaultPalette();
    palette.name         = "///";

    std::string error;
    CHECK_FALSE(library.store(palette, error));
    CHECK(!error.empty());
    CHECK(userCount(library) == 0);
}

TEST_CASE("the folder decides the name, so a renamed file is the theme it says it is")
{
    const std::filesystem::path directory = scratchDirectory("renamed");
    const ThemeLibrary          library(directory);

    // What a duplicate made outside INCDAW looks like: the file renamed, the
    // name inside it still the original.
    ThemePalette palette = defaultPalette();
    palette.name         = "Midnight";
    REQUIRE(palette.save(directory / "Renamed By Hand.json"));

    CHECK(library.resolve("Renamed By Hand").name == "Renamed By Hand");
    CHECK(userCount(library) == 1);
}

TEST_CASE("files that are not themes are ignored")
{
    const std::filesystem::path directory = scratchDirectory("junk");

    std::ofstream(directory / "notes.txt") << "hello";
    std::error_code failed;
    std::filesystem::create_directories(directory / "Subfolder.json", failed);

    const ThemeLibrary library(directory);
    CHECK(userCount(library) == 0);
}
