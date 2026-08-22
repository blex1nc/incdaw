// Presets (A5) — the portable form of an instrument's or an effect's sound.
//
// Two properties carry this file. The first is that a preset is a FILE: the
// name on disk is the name the user sees, a copy renamed in Finder is a
// different preset, and nothing INCDAW ships can be edited away. The second is
// that a preset is UNTRUSTED input — it may have been written by a newer
// build, by a different effect, or by a text editor at three in the morning —
// and every one of those must degrade into "no preset" rather than into a
// wrong sound or a crash.

#include "doctest.h"

#include "engine/dsp/effects/BuiltinEffects.h"
#include "engine/instrument/BuiltinInstruments.h"
#include "project/PresetLibrary.h"

#include <filesystem>
#include <fstream>
#include <set>
#include <string>

using namespace incdaw;
using project::Preset;
using project::PresetLibrary;

namespace {

/// A fresh directory per test: presets are files, and a test that inherits
/// yesterday's files is testing yesterday.
std::filesystem::path scratchDirectory(const char* name)
{
    const std::filesystem::path directory =
        std::filesystem::temp_directory_path() / "incdaw-preset-tests" / name;

    std::error_code failed;
    std::filesystem::remove_all(directory, failed);
    std::filesystem::create_directories(directory, failed);

    return directory;
}

void writeText(const std::filesystem::path& file, const std::string& text)
{
    std::error_code failed;
    std::filesystem::create_directories(file.parent_path(), failed);

    std::ofstream stream(file, std::ios::binary | std::ios::trunc);
    stream << text;
}

Preset samplePreset(std::string uid, std::string name)
{
    Preset preset;
    preset.uid    = std::move(uid);
    preset.name   = std::move(name);
    preset.values = {{0u, 123.5}, {2u, -4.25}};
    return preset;
}

bool hasEntry(const std::vector<PresetLibrary::Entry>& entries, std::string_view name)
{
    for (const PresetLibrary::Entry& entry : entries)
        if (entry.name == name)
            return true;

    return false;
}

} // namespace

// ── The catalogues ───────────────────────────────────────────────────────────

TEST_CASE("every catalogue entry with parameters ships factory presets")
{
    for (const engine::dsp::BuiltinEffectInfo& info : engine::dsp::builtinEffects()) {
        CAPTURE(info.uid);

        if (info.parameterCount == 0) {
            // A meter has nothing to store; a preset of nothing is nothing.
            CHECK(info.presets.count == 0);
            continue;
        }

        CHECK(info.presets.count > 0);
    }

    for (const engine::BuiltinInstrumentInfo& info : engine::builtinInstruments()) {
        CAPTURE(info.uid);
        CHECK(info.presets.count > 0);
    }
}

TEST_CASE("a factory preset only names parameters its target actually has")
{
    const auto check = [](const char* uid, const engine::dsp::EffectParameter* parameters,
                          std::size_t parameterCount,
                          engine::dsp::FactoryPresetTable presets) {
        std::set<std::uint32_t> known;
        for (std::size_t index = 0; index < parameterCount; ++index)
            known.insert(parameters[index].id);

        for (std::size_t slot = 0; slot < presets.count; ++slot) {
            const engine::dsp::FactoryPreset& preset = presets.items[slot];
            CAPTURE(uid);
            CAPTURE(preset.name);

            REQUIRE(preset.valueCount > 0);
            REQUIRE(preset.values != nullptr);

            for (std::size_t which = 0; which < preset.valueCount; ++which) {
                const engine::dsp::PresetValue& value = preset.values[which];
                CAPTURE(value.parameterId);
                CHECK(known.count(value.parameterId) == 1);

                // And within the parameter's declared range, or the panel
                // would show a slider somewhere it cannot be dragged to.
                for (std::size_t index = 0; index < parameterCount; ++index) {
                    if (parameters[index].id != value.parameterId)
                        continue;

                    CHECK(value.value >= parameters[index].minValue);
                    CHECK(value.value <= parameters[index].maxValue);
                }
            }
        }
    };

    for (const engine::dsp::BuiltinEffectInfo& info : engine::dsp::builtinEffects())
        check(info.uid, info.parameters, info.parameterCount, info.presets);

    for (const engine::BuiltinInstrumentInfo& info : engine::builtinInstruments())
        check(info.uid, info.parameters, info.parameterCount, info.presets);
}

