#include "project/ProjectFile.h"

#include <cstdio>

#include "project/Json.h"

#include <fstream>
#include <sstream>

namespace incdaw::project {
namespace fs = std::filesystem;

namespace {

constexpr const char* manifestFileName = "manifest.json";
constexpr const char* projectFileName  = "project.json";
constexpr const char* patternsDirName  = "patterns";
constexpr const char* historyDirName   = "history";

bool writeTextFile(const fs::path& path, const std::string& text, std::string& error)
{
    // Staged through a sibling temporary and then renamed. A crash mid-write
    // then loses the new version rather than the old one — the opposite of what
    // a plain truncating write does.
    const fs::path staging = path.string() + ".writing";

    {
        std::ofstream stream(staging, std::ios::binary | std::ios::trunc);
        if (!stream) {
            error = "cannot open for writing: " + staging.string();
            return false;
        }

        stream.write(text.data(), static_cast<std::streamsize>(text.size()));
        if (!stream) {
            error = "write failed: " + staging.string();
            return false;
        }
    }

    std::error_code code;
    fs::rename(staging, path, code);

    if (code) {
        error = "could not replace " + path.string() + ": " + code.message();
        fs::remove(staging, code);
        return false;
    }

    return true;
}

bool readTextFile(const fs::path& path, std::string& text, std::string& error)
{
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        error = "cannot open: " + path.string();
        return false;
    }

    std::ostringstream buffer;
    buffer << stream.rdbuf();
    text = buffer.str();
    return true;
}

// ── Model <-> Json ────────────────────────────────────────────────────────────
//
// Every field is written explicitly and read with a fallback. A project file is
// untrusted input: a missing or wrong-typed field must degrade to a sensible
// default, never throw and never abort the load.

Json toJson(EntityId id) { return Json{static_cast<std::int64_t>(id.value())}; }
EntityId idFrom(const Json& value) { return EntityId{static_cast<EntityId::Value>(value.asInt(0))}; }

Json toJson(const MidiEvent& event)
{
    Json json = Json::object();
    json.set("type", static_cast<std::int64_t>(event.type));
    json.set("tick", static_cast<std::int64_t>(event.tick));
    json.set("duration", static_cast<std::int64_t>(event.duration));
    json.set("channel", static_cast<std::int64_t>(event.channel));
    json.set("key", static_cast<std::int64_t>(event.key));
    json.set("value", static_cast<std::int64_t>(event.value));
    json.set("releaseValue", static_cast<std::int64_t>(event.releaseValue));
    json.set("probability", event.probability);
    json.set("pan", event.pan);
    json.set("fineTune", event.fineTune);
    json.set("label", event.label);
    return json;
}

MidiEvent midiEventFrom(const Json& json)
{
    MidiEvent event;
    event.type         = static_cast<MidiEventType>(json["type"].asInt(0));
    event.tick         = json["tick"].asInt(0);
    event.duration     = json["duration"].asInt(0);
    event.channel      = static_cast<int>(json["channel"].asInt(0));
    event.key          = static_cast<int>(json["key"].asInt(60));
    event.value        = static_cast<int>(json["value"].asInt(100));
    event.releaseValue = static_cast<int>(json["releaseValue"].asInt(64));
    event.probability  = json["probability"].asDouble(1.0);
    event.pan          = json["pan"].asDouble(0.0);
    event.fineTune     = json["fineTune"].asDouble(0.0);
    event.label        = json["label"].asString();
    return event;
}

Json toJson(const AutomationPoint& point)
{
    Json json = Json::object();
    json.set("tick", static_cast<std::int64_t>(point.tick));
    json.set("value", point.value);
    json.set("curve", static_cast<std::int64_t>(point.curve));
    json.set("tension", point.tension);
    return json;
}

AutomationPoint automationPointFrom(const Json& json)
{
    AutomationPoint point;
    point.tick    = json["tick"].asInt(0);
    point.value   = json["value"].asDouble(0.0);
    point.curve   = static_cast<AutomationCurve>(json["curve"].asInt(0));
    point.tension = json["tension"].asDouble(0.0);
    return point;
}

Json toJson(const plugins::PluginIdentifier& plugin)
{
    return Json{plugin.isValid() ? plugin.toString() : std::string{}};
}

plugins::PluginIdentifier pluginFrom(const Json& json)
{
    plugins::PluginIdentifier plugin;
    const std::string text = json.asString();

    if (!text.empty())
        (void)plugins::PluginIdentifier::fromString(text, plugin);

    return plugin;
}

Json toJson(const PluginSlot& slot)
{
    Json json = Json::object();
    json.set("id", toJson(slot.id));
    json.set("plugin", toJson(slot.plugin));
    json.set("bypassed", slot.bypassed);
    json.set("stateFile", slot.stateFile);
    return json;
}

PluginSlot pluginSlotFrom(const Json& json)
{
    PluginSlot slot;
    slot.id        = idFrom(json["id"]);
    slot.plugin    = pluginFrom(json["plugin"]);
    slot.bypassed  = json["bypassed"].asBool(false);
    slot.stateFile = json["stateFile"].asString();
    return slot;
}

} // namespace

