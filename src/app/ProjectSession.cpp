#include "app/ProjectSession.h"

#include <algorithm>
#include <cctype>
#include <system_error>

namespace incdaw::app::session {

std::vector<std::string> updatedRecents(std::vector<std::string> list,
                                        const std::string&       path,
                                        std::size_t              cap)
{
    if (path.empty() || cap == 0)
        return list;

    list.erase(std::remove(list.begin(), list.end(), path), list.end());
    list.insert(list.begin(), path);

    if (list.size() > cap)
        list.resize(cap);

    return list;
}

std::filesystem::path autosavePathFor(const std::filesystem::path& projectPath,
                                      const std::filesystem::path& supportDirectory)
{
    if (!projectPath.empty())
        return projectPath.parent_path()
             / (projectPath.stem().string() + ".autosave.incdaw");

    if (!supportDirectory.empty())
        return supportDirectory / "Autosave" / "Untitled.autosave.incdaw";

    return {};
}

std::string exportFileName(const std::string& name, const std::string& extension,
                           const std::vector<std::string>& taken)
{
    std::string base;
    for (const char character : name)
        base += (character == '/' || character == '\\' || character == ':'
                 || static_cast<unsigned char>(character) < 0x20)
                    ? '_'
                    : character;

    if (base.find_first_not_of("_ .") == std::string::npos)
        base = "Untitled";

    const auto lowered = [](std::string text) {
        std::transform(text.begin(), text.end(), text.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return text;
    };

    const auto isTaken = [&](const std::string& candidate) {
        const std::string needle = lowered(candidate);
        return std::any_of(taken.begin(), taken.end(),
                           [&](const std::string& held) { return lowered(held) == needle; });
    };

    std::string candidate = base + extension;
    for (int suffix = 2; isTaken(candidate); ++suffix)
        candidate = base + " " + std::to_string(suffix) + extension;

    return candidate;
}

bool autosaveIsNewer(const std::filesystem::path& projectPackage,
                     const std::filesystem::path& autosavePackage)
{
    std::error_code failed;

    const auto projectTime =
        std::filesystem::last_write_time(projectPackage / "project.json", failed);
    if (failed)
        return false;

    const auto autosaveTime =
        std::filesystem::last_write_time(autosavePackage / "project.json", failed);
    if (failed)
        return false;

    return autosaveTime > projectTime;
}

} // namespace incdaw::app::session