TEST_CASE("factory preset names are unique within one target")
{
    for (const engine::dsp::BuiltinEffectInfo& info : engine::dsp::builtinEffects()) {
        std::set<std::string> seen;
        for (std::size_t slot = 0; slot < info.presets.count; ++slot) {
            CAPTURE(info.uid);
            CAPTURE(info.presets.items[slot].name);
            CHECK(seen.insert(info.presets.items[slot].name).second);
        }

        // "Default" is synthesised; a table that also declares it would give
        // the list two entries that mean different things.
        CHECK(seen.count(PresetLibrary::defaultPresetName) == 0);
    }
}

TEST_CASE("Default is the catalogue's own defaults, not a second copy of them")
{
    const engine::dsp::BuiltinEffectInfo* info = engine::dsp::findBuiltinEffect("incdaw.compressor");
    REQUIRE(info != nullptr);

    const std::vector<engine::dsp::PresetValue> values =
        project::defaultPresetValues("incdaw.compressor");

    REQUIRE(values.size() == info->parameterCount);
    for (std::size_t index = 0; index < info->parameterCount; ++index) {
        CHECK(values[index].parameterId == info->parameters[index].id);
        CHECK(values[index].value == doctest::Approx(info->parameters[index].defaultValue));
    }
}

TEST_CASE("an unknown uid has no defaults and no presets")
{
    CHECK(project::defaultPresetValues("incdaw.nothing").empty());
    CHECK(project::factoryPresetsFor("incdaw.nothing").count == 0);
    CHECK(project::factoryPresetsFor("incdaw.nothing").items == nullptr);
}

// ── The document ─────────────────────────────────────────────────────────────

TEST_CASE("a preset round-trips through its own JSON")
{
    const Preset preset = samplePreset("incdaw.eq", "My Curve");

    Preset      back;
    std::string error;
    REQUIRE(Preset::fromJson(preset.toJson(), "incdaw.eq", "My Curve", back, error));
    CHECK(error.empty());

    CHECK(back.uid == preset.uid);
    CHECK(back.name == preset.name);
    REQUIRE(back.values.size() == preset.values.size());
    for (std::size_t index = 0; index < back.values.size(); ++index) {
        CHECK(back.values[index].parameterId == preset.values[index].parameterId);
        CHECK(back.values[index].value == doctest::Approx(preset.values[index].value));
    }
}

TEST_CASE("saving the same preset twice produces the same bytes")
{
    const Preset preset = samplePreset("incdaw.eq", "Stable");
    CHECK(preset.toJson() == preset.toJson());
}

TEST_CASE("a preset for another effect is refused rather than misapplied")
{
    const Preset preset = samplePreset("incdaw.reverb", "Hall-ish");

    Preset      back;
    std::string error;
    CHECK_FALSE(Preset::fromJson(preset.toJson(), "incdaw.delay", "Hall-ish", back, error));
    CHECK_FALSE(error.empty());
}

TEST_CASE("a preset from a newer INCDAW is refused rather than half-read")
{
    const std::string text =
        R"({"format":"incdaw.preset","version":99,"uid":"incdaw.eq","name":"Future",)"
        R"("values":[{"id":0,"value":1.0}]})";

    Preset      back;
    std::string error;
    CHECK_FALSE(Preset::fromJson(text, "incdaw.eq", "Future", back, error));
    CHECK_FALSE(error.empty());
}

