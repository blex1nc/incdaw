// FL2026 P9 (part 1) — the Browser's file model.
//
// The browser is the one place where INCDAW walks a user's disk, and every
// mistake it can make is a quiet one: a package opened as if it were a folder,
// a favourite silently dropped because a library moved, a search that walks an
// entire volume. Those are the properties asserted here — the pane in the
// shell only draws what these calls return.

#include "doctest.h"

#include "app/Browser.h"

#include <filesystem>
#include <fstream>
#include <string>

using namespace incdaw;
using namespace incdaw::app;
namespace fs = std::filesystem;

namespace {

struct ScratchDir {
    fs::path path;

    explicit ScratchDir(const char* name)
        : path(fs::temp_directory_path() / name)
    {
        std::error_code code;
        fs::remove_all(path, code);
        fs::create_directories(path, code);
    }

    ~ScratchDir()
    {
        std::error_code code;
        fs::remove_all(path, code);
    }
};

void writeFile(const fs::path& path, const std::string& text = "x")
{
    std::error_code code;
    fs::create_directories(path.parent_path(), code);

    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    stream << text;
}

/// The library the cases below browse: folders, a project package, a plugin
/// bundle, audio INCDAW can read and audio it cannot, and a dot-file.
void buildLibrary(const fs::path& root)
{
    writeFile(root / ".hidden.wav");
    writeFile(root / "Bass.WAV");
    writeFile(root / "take.aiff");
    writeFile(root / "loop.mid");
    writeFile(root / "notes.txt");

    writeFile(root / "drums" / "kick.wav");
    writeFile(root / "drums" / "sub" / "snare.wav");

    std::error_code code;
    fs::create_directories(root / "Alpha", code);

    // A package: a directory with a manifest in it (project::ProjectFile).
    writeFile(root / "Song.incdaw" / "manifest.json", "{}");
    writeFile(root / "Song.incdaw" / "inside.wav");

    // A bundle, which on macOS is also a directory.
    writeFile(root / "Delay.clap" / "inside.wav");
}

std::vector<std::string> namesOf(const std::vector<BrowserItem>& items)
{
    std::vector<std::string> names;
    names.reserve(items.size());

    for (const BrowserItem& item : items)
        names.push_back(item.name);

    return names;
}

} // namespace

TEST_CASE("The browser says what a path is, and whether it can be read")
{
    ScratchDir scratch("incdaw-browser-classify");
    buildLibrary(scratch.path);

    CHECK(Browser::classify(scratch.path / "drums") == BrowserItemKind::folder);
    CHECK(Browser::classify(scratch.path / "Song.incdaw") == BrowserItemKind::project);
    CHECK(Browser::classify(scratch.path / "Delay.clap") == BrowserItemKind::plugin);
    CHECK(Browser::classify(scratch.path / "Bass.WAV") == BrowserItemKind::audio);
    CHECK(Browser::classify(scratch.path / "take.aiff") == BrowserItemKind::audio);
    CHECK(Browser::classify(scratch.path / "loop.mid") == BrowserItemKind::midi);
    CHECK(Browser::classify(scratch.path / "notes.txt") == BrowserItemKind::unknown);

    // A package without the extension is still a package: the manifest decides.
    writeFile(scratch.path / "Unnamed" / "manifest.json", "{}");
    CHECK(Browser::classify(scratch.path / "Unnamed") == BrowserItemKind::project);

    // A path that is gone classifies by name, which is what a stale favourite
    // needs in order to keep its icon.
    CHECK(Browser::classify(scratch.path / "vanished.wav") == BrowserItemKind::audio);

    // Audio INCDAW names is not the same as audio INCDAW can decode.
    CHECK(Browser::canDecodeAudio(scratch.path / "Bass.WAV"));
    CHECK_FALSE(Browser::canDecodeAudio(scratch.path / "take.aiff"));
    CHECK_FALSE(Browser::canDecodeAudio(scratch.path / "loop.mid"));
}

TEST_CASE("A listing is folders first, case-insensitive, without dot-files")
{
    ScratchDir scratch("incdaw-browser-listing");
    buildLibrary(scratch.path);

    Browser     browser;
    std::string error;

    const auto items = browser.list(scratch.path, error);

    CHECK(error.empty());
    CHECK(namesOf(items)
          == std::vector<std::string>{"Alpha", "drums", "Bass.WAV", "Delay.clap", "loop.mid",
                                      "notes.txt", "Song.incdaw", "take.aiff"});

    // Sizes come from the listing, so a pane can show them without another pass.
    const auto bass = std::find_if(items.begin(), items.end(),
                                   [](const BrowserItem& item) { return item.name == "Bass.WAV"; });
    REQUIRE(bass != items.end());
    CHECK(bass->sizeBytes == 1);
    CHECK(bass->exists);
}

TEST_CASE("An unreadable folder is a message, not a crash")
{
    ScratchDir scratch("incdaw-browser-errors");
    writeFile(scratch.path / "file.wav");

    Browser     browser;
    std::string error;

    CHECK(browser.list(scratch.path / "nowhere", error).empty());
    CHECK_FALSE(error.empty());

    CHECK(browser.list(scratch.path / "file.wav", error).empty());
    CHECK_FALSE(error.empty());
}

