// Phase 13 — the editor bridge (CLAP_EXT_GUI), headless.
//
// The bridge's job is a strict call sequence — create, scale, size, parent,
// show; hide, destroy — with no editor left behind on any refusal. The test
// gain plugin records those calls instead of drawing, which is all a
// headless test can honestly verify; pixels belong to a windowed session.

#include "doctest.h"

#include "plugins/clap/ClapLibrary.h"

#include <cstdint>
#include <string>

using namespace incdaw;

namespace {

constexpr std::uint32_t blockSize = 256;

} // namespace

TEST_CASE("the editor opens embedded, reports its size, and closes cleanly")
{
    plugins::ClapLibrary library;
    std::string          error;
    REQUIRE(library.open(INCDAW_TESTGAIN_PLUGIN, error));

    auto instance = library.create("com.incdaw.testgain", 48000.0, blockSize, error);
    REQUIRE(instance != nullptr);

    CHECK(instance->hasEditor());
    CHECK(!instance->isEditorOpen());

    int   parentDummy = 0;
    void* parent      = &parentDummy;

    std::uint32_t width  = 0;
    std::uint32_t height = 0;

    REQUIRE(instance->openEditor(parent, width, height));
    CHECK(instance->isEditorOpen());
    CHECK(width == 300);
    CHECK(height == 200);

    // A second open while one is up is refused, not stacked.
    CHECK(!instance->openEditor(parent, width, height));

    instance->closeEditor();
    CHECK(!instance->isEditorOpen());

    // Close twice is safe; reopen works.
    instance->closeEditor();
    REQUIRE(instance->openEditor(parent, width, height));

    // Destruction with an editor still open must not double-free: the
    // destructor closes it first. Reaching the end of scope IS the test.
}

TEST_CASE("no parent, no editor — and a plugin without a gui refuses politely")
{
    plugins::ClapLibrary library;
    std::string          error;
    REQUIRE(library.open(INCDAW_TESTGAIN_PLUGIN, error));

    auto instance = library.create("com.incdaw.testgain", 48000.0, blockSize, error);
    REQUIRE(instance != nullptr);

    std::uint32_t width  = 0;
    std::uint32_t height = 0;
    CHECK(!instance->openEditor(nullptr, width, height));
    CHECK(!instance->isEditorOpen());

    plugins::ClapLibrary latencyLibrary;
    REQUIRE(latencyLibrary.open(INCDAW_TESTLATENCY_PLUGIN, error));

    auto latent = latencyLibrary.create("com.incdaw.testlatency", 48000.0, blockSize, error);
    REQUIRE(latent != nullptr);

    int   parentDummy = 0;
    CHECK(!latent->hasEditor());
    CHECK(!latent->openEditor(&parentDummy, width, height));
}
