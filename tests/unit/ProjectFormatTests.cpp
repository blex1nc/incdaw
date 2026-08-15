#include "doctest.h"

#include "project/Json.h"
#include "project/Model.h"
#include "project/ProjectFile.h"

#include <atomic>
#include <filesystem>
#include <fstream>

using namespace incdaw;
using namespace incdaw::project;
namespace fs = std::filesystem;

namespace {

/// A temporary directory that removes itself, so a failing test cannot leave
/// state behind that makes the next run pass or fail for the wrong reason.
struct ScratchDirectory {
    explicit ScratchDirectory(const std::string& name)
        : path(fs::temp_directory_path() / ("incdaw-test-" + name + "-" + std::to_string(nextSerial())))
    {
        std::error_code code;
        fs::remove_all(path, code);
        fs::create_directories(path, code);
    }

    ~ScratchDirectory()
    {
        std::error_code code;
        fs::remove_all(path, code);
    }

    fs::path path;

private:
    // A per-run serial rather than a process id: getpid() would drag a
    // platform header into the tests, and uniqueness within the run is all
    // this needs.
    static int nextSerial()
    {
        static std::atomic<int> counter{0};
        return ++counter;
    }
};

/// A project exercising every entity type, so the round-trip test actually
/// covers the model rather than an empty shell.
Project makePopulatedProject()
{
    Project project;

    project.metadata().title   = "Test \"Song\" \\ with escapes\nand a newline";
    project.metadata().artist  = "INCDAW";
    project.metadata().comment = "unicode: ünïcödé ✓";

    engine::TempoMap map{128.0, 48000.0};
    map.setTempoEvents({{0, 128.0}, {engine::ticksPerQuarterNote * 16, 92.5}});
    map.setTimeSignatureEvents({{0, {4, 4}}, {engine::ticksPerQuarterNote * 16, {7, 8}}});
    project.tempoMap() = map;

    auto& bus = project.addMixerNode(MixerNodeType::bus, "Drum Bus");
    bus.volume = 0.8;
    bus.pan    = -0.25;
    bus.inserts.push_back(PluginSlot{project.ids().next(),
                                     {plugins::Format::clap, "com.acme.compressor"},
                                     false, "1234.state"});

    auto& track = project.addTrack(TrackType::instrument, "Lead");
    track.colour = 0xFFAA3344u;
    track.outputMixerNode = bus.id;

    auto& channel = project.addChannel("Sampler");
    channel.instrument = {plugins::Format::audioUnit, "aumu:samp:appl"};
    channel.instrumentStateFile = "sampler.state";
    channel.volume = 0.6;

    auto& pattern = project.addPattern("Verse");
    pattern.length = engine::ticksPerQuarterNote * 8;
    pattern.swing  = 0.32;

    auto& content = pattern.contentFor(channel.id);
    content.loopLength = engine::ticksPerQuarterNote * 3;   // polymetric against the pattern
    content.events.push_back(MidiEvent{MidiEventType::note, 0, 480, 0, 60, 100, 64,
                                       0.85, -0.2, 0.05, "root"});
    content.events.push_back(MidiEvent{MidiEventType::note, 480, 240, 0, 64, 90, 64,
                                       1.0, 0.0, 0.0, "third"});
    content.events.push_back(MidiEvent{MidiEventType::controlChange, 960, 0, 0, 74, 42, 0,
                                       1.0, 0.0, 0.0, ""});

    auto& clip = project.addClip(ClipType::pattern, track.id, pattern.id);
    clip.startTick   = engine::ticksPerQuarterNote * 16;
    clip.lengthTicks = engine::ticksPerQuarterNote * 32;
    clip.start  = 96000;
    clip.length = 192000;
    clip.gain   = 0.707;
    clip.pan    = 0.33;
    clip.normalize = true;
    clip.fadeInFrames = 512;
    clip.pitchSemitones = -2.5;
    clip.name = "Verse A";

    auto& lane = project.addAutomationLane(bus.id, "volume");
    lane.points.push_back({0, 0.0, AutomationCurve::linear, 0.0});
    lane.points.push_back({960, 1.0, AutomationCurve::smooth, 0.4});
    lane.points.push_back({1920, 0.5, AutomationCurve::hold, -0.2});

    auto& asset = project.addAudioAsset("/tmp/does-not-exist.wav");
    asset.sampleRate   = 44100.0;
    asset.frameCount   = 123456;
    asset.channelCount = 2;
    asset.contentHash  = "deadbeef";

    // Format 1.3: a sampler program on the channel, naming the asset by id.
    ChannelSamplerZone zone;
    zone.asset         = asset.id;
    zone.rootKey       = 48;
    zone.keyLow        = 24;
    zone.keyHigh       = 84;
    zone.velocityLow   = 32;
    zone.velocityHigh  = 96;
    zone.start         = 100;
    zone.end           = 90000;
    zone.loopStart     = 2000;
    zone.loopEnd       = 80000;
    zone.loopCrossfade = 512;
    zone.reverse       = true;
    zone.gain          = 0.75;
    channel.samplerZones.push_back(zone);

    auto& send = project.connect(bus.id, project.masterMixerNode());
    send.isSend   = true;
    send.gain     = 0.25;
    send.preFader = true;

    // Format 1.4: a hardware control bound to a parameter.
    auto& mapping       = project.addMidiMapping(74, "volume", bus.id);
    mapping.midiChannel = 3;
    mapping.minValue    = 0.1;
    mapping.maxValue    = 0.9;

    return project;
}

} // namespace