std::string projectFormatVersionString()
{
    return std::to_string(projectFormatMajor) + "." + std::to_string(projectFormatMinor);
}

// ── Save ──────────────────────────────────────────────────────────────────────

ProjectFile::Result ProjectFile::save(const Project& project, const fs::path& path)
{
    Result result;
    std::error_code code;

    fs::create_directories(path, code);
    if (code) {
        result.error = "cannot create package: " + code.message();
        return result;
    }

    fs::create_directories(path / patternsDirName, code);
    fs::create_directories(path / historyDirName, code);

    // ── manifest.json ────────────────────────────────────────────────────────
    // Kept minimal and plain on purpose: whatever else changes in the format,
    // any future version must be able to read this file to find out what it is
    // looking at (docs/PROJECT_FORMAT.md §2).
    Json manifest = Json::object();
    manifest.set("incdaw_project_version", projectFormatVersionString());
    manifest.set("created_with", project.metadata().createdWith);
    manifest.set("last_saved_with", project.metadata().lastSavedWith);
    manifest.set("created", project.metadata().created);
    manifest.set("modified", project.metadata().modified);

    if (!writeTextFile(path / manifestFileName, manifest.dump(), result.error))
        return result;

    // ── project.json ─────────────────────────────────────────────────────────
    Json document = Json::object();

    Json metadata = Json::object();
    metadata.set("title", project.metadata().title);
    metadata.set("artist", project.metadata().artist);
    metadata.set("comment", project.metadata().comment);
    document.set("metadata", std::move(metadata));

    Json tempo = Json::object();
    Json tempoEvents = Json::array();
    for (const auto& event : project.tempoMap().tempoEvents()) {
        Json entry = Json::object();
        entry.set("tick", static_cast<std::int64_t>(event.tick));
        entry.set("bpm", event.beatsPerMinute);
        tempoEvents.append(std::move(entry));
    }
    tempo.set("events", std::move(tempoEvents));

    Json signatureEvents = Json::array();
    for (const auto& event : project.tempoMap().timeSignatureEvents()) {
        Json entry = Json::object();
        entry.set("tick", static_cast<std::int64_t>(event.tick));
        entry.set("numerator", static_cast<std::int64_t>(event.signature.numerator));
        entry.set("denominator", static_cast<std::int64_t>(event.signature.denominator));
        signatureEvents.append(std::move(entry));
    }
    tempo.set("timeSignatures", std::move(signatureEvents));
    document.set("tempo", std::move(tempo));

    document.set("masterMixerNode", toJson(project.masterMixerNode()));
    document.set("nextEntityId", static_cast<std::int64_t>(
        const_cast<Project&>(project).ids().peekNext()));

    Json mixerNodes = Json::array();
    for (const MixerNode& node : project.mixerNodes()) {
        Json json = Json::object();
        json.set("id", toJson(node.id));
        json.set("type", static_cast<std::int64_t>(node.type));
        json.set("name", node.name);
        json.set("colour", static_cast<std::int64_t>(node.colour));
        json.set("volume", node.volume);
        json.set("pan", node.pan);
        json.set("muted", node.muted);
        json.set("soloed", node.soloed);
        json.set("polarityFlip", node.polarityFlip);

        Json inserts = Json::array();
        for (const PluginSlot& slot : node.inserts)
            inserts.append(toJson(slot));
        json.set("inserts", std::move(inserts));

        mixerNodes.append(std::move(json));
    }
    document.set("mixerNodes", std::move(mixerNodes));

    Json tracks = Json::array();
    for (const Track& track : project.tracks()) {
        Json json = Json::object();
        json.set("id", toJson(track.id));
        json.set("type", static_cast<std::int64_t>(track.type));
        json.set("name", track.name);
        json.set("colour", static_cast<std::int64_t>(track.colour));
        json.set("parent", toJson(track.parent));
        json.set("outputMixerNode", toJson(track.outputMixerNode));
        json.set("muted", track.muted);
        json.set("soloed", track.soloed);
        json.set("height", static_cast<std::int64_t>(track.height));
        tracks.append(std::move(json));
    }
    document.set("tracks", std::move(tracks));

    Json channels = Json::array();
    for (const Channel& channel : project.channels()) {
        Json json = Json::object();
        json.set("id", toJson(channel.id));
        json.set("name", channel.name);
        json.set("colour", static_cast<std::int64_t>(channel.colour));
        json.set("outputMixerNode", toJson(channel.outputMixerNode));
        json.set("volume", channel.volume);
        json.set("pan", channel.pan);
        json.set("muted", channel.muted);
        json.set("soloed", channel.soloed);
        json.set("instrument", toJson(channel.instrument));
        json.set("instrumentStateFile", channel.instrumentStateFile);
        channels.append(std::move(json));
    }
    document.set("channels", std::move(channels));

    Json clips = Json::array();
    for (const Clip& clip : project.clips()) {
        Json json = Json::object();
        json.set("id", toJson(clip.id));
        json.set("type", static_cast<std::int64_t>(clip.type));
        json.set("track", toJson(clip.track));
        json.set("source", toJson(clip.source));
        json.set("startTick", static_cast<std::int64_t>(clip.startTick));
        json.set("lengthTicks", static_cast<std::int64_t>(clip.lengthTicks));
        json.set("sourceOffsetTicks", static_cast<std::int64_t>(clip.sourceOffsetTicks));
        json.set("start", static_cast<std::int64_t>(clip.start));
        json.set("length", static_cast<std::int64_t>(clip.length));
        json.set("sourceOffset", static_cast<std::int64_t>(clip.sourceOffset));
        json.set("gain", clip.gain);
        json.set("pan", clip.pan);
        json.set("normalize", clip.normalize);
        json.set("reversed", clip.reversed);
        json.set("muted", clip.muted);
        json.set("locked", clip.locked);
        json.set("fadeInFrames", static_cast<std::int64_t>(clip.fadeInFrames));
        json.set("fadeOutFrames", static_cast<std::int64_t>(clip.fadeOutFrames));
        json.set("pitchSemitones", clip.pitchSemitones);
        json.set("stretchRatio", clip.stretchRatio);
        json.set("name", clip.name);
        json.set("colour", static_cast<std::int64_t>(clip.colour));
        clips.append(std::move(json));
    }
    document.set("clips", std::move(clips));

    Json automation = Json::array();
    for (const AutomationLane& lane : project.automation()) {
        Json json = Json::object();
        json.set("id", toJson(lane.id));
        json.set("targetEntity", toJson(lane.targetEntity));
        json.set("parameterKey", lane.parameterKey);

        Json points = Json::array();
        for (const AutomationPoint& point : lane.points)
            points.append(toJson(point));
        json.set("points", std::move(points));

        automation.append(std::move(json));
    }
    document.set("automation", std::move(automation));

    Json assets = Json::array();
    for (const AudioAsset& asset : project.audioAssets()) {
        Json json = Json::object();
        json.set("id", toJson(asset.id));
        json.set("relativePath", asset.relativePath);
        json.set("absolutePath", asset.absolutePath);
        json.set("contentHash", asset.contentHash);
        json.set("embedded", asset.embedded);
        json.set("sampleRate", asset.sampleRate);
        json.set("frameCount", static_cast<std::int64_t>(asset.frameCount));
        json.set("channelCount", static_cast<std::int64_t>(asset.channelCount));
        assets.append(std::move(json));
    }
    document.set("audioAssets", std::move(assets));

    Json routing = Json::array();
    for (const RoutingConnection& connection : project.routing()) {
        Json json = Json::object();
        json.set("id", toJson(connection.id));
        json.set("source", toJson(connection.source));
        json.set("destination", toJson(connection.destination));
        json.set("gain", connection.gain);
        json.set("isSend", connection.isSend);
        json.set("preFader", connection.preFader);
        json.set("sidechain", connection.sidechain);
        routing.append(std::move(json));
    }
    document.set("routing", std::move(routing));

    // Patterns are written one file each rather than inline. A corrupted
    // pattern then costs one pattern, and the encoding can change per file in a
    // later version without touching project.json.
    Json patternIndex = Json::array();
    for (const Pattern& pattern : project.patterns()) {
        const std::string fileName = std::to_string(pattern.id.value()) + ".pat";

        Json json = Json::object();
        json.set("id", toJson(pattern.id));
        json.set("name", pattern.name);
        json.set("colour", static_cast<std::int64_t>(pattern.colour));
        json.set("length", static_cast<std::int64_t>(pattern.length));
        json.set("swing", pattern.swing);
        json.set("swingGrid", static_cast<std::int64_t>(pattern.swingGrid));

        Json patternChannels = Json::array();
        for (const PatternChannelContent& content : pattern.channels) {
            Json entry = Json::object();
            entry.set("channel", toJson(content.channel));
            entry.set("loopLength", static_cast<std::int64_t>(content.loopLength));

            Json contentEvents = Json::array();
            for (const MidiEvent& event : content.events)
                contentEvents.append(toJson(event));
            entry.set("events", std::move(contentEvents));

            patternChannels.append(std::move(entry));
        }
        json.set("channels", std::move(patternChannels));

        Json lanes = Json::array();
        for (const EntityId lane : pattern.automationLanes)
            lanes.append(toJson(lane));
        json.set("automationLanes", std::move(lanes));

        if (!writeTextFile(path / patternsDirName / fileName, json.dump(), result.error))
            return result;

        Json entry = Json::object();
        entry.set("id", toJson(pattern.id));
        entry.set("file", fileName);
        patternIndex.append(std::move(entry));
    }
    document.set("patterns", std::move(patternIndex));

    if (!writeTextFile(path / projectFileName, document.dump(), result.error))
        return result;

    result.succeeded = true;
    return result;
}

