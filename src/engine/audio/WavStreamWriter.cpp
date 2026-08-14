#include "engine/audio/WavStreamWriter.h"

#include "engine/audio/WavBytes.h"

namespace incdaw::engine {
namespace {

/// The data chunk's size field is 32-bit; past this many bytes the file needs
/// RF64, which INCDAW does not write yet. Refusing loudly beats wrapping the
/// counter and producing a file whose header lies about its length.
constexpr std::size_t maxDataBytes = 0xFFFFFFFFu - 36u;

} // namespace

WavStreamWriter::~WavStreamWriter()
{
    if (isOpen())
        (void)finalize();
}

WavFile::Result WavStreamWriter::open(const std::filesystem::path& path, SampleRate sampleRate,
                                      std::size_t channelCount, WavFile::Format format)
{
    WavFile::Result result;

    if (isOpen()) {
        result.error = "writer already open: " + path_.string();
        return result;
    }

    if (channelCount == 0 || sampleRate <= 0.0) {
        result.error = "invalid format: " + std::to_string(channelCount) + " channels at "
                     + std::to_string(sampleRate) + " Hz";
        return result;
    }

    file_.open(path, std::ios::binary | std::ios::trunc);
    if (!file_) {
        result.error = "cannot create: " + path.string();
        return result;
    }

    path_          = path;
    format_        = format;
    channelCount_  = channelCount;
    framesWritten_ = 0;

    // Placeholder sizes; finalize() patches them. Written as zero rather than
    // left as garbage so that a reader encountering an unfinalized file sees
    // an empty take, not a header claiming 4 GB of audio.
    encoded_.clear();
    wav::appendCanonicalHeader(encoded_, format_, channelCount_, sampleRate, 0);
    return flushEncoded();
}

WavFile::Result WavStreamWriter::append(const Sample* const* channels, std::size_t channelCount,
                                        FrameCount frameCount)
{
    WavFile::Result result;

    if (!isOpen()) {
        result.error = "writer is not open";
        return result;
    }

    if (channelCount != channelCount_) {
        result.error = "channel count changed mid-stream: expected " + std::to_string(channelCount_)
                     + ", got " + std::to_string(channelCount);
        return result;
    }

    encoded_.clear();

    for (FrameCount frame = 0; frame < frameCount; ++frame)
        for (std::size_t channel = 0; channel < channelCount_; ++channel)
            wav::encodeSample(encoded_, channels[channel][static_cast<std::size_t>(frame)], format_);

    result = flushEncoded();
    if (result)
        framesWritten_ += frameCount;

    return result;
}

WavFile::Result WavStreamWriter::appendInterleaved(const Sample* samples, FrameCount frameCount)
{
    WavFile::Result result;

    if (!isOpen()) {
        result.error = "writer is not open";
        return result;
    }

    encoded_.clear();

    const std::size_t total = static_cast<std::size_t>(frameCount) * channelCount_;
    for (std::size_t index = 0; index < total; ++index)
        wav::encodeSample(encoded_, samples[index], format_);

    result = flushEncoded();
    if (result)
        framesWritten_ += frameCount;

    return result;
}

WavFile::Result WavStreamWriter::finalize()
{
    WavFile::Result result;

    if (!isOpen()) {
        result.error = "writer is not open";
        return result;
    }

    const std::size_t frameBytes = (wav::bitsFor(format_) / 8u) * channelCount_;
    const std::size_t dataBytes  = frameBytes * static_cast<std::size_t>(framesWritten_);

    std::vector<std::uint8_t> patch;
    wav::writeU32(patch, static_cast<std::uint32_t>(36 + dataBytes));

    file_.seekp(static_cast<std::streamoff>(wav::riffSizeOffset));
    file_.write(reinterpret_cast<const char*>(patch.data()), static_cast<std::streamsize>(patch.size()));

    patch.clear();
    wav::writeU32(patch, static_cast<std::uint32_t>(dataBytes));

    file_.seekp(static_cast<std::streamoff>(wav::dataSizeOffset));
    file_.write(reinterpret_cast<const char*>(patch.data()), static_cast<std::streamsize>(patch.size()));

    file_.close();

    if (file_.fail()) {
        result.error = "finalize failed: " + path_.string();
        return result;
    }

    result.succeeded = true;
    return result;
}

WavFile::Result WavStreamWriter::flushEncoded()
{
    WavFile::Result result;

    const std::size_t frameBytes   = (wav::bitsFor(format_) / 8u) * channelCount_;
    const std::size_t writtenBytes = frameBytes * static_cast<std::size_t>(framesWritten_);

    if (writtenBytes + encoded_.size() > maxDataBytes) {
        result.error = "WAV data chunk would exceed 4 GB; RF64 is not supported";
        return result;
    }

    if (!file_.write(reinterpret_cast<const char*>(encoded_.data()),
                     static_cast<std::streamsize>(encoded_.size()))) {
        result.error = "write failed: " + path_.string();
        return result;
    }

    result.succeeded = true;
    return result;
}

} // namespace incdaw::engine