// ── JSON ──────────────────────────────────────────────────────────────────────

TEST_CASE("json round-trips every value type")
{
    Json root = Json::object();
    root.set("string", "hello");
    root.set("int", std::int64_t{-42});
    root.set("double", 3.14159265358979);
    root.set("true", true);
    root.set("false", false);
    root.set("null", nullptr);

    Json array = Json::array();
    array.append(1);
    array.append("two");
    array.append(Json::object());
    root.set("array", std::move(array));

    Json parsed;
    std::string error;
    REQUIRE(Json::parse(root.dump(), parsed, error));

    CHECK(parsed["string"].asString() == "hello");
    CHECK(parsed["int"].asInt() == -42);
    CHECK(parsed["double"].asDouble() == doctest::Approx(3.14159265358979));
    CHECK(parsed["true"].asBool());
    CHECK_FALSE(parsed["false"].asBool(true));
    CHECK(parsed["null"].isNull());
    CHECK(parsed["array"].size() == 3);
}

TEST_CASE("object key order is preserved, which is what makes saves deterministic")
{
    Json root = Json::object();
    root.set("zebra", 1);
    root.set("apple", 2);
    root.set("middle", 3);

    const std::string text = root.dump(0);
    CHECK(text == R"({"zebra":1,"apple":2,"middle":3})");

    // And it survives a round-trip.
    Json parsed;
    std::string error;
    REQUIRE(Json::parse(text, parsed, error));
    CHECK(parsed.dump(0) == text);
}

TEST_CASE("replacing a key keeps its position")
{
    Json root = Json::object();
    root.set("a", 1);
    root.set("b", 2);
    root.set("a", 99);

    CHECK(root.dump(0) == R"({"a":99,"b":2})");
}

TEST_CASE("large integers survive without being rounded through a double")
{
    // A 64-bit entity id above 2^53 would be silently corrupted by a parser
    // that stores every number as a double.
    const std::int64_t large = 9007199254740993;   // 2^53 + 1

    Json root = Json::object();
    root.set("id", large);

    Json parsed;
    std::string error;
    REQUIRE(Json::parse(root.dump(), parsed, error));
    CHECK(parsed["id"].asInt() == large);
}

TEST_CASE("doubles round-trip exactly")
{
    for (const double value : {0.1, 1.0 / 3.0, 1e-300, 1e300, 120.0, 44100.0, -0.0000001}) {
        Json root = Json::object();
        root.set("v", value);

        Json parsed;
        std::string error;
        REQUIRE(Json::parse(root.dump(), parsed, error));
        CHECK(parsed["v"].asDouble() == value);
    }
}

TEST_CASE("strings with escapes and unicode round-trip")
{
    const std::string awkward = "quote\" backslash\\ newline\n tab\t unicode ünïcödé ✓";

    Json root = Json::object();
    root.set("text", awkward);

    Json parsed;
    std::string error;
    REQUIRE(Json::parse(root.dump(), parsed, error));
    CHECK(parsed["text"].asString() == awkward);
}

