#include "project/Autosave.h"

#include <algorithm>
#include <cstdio>
#include <ctime>

namespace incdaw::project {
namespace {

std::string autosavePrefixFor(const std::string& projectStem)
{
    return projectStem + ".autosave-";
}

} // namespace

std::string Autosave::stampFor(std::chrono::system_clock::time_point when)
{
    const std::time_t seconds = std::chrono::system_clock::to_time_t(when);

    std::tm local{};
    localtime_r(&seconds, &local);

    char text[32];
    std::snprintf(text, sizeof text, "%04d%02d%02d-%02d%02d%02d", local.tm_year + 1900,
                  local.tm_mon + 1, local.tm_mday, local.tm_hour, local.tm_min, local.tm_sec);
    return text;
}

std::filesystem::path Autosave::pathFor(const std::filesystem::path& directory,
                                        const std::string&           projectStem,
                                        std::chrono::system_clock::time_point when)
{
    return directory / (autosavePrefixFor(projectStem) + stampFor(when) + ".incdaw");
}

std::vector<std::filesystem::path> Autosave::list(const std::filesystem::path& directory,
                                                  const std::string&           projectStem)
{
    std::vector<std::filesystem::path> found;

    std::error_code listError;
    for (const auto& entry : std::filesystem::directory_iterator{directory, listError}) {
        const std::string name = entry.path().filename().string();
        if (name.starts_with(autosavePrefixFor(projectStem)) && entry.path().extension() == ".incdaw")
            found.push_back(entry.path());
    }

    // The stamp is zero-padded local time: name order is age order.
    std::sort(found.begin(), found.end());
    return found;
}

std::size_t Autosave::prune(const std::filesystem::path& directory,
                            const std::string& projectStem, std::size_t keepCount)
{
    const std::vector<std::filesystem::path> found = list(directory, projectStem);
    if (found.size() <= keepCount)
        return 0;

    std::size_t removed = 0;
    for (std::size_t index = 0; index < found.size() - keepCount; ++index) {
        // A package is a directory; remove_all is the only correct removal.
        std::error_code removeError;
        if (std::filesystem::remove_all(found[index], removeError) > 0 && !removeError)
            ++removed;
    }

    return removed;
}

} // namespace incdaw::project
