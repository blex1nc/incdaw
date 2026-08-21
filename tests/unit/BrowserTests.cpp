// The browser's file-system side (CLAUDE.md §19).
//
// The interesting properties here are all about what a browser must survive
// rather than what it displays: a folder it cannot read, a search over a tree
// too large to walk, a favourite on a volume that has been unmounted. A
// browser that throws on any of those takes an AppKit callback down with it,
// so every one of them is a test.

#include "doctest.h"

#include "app/BrowserModel.h"

#include <filesystem>
#include <fstream>
#include <string>

using namespace incdaw;

namespace {

std::filesystem::path makeTree(const char* name)
{
    const std::filesystem::path root =
        std::filesystem::temp_directory_path() / "incdaw-browser-tests" / name;

    std::error_code failed;
    std::filesystem::remove_all(root, failed);
    std::filesystem::create_directories(root, failed);

    return root;
}

void touch(const std::filesystem::path& file, std::size_t bytes = 4)
{
    std::error_code failed;
    std::filesystem::create_directories(file.parent_path(), failed);

    std::ofstream stream(file, std::ios::binary | std::ios::trunc);
    stream << std::string(bytes, 'x');
}

bool listed(const std::vector<app::BrowserItem>& items, const std::string& name)
{
    for (const app::BrowserItem& item : items)
        if (item.name == name)
            return true;

    return false;
}

} // namespace

TEST_CASE("extensions classify into the kinds the browser can act on")
{
    using app::BrowserItemKind;
    using app::BrowserModel;

    CHECK(BrowserModel::kindOf("/x/kick.wav")        == BrowserItemKind::audio);
    CHECK(BrowserModel::kindOf("/x/kick.WAV")        == BrowserItemKind::audio);   // case-insensitive
    CHECK(BrowserModel::kindOf("/x/loop.aiff")       == BrowserItemKind::audio);
    CHECK(BrowserModel::kindOf("/x/part.mid")        == BrowserItemKind::midi);
    CHECK(BrowserModel::kindOf("/x/song.incdaw")     == BrowserItemKind::project);

    // A plugin state blob is a preset, never a project: the two live side by
    // side in a project folder and opening the wrong one must be impossible.
    CHECK(BrowserModel::kindOf("/x/reverb.incdawstate") == BrowserItemKind::preset);

    CHECK(BrowserModel::kindOf("/x/readme.txt")      == BrowserItemKind::other);
    CHECK(BrowserModel::kindOf("/x/no-extension")    == BrowserItemKind::other);
}

TEST_CASE("a folder lists folders first, then files, case-insensitively")
{
    const std::filesystem::path root = makeTree("listing");

    touch(root / "zap.wav");
    touch(root / "Kick.wav");
    touch(root / "notes.txt");
    touch(root / "Drums" / "snare.wav");
    touch(root / "ambience" / "pad.wav");

    app::BrowserModel browser;
    const std::vector<app::BrowserItem> items = browser.childrenOf(root);

    REQUIRE(items.size() == 5);
    CHECK(items[0].name == "ambience");
    CHECK(items[1].name == "Drums");
    CHECK(items[0].isFolder());
    CHECK(items[1].isFolder());
    CHECK(items[2].name == "Kick.wav");
    CHECK(items[3].name == "notes.txt");
    CHECK(items[4].name == "zap.wav");

    CHECK(items[2].kind == app::BrowserItemKind::audio);
    CHECK(items[2].sizeBytes == 4);
}

TEST_CASE("hidden entries and macOS bundles are skipped")
{
    const std::filesystem::path root = makeTree("hidden");

    touch(root / "kick.wav");
    touch(root / ".DS_Store");
    touch(root / ".hidden" / "secret.wav");
    touch(root / "Some.app" / "Contents" / "kick.wav");

    app::BrowserModel browser;
    const std::vector<app::BrowserItem> items = browser.childrenOf(root);

    REQUIRE(items.size() == 1);
    CHECK(items[0].name == "kick.wav");

    browser.addRoot(root);
    CHECK(browser.search("kick").size() == 1);       // not the one inside the bundle
}

TEST_CASE("an unreadable or missing folder lists as empty rather than throwing")
{
    app::BrowserModel browser;

    CHECK(browser.childrenOf("/nowhere/at/all").empty());
    CHECK(browser.childrenOf("").empty());

    const std::filesystem::path file = makeTree("notadir") / "kick.wav";
    touch(file);
    CHECK(browser.childrenOf(file).empty());          // a file is not a folder
}

TEST_CASE("the kind filter hides files but never the folders that lead to them")
{
    const std::filesystem::path root = makeTree("filter");

    touch(root / "kick.wav");
    touch(root / "part.mid");
    touch(root / "readme.txt");
    touch(root / "Deeper" / "snare.wav");

    app::BrowserModel browser;
    browser.setVisibleKinds(app::BrowserModel::bitFor(app::BrowserItemKind::audio));

    const std::vector<app::BrowserItem> items = browser.childrenOf(root);

    CHECK(listed(items, "kick.wav"));
    CHECK(listed(items, "Deeper"));                   // the path to a filtered-in file
    CHECK_FALSE(listed(items, "part.mid"));
    CHECK_FALSE(listed(items, "readme.txt"));
}