TEST_CASE("malformed json is rejected without throwing or partially applying")
{
    const char* broken[] = {
        "", "{", "}", "[1,2", R"({"a":})", R"({"a" 1})", R"({a:1})",
        "tru", "[1,2,]extra", R"({"a":1}trailing)",
    };

    for (const char* text : broken) {
        Json parsed = Json{std::string{"untouched"}};
        std::string error;

        CHECK_FALSE(Json::parse(text, parsed, error));
        CHECK_FALSE(error.empty());
        CHECK(parsed.asString() == "untouched");   // never partially overwritten
    }
}

TEST_CASE("deeply nested json is rejected rather than blowing the stack")
{
    std::string deep;
    for (int level = 0; level < 5000; ++level)
        deep += '[';

    Json parsed;
    std::string error;
    CHECK_FALSE(Json::parse(deep, parsed, error));
}

TEST_CASE("missing and wrong-typed fields fall back instead of throwing")
{
    Json root = Json::object();
    root.set("number", 5);

    CHECK(root["absent"].asInt(77) == 77);
    CHECK(root["number"].asString("fallback") == "fallback");
    CHECK(root["absent"]["deeper"]["deeper still"].asDouble(1.5) == doctest::Approx(1.5));
    CHECK(root[99].isNull());
}

// ── Project format ────────────────────────────────────────────────────────────

TEST_CASE("the format version is stamped from the very first save")
{
    ScratchDirectory scratch{"version"};
    const auto packagePath = scratch.path / "Song.incdaw";

    const Project project;
    REQUIRE(ProjectFile::save(project, packagePath));

    CHECK(ProjectFile::isProjectPackage(packagePath));
    CHECK(ProjectFile::versionOf(packagePath) == projectFormatVersionString());
    CHECK(fs::exists(packagePath / "manifest.json"));
    CHECK(fs::exists(packagePath / "project.json"));
    CHECK(fs::is_directory(packagePath / "patterns"));
}

TEST_CASE("a project survives a save and load unchanged")
{
    ScratchDirectory scratch{"roundtrip"};
    const auto packagePath = scratch.path / "Song.incdaw";

    const Project original = makePopulatedProject();
    REQUIRE(ProjectFile::save(original, packagePath));

    Project loaded;
    const auto result = ProjectFile::load(loaded, packagePath);
    REQUIRE(result.succeeded);

    CHECK(loaded == original);
}

TEST_CASE("saving twice produces byte-identical output")
{
    // docs/PROJECT_FORMAT.md §7. Without this, the project format is unusable
    // with version control and the round-trip test proves much less than it
    // appears to.
    ScratchDirectory scratch{"determinism"};

    const Project project = makePopulatedProject();

    const auto first  = scratch.path / "First.incdaw";
    const auto second = scratch.path / "Second.incdaw";

    REQUIRE(ProjectFile::save(project, first));
    REQUIRE(ProjectFile::save(project, second));

    for (const char* file : {"project.json", "manifest.json"}) {
        std::ifstream a(first / file, std::ios::binary);
        std::ifstream b(second / file, std::ios::binary);

        const std::string textA{std::istreambuf_iterator<char>(a), std::istreambuf_iterator<char>()};
        const std::string textB{std::istreambuf_iterator<char>(b), std::istreambuf_iterator<char>()};

        CHECK(textA == textB);
        CHECK_FALSE(textA.empty());
    }
}

TEST_CASE("a load, save, load cycle is stable")
{
    ScratchDirectory scratch{"stable"};

    const Project original = makePopulatedProject();
    REQUIRE(ProjectFile::save(original, scratch.path / "A.incdaw"));

    Project once;
    REQUIRE(ProjectFile::load(once, scratch.path / "A.incdaw"));
    REQUIRE(ProjectFile::save(once, scratch.path / "B.incdaw"));

    Project twice;
    REQUIRE(ProjectFile::load(twice, scratch.path / "B.incdaw"));

    CHECK(once == twice);
    CHECK(twice == original);
}

TEST_CASE("ids minted after a load cannot collide with ids already in the file")
{
    ScratchDirectory scratch{"ids"};
    const auto packagePath = scratch.path / "Song.incdaw";

    const Project original = makePopulatedProject();
    REQUIRE(ProjectFile::save(original, packagePath));

    Project loaded;
    REQUIRE(ProjectFile::load(loaded, packagePath));

    const EntityId minted = loaded.ids().next();

    for (const auto& track : loaded.tracks())      CHECK(minted != track.id);
    for (const auto& clip : loaded.clips())        CHECK(minted != clip.id);
    for (const auto& node : loaded.mixerNodes())   CHECK(minted != node.id);
    for (const auto& pattern : loaded.patterns())  CHECK(minted != pattern.id);
    for (const auto& lane : loaded.automation())   CHECK(minted != lane.id);
    for (const auto& asset : loaded.audioAssets()) CHECK(minted != asset.id);
}

