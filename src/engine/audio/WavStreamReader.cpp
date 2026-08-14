#include "engine/audio/WavStreamReader.h"

#include "engine/audio/WavBytes.h"

#include <cstring>

namespace incdaw::engine {

WavFile::Result WavStreamReader::open(const std::filesystem::path& path)
{
    WavFile::Result result;

    close();

    file_.open(path, std::ios::binary);
    if (!file_) {
        result.error = "cannot open: " + path.string();
        return result;
    }

    path_ = path;

    // A streaming chunk walk: 12-byte RIFF header, then chunk headers with
    // seeks past the bodies we do not need. The data chunk's BODY is never
    // read here — that is the whole point of this class.
    std::uint8_t head[12];
    if (!file_.read(reinterpret_cast<char*>(head), sizeof(head))
        || std::memcmp(head, "RIFF", 4) != 0 || std::memcmp(head + 8, "WAVE", 4) != 0) {
        result.error = "not a RIFF/WAVE file: " + path.string();
        close();
        return result;
    }

    wav::FormatInfo info;
    bool          sawFmt   = false;
    std::uint64_t dataSize = 0;
    bool          sawData  = false;

    while (!sawData || !sawFmt) {
        std::uint8_t chunkHeader[8];
        if (!file_.read(reinterpret_cast<char*>(chunkHeader), sizeof(chunkHeader)))
            break;

        const std::uint32_t chunkSize = wav::readU32(chunkHeader + 4);

        if (std::memcmp(chunkHeader, "fmt ", 4) == 0) {
            std::uint8_t body[40] = {};
            const std::uint32_t want = chunkSize < sizeof(body)
                                           ? chunkSize : static_cast<std::uint32_t>(sizeof(body));

            if (!file_.read(reinterpret_cast<char*>(body), want))
                break;

            sawFmt = wav::interpretFmtChunk(body, want, info);

            // Skip whatever part of the chunk the fixed buffer did not cover,
            // plus the pad byte on an odd size.
            const std::uint32_t skip = chunkSize - want + (chunkSize & 1u);
            if (skip > 0)
                file_.seekg(skip, std::ios::cur);
        } else if (std::memcmp(chunkHeader, "data", 4) == 0) {
            dataOffset_ = static_cast<std::uint64_t>(file_.tellg());
            dataSize    = chunkSize;
            sawData     = true;

            // Continue walking (fmt may follow data in a pathological file).
            file_.seekg(static_cast<std::streamoff>(chunkSize + (chunkSize & 1u)), std::ios::cur);
        } else {
            file_.seekg(static_cast<std::streamoff>(chunkSize + (chunkSize & 1u)), std::ios::cur);
        }

        if (!file_)
            break;
    }

    if (!sawFmt || !sawData) {
        result.error = std::string{!sawFmt ? "no fmt chunk" : "no data chunk"} + ": " + path.string();
        close();
        return result;
    }

    if (!wav::isSupportedFormat(info) || info.channels == 0 || info.sampleRate == 0) {
        result.error = "unsupported format: code " + std::to_string(info.format) + ", "
                     + std::to_string(info.bitsPerSample) + " bits: " + path.string();
        close();
        return result;
    }

    format_        = info.format;
    bitsPerSample_ = info.bitsPerSample;
    channelCount_  = info.channels;
    sampleRate_    = static_cast<SampleRate>(info.sampleRate);

    const std::size_t frameBytes = (bitsPerSample_ / 8u) * channelCount_;
    frameCount_ = frameBytes > 0 ? static_cast<FrameCount>(dataSize / frameBytes) : 0;

    // The walk may have stopped on EOF with the flags set; clear them so the
    // first readAt starts from a clean stream.
    file_.clear();

    result.succeeded = true;
    return result;
}

void WavStreamReader::close()
{
    if (file_.is_open())
        file_.close();

    file_.clear();
    sampleRate_    = 0.0;
    channelCount_  = 0;
    frameCount_    = 0;
    dataOffset_    = 0;
    format_        = 0;
    bitsPerSample_ = 0;
}

bool WavStreamReader::readAt(FrameCount firstFrame, FrameCount frameCount,
                             Sample* const* channels, std::size_t channelCountOut)
{
    if (!isOpen() || frameCount <= 0)
        return isOpen();

    // The part of the request that exists in the file. Everything outside it
    // is time with no audio, which decodes to silence.
    const FrameCount from = firstFrame > 0 ? firstFrame : 0;
    const FrameCount to   = firstFrame + frameCount < frameCount_ ? firstFrame + frameCount
                                                                  : frameCount_;

    for (std::size_t channel = 0; channel < channelCountOut; ++channel)
        std::memset(channels[channel], 0, static_cast<std::size_t>(frameCount) * sizeof(Sample));

    if (from >= to)
        return true;

    const std::size_t bytesPerSample = bitsPerSample_ / 8u;
    const std::size_t frameBytes     = bytesPerSample * channelCount_;
    const std::size_t wantBytes      = static_cast<std::size_t>(to - from) * frameBytes;

    raw_.resize(wantBytes);

    file_.seekg(static_cast<std::streamoff>(dataOffset_
                                            + static_cast<std::uint64_t>(from) * frameBytes));

    if (!file_.read(reinterpret_cast<char*>(raw_.data()), static_cast<std::streamsize>(wantBytes))) {
        file_.clear();
        return false;
    }

    for (FrameCount frame = from; frame < to; ++frame) {
        const std::uint8_t* frameStart =
            raw_.data() + static_cast<std::size_t>(frame - from) * frameBytes;

        for (std::size_t channel = 0; channel < channelCountOut; ++channel) {
            // Mono fills every requested channel; extra file channels beyond
            // the request are simply not decoded.
            const std::size_t sourceChannel =
                channel < channelCount_ ? channel : channelCount_ - 1;

            channels[channel][static_cast<std::size_t>(frame - firstFrame)] =
                wav::decodeSample(frameStart + sourceChannel * bytesPerSample,
                                  format_, bitsPerSample_);
        }
    }

    return true;
}

} // namespace incdaw::engine
