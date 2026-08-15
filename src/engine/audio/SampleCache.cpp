#include "engine/audio/SampleCache.h"

namespace incdaw::engine {

namespace {

bool statFile(const std::filesystem::path& path, std::uintmax_t& size, std::int64_t& mtime)
{
    std::error_code errorCode;

    size = std::filesystem::file_size(path, errorCode);
    if (errorCode)
        return false;

    const auto time = std::filesystem::last_write_time(path, errorCode);
    if (errorCode)
        return false;

    mtime = static_cast<std::int64_t>(time.time_since_epoch().count());
    return true;
}

} // namespace

std::shared_ptr<const AudioFileData> SampleCache::load(const std::filesystem::path& path,
                                                       std::string& error)
{
    std::uintmax_t size  = 0;
    std::int64_t   mtime = 0;
    if (!statFile(path, size, mtime)) {
        error = "file not found: " + path.string();
        return nullptr;
    }

    const std::string key = path.string();

    {
        const std::lock_guard<std::mutex> lock(mutex_);
        if (const auto found = entries_.find(key);
            found != entries_.end() && found->second.size == size
            && found->second.mtime == mtime)
            return found->second.data;
    }

    // Decoding happens outside the lock: a second caller racing on the same
    // file decodes redundantly and the last one wins, which is harmless —
    // holding the lock through a multi-second decode would not be.
    auto data = std::make_shared<AudioFileData>();
    if (const auto read = WavFile::read(path, *data); !read) {
        error = read.error;
        return nullptr;
    }

    std::shared_ptr<const AudioFileData> immutable = std::move(data);

    const std::lock_guard<std::mutex> lock(mutex_);
    entries_[key] = Entry{size, mtime, immutable};
    return immutable;
}

std::size_t SampleCache::entryCount() const
{
    const std::lock_guard<std::mutex> lock(mutex_);
    return entries_.size();
}

void SampleCache::clear()
{
    const std::lock_guard<std::mutex> lock(mutex_);
    entries_.clear();
}

} // namespace incdaw::engine
