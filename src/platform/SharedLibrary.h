#pragma once

#include <filesystem>
#include <string>

namespace incdaw::platform {

/// A dynamically loaded library. The plugin host's only door into foreign
/// code, kept in platform/ because dlopen is an OS API and the Windows port
/// will swap the implementation, not the callers.
class SharedLibrary {
public:
    SharedLibrary() = default;
    ~SharedLibrary();

    SharedLibrary(const SharedLibrary&)            = delete;
    SharedLibrary& operator=(const SharedLibrary&) = delete;

    [[nodiscard]] bool open(const std::filesystem::path& path, std::string& error);
    void close();

    [[nodiscard]] bool isOpen() const noexcept { return handle_ != nullptr; }

    /// The raw symbol, or nullptr. The caller owns the cast.
    [[nodiscard]] void* symbol(const char* name) const noexcept;

private:
    void* handle_ = nullptr;
};

} // namespace incdaw::platform
