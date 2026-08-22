#include "project/PresetLibrary.h"

#include "engine/dsp/effects/BuiltinEffects.h"
#include "engine/instrument/BuiltinInstruments.h"
#include "project/Json.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <system_error>
#include <utility>

namespace incdaw::project {
namespace {

constexpr const char* extension  = ".json";
constexpr const char* formatTag  = "incdaw.preset";

/// The parameter table behind a uid, whichever catalogue owns it. Presets do
/// not distinguish instruments from effects: both are "a uid with parameters",
/// which is the whole reason one library serves both.
struct ParameterTable {
    const engine::dsp::EffectParameter* items = nullptr;
    std::size_t                         count = 0;
};

[[nodiscard]] ParameterTable parametersFor(std::string_view uid)
{
    const std::string key{uid};

    if (const engine::dsp::BuiltinEffectInfo* effect = engine::dsp::findBuiltinEffect(key))
        return {effect->parameters, effect->parameterCount};

    if (const engine::BuiltinInstrumentInfo* instrument = engine::findBuiltinInstrument(key))
        return {instrument->parameters, instrument->parameterCount};

    return {};
}

[[nodiscard]] std::string readTextFile(const std::filesystem::path& file)
{
    std::ifstream stream(file, std::ios::binary);
    if (!stream)
        return {};

    std::ostringstream text;
    text << stream.rdbuf();
    return text.str();
}

/// Writes through a temporary and renames, so an interrupted save leaves the
/// previous preset intact rather than half a file.
[[nodiscard]] bool writeTextFile(const std::filesystem::path& file, const std::string& text)
{
    if (file.empty())
        return false;

    std::error_code code;
    if (const std::filesystem::path parent = file.parent_path(); !parent.empty()) {
        std::filesystem::create_directories(parent, code);
        if (code)
            return false;
    }

    const std::filesystem::path temporary = std::filesystem::path(file).concat(".tmp");

    {
        std::ofstream stream(temporary, std::ios::binary | std::ios::trunc);
        if (!stream)
            return false;

        stream << text << '\n';
        if (!stream)
            return false;
    }

    std::filesystem::rename(temporary, file, code);
    if (code) {
        std::filesystem::remove(temporary, code);
        return false;
    }

    return true;
}

} // namespace

// ── Preset ───────────────────────────────────────────────────────────────────

std::string Preset::toJson() const
{
    Json root = Json::object();
    root.set("format", formatTag);
    root.set("version", presetFormatVersion);
    root.set("uid", uid);
    root.set("name", name);

    Json list = Json::array();
    for (const engine::dsp::PresetValue& value : values) {
        Json entry = Json::object();
        entry.set("id", static_cast<std::int64_t>(value.parameterId));
        entry.set("value", value.value);
        list.append(std::move(entry));
    }
    root.set("values", std::move(list));

    return root.dump();
}

bool Preset::fromJson(const std::string& text,
                      std::string_view   expectedUid,
                      std::string_view   name,
                      Preset&            out,
                      std::string&       error)
{
    Json root;
    if (!Json::parse(text, root, error) || !root.isObject()) {
        if (error.empty())
            error = "not a preset document";
        return false;
    }

    if (root["format"].asString() != formatTag) {
        error = "not an INCDAW preset";
        return false;
    }

    // A preset written by a newer INCDAW may mean something this build would
    // misread; refusing it keeps a downgrade from silently changing a sound.
    const std::int64_t version = root["version"].asInt(0);
    if (version <= 0 || version > presetFormatVersion) {
        error = "preset version " + std::to_string(version) + " is not supported";
        return false;
    }

    const std::string uid = root["uid"].asString();
    if (uid != std::string{expectedUid}) {
        error = "preset belongs to " + (uid.empty() ? std::string{"nothing"} : uid);
        return false;
    }

    Preset preset;
    preset.uid  = uid;
    preset.name = std::string{name};

    const Json& list = root["values"];
    if (!list.isArray()) {
        error = "preset has no values";
        return false;
    }

    for (const Json& entry : list.elements()) {
        if (!entry.isObject())
            continue;

        engine::dsp::PresetValue value;
        value.parameterId = static_cast<std::uint32_t>(entry["id"].asInt(0));
        value.value       = entry["value"].asDouble(0.0);
        preset.values.push_back(value);
    }

    out = std::move(preset);
    error.clear();
    return true;
}

// ── Catalogue lookups ────────────────────────────────────────────────────────

engine::dsp::FactoryPresetTable factoryPresetsFor(std::string_view uid)
{
    const std::string key{uid};

    if (const engine::dsp::BuiltinEffectInfo* effect = engine::dsp::findBuiltinEffect(key))
        return effect->presets;

    if (const engine::BuiltinInstrumentInfo* instrument = engine::findBuiltinInstrument(key))
        return instrument->presets;

    return {};
}

std::vector<engine::dsp::PresetValue> defaultPresetValues(std::string_view uid)
{
    const ParameterTable table = parametersFor(uid);

    std::vector<engine::dsp::PresetValue> values;
    values.reserve(table.count);

    for (std::size_t index = 0; index < table.count; ++index)
        values.push_back({table.items[index].id, table.items[index].defaultValue});

    return values;
}

// ── PresetLibrary ────────────────────────────────────────────────────────────

PresetLibrary::PresetLibrary(std::filesystem::path directory)
    : directory_(std::move(directory))
{
}

std::string PresetLibrary::sanitiseName(std::string_view name)
{
    std::string result;
    result.reserve(name.size());

    for (const char character : name) {
        const unsigned char raw = static_cast<unsigned char>(character);

        // Path separators and control characters would make the name mean
        // something to the filesystem that it does not mean to the user.
        if (raw < 0x20 || character == '/' || character == '\\' || character == ':')
            continue;

        result.push_back(character);
    }

    const std::size_t first = result.find_first_not_of(" \t.");
    const std::size_t last  = result.find_last_not_of(" \t.");
    if (first == std::string::npos)
        return {};

    result = result.substr(first, last - first + 1);

    if (result.size() > 64)
        result.resize(64);

    return result;
}

std::filesystem::path PresetLibrary::directoryFor(std::string_view uid) const
{
    if (directory_.empty())
        return {};

    // The uid comes from the catalogue rather than from a user, but it still
    // becomes a folder name, and a uid is not automatically a legal one.
    const std::string safe = sanitiseName(uid);
    if (safe.empty())
        return {};

    return directory_ / safe;
}

std::filesystem::path PresetLibrary::fileFor(std::string_view uid, std::string_view name) const
{
    const std::filesystem::path folder = directoryFor(uid);
    if (folder.empty())
        return {};

    const std::string safe = sanitiseName(name);
    if (safe.empty())
        return {};

    return folder / (safe + extension);
}

bool PresetLibrary::isFactoryName(std::string_view uid, std::string_view name)
{
    if (name == defaultPresetName)
        return !defaultPresetValues(uid).empty();

    const engine::dsp::FactoryPresetTable table = factoryPresetsFor(uid);
    for (std::size_t index = 0; index < table.count; ++index)
        if (name == table.items[index].name)
            return true;

    return false;
}

std::vector<PresetLibrary::Entry> PresetLibrary::entries(std::string_view uid) const
{
    std::vector<Entry> result;

    // "Default" first, and only when there is something to default: a target
    // with no parameters has no preset worth listing.
    if (!defaultPresetValues(uid).empty())
        result.push_back(Entry{defaultPresetName, true, {}});

    const engine::dsp::FactoryPresetTable table = factoryPresetsFor(uid);
    for (std::size_t index = 0; index < table.count; ++index)
        result.push_back(Entry{table.items[index].name, true, {}});

    const std::filesystem::path folder = directoryFor(uid);
    if (folder.empty())
        return result;

    std::error_code code;
    std::filesystem::directory_iterator walk(folder, code);
    if (code)
        return result;

    std::vector<Entry> user;
    for (const std::filesystem::directory_entry& item : walk) {
        if (!item.is_regular_file(code) || item.path().extension() != extension)
            continue;

        const std::string name = item.path().stem().string();
        if (name.empty())
            continue;

        user.push_back(Entry{name, false, item.path()});
    }

    std::sort(user.begin(), user.end(), [](const Entry& a, const Entry& b) {
        return a.name < b.name;
    });

    result.insert(result.end(), user.begin(), user.end());
    return result;
}

bool PresetLibrary::contains(std::string_view uid, std::string_view name) const
{
    if (isFactoryName(uid, name))
        return true;

    const std::filesystem::path file = fileFor(uid, name);
    if (file.empty())
        return false;

    std::error_code code;
    return std::filesystem::exists(file, code) && !code;
}

std::optional<Preset> PresetLibrary::resolve(std::string_view uid, std::string_view name) const
{
    if (name == defaultPresetName) {
        std::vector<engine::dsp::PresetValue> values = defaultPresetValues(uid);
        if (values.empty())
            return std::nullopt;

        return Preset{std::string{uid}, defaultPresetName, std::move(values)};
    }

    const engine::dsp::FactoryPresetTable table = factoryPresetsFor(uid);
    for (std::size_t index = 0; index < table.count; ++index) {
        const engine::dsp::FactoryPreset& factory = table.items[index];
        if (name != factory.name)
            continue;

        Preset preset;
        preset.uid  = std::string{uid};
        preset.name = factory.name;
        preset.values.assign(factory.values, factory.values + factory.valueCount);
        return preset;
    }

    const std::filesystem::path file = fileFor(uid, name);
    if (file.empty())
        return std::nullopt;

    std::error_code code;
    if (!std::filesystem::exists(file, code) || code)
        return std::nullopt;

    // The name is the file's, not the document's — the same rule the theme
    // folder follows, and for the same reason: a preset copied in Finder and
    // renamed there is the name the user now means.
    Preset      preset;
    std::string error;
    if (!Preset::fromJson(readTextFile(file), uid, file.stem().string(), preset, error))
        return std::nullopt;

    return preset;
}

bool PresetLibrary::store(const Preset& preset, std::string& error) const
{
    if (preset.uid.empty()) {
        error = "a preset must name what it belongs to";
        return false;
    }

    if (isFactoryName(preset.uid, preset.name)) {
        error = "factory presets cannot be overwritten";
        return false;
    }

    if (preset.values.empty()) {
        error = "a preset with no values would change nothing";
        return false;
    }

    const std::filesystem::path file = fileFor(preset.uid, preset.name);
    if (file.empty()) {
        error = "no writable presets folder";
        return false;
    }

    if (!writeTextFile(file, preset.toJson())) {
        error = "the preset could not be written";
        return false;
    }

    error.clear();
    return true;
}

std::string PresetLibrary::uniqueName(std::string_view uid, std::string_view preferred) const
{
    const std::string base = sanitiseName(preferred);
    if (base.empty())
        return {};

    if (!contains(uid, base))
        return base;

    for (int suffix = 2; suffix < 1000; ++suffix) {
        std::string candidate = base + " " + std::to_string(suffix);
        if (!contains(uid, candidate))
            return candidate;
    }

    return {};
}

std::string PresetLibrary::duplicate(std::string_view uid,
                                     std::string_view name,
                                     std::string_view preferred,
                                     std::string&     error) const
{
    const std::optional<Preset> source = resolve(uid, name);
    if (!source.has_value()) {
        error = "no such preset";
        return {};
    }

    const std::string copyName = uniqueName(uid, preferred);
    if (copyName.empty()) {
        error = "no usable name for the copy";
        return {};
    }

    Preset copy = *source;
    copy.name   = copyName;

    if (!store(copy, error))
        return {};

    error.clear();
    return copyName;
}

bool PresetLibrary::rename(std::string_view uid,
                           std::string_view from,
                           std::string_view to,
                           std::string&     error) const
{
    if (isFactoryName(uid, from)) {
        error = "factory presets cannot be renamed";
        return false;
    }

    if (isFactoryName(uid, to)) {
        error = "that name belongs to a factory preset";
        return false;
    }

    const std::filesystem::path source = fileFor(uid, from);
    const std::filesystem::path target = fileFor(uid, to);
    if (source.empty() || target.empty()) {
        error = "no usable name";
        return false;
    }

    std::error_code code;
    if (!std::filesystem::exists(source, code) || code) {
        error = "no such preset";
        return false;
    }

    if (source != target && std::filesystem::exists(target, code)) {
        error = "a preset of that name already exists";
        return false;
    }

    // The name lives in the file as well as in its path; rewriting rather
    // than renaming keeps the two agreeing even for a reader that trusts the
    // document.
    std::optional<Preset> preset = resolve(uid, from);
    if (!preset.has_value()) {
        error = "the preset could not be read";
        return false;
    }

    preset->name = sanitiseName(to);
    if (!writeTextFile(target, preset->toJson())) {
        error = "the preset could not be written";
        return false;
    }

    if (source != target)
        std::filesystem::remove(source, code);

    error.clear();
    return true;
}

bool PresetLibrary::remove(std::string_view uid, std::string_view name, std::string& error) const
{
    if (isFactoryName(uid, name)) {
        error = "factory presets cannot be deleted";
        return false;
    }

    const std::filesystem::path file = fileFor(uid, name);
    if (file.empty()) {
        error = "no such preset";
        return false;
    }

    std::error_code code;
    if (!std::filesystem::remove(file, code) || code) {
        error = "the preset could not be deleted";
        return false;
    }

    error.clear();
    return true;
}

} // namespace incdaw::project
