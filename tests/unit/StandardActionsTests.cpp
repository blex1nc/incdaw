// The action table the palette and the menu bar search.
//
// These tests exist because the table used to be built inside the macOS shell,
// where nothing could reach it: an action that fails to construct, names its
// channel wrongly, or cannot be undone would only be discovered by a person
// running the application and noticing. Registration is app-layer logic now,
// so the guarantees an action must hold — addressable by id, reversible,
// named from the project as it stands when it runs — are checked here.

#include "doctest.h"

#include "app/CommandRegistry.h"
#include "app/StandardActions.h"
#include "project/Model.h"

#include <string>

using namespace incdaw;
using incdaw::app::CommandRegistry;

namespace {

/// Every standard action, so a new one cannot be added without deciding what
/// it is called and where it belongs.
constexpr const char* expectedIds[] = {"channel.add", "pattern.add", "track.add"};

} // namespace

TEST_CASE("every standard action is addressable, named and categorised")
{
    project::Project project;
    CommandRegistry  registry{project};

    app::registerStandardActions(registry);

    CHECK(registry.actions().size() == std::size(expectedIds));

    for (const char* id : expectedIds) {
        const CommandRegistry::Entry* entry = registry.findAction(id);

        REQUIRE(entry != nullptr);
        CHECK_FALSE(entry->displayName.empty());
        CHECK_FALSE(entry->category.empty());
        CHECK(entry->create != nullptr);
    }
}

TEST_CASE("registering twice replaces rather than duplicates")
{
    project::Project project;
    CommandRegistry  registry{project};

    app::registerStandardActions(registry);
    app::registerStandardActions(registry);

    // A shell that rebuilds its menus must be able to re-register without the
    // palette listing everything twice.
    CHECK(registry.actions().size() == std::size(expectedIds));
}

TEST_CASE("each standard action runs and is reversible")
{
    project::Project project;
    CommandRegistry  registry{project};

    app::registerStandardActions(registry);

    const std::size_t channels = project.channels().size();
    const std::size_t patterns = project.patterns().size();
    const std::size_t tracks   = project.tracks().size();

    for (const char* id : expectedIds)
        CHECK(registry.invoke(id));

    CHECK(project.channels().size() == channels + 1);
    CHECK(project.patterns().size() == patterns + 1);
    CHECK(project.tracks().size() == tracks + 1);

    // Reversible in the order they were run, which is what the undo stack
    // promises and what the palette's entries rely on.
    for (std::size_t undone = 0; undone < std::size(expectedIds); ++undone)
        CHECK(registry.undo());

    CHECK(project.channels().size() == channels);
    CHECK(project.patterns().size() == patterns);
    CHECK(project.tracks().size() == tracks);
    CHECK_FALSE(registry.canUndo());
}

TEST_CASE("names are minted from the project as it stands when the action runs")
{
    project::Project project;
    CommandRegistry  registry{project};

    app::registerStandardActions(registry);

    REQUIRE(registry.invoke("channel.add"));
    REQUIRE(registry.invoke("channel.add"));

    REQUIRE(project.channels().size() >= 2);

    // Not "Channel 1" twice: the count is read when the command is created,
    // not when the action was registered.
    const std::string first  = project.channels()[project.channels().size() - 2].name;
    const std::string second = project.channels().back().name;

    CHECK(first != second);
}

TEST_CASE("standard actions are findable by command search")
{
    project::Project project;
    CommandRegistry  registry{project};

    app::registerStandardActions(registry);

    // What someone typing into the palette actually types.
    CHECK(registry.search("channel").size() == 1);
    CHECK(registry.search("Add").size() == std::size(expectedIds));
    CHECK(registry.search("playlist").size() == 1);   // by category
}
