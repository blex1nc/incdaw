#include "plugins/PluginRegistry.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <sstream>

namespace incdaw::plugins {
namespace {

constexpr const char* formatHeader = "INCDAW-PLUGIN-REGISTRY 1";

std::int64_t mtimeSecondsOf(const std::filesystem::path& path)
{
    std::error_code code;
    const auto time = std::filesystem::last_write_time(path, code);
    if (code)
        return 0;

    return std::chrono::duration_cast<std::chrono::seconds>(
               time.time_since_epoch()).count();
}

std::uint64_t sizeOf(const std::filesystem::path& path)
{
    std::error_code code;

    if (std::filesystem::is_directory(path, code))
        return 0;   // a bundle's identity rides on its mtime

    const auto size = std::filesystem::file_size(path, code);
    return code ? 0 : static_cast<std::uint64_t>(size);
}

/// Tab-joined fields, tab-free by construction: paths and names with a tab
/// in them are not worth a quoting grammar, so tabs are flattened to spaces.
std::string field(std::string text)
{
    std::replace(text.begin(), text.end(), '\t', ' ');
    std::replace(text.begin(), text.end(), '\n', ' ');
    return text;
}

std::vector<std::string> splitFields(const std::string& line)
{
    std::vector<std::string> fields;
    std::size_t start = 0;

    for (std::size_t index = 0; index <= line.size(); ++index) {
        if (index == line.size() || line[index] == '\t') {
            fields.push_back(line.substr(start, index - start));
            start = index + 1;
        }
    }

    return fields;
}

} // namespace

std::size_t PluginRegistry::scanDirectory(const std::filesystem::path& directory,
                                          const std::filesystem::path& scannerBinary)
{
    std::size_t scansRun = 0;

    std::error_code code;
    for (auto entry = std::filesystem::recursive_directory_iterator(directory, code);
         !code && entry != std::filesystem::recursive_directory_iterator(); ++entry) {
        const std::filesystem::path& path = entry->path();

        if (path.extension() != ".clap")
            continue;

        // A bundle is a directory; do not descend into it looking for more.
        if (entry->is_directory())
            entry.disable_recursion_pending();

        const std::uint64_t size  = sizeOf(path);
        const std::int64_t  mtime = mtimeSecondsOf(path);

        Library* known = nullptr;
        for (Library& candidate : libraries_)
            if (candidate.path == path.string())
                known = &candidate;

        // The cache hit: same file, same answer — including "blacklisted".
        if (known != nullptr && known->fileSize == size && known->mtimeSeconds == mtime)
            continue;

        const ScanOutcome outcome = scanOutOfProcess(scannerBinary, path);
        ++scansRun;

        Library fresh;
        fresh.path         = path.string();
        fresh.fileSize     = size;
        fresh.mtimeSeconds = mtime;

        if (outcome.status == ScanOutcome::Status::ok) {
            fresh.plugins = outcome.plugins;
        } else {
            fresh.blacklisted     = true;
            fresh.blacklistReason = outcome.detail;
        }

        if (known != nullptr)
            *known = std::move(fresh);
        else
            libraries_.push_back(std::move(fresh));
    }

    return scansRun;
}

std::vector<PluginRegistry::Located> PluginRegistry::plugins() const
{
    std::vector<Located> results;

    for (const Library& library : libraries_) {
        if (library.blacklisted)
            continue;

        for (const ClapDescriptor& plugin : library.plugins)
            results.push_back({&library, &plugin});
    }

    return results;
}

PluginRegistry::Located PluginRegistry::find(const std::string& pluginId) const
{
    for (const Library& library : libraries_) {
        if (library.blacklisted)
            continue;

        for (const ClapDescriptor& plugin : library.plugins)
            if (plugin.id == pluginId)
                return {&library, &plugin};
    }

    return {};
}

void PluginRegistry::clearBlacklist()
{
    // Erasing the entry entirely (rather than clearing the flag) forces the
    // next scan to re-examine the file: the user wants a retry, not a
    // cached refusal with a clean conscience.
    libraries_.erase(std::remove_if(libraries_.begin(), libraries_.end(),
                                    [](const Library& library) { return library.blacklisted; }),
                     libraries_.end());
}

bool PluginRegistry::save(const std::filesystem::path& path) const
{
    std::ofstream file{path, std::ios::trunc};
    if (!file)
        return false;

    file << formatHeader << "\n";

    for (const Library& library : libraries_) {
        file << "LIBRARY\t" << field(library.path) << "\t" << library.fileSize << "\t"
             << library.mtimeSeconds << "\t" << (library.blacklisted ? "blacklisted" : "ok")
             << "\t" << field(library.blacklistReason) << "\n";

        for (const ClapDescriptor& plugin : library.plugins)
            file << "PLUGIN\t" << field(plugin.id) << "\t" << field(plugin.name) << "\t"
                 << field(plugin.vendor) << "\t" << field(plugin.version) << "\n";
    }

    return bool(file);
}

bool PluginRegistry::load(const std::filesystem::path& path)
{
    std::ifstream file{path};
    if (!file)
        return false;

    std::string line;
    if (!std::getline(file, line) || line != formatHeader)
        return false;   // an unknown version is a rescan, not a guess

    std::vector<Library> loaded;

    while (std::getline(file, line)) {
        const auto fields = splitFields(line);
        if (fields.empty())
            continue;

        if (fields[0] == "LIBRARY" && fields.size() >= 6) {
            Library library;
            library.path            = fields[1];
            library.fileSize        = std::strtoull(fields[2].c_str(), nullptr, 10);
            library.mtimeSeconds    = std::strtoll(fields[3].c_str(), nullptr, 10);
            library.blacklisted     = fields[4] == "blacklisted";
            library.blacklistReason = fields[5];
            loaded.push_back(std::move(library));
        } else if (fields[0] == "PLUGIN" && fields.size() >= 5 && !loaded.empty()) {
            ClapDescriptor plugin;
            plugin.id      = fields[1];
            plugin.name    = fields[2];
            plugin.vendor  = fields[3];
            plugin.version = fields[4];
            loaded.back().plugins.push_back(std::move(plugin));
        }
    }

    libraries_ = std::move(loaded);
    return true;
}

} // namespace incdaw::plugins
