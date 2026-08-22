// Application settings — the preferences file behind Audio and MIDI Settings.
//
// Deliberately narrow: the Browser keeps its own libraries and recents
// (app::Browser::save) and the recent-projects menu is user defaults, so this
// file carries only the device, the MIDI sources and the workspace.
//
// One property carries this file: a settings file is a cache of a preference,
// never a precondition for launching. Every test below is a way of corrupting,
// truncating or ageing that file, and every one of them must still yield a
// usable configuration rather than an error — because the alternative is a DAW
// that will not open after an unclean shutdown.

#include "doctest.h"

#include "app/AppSettings.h"

#include <filesystem>
#include <fstream>
#include <string>

using namespace incdaw;

namespace {

std::filesystem::path scratchFile(const char* name)
{
    const std::filesystem::path directory =
        std::filesystem::temp_directory_path() / "incdaw-settings-tests";

    std::error_code failed;
    std::filesystem::create_directories(directory, failed);

    return directory / name;
}

void writeText(const std::filesystem::path& file, const std::string& text)
{
    std::ofstream stream(file, std::ios::binary | std::ios::trunc);
    stream << text;
}

} // namespace

TEST_CASE("defaults match what the shell used to hardcode")
{
    const app::AppSettings settings;

    CHECK(settings.audio.sampleRate == doctest::Approx(48000.0));
    CHECK(settings.audio.bufferSize == 512);
    CHECK(settings.audio.outputChannels == 2);
    CHECK(settings.audio.outputDeviceIdentifier.empty());   // the system default
    CHECK(settings.openInputAtLaunch == false);             // never open the mic unasked
    CHECK(settings.midiInputIdentifiers.empty());           // every source
}

TEST_CASE("settings round trip through JSON")
{
    app::AppSettings settings;
    settings.audio.outputDeviceIdentifier = "AppleHDAEngineOutput:1B,0,1,2";
    settings.audio.inputDeviceIdentifier  = "BuiltInMicrophoneDevice";
    settings.audio.sampleRate             = 44100.0;
    settings.audio.bufferSize             = 128;
    settings.audio.outputChannels         = 2;
    settings.audio.inputChannels          = 1;
    settings.openInputAtLaunch            = true;
    settings.midiInputIdentifiers         = {"keystation", "launchpad"};
    settings.workspace.windowX            = 120.0;
    settings.workspace.windowY            = 64.0;
    settings.workspace.windowWidth        = 1400.0;
    settings.workspace.windowHeight       = 900.0;
    settings.workspace.activeEditor       = 2;
    settings.workspace.songMode           = true;

    const app::AppSettings reloaded = app::AppSettings::fromJson(settings.toJson());

    CHECK(reloaded.audio.outputDeviceIdentifier == settings.audio.outputDeviceIdentifier);
    CHECK(reloaded.audio.inputDeviceIdentifier == settings.audio.inputDeviceIdentifier);
    CHECK(reloaded.audio.sampleRate == doctest::Approx(44100.0));
    CHECK(reloaded.audio.bufferSize == 128);
    CHECK(reloaded.audio.inputChannels == 1);
    CHECK(reloaded.openInputAtLaunch == true);
    CHECK(reloaded.midiInputIdentifiers == settings.midiInputIdentifiers);
    CHECK(reloaded.workspace.windowWidth == doctest::Approx(1400.0));
    CHECK(reloaded.workspace.activeEditor == 2);
    CHECK(reloaded.workspace.songMode == true);
}

TEST_CASE("writing the same settings twice produces identical bytes")
{
    // The same determinism rule the project format lives under: a preferences
    // file that churns on every save is one that a backup tool copies forever.
    app::AppSettings settings;
    settings.midiInputIdentifiers = {"one", "two"};
    settings.workspace.windowWidth = 1280.0;

    CHECK(settings.toJson() == settings.toJson());
}

TEST_CASE("a corrupt file yields defaults rather than an error")
{
    CHECK(app::AppSettings::fromJson("").audio.bufferSize == 512);
    CHECK(app::AppSettings::fromJson("{").audio.bufferSize == 512);
    CHECK(app::AppSettings::fromJson("[1, 2, 3]").audio.bufferSize == 512);
    CHECK(app::AppSettings::fromJson("null").audio.sampleRate == doctest::Approx(48000.0));
}

TEST_CASE("hostile values are clamped, not adopted")
{
    // A device asked for zero frames does not fail loudly; it fails as silence.
    const app::AppSettings zeroed =
        app::AppSettings::fromJson(R"({"audio":{"sampleRate":0,"bufferSize":0}})");

    CHECK(zeroed.audio.sampleRate == doctest::Approx(48000.0));
    CHECK(zeroed.audio.bufferSize == 512);

    const app::AppSettings absurd =
        app::AppSettings::fromJson(R"({"audio":{"sampleRate":-44100,"bufferSize":1000000,
                                                "outputChannels":-4}})");

    CHECK(absurd.audio.sampleRate == doctest::Approx(48000.0));
    CHECK(absurd.audio.bufferSize == 512);
    CHECK(absurd.audio.outputChannels == 2);
}

TEST_CASE("fields the file does not carry keep their defaults")
{
    // What reading a file written by an older build looks like.
    const app::AppSettings partial =
        app::AppSettings::fromJson(R"({"format":"incdaw-settings","version":1,
                                       "audio":{"bufferSize":256}})");

    CHECK(partial.audio.bufferSize == 256);
    CHECK(partial.audio.sampleRate == doctest::Approx(48000.0));
    CHECK(partial.workspace.windowWidth == doctest::Approx(0.0));
    CHECK(partial.midiInputIdentifiers.empty());
}