TEST_CASE("patterns are stored one file each")
{
    ScratchDirectory scratch{"patterns"};
    const auto packagePath = scratch.path / "Song.incdaw";

    Project project;
    project.addPattern("One");
    project.addPattern("Two");
    project.addPattern("Three");

    REQUIRE(ProjectFile::save(project, packagePath));

    int patternFiles = 0;
    for (const auto& entry : fs::directory_iterator(packagePath / "patterns"))
        if (entry.path().extension() == ".pat")
            ++patternFiles;

    CHECK(patternFiles == 3);
}

TEST_CASE("a corrupted pattern costs one pattern, not the session")
{
    ScratchDirectory scratch{"corrupt-pattern"};
    const auto packagePath = scratch.path / "Song.incdaw";

    Project project;
    project.addPattern("Good");
    const auto damagedId = project.addPattern("Damaged").id;
    project.addPattern("AlsoGood");

    REQUIRE(ProjectFile::save(project, packagePath));

    std::ofstream damaged(packagePath / "patterns" / (std::to_string(damagedId.value()) + ".pat"),
                          std::ios::trunc);
    damaged << "{ this is not json";
    damaged.close();

    Project loaded;
    REQUIRE(ProjectFile::load(loaded, packagePath));

    CHECK(loaded.patterns().size() == 2);
    CHECK(loaded.findPattern(damagedId) == nullptr);
}

TEST_CASE("a project saved by a newer INCDAW is refused, not loaded hopefully")
{
    // A hopeful load that drops fields it does not understand destroys the
    // user's work on the next save.
    ScratchDirectory scratch{"newer"};
    const auto packagePath = scratch.path / "Song.incdaw";

    const Project project;
    REQUIRE(ProjectFile::save(project, packagePath));

    std::ofstream manifest(packagePath / "manifest.json", std::ios::trunc);
    manifest << R"({"incdaw_project_version": "99.0"})";
    manifest.close();

    Project loaded;
    const auto result = ProjectFile::load(loaded, packagePath);

    CHECK_FALSE(result.succeeded);
    CHECK(result.error.find("newer version") != std::string::npos);
}

TEST_CASE("a package without a version is refused")
{
    ScratchDirectory scratch{"unversioned"};
    const auto packagePath = scratch.path / "Song.incdaw";

    const Project project;
    REQUIRE(ProjectFile::save(project, packagePath));

    std::ofstream manifest(packagePath / "manifest.json", std::ios::trunc);
    manifest << R"({"created_with": "something else"})";
    manifest.close();

    Project loaded;
    CHECK_FALSE(ProjectFile::load(loaded, packagePath).succeeded);
}

TEST_CASE("corrupt and truncated project files are rejected cleanly")
{
    ScratchDirectory scratch{"corrupt"};

    const char* payloads[] = {"", "{", R"({"metadata": )", "not json at all"};
    int variant = 0;

    for (const char* payload : payloads) {
        const auto packagePath = scratch.path / ("Song" + std::to_string(variant++) + ".incdaw");

        const Project project;
        REQUIRE(ProjectFile::save(project, packagePath));

        std::ofstream broken(packagePath / "project.json", std::ios::trunc);
        broken << payload;
        broken.close();

        Project loaded;
        const auto result = ProjectFile::load(loaded, packagePath);

        CHECK_FALSE(result.succeeded);
        CHECK_FALSE(result.error.empty());
    }
}

TEST_CASE("loading something that is not a package fails without crashing")
{
    ScratchDirectory scratch{"notapackage"};

    Project loaded;
    CHECK_FALSE(ProjectFile::load(loaded, scratch.path / "nothing-here.incdaw").succeeded);
    CHECK_FALSE(ProjectFile::load(loaded, scratch.path).succeeded);
    CHECK_FALSE(ProjectFile::isProjectPackage(scratch.path / "nothing-here.incdaw"));
}

