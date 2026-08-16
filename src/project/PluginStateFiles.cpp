#include "project/PluginStateFiles.h"

#include <cstdint>
#include <fstream>

namespace incdaw::project {
namespace fs = std::filesystem;

namespace {

constexpr const char* pluginsDirName = "plugins";

/// Staged through a sibling temporary and renamed, like ProjectFile's text
/// writer: a crash mid-write loses the new blob, never the old one — which is
/// the §6 crash-survival rule at the filesystem level.
bool writeBlobFile(const fs::path& path, const std::vector<std::uint8_t>& blob,
                   std::string& error)
{
    const fs::path staging = path.string() + ".writing";

    {
        std::ofstream stream(staging, std::ios::binary | std::ios::trunc);
        if (!stream) {
            error = "cannot open for writing: " + staging.string();
            return false;
        }

        stream.write(reinterpret_cast<const char*>(blob.data()),
                     static_cast<std::streamsize>(blob.size()));
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

bool readBlobFile(const fs::path& path, std::vector<std::uint8_t>& blob, std::string& error)
{
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if (!stream) {
        error = "cannot open: " + path.string();
        return false;
    }

    const std::streamsize size = stream.tellg();
    stream.seekg(0);

    blob.resize(static_cast<std::size_t>(size));
    stream.read(reinterpret_cast<char*>(blob.data()), size);

    if (!stream) {
        error = "read failed: " + path.string();
        return false;
    }

    return true;
}

std::string stateFileNameFor(const PluginSlot& slot)
{
    return std::string(pluginsDirName) + "/insert-" + std::to_string(slot.id.value())
         + ".state";
}

} // namespace

std::vector<std::string> capturePluginState(Project& project, const CompiledProjectGraph& graph,
                                            const fs::path& packageDir)
{
    std::vector<std::string> warnings;

    std::error_code code;
    fs::create_directories(packageDir / pluginsDirName, code);
    if (code) {
        warnings.push_back("cannot create plugins directory: " + code.message());
        return warnings;
    }

    for (MixerNode& node : project.mixerNodes()) {
        for (PluginSlot& slot : node.inserts) {
            engine::StateIO* state = graph.insertStateFor(slot.id);
            if (state == nullptr)
                continue;   // not live: the slot keeps whatever stateFile it had

            std::vector<std::uint8_t> blob;
            if (!state->saveState(blob)) {
                // The previous blob (if any) stays on disk and in stateFile:
                // state survives a plugin that will not save (§6).
                warnings.push_back("insert \"" + slot.plugin.toString()
                                   + "\" did not save its state; keeping the previous one");
                continue;
            }

            const std::string relative = stateFileNameFor(slot);

            std::string error;
            if (!writeBlobFile(packageDir / relative, blob, error)) {
                warnings.push_back("insert \"" + slot.plugin.toString()
                                   + "\" state not written: " + error);
                continue;
            }

            slot.stateFile = relative;
        }
    }

    return warnings;
}

std::vector<std::string> restorePluginState(const Project& project,
                                            const CompiledProjectGraph& graph,
                                            const fs::path& packageDir)
{
    std::vector<std::string> warnings;

    for (const MixerNode& node : project.mixerNodes()) {
        for (const PluginSlot& slot : node.inserts) {
            if (slot.stateFile.empty())
                continue;

            engine::StateIO* state = graph.insertStateFor(slot.id);
            if (state == nullptr)
                continue;   // missing plugin: the blob stays for when it returns

            std::vector<std::uint8_t> blob;
            std::string               error;

            if (!readBlobFile(packageDir / slot.stateFile, blob, error)) {
                warnings.push_back("insert \"" + slot.plugin.toString()
                                   + "\" state not read: " + error);
                continue;
            }

            if (!state->loadState(blob.data(), blob.size()))
                warnings.push_back("insert \"" + slot.plugin.toString()
                                   + "\" rejected its saved state; using defaults");
        }
    }

    return warnings;
}

std::vector<CarriedInsertState> captureBuiltinInsertState(const Project& project,
                                                          const CompiledProjectGraph& graph)
{
    std::vector<CarriedInsertState> carried;

    for (const MixerNode& node : project.mixerNodes()) {
        for (const PluginSlot& slot : node.inserts) {
            if (slot.plugin.format != plugins::Format::builtin)
                continue;

            engine::StateIO* state = graph.insertStateFor(slot.id);
            if (state == nullptr)
                continue;   // bypassed or unbuilt: nothing live to carry

            CarriedInsertState entry;
            entry.slot = slot.id;

            if (state->saveState(entry.blob))
                carried.push_back(std::move(entry));
        }
    }

    return carried;
}

void restoreBuiltinInsertState(const std::vector<CarriedInsertState>& carried,
                               const CompiledProjectGraph& graph)
{
    for (const CarriedInsertState& entry : carried)
        if (engine::StateIO* state = graph.insertStateFor(entry.slot))
            (void)state->loadState(entry.blob.data(), entry.blob.size());
}

} // namespace incdaw::project