TEST_CASE("malformed and foreign documents degrade into no preset")
{
    Preset      back;
    std::string error;

    CHECK_FALSE(Preset::fromJson("", "incdaw.eq", "x", back, error));
    CHECK_FALSE(Preset::fromJson("{", "incdaw.eq", "x", back, error));
    CHECK_FALSE(Preset::fromJson("[]", "incdaw.eq", "x", back, error));
    CHECK_FALSE(Preset::fromJson(R"({"format":"something.else"})", "incdaw.eq", "x", back, error));
    CHECK_FALSE(Preset::fromJson(
        R"({"format":"incdaw.preset","version":1,"uid":"incdaw.eq"})", "incdaw.eq", "x",
        back, error));
}

TEST_CASE("the name comes from the caller, never from the document")
{
    const Preset preset = samplePreset("incdaw.eq", "Written As");

    Preset      back;
    std::string error;
    REQUIRE(Preset::fromJson(preset.toJson(), "incdaw.eq", "Renamed On Disk", back, error));
    CHECK(back.name == "Renamed On Disk");
}

// ── The folder ───────────────────────────────────────────────────────────────

TEST_CASE("a library with no directory still lists what INCDAW ships")
{
    const PresetLibrary library{{}};

    const std::vector<PresetLibrary::Entry> entries = library.entries("incdaw.reverb");
    REQUIRE(entries.size() > 1);
    CHECK(entries.front().name == PresetLibrary::defaultPresetName);
    CHECK(hasEntry(entries, "Hall"));

    for (const PresetLibrary::Entry& entry : entries)
        CHECK(entry.factory);

    std::string error;
    CHECK_FALSE(library.store(samplePreset("incdaw.reverb", "Mine"), error));
    CHECK_FALSE(error.empty());
}

TEST_CASE("save, list, load, rename, duplicate and delete")
{
    const PresetLibrary library{scratchDirectory("lifecycle")};

    std::string error;
    REQUIRE(library.store(samplePreset("incdaw.eq", "Mine"), error));
    CHECK(error.empty());

    CHECK(library.contains("incdaw.eq", "Mine"));
    CHECK(hasEntry(library.entries("incdaw.eq"), "Mine"));

    // Another target's folder is untouched by this one.
    CHECK_FALSE(library.contains("incdaw.reverb", "Mine"));

    const std::optional<Preset> loaded = library.resolve("incdaw.eq", "Mine");
    REQUIRE(loaded.has_value());
    CHECK(loaded->uid == "incdaw.eq");
    CHECK(loaded->name == "Mine");
    REQUIRE(loaded->values.size() == 2);
    CHECK(loaded->values[0].value == doctest::Approx(123.5));

    REQUIRE(library.rename("incdaw.eq", "Mine", "Yours", error));
    CHECK_FALSE(library.contains("incdaw.eq", "Mine"));
    CHECK(library.contains("incdaw.eq", "Yours"));

    const std::string copy = library.duplicate("incdaw.eq", "Yours", "Yours", error);
    CHECK(copy == "Yours 2");
    CHECK(library.contains("incdaw.eq", "Yours 2"));

    REQUIRE(library.remove("incdaw.eq", "Yours 2", error));
    CHECK_FALSE(library.contains("incdaw.eq", "Yours 2"));
    CHECK_FALSE(library.remove("incdaw.eq", "Yours 2", error));
}

TEST_CASE("user presets are listed after the factory ones, sorted")
{
    const PresetLibrary library{scratchDirectory("order")};

    std::string error;
    REQUIRE(library.store(samplePreset("incdaw.delay", "Zulu"), error));
    REQUIRE(library.store(samplePreset("incdaw.delay", "Alpha"), error));

    const std::vector<PresetLibrary::Entry> entries = library.entries("incdaw.delay");

    std::size_t firstUser = entries.size();
    for (std::size_t index = 0; index < entries.size(); ++index)
        if (!entries[index].factory) {
            firstUser = index;
            break;
        }

    REQUIRE(firstUser + 2 == entries.size());
    CHECK(entries[firstUser].name == "Alpha");
    CHECK(entries[firstUser + 1].name == "Zulu");

    for (std::size_t index = 0; index < firstUser; ++index)
        CHECK(entries[index].factory);
}

