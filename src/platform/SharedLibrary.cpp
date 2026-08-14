#include "platform/SharedLibrary.h"

#include <dlfcn.h>

namespace incdaw::platform {

SharedLibrary::~SharedLibrary()
{
    close();
}

bool SharedLibrary::open(const std::filesystem::path& path, std::string& error)
{
    close();

    // RTLD_LOCAL: a plugin's symbols must not leak into the process and
    // collide with another plugin's — the classic two-plugins-share-a-static
    // crash. LAZY because a library with an unresolved symbol it never calls
    // should still scan.
    handle_ = ::dlopen(path.c_str(), RTLD_LAZY | RTLD_LOCAL);

    if (handle_ == nullptr) {
        const char* reason = ::dlerror();
        error = reason != nullptr ? reason : ("dlopen failed: " + path.string());
        return false;
    }

    return true;
}

void SharedLibrary::close()
{
    if (handle_ != nullptr) {
        ::dlclose(handle_);
        handle_ = nullptr;
    }
}

void* SharedLibrary::symbol(const char* name) const noexcept
{
    return handle_ != nullptr ? ::dlsym(handle_, name) : nullptr;
}

} // namespace incdaw::platform