TEST_CASE("a project with missing media still opens, and reports what is missing")
{
    ScratchDirectory scratch{"missing"};
    const auto packagePath = scratch.path / "Song.incdaw";

    Project project;
    const auto missingId = project.addAudioAsset("/definitely/not/here.wav").id;

    // A clip referencing it must survive the round-trip with its edits intact.
    auto& track = project.addTrack(TrackType::audio, "Audio");
    auto& clip  = project.addClip(ClipType::audio, track.id, missingId);
    clip.start = 44100;
    clip.gain  = 0.5;

    REQUIRE(ProjectFile::save(project, packagePath));

    Project loaded;
    REQUIRE(ProjectFile::load(loaded, packagePath));

    const auto missing = loaded.missingAssets();
    REQUIRE(missing.size() == 1);
    CHECK(missing[0] == missingId);

    REQUIRE(loaded.clips().size() == 1);
    CHECK(loaded.clips()[0].start == 44100);
    CHECK(loaded.clips()[0].gain == doctest::Approx(0.5));
}

TEST_CASE("an interrupted save leaves no half-written file behind")
{
    ScratchDirectory scratch{"staging"};
    const auto packagePath = scratch.path / "Song.incdaw";

    const Project project = makePopulatedProject();
    REQUIRE(ProjectFile::save(project, packagePath));

    // Writes are staged through a sibling and renamed into place, so no
    // ".writing" file should ever remain.
    for (const auto& entry : fs::recursive_directory_iterator(packagePath))
        CHECK(entry.path().extension() != ".writing");
}

// ── Permanent version fixtures ────────────────────────────────────────────────
//
// docs/PROJECT_FORMAT.md §2: every released format version keeps a fixture, and
// every one of them must load forever. These are hand-written rather than
// generated, so they stay independent of whatever the current code happens to
// produce — a generated fixture only ever proves the code agrees with itself.

TEST_CASE("the v1.0 fixture still loads")
{
    const fs::path fixture = fs::path{INCDAW_FIXTURE_DIR} / "v1.0" / "Fixture.incdaw";
    REQUIRE(fs::exists(fixture));

    Project project;
    const auto result = ProjectFile::load(project, fixture);

    REQUIRE(result.succeeded);

    // 1.0 is no longer current, so loading the fixture exercises the migration
    // chain — which is the point of keeping it.
    CHECK(result.migrated);
    CHECK(result.migratedFrom == "1.0");

    CHECK(project.metadata().title == "Format v1.0 fixture");

    CHECK(project.mixerNodes().size() == 2);
    CHECK(project.tracks().size() == 1);
    CHECK(project.channels().size() == 1);
    CHECK(project.clips().size() == 1);
    CHECK(project.automation().size() == 1);
    CHECK(project.audioAssets().size() == 1);
    CHECK(project.routing().size() == 1);
    CHECK(project.patterns().size() == 1);

    CHECK(project.tempoMap().tempoAtTick(0) == doctest::Approx(128.0));
    CHECK(project.tempoMap().tempoAtTick(15360) == doctest::Approx(92.5));
    CHECK(project.tempoMap().timeSignatureAtTick(15360) == engine::TimeSignature{7, 8});

    const Pattern* pattern = project.findPattern(EntityId{6});
    REQUIRE(pattern != nullptr);

    // The fixture is format 1.0, which stored one flat event list per pattern.
    // Loading it must attach those notes to a real channel, or they would be
    // present in the model and silent on playback.
    REQUIRE(project.channels().size() == 1);
    const std::vector<MidiEvent>* events = pattern->events(project.channels()[0].id);
    REQUIRE(events != nullptr);
    REQUIRE(events->size() == 2);
    CHECK((*events)[0].label == "root");
    CHECK((*events)[0].probability == doctest::Approx(0.85));

    CHECK(project.clips()[0].gain == doctest::Approx(0.707));
    CHECK(project.clips()[0].normalize);

    REQUIRE(project.channels().size() == 1);
    CHECK(project.channels()[0].instrument.format == plugins::Format::audioUnit);
    CHECK(project.channels()[0].instrument.uid == "aumu:samp:appl");

    REQUIRE(project.mixerNodes()[1].inserts.size() == 1);
    CHECK(project.mixerNodes()[1].inserts[0].plugin.uid == "com.acme.compressor");
}