TEST_CASE("what INCDAW ships cannot be overwritten, renamed or deleted")
{
    const PresetLibrary library{scratchDirectory("factory-protection")};

    std::string error;
    CHECK_FALSE(library.store(samplePreset("incdaw.reverb", "Hall"), error));
    CHECK_FALSE(library.store(samplePreset("incdaw.reverb",
                                           PresetLibrary::defaultPresetName), error));
    CHECK_FALSE(library.rename("incdaw.reverb", "Hall", "My Hall", error));
    CHECK_FALSE(library.remove("incdaw.reverb", "Hall", error));

    // And a factory name still resolves to what the catalogue says.
    const std::optional<Preset> hall = library.resolve("incdaw.reverb", "Hall");
    REQUIRE(hall.has_value());
    CHECK(hall->values.size() == 3);
}

TEST_CASE("a user file that shadows a factory name loses to the factory preset")
{
    const PresetLibrary library{scratchDirectory("shadow")};

    // Written past `store`, which would have refused it — the folder is the
    // user's, so the file can appear by other means than INCDAW.
    Preset impostor = samplePreset("incdaw.reverb", "Hall");
    impostor.values = {{0u, 0.2}};
    writeText(library.fileFor("incdaw.reverb", "Hall"), impostor.toJson());

    const std::optional<Preset> resolved = library.resolve("incdaw.reverb", "Hall");
    REQUIRE(resolved.has_value());
    CHECK(resolved->values.size() == 3);
}

TEST_CASE("duplicating a factory preset gives the user an editable copy")
{
    const PresetLibrary library{scratchDirectory("duplicate-factory")};

    std::string       error;
    const std::string copy = library.duplicate("incdaw.reverb", "Plate", "Plate", error);
    REQUIRE_FALSE(copy.empty());
    CHECK(copy == "Plate 2");

    const std::optional<Preset> loaded = library.resolve("incdaw.reverb", copy);
    REQUIRE(loaded.has_value());
    CHECK(loaded->values.size() == 3);

    REQUIRE(library.remove("incdaw.reverb", copy, error));
}

TEST_CASE("names that cannot be files are refused, not mangled into one")
{
    const PresetLibrary library{scratchDirectory("names")};

    CHECK(PresetLibrary::sanitiseName("a/b") == "ab");
    CHECK(PresetLibrary::sanitiseName("  ..  ").empty());
    CHECK(PresetLibrary::sanitiseName(std::string(200, 'x')).size() == 64);

    std::string error;
    CHECK_FALSE(library.store(samplePreset("incdaw.eq", "..."), error));
    CHECK(library.fileFor("incdaw.eq", "").empty());
}

TEST_CASE("an empty preset is refused: it would change nothing")
{
    const PresetLibrary library{scratchDirectory("empty")};

    Preset preset;
    preset.uid  = "incdaw.eq";
    preset.name = "Nothing";

    std::string error;
    CHECK_FALSE(library.store(preset, error));
    CHECK_FALSE(error.empty());
}

TEST_CASE("a corrupt file on disk is not a preset, and does not stop the list")
{
    const PresetLibrary library{scratchDirectory("corrupt")};

    writeText(library.fileFor("incdaw.eq", "Broken"), "{ this is not json");

    std::string error;
    REQUIRE(library.store(samplePreset("incdaw.eq", "Good"), error));

    CHECK_FALSE(library.resolve("incdaw.eq", "Broken").has_value());
    CHECK(library.resolve("incdaw.eq", "Good").has_value());
    CHECK(hasEntry(library.entries("incdaw.eq"), "Broken"));
}

TEST_CASE("renaming onto a taken name is refused rather than overwriting it")
{
    const PresetLibrary library{scratchDirectory("collision")};

    std::string error;
    REQUIRE(library.store(samplePreset("incdaw.eq", "One"), error));
    REQUIRE(library.store(samplePreset("incdaw.eq", "Two"), error));

    CHECK_FALSE(library.rename("incdaw.eq", "One", "Two", error));
    CHECK(library.contains("incdaw.eq", "One"));
    CHECK(library.contains("incdaw.eq", "Two"));
}
