#include "app/RecentProjects.h"

#include <algorithm>

namespace incdaw::app {

std::vector<std::string> RecentProjects::updated(std::vector<std::string> list,
                                                 const std::string&       path)
{
    list.erase(std::remove(list.begin(), list.end(), path), list.end());
    list.insert(list.begin(), path);

    if (list.size() > maximumCount)
        list.resize(maximumCount);

    return list;
}

std::vector<std::string> RecentProjects::without(std::vector<std::string> list,
                                                 const std::string&       path)
{
    list.erase(std::remove(list.begin(), list.end(), path), list.end());
    return list;
}

} // namespace incdaw::app