TEST_CASE("search matches names case-insensitively across roots")
{
    const std::filesystem::path first  = makeTree("search-a");
    const std::filesystem::path second = makeTree("search-b");

    touch(first / "Drums" / "Kick_01.wav");
    touch(first / "Drums" / "snare.wav");
    touch(second / "Bass" / "kick-sub.wav");
    touch(second / "Bass" / "bass.wav");

    app::BrowserModel browser;
    browser.addRoot(first);
    browser.addRoot(second);

    const std::vector<app::BrowserItem> hits = browser.search("KICK");

    REQUIRE(hits.size() == 2);
    CHECK(listed(hits, "Kick_01.wav"));
    CHECK(listed(hits, "kick-sub.wav"));
    CHECK_FALSE(browser.lastSearchWasTruncated());
}

TEST_CASE("an empty query returns nothing, not everything")
{
    const std::filesystem::path root = makeTree("empty-query");
    touch(root / "kick.wav");

    app::BrowserModel browser;
    browser.addRoot(root);

    CHECK(browser.search("").empty());
}

TEST_CASE("search stops at the result ceiling and says so")
{
    const std::filesystem::path root = makeTree("ceiling");

    for (int index = 0; index < 40; ++index)
        touch(root / ("kick-" + std::to_string(index) + ".wav"));

    app::BrowserModel browser;
    browser.addRoot(root);
    browser.setMaximumResults(10);

    const std::vector<app::BrowserItem> hits = browser.search("kick");

    CHECK(hits.size() == 10);
    CHECK(browser.lastSearchWasTruncated());
}

TEST_CASE("search stops descending at the depth ceiling")
{
    const std::filesystem::path root = makeTree("depth");

    touch(root / "shallow-kick.wav");
    touch(root / "a" / "b" / "c" / "d" / "deep-kick.wav");

    app::BrowserModel browser;
    browser.addRoot(root);
    browser.setMaximumDepth(2);

    const std::vector<app::BrowserItem> hits = browser.search("kick");

    CHECK(listed(hits, "shallow-kick.wav"));
    CHECK_FALSE(listed(hits, "deep-kick.wav"));

    browser.setMaximumDepth(app::BrowserModel::defaultMaximumDepth);
    CHECK(listed(browser.search("kick"), "deep-kick.wav"));
}

TEST_CASE("a missing root is skipped, not fatal")
{
    const std::filesystem::path root = makeTree("mixed-roots");
    touch(root / "kick.wav");

    app::BrowserModel browser;
    browser.addRoot("/definitely/not/here");
    browser.addRoot(root);

    CHECK(browser.search("kick").size() == 1);
}

TEST_CASE("roots are de-duplicated and named from the folder")
{
    const std::filesystem::path root = makeTree("roots");

    app::BrowserModel browser;
    browser.addRoot(root);
    browser.addRoot(root);                            // the same folder twice
    browser.addRoot("", "ignored");

    REQUIRE(browser.roots().size() == 1);
    CHECK(browser.roots()[0].name == "roots");

    browser.addRoot(root / "sub", "My Samples");
    CHECK(browser.roots()[1].name == "My Samples");

    browser.removeRoot(root);
    REQUIRE(browser.roots().size() == 1);
    CHECK(browser.roots()[0].name == "My Samples");
}

TEST_CASE("favourites toggle, survive a round trip, and outlive their files")
{
    const std::filesystem::path root = makeTree("favourites");
    touch(root / "kick.wav", 128);

    app::BrowserModel browser;
    browser.toggleFavourite(root / "kick.wav");
    CHECK(browser.isFavourite(root / "kick.wav"));

    REQUIRE(browser.favourites().size() == 1);
    CHECK(browser.favourites()[0].kind == app::BrowserItemKind::audio);
    CHECK(browser.favourites()[0].sizeBytes == 128);

    // The listing marks it, which is what draws the star in the UI.
    CHECK(browser.childrenOf(root)[0].favourite);

    // A favourite on an unmounted volume is not a mistake to be corrected.
    std::filesystem::remove(root / "kick.wav");
    REQUIRE(browser.favourites().size() == 1);
    CHECK(browser.favourites()[0].sizeBytes == 0);

    browser.toggleFavourite(root / "kick.wav");
    CHECK_FALSE(browser.isFavourite(root / "kick.wav"));
    CHECK(browser.favourites().empty());

    browser.setFavourites({root / "a.wav", root / "b.wav", root / "a.wav"});
    CHECK(browser.favouritePaths().size() == 2);      // de-duplicated
}

TEST_CASE("a project package is a project, not a folder to walk into")
{
    // .incdaw is a package: a directory as far as the file system is concerned,
    // one document as far as the user is. Listing it as a folder would invite
    // browsing its internals instead of opening it.
    const std::filesystem::path root = makeTree("packages");

    touch(root / "Song.incdaw" / "project.json");
    touch(root / "Song.incdaw" / "audio" / "take-kick.wav");
    touch(root / "Samples" / "kick.wav");

    app::BrowserModel browser;
    browser.addRoot(root);

    const std::vector<app::BrowserItem> items = browser.childrenOf(root);
    REQUIRE(items.size() == 2);

    // Folders sort first, so the package — being a file to the browser —
    // follows the ordinary folder.
    CHECK(items[0].name == "Samples");
    CHECK(items[0].kind == app::BrowserItemKind::folder);
    CHECK(items[1].name == "Song.incdaw");
    CHECK(items[1].kind == app::BrowserItemKind::project);
    CHECK_FALSE(items[1].isFolder());

    // And the search does not descend into it: the take inside the package is
    // not a sample the user can browse to.
    const std::vector<app::BrowserItem> hits = browser.search("kick");
    REQUIRE(hits.size() == 1);
    CHECK(hits[0].name == "kick.wav");

    CHECK(listed(browser.search("Song"), "Song.incdaw"));
}
