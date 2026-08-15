#include "doctest.h"

#include "plugins/PluginIdentifier.h"

using namespace incdaw::plugins;

TEST_CASE("format names are stable — project files depend on them")
{
    // Changing any of these strings breaks every project file that references
    // a plugin. If it must change, it needs a project format migration.
    CHECK(std::string{formatName(Format::clap)} == "clap");
    CHECK(std::string{formatName(Format::audioUnit)} == "au");
    CHECK(std::string{formatName(Format::vst3)} == "vst3");
    CHECK(std::string{formatName(Format::builtin)} == "builtin");
}

TEST_CASE("identifiers round-trip through their serialized form")
{
    const PluginIdentifier originals[] = {
        {Format::clap,      "com.acme.reverb"},
        {Format::audioUnit, "aufx:dely:appl"},
        {Format::vst3,      "56535444656C61792D42617A"},
        builtinSampler(),
        builtinSimpleSynth(),
    };

    for (const auto& original : originals) {
        PluginIdentifier parsed;
        REQUIRE(PluginIdentifier::fromString(original.toString(), parsed));
        CHECK(parsed == original);
    }
}

TEST_CASE("uids containing colons survive parsing")
{
    // Audio Unit ids are colon-separated by nature. Splitting on the last colon
    // instead of the first would silently truncate them.
    PluginIdentifier parsed;
    REQUIRE(PluginIdentifier::fromString("au:aufx:dely:appl", parsed));
    CHECK(parsed.format == Format::audioUnit);
    CHECK(parsed.uid == "aufx:dely:appl");
}

TEST_CASE("malformed identifiers are rejected rather than guessed at")
{
    PluginIdentifier parsed;
    CHECK_FALSE(PluginIdentifier::fromString("", parsed));
    CHECK_FALSE(PluginIdentifier::fromString("clap", parsed));
    CHECK_FALSE(PluginIdentifier::fromString("clap:", parsed));
    CHECK_FALSE(PluginIdentifier::fromString(":uid", parsed));
    CHECK_FALSE(PluginIdentifier::fromString("vst2:legacy", parsed));  // deliberately unsupported
}

TEST_CASE("an identifier without a uid is not valid")
{
    CHECK_FALSE(PluginIdentifier{Format::clap, ""}.isValid());
    CHECK(PluginIdentifier{Format::clap, "x"}.isValid());
}