TEST_CASE("unknown keys from a newer build are ignored, not fatal")
{
    const app::AppSettings future =
        app::AppSettings::fromJson(R"({"version":99,"audio":{"bufferSize":64,"quantumFoam":true},
                                       "somethingNew":{"nested":[1,2]}})");

    CHECK(future.audio.bufferSize == 64);
}

TEST_CASE("save and load survive a round trip on disk")
{
    const std::filesystem::path file = scratchFile("roundtrip.json");
    std::filesystem::remove(file);

    app::AppSettings settings;
    settings.audio.bufferSize     = 256;
    settings.midiInputIdentifiers = {"keystation"};

    REQUIRE(settings.save(file));

    const app::AppSettings reloaded = app::AppSettings::load(file);
    CHECK(reloaded.audio.bufferSize == 256);
    CHECK(reloaded.midiInputIdentifiers == std::vector<std::string>{"keystation"});

    // No leftover temporary beside the file: saving twice a second during a
    // session must not litter the support directory.
    CHECK_FALSE(std::filesystem::exists(std::filesystem::path{file}.concat(".tmp")));
}

TEST_CASE("a missing settings file is the normal first run")
{
    const std::filesystem::path file = scratchFile("does-not-exist.json");
    std::filesystem::remove(file);

    CHECK(app::AppSettings::load(file).audio.bufferSize == 512);
}

TEST_CASE("a truncated settings file does not take the previous one down with it")
{
    const std::filesystem::path file = scratchFile("truncated.json");

    app::AppSettings good;
    good.audio.bufferSize = 64;
    REQUIRE(good.save(file));

    writeText(file, R"({"audio":{"bufferSi)");        // an interrupted write

    CHECK(app::AppSettings::load(file).audio.bufferSize == 512);

    // And the next honest save repairs it.
    REQUIRE(good.save(file));
    CHECK(app::AppSettings::load(file).audio.bufferSize == 64);
}

TEST_CASE("saving to an unwritable path reports failure instead of throwing")
{
    const app::AppSettings settings;

    CHECK_FALSE(settings.save({}));
    CHECK_FALSE(settings.save("/this/directory/does/not/exist/settings.json"));
}

TEST_CASE("the update preference round trips, and an older file keeps the default")
{
    const std::filesystem::path file = scratchFile("updates.json");

    app::AppSettings settings;
    CHECK(settings.updates.checkAtLaunch);              // on unless turned off
    CHECK(settings.updates.lastCheckedUnix == 0);
    CHECK(settings.updates.skippedVersion.empty());

    settings.updates.checkAtLaunch   = false;
    settings.updates.lastCheckedUnix = 1'800'000'000;
    settings.updates.skippedVersion  = "0.9.1";

    REQUIRE(settings.save(file));

    const app::AppSettings reloaded = app::AppSettings::load(file);
    CHECK_FALSE(reloaded.updates.checkAtLaunch);
    CHECK(reloaded.updates.lastCheckedUnix == 1'800'000'000);
    CHECK(reloaded.updates.skippedVersion == "0.9.1");

    // A settings file written before the block existed. The field is an
    // addition, not a change of meaning, so the version does not move and the
    // old file simply reads as "never asked" — which is the default, on.
    writeText(file, R"({"format":"incdaw-settings","version":1,"audio":{"bufferSize":256}})");

    const app::AppSettings older = app::AppSettings::load(file);
    CHECK(older.audio.bufferSize == 256);
    CHECK(older.updates.checkAtLaunch);
    CHECK(older.updates.lastCheckedUnix == 0);
}

TEST_CASE("a nonsense last-checked stamp leaves the check due rather than locked out")
{
    const std::filesystem::path file = scratchFile("updates-clock.json");

    // Hand-edited, or written by a machine whose clock was wrong. Treating it
    // as a real stamp would postpone the next check by however far it is out.
    writeText(file, R"({"updates":{"lastChecked":-9000000000}})");

    CHECK(app::AppSettings::load(file).updates.lastCheckedUnix == 0);
}

TEST_CASE("the appearance theme survives a round trip and defaults to Midnight")
{
    app::AppSettings settings;
    CHECK(settings.appearance.themeName == "Midnight");

    settings.appearance.themeName = "Neon";

    const app::AppSettings restored = app::AppSettings::fromJson(settings.toJson());
    CHECK(restored.appearance.themeName == "Neon");
}

TEST_CASE("a settings file written before themes existed still names a theme")
{
    // Every settings.json already on disk looks like this: no appearance
    // object at all. It must resolve to the scheme the shell was designed
    // against rather than to an empty name nothing answers to.
    const app::AppSettings older = app::AppSettings::fromJson(
        R"({"format":"incdaw-settings","version":1,"audio":{"sampleRate":44100.0}})");

    CHECK(older.appearance.themeName == "Midnight");
    CHECK(older.audio.sampleRate == doctest::Approx(44100.0));

    // A blanked-out key is a hand-edited file, not a request for no theme.
    const app::AppSettings blank =
        app::AppSettings::fromJson(R"({"appearance":{"theme":""}})");

    CHECK(blank.appearance.themeName == "Midnight");
}