TEST_CASE("Search walks folders, in tree order, and never into a package")
{
    ScratchDir scratch("incdaw-browser-search");
    buildLibrary(scratch.path);

    Browser browser;

    const auto kick = browser.search(scratch.path, "kick");
    REQUIRE(kick.size() == 1);
    CHECK(kick.front().path == scratch.path / "drums" / "kick.wav");

    // Case-insensitive, and deeper than one level.
    const auto snare = browser.search(scratch.path, "SNARE");
    REQUIRE(snare.size() == 1);
    CHECK(snare.front().path == scratch.path / "drums" / "sub" / "snare.wav");

    // Both packages contain an inside.wav. Neither is descended into.
    CHECK(browser.search(scratch.path, "inside").empty());

    // Shallow matches come before deep ones.
    const auto wav = browser.search(scratch.path, "wav");
    REQUIRE(wav.size() == 3);
    CHECK(wav[0].name == "Bass.WAV");
    CHECK(wav[1].name == "kick.wav");
    CHECK(wav[2].name == "snare.wav");

    // The cap is honoured, and an empty query searches for nothing.
    CHECK(browser.search(scratch.path, "wav", 2).size() == 2);
    CHECK(browser.search(scratch.path, "").empty());
}

TEST_CASE("Favourites are marked in listings and survive a save")
{
    ScratchDir scratch("incdaw-browser-favourites");
    buildLibrary(scratch.path);

    Browser browser;
    CHECK(browser.toggleFavourite(scratch.path / "Bass.WAV"));
    CHECK(browser.isFavourite(scratch.path / "Bass.WAV"));

    std::string error;
    const auto  items = browser.list(scratch.path, error);
    const auto  bass  = std::find_if(items.begin(), items.end(),
                                     [](const BrowserItem& item) { return item.name == "Bass.WAV"; });
    REQUIRE(bass != items.end());
    CHECK(bass->favourite);

    // A favourite whose file moved is reported, not dropped.
    browser.setFavourite(scratch.path / "gone" / "old.wav", true);

    const auto favourites = browser.favouriteItems();
    REQUIRE(favourites.size() == 2);
    CHECK(favourites[0].exists);
    CHECK_FALSE(favourites[1].exists);
    CHECK(favourites[1].kind == BrowserItemKind::audio);

    CHECK_FALSE(browser.toggleFavourite(scratch.path / "Bass.WAV"));
    CHECK(browser.favourites().size() == 1);
}

TEST_CASE("Recents are most recent first, deduplicated and capped")
{
    Browser browser;

    for (int index = 0; index < 30; ++index)
        browser.noteRecent(fs::path("/library") / (std::to_string(index) + ".wav"));

    REQUIRE(browser.recent().size() == Browser::recentLimit);
    CHECK(browser.recent().front() == fs::path("/library/29.wav"));

    browser.noteRecent(fs::path("/library/20.wav"));
    CHECK(browser.recent().front() == fs::path("/library/20.wav"));
    CHECK(browser.recent().size() == Browser::recentLimit);

    std::size_t appearances = 0;
    for (const fs::path& path : browser.recent())
        appearances += path == fs::path("/library/20.wav") ? 1u : 0u;

    CHECK(appearances == 1);
}

TEST_CASE("Browser settings round-trip, and a first launch is not a failure")
{
    ScratchDir scratch("incdaw-browser-settings");
    buildLibrary(scratch.path);

    const fs::path settings = scratch.path / "state" / "browser.json";

    Browser saved;
    saved.addRoot("Library", scratch.path);
    saved.addRoot("Drums", scratch.path / "drums");
    saved.setFavourite(scratch.path / "Bass.WAV", true);
    saved.noteRecent(scratch.path / "loop.mid");
    saved.noteRecent(scratch.path / "Bass.WAV");

    std::string error;
    REQUIRE(saved.save(settings, error));
    CHECK(error.empty());

    Browser loaded;
    REQUIRE(loaded.load(settings, error));

    REQUIRE(loaded.roots().size() == 2);
    CHECK(loaded.roots()[0].name == "Library");
    CHECK(loaded.roots()[1].path == scratch.path / "drums");
    CHECK(loaded.isFavourite(scratch.path / "Bass.WAV"));
    REQUIRE(loaded.recent().size() == 2);
    CHECK(loaded.recent().front() == scratch.path / "Bass.WAV");

    // Saving twice writes the same bytes: settings are a file under version
    // control of the user's habits, and churn in it is noise.
    std::string first;
    std::string second;
    {
        std::ifstream stream(settings, std::ios::binary);
        first.assign(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
    }
    REQUIRE(loaded.save(settings, error));
    {
        std::ifstream stream(settings, std::ios::binary);
        second.assign(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
    }
    CHECK(first == second);

    // No file at all: a clean, empty browser and no error.
    Browser fresh;
    CHECK(fresh.load(scratch.path / "never-written.json", error));
    CHECK(error.empty());
    CHECK(fresh.roots().empty());

    // A file that is there but broken IS an error, and leaves nothing behind.
    writeFile(scratch.path / "broken.json", "{ this is not json");
    Browser refused;
    refused.addRoot("Stale", scratch.path);
    CHECK_FALSE(refused.load(scratch.path / "broken.json", error));
    CHECK_FALSE(error.empty());
    CHECK(refused.roots().empty());
}

TEST_CASE("Default roots are the folders that actually exist")
{
    ScratchDir scratch("incdaw-browser-defaults");

    std::error_code code;
    fs::create_directories(scratch.path / "Music", code);
    fs::create_directories(scratch.path / "Downloads", code);

    Browser browser;
    browser.addDefaultRoots(scratch.path);

    REQUIRE(browser.roots().size() == 2);
    CHECK(browser.roots()[0].name == "Music");
    CHECK(browser.roots()[1].name == "Downloads");

    // Adding them again neither duplicates nor reorders.
    browser.addDefaultRoots(scratch.path);
    CHECK(browser.roots().size() == 2);

    CHECK(browser.removeRoot(scratch.path / "Music"));
    CHECK(browser.roots().size() == 1);
    CHECK_FALSE(browser.removeRoot(scratch.path / "Music"));
}
