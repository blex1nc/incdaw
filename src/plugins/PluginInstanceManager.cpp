#include "plugins/PluginInstanceManager.h"

#include "plugins/au/AudioUnitInstance.h"

#include "plugins/PluginNode.h"

#include <algorithm>

namespace incdaw::plugins {

ClapLibrary* PluginInstanceManager::libraryFor(const std::string& path, std::string& error)
{
    if (const auto found = libraries_.find(path); found != libraries_.end())
        return found->second.get();

    auto library = std::make_unique<ClapLibrary>();

    if (!library->open(path, error))
        return nullptr;   // not cached: a library that failed to open is retried later

    return libraries_.emplace(path, std::move(library)).first->second.get();
}

std::unique_ptr<engine::Node> PluginInstanceManager::createInsert(
    std::uint64_t slotKey, const PluginIdentifier& identifier, double sampleRate,
    std::uint32_t maxFrames, std::string& error)
{
    if (!identifier.isValid()) {
        error = "empty plugin identifier";
        return nullptr;
    }

    if (identifier.format != Format::clap && identifier.format != Format::audioUnit) {
        // Named rather than silently ignored: the project file is telling the
        // truth about what the user loaded, and the UI should say which format
        // is missing instead of showing an empty slot (D-007 orders the work).
        error = std::string(formatName(identifier.format)) + " hosting is not implemented yet";
        return nullptr;
    }

    const std::lock_guard<std::mutex> lock(mutex_);

    // The reuse path: same plugin at the same activation terms means the
    // slot's live instance — and its live state — carries straight on.
    std::vector<std::uint8_t> carriedState;
    bool                      carryState = false;

    if (const auto found = instances_.find(slotKey); found != instances_.end()) {
        Held& held = found->second;

        if (held.uid == identifier.uid && held.sampleRate == sampleRate
            && held.maxFrames == maxFrames)
            return std::make_unique<PluginNode>(held.instance.get());

        // Same plugin at new terms: recreate, but carry the state blob so
        // the plugin does not audibly reset on a device change (D-031).
        if (held.uid == identifier.uid)
            carryState = held.instance->saveState(carriedState);

        instances_.erase(found);
    }

    // Audio Units are not scanned into the registry and have no library on
    // disk to open: the system's component registry IS the catalogue, and the
    // identifier is enough to instantiate one (docs/PLUGIN_HOST.md).
    std::unique_ptr<HostedPlugin> instance;

    if (identifier.format == Format::audioUnit) {
        instance = AudioUnitInstance::create(identifier.uid, sampleRate, maxFrames, error);

        if (instance == nullptr)
            return nullptr;

        return [&]() -> std::unique_ptr<engine::Node> {
            if (carryState)
                (void)instance->loadState(carriedState.data(), carriedState.size());

            parameters_.try_emplace(identifier.uid, instance->parameters());

            Held held;
            held.instance   = std::move(instance);
            held.uid        = identifier.uid;
            held.sampleRate = sampleRate;
            held.maxFrames  = maxFrames;

            HostedPlugin* borrowed = held.instance.get();
            instances_[slotKey]    = std::move(held);

            return std::make_unique<PluginNode>(borrowed);
        }();
    }

    const PluginRegistry::Located located = registry_->find(identifier.uid);
    if (located.library == nullptr) {
        // `find` already skips blacklisted libraries, so this covers both "never
        // scanned" and "scanned and rejected". The registry holds the reason.
        error = "plugin not found in the registry: " + identifier.uid;
        return nullptr;
    }

    ClapLibrary* library = libraryFor(located.library->path, error);
    if (library == nullptr)
        return nullptr;

    instance = library->create(identifier.uid, sampleRate, maxFrames, error);
    if (instance == nullptr)
        return nullptr;

    if (carryState)
        (void)instance->loadState(carriedState.data(), carriedState.size());

    // Cache the discovered parameters on the first successful instance: the
    // list describes the plugin TYPE, so later instances reuse it.
    parameters_.try_emplace(identifier.uid, instance->parameters());

    Held held;
    held.instance   = std::move(instance);
    held.uid        = identifier.uid;
    held.sampleRate = sampleRate;
    held.maxFrames  = maxFrames;

    HostedPlugin* borrowed = held.instance.get();
    instances_[slotKey]    = std::move(held);

    return std::make_unique<PluginNode>(borrowed);
}

void PluginInstanceManager::retainOnlyInstances(const std::vector<std::uint64_t>& slotKeys)
{
    const std::lock_guard<std::mutex> lock(mutex_);

    for (auto it = instances_.begin(); it != instances_.end();) {
        const bool keep = std::find(slotKeys.begin(), slotKeys.end(), it->first)
                       != slotKeys.end();

        it = keep ? std::next(it) : instances_.erase(it);
    }
}

HostedPlugin* PluginInstanceManager::instanceFor(std::uint64_t slotKey) const
{
    const std::lock_guard<std::mutex> lock(mutex_);

    const auto found = instances_.find(slotKey);
    return found != instances_.end() ? found->second.instance.get() : nullptr;
}

const std::vector<PluginParameterInfo>* PluginInstanceManager::parametersFor(
    const std::string& uid) const
{
    const std::lock_guard<std::mutex> lock(mutex_);

    const auto found = parameters_.find(uid);
    return found != parameters_.end() ? &found->second : nullptr;
}

std::size_t PluginInstanceManager::loadedLibraryCount() const
{
    const std::lock_guard<std::mutex> lock(mutex_);
    return libraries_.size();
}

std::size_t PluginInstanceManager::liveInstanceCount() const
{
    const std::lock_guard<std::mutex> lock(mutex_);
    return instances_.size();
}

} // namespace incdaw::plugins
