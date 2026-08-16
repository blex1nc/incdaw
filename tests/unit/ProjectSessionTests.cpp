#include "doctest.h"

#include "app/ProjectSession.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

using namespace incdaw;
namespace fs = std::filesystem;

namespace {

/// A fresh directory per fixture, removed up front so a crashed previous run
/// cannot leak state into this one.
struct SessionFixture final {
    fs::path root;

    SessionFixture()
    {
        root = fs::temp_directory_path() / "incdaw-project-session-tests";
        fs::remove_all(root);
        fs::create_directories(root);
    }

    ~SessionFixture() { fs::remove_all(root); }

    /// A minimal package: a directory holding a project.json whose mtime is
    /// `secondsAgo` seconds in the past, so ordering is deterministic without
    /// sleeping.
    fs::path makePackage(const std::string& name, int secondsAgo) const
    {
        const fs::path package = root / name;
        fs::create_directories(package);

        std::ofstream{package / "project.json"} << "{}";

        fs::last_write_time(package / "project.json",
                            fs::file_time_type::clock::now()
                                - std::chrono::seconds{secondsAgo});
        return package;
    }
};

} // namespace

TEST_CASE("updatedRecents inserts a new path at the front")
{
    const auto list = app::session::updatedRecents({"/a.incdaw", "/b.incdaw"},
                                                   "/c.incdaw", 10);

    REQUIRE(list.size() == 3);
    CHECK(list[0] == "/c.incdaw");
    CHECK(list[1] == "/a.incdaw");
    CHECK(list[2] == "/b.incdaw");
}

TEST_CASE("updatedRecents moves a known path to the front without duplicating")
{
    const auto list = app::session::updatedRecents(
        {"/a.incdaw", "/b.incdaw", "/c.incdaw"}, "/b.incdaw", 10);

    REQUIRE(list.size() == 3);
    CHECK(list[0] == "/b.incdaw");
    CHECK(list[1] == "/a.incdaw");
    CHECK(list[2] == "/c.incdaw");
}

TEST_CASE("updatedRecents caps the list from the back")
{
    const auto list = app::session::updatedRecents({"/a", "/b", "/c"}, "/d", 3);

    REQUIRE(list.size() == 3);
    CHECK(list[0] == "/d");
    CHECK(list[1] == "/a");
    CHECK(list[2] == "/b");
}

TEST_CASE("updatedRecents ignores an empty path and a zero cap")
{
    const std::vector<std::string> original{"/a", "/b"};

    CHECK(app::session::updatedRecents(original, "", 10) == original);
    CHECK(app::session::updatedRecents(original, "/c", 0) == original);
}

TEST_CASE("autosavePathFor places a saved project's autosave beside it")
{
    const fs::path autosave =
        app::session::autosavePathFor("/music/Song.incdaw", "/support");

    CHECK(autosave == fs::path{"/music/Song.autosave.incdaw"});
}

TEST_CASE("autosavePathFor places an unsaved project's autosave in support")
{
    const fs::path autosave = app::session::autosavePathFor({}, "/support");

    CHECK(autosave == fs::path{"/support/Autosave/Untitled.autosave.incdaw"});
}

TEST_CASE("autosavePathFor is empty when there is nowhere to aim")
{
    CHECK(app::session::autosavePathFor({}, {}).empty());
}

TEST_CASE("autosaveIsNewer requires both packages and a strictly newer autosave")
{
    SessionFixture temp;

    const fs::path project  = temp.makePackage("Song.incdaw", 60);
    const fs::path autosave = temp.makePackage("Song.autosave.incdaw", 10);

    CHECK(app::session::autosaveIsNewer(project, autosave));
    CHECK_FALSE(app::session::autosaveIsNewer(autosave, project));

    CHECK_FALSE(app::session::autosaveIsNewer(project, temp.root / "missing.incdaw"));
    CHECK_FALSE(app::session::autosaveIsNewer(temp.root / "missing.incdaw", autosave));
}
