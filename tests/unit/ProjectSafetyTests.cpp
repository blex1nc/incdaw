#include "doctest.h"

#include "app/RecentProjects.h"
#include "project/Autosave.h"

#include <filesystem>
#include <fstream>

using namespace incdaw;

namespace {

/// A scratch directory this test owns outright, emptied on construction and
/// destruction so no case inherits another's files.
struct ScratchDirectory {
    std::filesystem::path path;

    ScratchDirectory()
        : path(std::filesystem::temp_directory_path() / "incdaw-project-safety-tests")
    {
        std::filesystem::remove_all(path);
        std::filesystem::create_directories(path);
    }

    ~ScratchDirectory() { std::filesystem::remove_all(path); }
};

/// An autosave "package" as the pruner sees one: a directory with a file in
/// it, so removal exercises remove_all rather than remove.
void makeFakePackage(const std::filesystem::path& package)
{
    std::filesystem::create_directories(package);
    std::ofstream{package / "project.json"} << "{}";
}

} // namespace

// ── Autosave naming ───────────────────────────────────────────────────────────

TEST_CASE("autosave stamps are zero-padded and ordered like time")
{
    // A fixed instant, mid-January noon UTC: nowhere near any DST boundary,
    // so local rendering is monotonic across the offsets under test.
    const auto base = std::chrono::system_clock::time_point{std::chrono::seconds{1768478400}};

    const std::string first  = project::Autosave::stampFor(base);
    const std::string second = project::Autosave::stampFor(base + std::chrono::seconds{1});
    const std::string third  = project::Autosave::stampFor(base + std::chrono::seconds{61});

    CHECK(first.size() == 15);            // YYYYMMDD-HHMMSS
    CHECK(first[8] == '-');
    CHECK(first < second);
    CHECK(second < third);

    const auto path = project::Autosave::pathFor("/autosaves", "Song", base);
    CHECK(path.filename().string() == "Song.autosave-" + first + ".incdaw");
    CHECK(path.parent_path() == "/autosaves");
}

TEST_CASE("autosave listing is per project, oldest first, and tolerates a missing directory")
{
    ScratchDirectory scratch;

    makeFakePackage(scratch.path / "Song.autosave-20260816-120000.incdaw");
    makeFakePackage(scratch.path / "Song.autosave-20260816-100000.incdaw");
    makeFakePackage(scratch.path / "Other.autosave-20260816-110000.incdaw");
    makeFakePackage(scratch.path / "Song.incdaw");   // a real project is not an autosave

    const auto found = project::Autosave::list(scratch.path, "Song");
    REQUIRE(found.size() == 2);
    CHECK(found[0].filename().string() == "Song.autosave-20260816-100000.incdaw");
    CHECK(found[1].filename().string() == "Song.autosave-20260816-120000.incdaw");

    CHECK(project::Autosave::list(scratch.path / "does-not-exist", "Song").empty());
}

TEST_CASE("pruning keeps the newest autosaves and deletes whole packages")
{
    ScratchDirectory scratch;

    for (int hour = 10; hour < 15; ++hour) {
        makeFakePackage(scratch.path
                        / ("Song.autosave-20260816-1" + std::to_string(hour) + "000.incdaw"));
    }

    CHECK(project::Autosave::prune(scratch.path, "Song", 2) == 3);

    const auto kept = project::Autosave::list(scratch.path, "Song");
    REQUIRE(kept.size() == 2);
    CHECK(kept[0].filename().string() == "Song.autosave-20260816-113000.incdaw");
    CHECK(kept[1].filename().string() == "Song.autosave-20260816-114000.incdaw");

    // Under the limit: nothing to do.
    CHECK(project::Autosave::prune(scratch.path, "Song", 2) == 0);
}

// ── Recent projects ───────────────────────────────────────────────────────────

TEST_CASE("recent projects: newest first, deduplicated, capped")
{
    std::vector<std::string> list;

    list = app::RecentProjects::updated(list, "/a.incdaw");
    list = app::RecentProjects::updated(list, "/b.incdaw");
    CHECK(list == std::vector<std::string>{"/b.incdaw", "/a.incdaw"});

    // Reopening an old project moves it up rather than duplicating it.
    list = app::RecentProjects::updated(list, "/a.incdaw");
    CHECK(list == std::vector<std::string>{"/a.incdaw", "/b.incdaw"});

    for (int index = 0; index < 20; ++index)
        list = app::RecentProjects::updated(list, "/p" + std::to_string(index) + ".incdaw");

    CHECK(list.size() == app::RecentProjects::maximumCount);
    CHECK(list.front() == "/p19.incdaw");

    list = app::RecentProjects::without(list, "/p19.incdaw");
    CHECK(list.front() == "/p18.incdaw");
    CHECK(list.size() == app::RecentProjects::maximumCount - 1);
}