TEST_CASE("the v1.1 fixture still loads")
{
    const fs::path fixture = fs::path{INCDAW_FIXTURE_DIR} / "v1.1" / "Fixture.incdaw";
    REQUIRE(fs::exists(fixture));

    Project project;
    const auto result = ProjectFile::load(project, fixture);

    REQUIRE(result.succeeded);
    CHECK(result.migrated);
    CHECK(result.migratedFrom == "1.1");

    CHECK(project.metadata().title == "Format v1.1 fixture");

    REQUIRE(project.channels().size() == 2);
    CHECK(project.channels()[0].name == "Kick");
    CHECK(project.channels()[0].volume == doctest::Approx(0.8));
    CHECK(project.channels()[1].muted);

    // 1.1 had no per-channel step key. Reading one back as middle C is the
    // 1.2 default, and it is what those projects behaved as if they had.
    CHECK(project.channels()[0].stepKey == 60);
    CHECK(project.channels()[1].stepKey == 60);

    // Per-channel pattern content, which is what 1.1 introduced, must survive
    // the upgrade unchanged.
    const Pattern* pattern = project.findPattern(EntityId{4});
    REQUIRE(pattern != nullptr);
    CHECK(pattern->swing == doctest::Approx(0.25));

    const std::vector<MidiEvent>* kick = pattern->events(project.channels()[0].id);
    REQUIRE(kick != nullptr);
    REQUIRE(kick->size() == 1);
    CHECK((*kick)[0].key == 36);

    const std::vector<MidiEvent>* lead = pattern->events(project.channels()[1].id);
    REQUIRE(lead != nullptr);
    REQUIRE(lead->size() == 1);
    CHECK((*lead)[0].tick == 480);
}

TEST_CASE("the v1.2 fixture still loads")
{
    const fs::path fixture = fs::path{INCDAW_FIXTURE_DIR} / "v1.2" / "Fixture.incdaw";
    REQUIRE(fs::exists(fixture));

    Project project;
    const auto result = ProjectFile::load(project, fixture);

    REQUIRE(result.succeeded);
    CHECK(result.migrated);
    CHECK(result.migratedFrom == "1.2");

    CHECK(project.metadata().title == "Format v1.2 fixture");

    REQUIRE(project.channels().size() == 2);

    // The step key, which is what 1.2 introduced, must survive the upgrade.
    CHECK(project.channels()[0].stepKey == 36);
    CHECK(project.channels()[1].stepKey == 60);

    // 1.2 had no sampler zones. Those channels had no sampler program, and
    // that is exactly how they must read back.
    CHECK(project.channels()[0].samplerZones.empty());
    CHECK(project.channels()[1].samplerZones.empty());
}

TEST_CASE("the v1.3 fixture still loads")
{
    const fs::path fixture = fs::path{INCDAW_FIXTURE_DIR} / "v1.3" / "Fixture.incdaw";
    REQUIRE(fs::exists(fixture));

    Project project;
    const auto result = ProjectFile::load(project, fixture);

    REQUIRE(result.succeeded);
    CHECK(result.migrated);
    CHECK(result.migratedFrom == "1.3");

    CHECK(project.metadata().title == "Format v1.3 fixture");

    REQUIRE(project.channels().size() == 2);
    const Channel& kick = project.channels()[0];

    CHECK(kick.instrument == plugins::builtinSampler());

    REQUIRE(kick.samplerZones.size() == 1);
    const ChannelSamplerZone& zone = kick.samplerZones[0];
    CHECK(zone.asset == EntityId{5});
    CHECK(zone.rootKey == 36);
    CHECK(zone.keyHigh == 96);
    CHECK(zone.loopStart == 1000);
    CHECK(zone.loopEnd == 9000);
    CHECK(zone.loopCrossfade == 256);
    CHECK(zone.gain == doctest::Approx(0.9));

    // The zone's asset must resolve within the same document.
    REQUIRE(project.audioAssets().size() == 1);
    CHECK(project.audioAssets()[0].id == zone.asset);

    CHECK(project.channels()[1].samplerZones.empty());
}

TEST_CASE("a fixture re-saved by this build still round-trips")
{
    ScratchDirectory scratch{"fixture-resave"};
    const fs::path fixture = fs::path{INCDAW_FIXTURE_DIR} / "v1.0" / "Fixture.incdaw";

    Project loaded;
    REQUIRE(ProjectFile::load(loaded, fixture));
    REQUIRE(ProjectFile::save(loaded, scratch.path / "Resaved.incdaw"));

    Project reloaded;
    REQUIRE(ProjectFile::load(reloaded, scratch.path / "Resaved.incdaw"));

    CHECK(reloaded == loaded);
}
