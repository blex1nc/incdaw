#include "app/AppSettings.h"

#include "project/Json.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <system_error>

namespace incdaw::app {

using project::Json;

namespace {

/// Block sizes and rates arrive from a file that anyone can edit. A device
/// asked for zero frames does not fail loudly — it fails as silence — so the
/// reader clamps rather than trusts.
double sanitisedSampleRate(double value)
{
    return (value >= 8000.0 && value <= 384000.0) ? value : defaultAudioConfig().sampleRate;
}

std::int64_t sanitisedBufferSize(std::int64_t value)
{
    return (value >= 16 && value <= 8192) ? value : defaultAudioConfig().bufferSize;
}

std::size_t sanitisedChannelCount(std::int64_t value, std::size_t fallback)
{
    return (value >= 0 && value <= 64) ? static_cast<std::size_t>(value) : fallback;
}

} // namespace

std::string AppSettings::toJson() const
{
    Json root = Json::object();
    root.set("format", "incdaw-settings");
    root.set("version", currentVersion);

    Json audioObject = Json::object();
    audioObject.set("outputDevice", audio.outputDeviceIdentifier);
    audioObject.set("inputDevice", audio.inputDeviceIdentifier);
    audioObject.set("sampleRate", audio.sampleRate);
    audioObject.set("bufferSize", static_cast<std::int64_t>(audio.bufferSize));
    audioObject.set("outputChannels", static_cast<std::int64_t>(audio.outputChannels));
    audioObject.set("inputChannels", static_cast<std::int64_t>(audio.inputChannels));
    audioObject.set("openInputAtLaunch", openInputAtLaunch);
    root.set("audio", std::move(audioObject));

    Json midiObject = Json::object();
    Json inputs     = Json::array();
    for (const std::string& identifier : midiInputIdentifiers)
        inputs.append(Json(identifier));
    midiObject.set("inputs", std::move(inputs));
    midiObject.set("output", midiOutputIdentifier);
    midiObject.set("clockRole", midiClockRole);
    root.set("midi", std::move(midiObject));

    Json workspaceObject = Json::object();
    workspaceObject.set("windowX", workspace.windowX);
    workspaceObject.set("windowY", workspace.windowY);
    workspaceObject.set("windowWidth", workspace.windowWidth);
    workspaceObject.set("windowHeight", workspace.windowHeight);
    workspaceObject.set("activeEditor", static_cast<std::int64_t>(workspace.activeEditor));
    workspaceObject.set("songMode", workspace.songMode);
    root.set("workspace", std::move(workspaceObject));

    Json updatesObject = Json::object();
    updatesObject.set("checkAtLaunch", updates.checkAtLaunch);
    updatesObject.set("lastChecked", updates.lastCheckedUnix);
    updatesObject.set("skippedVersion", updates.skippedVersion);
    root.set("updates", std::move(updatesObject));

    Json appearanceObject = Json::object();
    appearanceObject.set("theme", appearance.themeName);
    root.set("appearance", std::move(appearanceObject));

    return root.dump();
}

AppSettings AppSettings::fromJson(const std::string& text)
{
    AppSettings settings;

    Json        root;
    std::string error;
    if (!Json::parse(text, root, error) || !root.isObject())
        return settings;

    const Json& audioObject = root["audio"];
    if (audioObject.isObject()) {
        settings.audio.outputDeviceIdentifier = audioObject["outputDevice"].asString();
        settings.audio.inputDeviceIdentifier  = audioObject["inputDevice"].asString();
        settings.audio.sampleRate     = sanitisedSampleRate(audioObject["sampleRate"].asDouble(settings.audio.sampleRate));
        settings.audio.bufferSize     = sanitisedBufferSize(audioObject["bufferSize"].asInt(settings.audio.bufferSize));
        settings.audio.outputChannels = sanitisedChannelCount(audioObject["outputChannels"].asInt(
                                                                  static_cast<std::int64_t>(settings.audio.outputChannels)),
                                                              settings.audio.outputChannels);
        settings.audio.inputChannels  = sanitisedChannelCount(audioObject["inputChannels"].asInt(
                                                                  static_cast<std::int64_t>(settings.audio.inputChannels)),
                                                              settings.audio.inputChannels);
        settings.openInputAtLaunch    = audioObject["openInputAtLaunch"].asBool(false);
    }

    const Json& inputs = root["midi"]["inputs"];
    if (inputs.isArray()) {
        for (const Json& element : inputs.elements()) {
            if (element.isString() && !element.asString().empty())
                settings.midiInputIdentifiers.push_back(element.asString());
        }
    }

    settings.midiOutputIdentifier = root["midi"]["output"].asString();

    // Anything this build does not recognise is off. A clock is something the
    // user switched on deliberately; guessing at an unknown value would drive
    // hardware nobody asked to drive.
    const std::string clockRole = root["midi"]["clockRole"].asString();
    settings.midiClockRole = (clockRole == "send" || clockRole == "receive") ? clockRole
                                                                              : std::string{"off"};

    const Json& workspaceObject = root["workspace"];
    if (workspaceObject.isObject()) {
        settings.workspace.windowX      = workspaceObject["windowX"].asDouble(0.0);
        settings.workspace.windowY      = workspaceObject["windowY"].asDouble(0.0);
        settings.workspace.windowWidth  = workspaceObject["windowWidth"].asDouble(0.0);
        settings.workspace.windowHeight = workspaceObject["windowHeight"].asDouble(0.0);
        settings.workspace.activeEditor = static_cast<int>(workspaceObject["activeEditor"].asInt(0));
        settings.workspace.songMode     = workspaceObject["songMode"].asBool(false);
    }

    const Json& updatesObject = root["updates"];
    if (updatesObject.isObject()) {
        settings.updates.checkAtLaunch = updatesObject["checkAtLaunch"].asBool(true);

        // A stamp is a fact about the past. A negative one is not, and neither
        // is a file written by hand; either way the check is simply due.
        settings.updates.lastCheckedUnix =
            std::max<std::int64_t>(0, updatesObject["lastChecked"].asInt(0));

        settings.updates.skippedVersion = updatesObject["skippedVersion"].asString();
    }

    const Json& appearanceObject = root["appearance"];
    if (appearanceObject.isObject()) {
        // An empty name is a file written by hand with the key blanked out. It
        // means the default, which is what the field already holds.
        if (const std::string named = appearanceObject["theme"].asString(); !named.empty())
            settings.appearance.themeName = named;
    }

    return settings;
}

bool AppSettings::save(const std::filesystem::path& file) const
{
    if (file.empty())
        return false;

    // Written beside the target and renamed over it: a crash midway through
    // must not leave a truncated preferences file that the next launch then
    // reads as "no preferences at all".
    const std::filesystem::path temporary = std::filesystem::path{file}.concat(".tmp");

    {
        std::ofstream stream(temporary, std::ios::binary | std::ios::trunc);
        if (!stream)
            return false;

        stream << toJson();
        if (!stream)
            return false;
    }

    std::error_code failed;
    std::filesystem::rename(temporary, file, failed);
    if (failed) {
        std::filesystem::remove(temporary, failed);
        return false;
    }

    return true;
}

AppSettings AppSettings::load(const std::filesystem::path& file)
{
    std::ifstream stream(file, std::ios::binary);
    if (!stream)
        return AppSettings{};

    std::ostringstream text;
    text << stream.rdbuf();

    return fromJson(text.str());
}

} // namespace incdaw::app
