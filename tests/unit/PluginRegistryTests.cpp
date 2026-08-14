// Phase 13 (part 2) — the plugin registry and its blacklist.
//
// The load-bearing properties: an unchanged library never spawns another
// scan, a crashing one lands on the blacklist WITH its reason and stays
// skipped, clearing the blacklist earns a retry, and the whole catalogue
// round-trips through its versioned file so startup touches no binaries.

#include "doctest.h"

#include "plugins/PluginRegistry.h"

#include <filesystem>
#include <fstream>

using namespace incdaw;
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

/// A plugin folder holding copies of the suite's own plugins, so mtime and
/// content are ours to manipulate.
struct PluginFolder {
    ScratchDir dir{"incdaw-registry-plugins"};
    fs::path   gain;
    fs::path   crash;

    PluginFolder()
    {
        gain  = dir.path / "gain.clap";
        crash = dir.path / "crash.clap";
        fs::copy_file(INCDAW_TESTGAIN_PLUGIN, gain);
        fs::copy_file(INCDAW_TESTCRASH_PLUGIN, crash);
    }
};

} // namespace

TEST_CASE("scanning catalogues the healthy and blacklists the hostile")
{
    PluginFolder folder;
    plugins::PluginRegistry registry;

    const auto scans = registry.scanDirectory(folder.dir.path, INCDAW_PLUGINSCAN_BINARY);
    CHECK(scans == 2);

    REQUIRE(registry.libraries().size() == 2);

    const auto located = registry.find("com.incdaw.testgain");
    REQUIRE(located.plugin != nullptr);
    CHECK(located.plugin->name == "INCDAW Test Gain");
    CHECK(located.library->path == folder.gain.string());

    // The crash library is known, blacklisted, and says why.
    bool sawBlacklisted = false;
    for (const auto& library : registry.libraries()) {
        if (library.path == folder.crash.string()) {
            sawBlacklisted = true;
            CHECK(library.blacklisted);
            CHECK(library.blacklistReason.find("signal") != std::string::npos);
            CHECK(library.plugins.empty());
        }
    }
    CHECK(sawBlacklisted);

    CHECK(registry.plugins().size() == 1);   // the blacklisted one is not offered
}

TEST_CASE("an unchanged library never spawns another scan — even a blacklisted one")
{
    PluginFolder folder;
    plugins::PluginRegistry registry;

    (void)registry.scanDirectory(folder.dir.path, INCDAW_PLUGINSCAN_BINARY);

    // Second pass over identical files: zero child processes.
    CHECK(registry.scanDirectory(folder.dir.path, INCDAW_PLUGINSCAN_BINARY) == 0);

    // Touch the healthy one: exactly it rescans.
    fs::last_write_time(folder.gain,
                        fs::last_write_time(folder.gain) + std::chrono::seconds(2));
    CHECK(registry.scanDirectory(folder.dir.path, INCDAW_PLUGINSCAN_BINARY) == 1);
}

TEST_CASE("clearing the blacklist earns the hostile library a retry")
{
    PluginFolder folder;
    plugins::PluginRegistry registry;

    (void)registry.scanDirectory(folder.dir.path, INCDAW_PLUGINSCAN_BINARY);
    CHECK(registry.scanDirectory(folder.dir.path, INCDAW_PLUGINSCAN_BINARY) == 0);

    registry.clearBlacklist();

    // The retry re-examines only the forgotten library — and it crashes
    // again, so it is blacklisted again. The user asked; the file answered.
    CHECK(registry.scanDirectory(folder.dir.path, INCDAW_PLUGINSCAN_BINARY) == 1);

    std::size_t blacklisted = 0;
    for (const auto& library : registry.libraries())
        blacklisted += library.blacklisted ? 1 : 0;
    CHECK(blacklisted == 1);
}

TEST_CASE("the catalogue round-trips through its file, blacklist included")
{
    PluginFolder folder;
    ScratchDir   store{"incdaw-registry-store"};

    plugins::PluginRegistry registry;
    (void)registry.scanDirectory(folder.dir.path, INCDAW_PLUGINSCAN_BINARY);

    REQUIRE(registry.save(store.path / "registry.tsv"));

    plugins::PluginRegistry loaded;
    REQUIRE(loaded.load(store.path / "registry.tsv"));

    REQUIRE(loaded.libraries().size() == registry.libraries().size());
    CHECK(loaded.find("com.incdaw.testgain").plugin != nullptr);
    CHECK(loaded.plugins().size() == 1);

    // And the loaded catalogue scans nothing for unchanged files: startup
    // touches no plugin binaries.
    CHECK(loaded.scanDirectory(folder.dir.path, INCDAW_PLUGINSCAN_BINARY) == 0);
}

TEST_CASE("an unknown registry version is refused, not guessed at")
{
    ScratchDir store{"incdaw-registry-badversion"};

    {
        std::ofstream file{store.path / "registry.tsv"};
        file << "INCDAW-PLUGIN-REGISTRY 999\n";
    }

    plugins::PluginRegistry registry;
    CHECK_FALSE(registry.load(store.path / "registry.tsv"));
}
