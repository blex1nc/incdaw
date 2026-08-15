#pragma once

#include "engine/audio/WavFile.h"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace incdaw::engine {

/// Decoded audio shared across graph rebuilds (docs/DECISIONS.md D-032).
///
/// A graph rebuild is INCDAW's answer to every structural edit, and a sampler
/// zone can name a file that is minutes long: decoding it again on every
/// rebuild would put a disk read and a large allocation between the user and
/// each edit. The cache hands out the same immutable decode until the file
/// itself changes — recognised the way the plugin registry recognises a
/// changed binary, by size and modification time.
///
/// Never touched by the audio thread. Callers are graph compiles and asset
/// imports, which allocate and block by design.
class SampleCache {
public:
    /// The decode for `path`, reused while the file's (size, mtime) are
    /// unchanged. Failure returns nullptr with `error` set and is NOT
    /// cached: the next call tries again, which is what a relinked or
    /// restored file wants.
    [[nodiscard]] std::shared_ptr<const AudioFileData>
    load(const std::filesystem::path& path, std::string& error);

    [[nodiscard]] std::size_t entryCount() const;

    void clear();

private:
    struct Entry {
        std::uintmax_t size  = 0;
        std::int64_t   mtime = 0;
        std::shared_ptr<const AudioFileData> data;
    };

    mutable std::mutex                     mutex_;
    std::unordered_map<std::string, Entry> entries_;
};

} // namespace incdaw::engine