// ── Load ──────────────────────────────────────────────────────────────────────

namespace {

/// Attaches pattern content that names no channel to a real one.
///
/// Only 1.0 files produce it, and only because that format had no notion of a
/// channel owning notes. Every such pattern is given the project's first
/// channel, creating one if the project has none — the alternative is a project
/// that loads with its notes present but unplayable.
void bindUnassignedContent(Project& project)
{
    bool needsChannel = false;
    for (const Pattern& pattern : project.patterns())
        for (const PatternChannelContent& content : pattern.channels)
            needsChannel = needsChannel || !content.channel.isValid();

    if (!needsChannel)
        return;

    const EntityId target = project.channels().empty() ? project.addChannel("Channel 1").id
                                                       : project.channels().front().id;

    for (Pattern& pattern : project.patterns()) {
        for (PatternChannelContent& content : pattern.channels) {
            if (!content.channel.isValid())
                content.channel = target;
        }

        // Two blocks for the same channel cannot happen in a 1.0 file, but
        // merging rather than trusting that keeps the invariant — one content
        // block per channel — true by construction.
        for (std::size_t index = pattern.channels.size(); index > 1; --index) {
            PatternChannelContent& later = pattern.channels[index - 1];
            for (std::size_t earlier = 0; earlier + 1 < index; ++earlier) {
                if (pattern.channels[earlier].channel != later.channel)
                    continue;

                auto& destination = pattern.channels[earlier].events;
                destination.insert(destination.end(), later.events.begin(), later.events.end());
                pattern.channels.erase(pattern.channels.begin()
                                       + static_cast<std::ptrdiff_t>(index - 1));
                break;
            }
        }
    }
}

} // namespace

