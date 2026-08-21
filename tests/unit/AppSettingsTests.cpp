// Application settings — the preferences file behind Audio and MIDI Settings.
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
    CHECK(settings.recentProjects.empty());
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
    settings.recentProjects               = {"/a/one.incdaw", "/b/two.incdaw"};
    settings.browser.roots                = {"/Users/x/Music", "/Volumes/Library"};
    settings.browser.favourites           = {"/Users/x/Music/kick.wav"};

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
    CHECK(reloaded.recentProjects == settings.recentProjects);
    CHECK(reloaded.browser.roots == settings.browser.roots);
    CHECK(reloaded.browser.favourites == settings.browser.favourites);
}

TEST_CASE("an emptied library list stays empty across a launch")
{
    // "No libraries yet" and "the user removed every library" must not look
    // the same, or the first-run defaults come back against an explicit choice.
    app::AppSettings settings;
    CHECK_FALSE(settings.browser.seeded);

    settings.browser.seeded = true;
    settings.browser.roots.clear();

    const app::AppSettings reloaded = app::AppSettings::fromJson(settings.toJson());
    CHECK(reloaded.browser.seeded);
    CHECK(reloaded.browser.roots.empty());
}

TEST_CASE("writing the same settings twice produces identical bytes")
{
    // The same determinism rule the project format lives under: a preferences
    // file that churns on every save is one that a backup tool copies forever.
    app::AppSettings settings;
    settings.midiInputIdentifiers = {"one", "two"};
    settings.noteRecentProject("/songs/first.incdaw");

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
    CHECK(partial.recentProjects.empty());
    CHECK(partial.browser.roots.empty());
}

TEST_CASE("unknown keys from a newer build are ignored, not fatal")
{
    const app::AppSettings future =
        app::AppSettings::fromJson(R"({"version":99,"audio":{"bufferSize":64,"quantumFoam":true},
                                       "somethingNew":{"nested":[1,2]}})");

    CHECK(future.audio.bufferSize == 64);
}

TEST_CASE("recent projects are newest first, de-duplicated and bounded")
{
    app::AppSettings settings;

    settings.noteRecentProject("/songs/a.incdaw");
    settings.noteRecentProject("/songs/b.incdaw");
    settings.noteRecentProject("/songs/a.incdaw");

    REQUIRE(settings.recentProjects.size() == 2);
    CHECK(settings.recentProjects[0] == "/songs/a.incdaw");
    CHECK(settings.recentProjects[1] == "/songs/b.incdaw");

    settings.noteRecentProject("");                      // never recorded
    CHECK(settings.recentProjects.size() == 2);

    for (int index = 0; index < 40; ++index)
        settings.noteRecentProject("/songs/" + std::to_string(index) + ".incdaw");

    CHECK(settings.recentProjects.size() == app::AppSettings::maximumRecentProjects);
    CHECK(settings.recentProjects.front() == "/songs/39.incdaw");
}

TEST_CASE("an over-long recent list in the file is trimmed on read")
{
    std::string text = R"({"recentProjects":[)";
    for (int index = 0; index < 50; ++index)
        text += (index == 0 ? "" : ",") + std::string("\"/p/") + std::to_string(index) + ".incdaw\"";
    text += "]}";

    CHECK(app::AppSettings::fromJson(text).recentProjects.size()
          == app::AppSettings::maximumRecentProjects);
}

TEST_CASE("save and load survive a round trip on disk")
{
    const std::filesystem::path file = scratchFile("roundtrip.json");
    std::filesystem::remove(file);

    app::AppSettings settings;
    settings.audio.bufferSize     = 256;
    settings.midiInputIdentifiers = {"keystation"};
    settings.noteRecentProject("/songs/only.incdaw");

    REQUIRE(settings.save(file));

    const app::AppSettings reloaded = app::AppSettings::load(file);
    CHECK(reloaded.audio.bufferSize == 256);
    CHECK(reloaded.midiInputIdentifiers == std::vector<std::string>{"keystation"});
    CHECK(reloaded.recentProjects == std::vector<std::string>{"/songs/only.incdaw"});

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
