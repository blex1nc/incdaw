#include "plugins/PluginInstanceManager.h"

#include "plugins/PluginNode.h"

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

std::unique_ptr<engine::Node> PluginInstanceManager::createInsert(const PluginIdentifier& identifier,
                                                                  double                  sampleRate,
                                                                  std::uint32_t           maxFrames,
                                                                  std::string&            error)
{
    if (!identifier.isValid()) {
        error = "empty plugin identifier";
        return nullptr;
    }

    if (identifier.format != Format::clap) {
        // Named rather than silently ignored: the project file is telling the
        // truth about what the user loaded, and the UI should say which format
        // is missing instead of showing an empty slot (D-007 orders the work).
        error = std::string(formatName(identifier.format)) + " hosting is not implemented yet";
        return nullptr;
    }

    const std::lock_guard<std::mutex> lock(mutex_);

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

    auto instance = library->create(identifier.uid, sampleRate, maxFrames, error);
    if (instance == nullptr)
        return nullptr;

    // Cache the discovered parameters on the first successful instance: the
    // list describes the plugin TYPE, so later instances reuse it.
    parameters_.try_emplace(identifier.uid, instance->parameters());

    return std::make_unique<PluginNode>(std::move(instance));
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

} // namespace incdaw::plugins