ProjectFile::Result ProjectFile::load(Project& project, const fs::path& path)
{
    Result result;

    std::error_code code;
    if (!fs::is_directory(path, code)) {
        result.error = "not an INCDAW package: " + path.string();
        return result;
    }

    std::string manifestText;
    if (!readTextFile(path / manifestFileName, manifestText, result.error))
        return result;

    Json        manifest;
    std::string parseError;
    if (!Json::parse(manifestText, manifest, parseError)) {
        result.error = "manifest is not valid JSON: " + parseError;
        return result;
    }

    const std::string version = manifest["incdaw_project_version"].asString();
    if (version.empty()) {
        result.error = "manifest has no format version";
        return result;
    }

    int major = 0;
    int minor = 0;
    if (std::sscanf(version.c_str(), "%d.%d", &major, &minor) != 2) {
        result.error = "unreadable format version: " + version;
        return result;
    }

    // Opening a project written by a newer INCDAW is refused rather than
    // attempted optimistically: a hopeful load that drops fields it does not
    // understand destroys the user's work on the next save.
    if (major > projectFormatMajor || (major == projectFormatMajor && minor > projectFormatMinor)) {
        result.error = "this project was saved by a newer version of INCDAW (format "
                     + version + "; this build reads up to " + projectFormatVersionString() + ")";
        return result;
    }

    std::string documentText;
    if (!readTextFile(path / projectFileName, documentText, result.error))
        return result;

    Json document;
    if (!Json::parse(documentText, document, parseError)) {
        result.error = "project.json is not valid JSON: " + parseError;
        return result;
    }

    if (major < projectFormatMajor || minor < projectFormatMinor) {
        const Result migration = migrate(document, major, minor);
        if (!migration.succeeded) {
            result.error = migration.error;
            return result;
        }

        result.migrated     = true;
        result.migratedFrom = version;
    }

    // ── Rebuild the model ────────────────────────────────────────────────────
    project = Project{};

    project.metadata().title   = document["metadata"]["title"].asString();
    project.metadata().artist  = document["metadata"]["artist"].asString();
    project.metadata().comment = document["metadata"]["comment"].asString();
    project.metadata().createdWith   = manifest["created_with"].asString();
    project.metadata().lastSavedWith = manifest["last_saved_with"].asString();
    project.metadata().created  = manifest["created"].asString();
    project.metadata().modified = manifest["modified"].asString();

    std::vector<engine::TempoEvent> tempoEvents;
    for (const Json& entry : document["tempo"]["events"].elements())
        tempoEvents.push_back({entry["tick"].asInt(0), entry["bpm"].asDouble(120.0)});

    if (!tempoEvents.empty())
        project.tempoMap().setTempoEvents(std::move(tempoEvents));

    std::vector<engine::TimeSignatureEvent> signatureEvents;
    for (const Json& entry : document["tempo"]["timeSignatures"].elements())
        signatureEvents.push_back({entry["tick"].asInt(0),
                                   engine::TimeSignature{static_cast<int>(entry["numerator"].asInt(4)),
                                                         static_cast<int>(entry["denominator"].asInt(4))}});

    if (!signatureEvents.empty())
        project.tempoMap().setTimeSignatureEvents(std::move(signatureEvents));

    // The default-constructed project already created a master node; the stored
    // one replaces it wholesale.
    project.mixerNodes().clear();

    for (const Json& json : document["mixerNodes"].elements()) {
        MixerNode node;
        node.id           = idFrom(json["id"]);
        node.type         = static_cast<MixerNodeType>(json["type"].asInt(0));
        node.name         = json["name"].asString();
        node.colour       = static_cast<std::uint32_t>(json["colour"].asInt(0xFF404040));
        node.volume       = json["volume"].asDouble(1.0);
        node.pan          = json["pan"].asDouble(0.0);
        node.muted        = json["muted"].asBool(false);
        node.soloed       = json["soloed"].asBool(false);
        node.polarityFlip = json["polarityFlip"].asBool(false);

        for (const Json& slot : json["inserts"].elements())
            node.inserts.push_back(pluginSlotFrom(slot));

        project.mixerNodes().push_back(std::move(node));
    }

    for (const Json& json : document["tracks"].elements()) {
        Track track;
        track.id              = idFrom(json["id"]);
        track.type            = static_cast<TrackType>(json["type"].asInt(0));
        track.name            = json["name"].asString();
        track.colour          = static_cast<std::uint32_t>(json["colour"].asInt(0xFF505050));
        track.parent          = idFrom(json["parent"]);
        track.outputMixerNode = idFrom(json["outputMixerNode"]);
        track.muted           = json["muted"].asBool(false);
        track.soloed          = json["soloed"].asBool(false);
        track.height          = static_cast<int>(json["height"].asInt(64));
        project.tracks().push_back(std::move(track));
    }

    for (const Json& json : document["channels"].elements()) {
        Channel channel;
        channel.id                  = idFrom(json["id"]);
        channel.name                = json["name"].asString();
        channel.colour              = static_cast<std::uint32_t>(json["colour"].asInt(0xFF808080));
        channel.outputMixerNode     = idFrom(json["outputMixerNode"]);
        channel.volume              = json["volume"].asDouble(1.0);
        channel.pan                 = json["pan"].asDouble(0.0);
        channel.muted               = json["muted"].asBool(false);
        channel.soloed              = json["soloed"].asBool(false);
        channel.instrument          = pluginFrom(json["instrument"]);
        channel.instrumentStateFile = json["instrumentStateFile"].asString();
        project.channels().push_back(std::move(channel));
    }

    // Format 1.0 placed clips in frames. Converting here rather than in
    // `migrate` is deliberate: the conversion needs the tempo map, which exists
    // only once the document has been read this far.
    const bool legacyClipTiming = major == 1 && minor < 1;

    for (const Json& json : document["clips"].elements()) {
        Clip clip;
        clip.id             = idFrom(json["id"]);
        clip.type           = static_cast<ClipType>(json["type"].asInt(0));
        clip.track          = idFrom(json["track"]);
        clip.source         = idFrom(json["source"]);
        clip.start          = json["start"].asInt(0);
        clip.length         = json["length"].asInt(0);
        clip.sourceOffset   = json["sourceOffset"].asInt(0);

        if (legacyClipTiming) {
            const engine::TempoMap& tempo = project.tempoMap();
            clip.startTick         = tempo.tickForFrame(clip.start);
            clip.lengthTicks       = tempo.tickForFrame(clip.length);
            clip.sourceOffsetTicks = tempo.tickForFrame(clip.sourceOffset);
        } else {
            clip.startTick         = json["startTick"].asInt(0);
            clip.lengthTicks       = json["lengthTicks"].asInt(0);
            clip.sourceOffsetTicks = json["sourceOffsetTicks"].asInt(0);
        }
        clip.gain           = json["gain"].asDouble(1.0);
        clip.pan            = json["pan"].asDouble(0.0);
        clip.normalize      = json["normalize"].asBool(false);
        clip.reversed       = json["reversed"].asBool(false);
        clip.muted          = json["muted"].asBool(false);
        clip.locked         = json["locked"].asBool(false);
        clip.fadeInFrames   = json["fadeInFrames"].asInt(0);
        clip.fadeOutFrames  = json["fadeOutFrames"].asInt(0);
        clip.pitchSemitones = json["pitchSemitones"].asDouble(0.0);
        clip.stretchRatio   = json["stretchRatio"].asDouble(1.0);
        clip.name           = json["name"].asString();
        clip.colour         = static_cast<std::uint32_t>(json["colour"].asInt(0xFF6699CC));
        project.clips().push_back(std::move(clip));
    }

    for (const Json& json : document["automation"].elements()) {
        AutomationLane lane;
        lane.id           = idFrom(json["id"]);
        lane.targetEntity = idFrom(json["targetEntity"]);
        lane.parameterKey = json["parameterKey"].asString();

        for (const Json& point : json["points"].elements())
            lane.points.push_back(automationPointFrom(point));

        project.automation().push_back(std::move(lane));
    }

    for (const Json& json : document["audioAssets"].elements()) {
        AudioAsset asset;
        asset.id           = idFrom(json["id"]);
        asset.relativePath = json["relativePath"].asString();
        asset.absolutePath = json["absolutePath"].asString();
        asset.contentHash  = json["contentHash"].asString();
        asset.embedded     = json["embedded"].asBool(false);
        asset.sampleRate   = json["sampleRate"].asDouble(0.0);
        asset.frameCount   = json["frameCount"].asInt(0);
        asset.channelCount = static_cast<std::size_t>(json["channelCount"].asInt(0));
        project.audioAssets().push_back(std::move(asset));
    }

    for (const Json& json : document["routing"].elements()) {
        RoutingConnection connection;
        connection.id          = idFrom(json["id"]);
        connection.source      = idFrom(json["source"]);
        connection.destination = idFrom(json["destination"]);
        connection.gain        = json["gain"].asDouble(1.0);
        connection.isSend      = json["isSend"].asBool(false);
        connection.preFader    = json["preFader"].asBool(false);
        connection.sidechain   = json["sidechain"].asBool(false);
        project.routing().push_back(connection);
    }

    for (const Json& entry : document["patterns"].elements()) {
        const std::string fileName = entry["file"].asString();
        if (fileName.empty())
            continue;

        std::string patternText;
        std::string readError;
        if (!readTextFile(path / patternsDirName / fileName, patternText, readError))
            continue;   // a lost pattern must not cost the whole session

        Json patternJson;
        if (!Json::parse(patternText, patternJson, parseError))
            continue;

        Pattern pattern;
        pattern.id     = idFrom(patternJson["id"]);
        pattern.name   = patternJson["name"].asString();
        pattern.colour = static_cast<std::uint32_t>(patternJson["colour"].asInt(0xFF808080));
        pattern.length = patternJson["length"].asInt(engine::ticksPerQuarterNote * 4);
        pattern.swing     = patternJson["swing"].asDouble(0.0);
        pattern.swingGrid = patternJson["swingGrid"].asInt(engine::ticksPerQuarterNote / 4);

        for (const Json& channelJson : patternJson["channels"].elements()) {
            PatternChannelContent content;
            content.channel    = idFrom(channelJson["channel"]);
            content.loopLength = channelJson["loopLength"].asInt(0);

            for (const Json& event : channelJson["events"].elements())
                content.events.push_back(midiEventFrom(event));

            pattern.channels.push_back(std::move(content));
        }

        // Format 1.0 wrote one flat event list per pattern. It is read into an
        // unassigned content block, which `bindUnassignedContent` then attaches
        // to a real channel once the id generator has been restored.
        if (pattern.channels.empty()) {
            PatternChannelContent legacy;
            for (const Json& event : patternJson["events"].elements())
                legacy.events.push_back(midiEventFrom(event));

            if (!legacy.events.empty())
                pattern.channels.push_back(std::move(legacy));
        }

        for (const Json& lane : patternJson["automationLanes"].elements())
            pattern.automationLanes.push_back(idFrom(lane));

        project.patterns().push_back(std::move(pattern));
    }

    // The generator must not mint an id that already exists in the file.
    project.ids() = IdGenerator{static_cast<EntityId::Value>(document["nextEntityId"].asInt(1)) - 1};

    for (const auto& track : project.tracks())      project.ids().observe(track.id);
    for (const auto& channel : project.channels())  project.ids().observe(channel.id);
    for (const auto& node : project.mixerNodes())   project.ids().observe(node.id);
    for (const auto& pattern : project.patterns())  project.ids().observe(pattern.id);
    for (const auto& clip : project.clips())        project.ids().observe(clip.id);
    for (const auto& lane : project.automation())   project.ids().observe(lane.id);
    for (const auto& asset : project.audioAssets()) project.ids().observe(asset.id);
    for (const auto& link : project.routing())      project.ids().observe(link.id);

    // After the generator, so that a channel minted here cannot collide with an
    // id already in the file.
    bindUnassignedContent(project);

    result.succeeded = true;
    return result;
}

