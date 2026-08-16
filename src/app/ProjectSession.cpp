#include "app/ProjectSession.h"

#include <algorithm>
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