ProjectFile::Result ProjectFile::migrate(Json& document, int major, int minor)
{
    (void)document;

    Result result;

    // Migrations form a chain (v1.0 -> v1.1 -> v2.0); there is deliberately no
    // direct path between distant versions to maintain. Version 1.0 is the
    // first format, so the chain is currently empty — but the hook and its test
    // fixture exist from the start, because a migration framework added later
    // is a migration framework that was needed earlier.
    if (major == projectFormatMajor && minor == projectFormatMinor) {
        result.succeeded = true;
        return result;
    }

    // 1.0 -> 1.1. The two shape changes (per-channel pattern content, tick-based
    // clip placement) both need context this hook does not have: the pattern
    // files are separate documents, and the frame-to-tick conversion needs the
    // tempo map. Both are therefore performed at their read sites, which know
    // the version they are reading. This hook remains the single place that
    // decides whether a path exists at all.
    if (major == 1 && minor == 0) {
        result.succeeded = true;
        return result;
    }

    result.error = "no migration path from format " + std::to_string(major) + "."
                 + std::to_string(minor);
    return result;
}

bool ProjectFile::isProjectPackage(const fs::path& path)
{
    std::error_code code;
    return fs::is_directory(path, code) && fs::exists(path / manifestFileName, code);
}

std::string ProjectFile::versionOf(const fs::path& path)
{
    std::string text;
    std::string error;

    if (!readTextFile(path / manifestFileName, text, error))
        return {};

    Json manifest;
    if (!Json::parse(text, manifest, error))
        return {};

    return manifest["incdaw_project_version"].asString();
}

} // namespace incdaw::project
