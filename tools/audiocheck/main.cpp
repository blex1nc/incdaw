// incdaw-audiocheck — Phase 2 verification harness.
//
// Opens a real audio device, renders a real graph through it, and reports what
// actually happened: callback load distribution, underruns, realtime-safety
// violations, and reported latency.
//
// This is how the Phase 2 exit criterion is measured (docs/ROADMAP.md). It is a
// diagnostic tool, not part of the application.
//
//   incdaw-audiocheck [--seconds N] [--buffer N] [--rate N] [--freq HZ]
//                     [--amplitude A] [--silent] [--list] [--device UID]

#include "engine/AudioEngine.h"
#include "engine/core/RealtimeGuard.h"
#include "engine/dsp/GainNode.h"
#include "engine/dsp/SineOscillatorNode.h"
#include "engine/graph/RenderGraph.h"

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <string>
#include <thread>

using namespace incdaw;

namespace {

struct Options {
    double      seconds   = 5.0;
    std::int64_t buffer   = 128;
    double      rate      = 48000.0;
    double      frequency = 440.0;
    float       amplitude = 0.05f;   // ~ -26 dBFS: audible, not alarming
    bool        listOnly  = false;
    std::string device;      ///< output device uid; empty selects the default
};

Options parseArguments(int argc, char** argv)
{
    Options options;

    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];
        const auto next = [&]() -> const char* { return index + 1 < argc ? argv[++index] : "0"; };

        if      (argument == "--seconds")   options.seconds   = std::atof(next());
        else if (argument == "--buffer")    options.buffer    = std::atoll(next());
        else if (argument == "--rate")      options.rate      = std::atof(next());
        else if (argument == "--freq")      options.frequency = std::atof(next());
        else if (argument == "--amplitude") options.amplitude = static_cast<float>(std::atof(next()));
        else if (argument == "--silent")    options.amplitude = 0.0f;
        else if (argument == "--list")      options.listOnly  = true;
        else if (argument == "--device")    options.device    = next();
    }

    return options;
}

void printDevices(const engine::AudioEngine& audioEngine)
{
    std::cout << "Audio devices:\n";

    for (const auto& device : audioEngine.availableDevices()) {
        std::cout << "  " << device.name
                  << "  (in " << device.inputChannels << " / out " << device.outputChannels << ")";
        if (device.isDefaultOutput) std::cout << "  [default output]";
        if (device.isDefaultInput)  std::cout << "  [default input]";
        std::cout << "\n      uid: " << device.identifier << "\n";
    }
}

void printHistogram(const engine::CallbackProfiler& profiler)
{
    const std::uint64_t total = profiler.callbackCount();
    if (total == 0)
        return;

    std::cout << "\nCallback load distribution (share of the block budget):\n";

    for (std::size_t bucket = 0; bucket < engine::CallbackProfiler::bucketCount; ++bucket) {
        const std::uint64_t count = profiler.bucket(bucket);
        if (count == 0)
            continue;

        const double share = static_cast<double>(count) / static_cast<double>(total);
        const auto   bars  = static_cast<int>(share * 60.0);

        std::cout << "  " << std::setw(4)
                  << static_cast<int>(static_cast<double>(bucket) * engine::CallbackProfiler::bucketWidth * 100.0)
                  << "% |" << std::string(static_cast<std::size_t>(bars > 0 ? bars : 1), '#')
                  << "  " << count
                  << (bucket >= engine::CallbackProfiler::overrunBucket ? "   <-- OVERRUN" : "")
                  << "\n";
    }
}

} // namespace

int main(int argc, char** argv)
{
    const Options options = parseArguments(argc, argv);

    engine::AudioEngine audioEngine;

    if (options.listOnly) {
        printDevices(audioEngine);
        return 0;
    }

    // ── Build the graph before starting the device ───────────────────────────
    engine::GraphBuilder builder;
    const auto oscillator = builder.addNode(
        std::make_unique<engine::dsp::SineOscillatorNode>(options.frequency, options.amplitude));
    const auto master = builder.addNode(std::make_unique<engine::dsp::GainNode>(1.0f));

    builder.connect(oscillator, master);
    builder.setMaster(master);

    platform::AudioDeviceConfig config;
    config.outputDeviceIdentifier = options.device;
    config.sampleRate     = options.rate;
    config.bufferSize     = options.buffer;
    config.outputChannels = 2;

    std::string error;
    if (!audioEngine.start(config, error)) {
        std::cerr << "error: could not start audio: " << error << "\n";
        return 1;
    }

    // Compile against the format the device actually granted, not the one asked
    // for: a device is free to ignore either request.
    auto graph = builder.compile(audioEngine.sampleRate(), audioEngine.bufferSize(),
                                 audioEngine.outputChannels());
    if (graph == nullptr) {
        std::cerr << "error: graph compilation failed: " << builder.lastError() << "\n";
        return 1;
    }

    const engine::FrameCount graphLatency = graph->latencyFrames();
    audioEngine.setGraph(std::move(graph));

    const double rate = audioEngine.sampleRate();

    std::cout << "Device      : " << audioEngine.deviceName() << "\n"
              << "Sample rate : " << rate << " Hz\n"
              << "Block size  : " << audioEngine.bufferSize() << " frames ("
              << std::fixed << std::setprecision(2)
              << (rate > 0.0 ? static_cast<double>(audioEngine.bufferSize()) / rate * 1000.0 : 0.0)
              << " ms)\n"
              << "Channels    : " << audioEngine.outputChannels() << "\n"
              << "Out latency : " << audioEngine.totalOutputLatencyFrames() << " frames ("
              << (rate > 0.0 ? static_cast<double>(audioEngine.totalOutputLatencyFrames()) / rate * 1000.0 : 0.0)
              << " ms)\n"
              << "Graph delay : " << graphLatency << " frames\n"
              << "Signal      : " << options.frequency << " Hz sine at amplitude "
              << std::setprecision(3) << options.amplitude << "\n"
              << "Guard       : " << (engine::rt::guardEnabled() ? "armed" : "NOT COMPILED IN") << "\n"
              << "\nRunning for " << std::setprecision(1) << options.seconds << " s ...\n"
              << std::flush;

    const auto started  = std::chrono::steady_clock::now();
    const auto deadline = started + std::chrono::duration<double>(options.seconds);

    while (std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(250));

        // Retiring graphs is non-realtime work, driven from here exactly as the
        // application will drive it from its own idle loop.
        audioEngine.collectRetiredGraphs();
    }

    const auto elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
    audioEngine.stop();

    const auto& profiler = audioEngine.profiler();

    printHistogram(profiler);

    const std::uint64_t blocks    = audioEngine.blockCount();
    const std::uint64_t overruns  = profiler.overrunCount();
    const std::size_t   allocated = engine::rt::allocationViolations();
    const std::size_t   freed     = engine::rt::deallocationViolations();
    const std::uint64_t nonFinite = audioEngine.nonFiniteBlockCount();

    const double expectedBlocks = rate > 0.0 && audioEngine.bufferSize() > 0
        ? elapsed * rate / static_cast<double>(audioEngine.bufferSize())
        : 0.0;

    std::cout << "\nResults\n"
              << "  elapsed          : " << std::setprecision(2) << elapsed << " s\n"
              << "  blocks rendered  : " << blocks
              << "  (expected ~" << static_cast<std::uint64_t>(expectedBlocks) << ")\n"
              << "  peak load        : " << std::setprecision(1) << profiler.peakLoad() * 100.0 << " %\n"
              << "  p50 / p95 / p99  : " << profiler.loadPercentile(0.50) * 100.0 << " % / "
                                         << profiler.loadPercentile(0.95) * 100.0 << " % / "
                                         << profiler.loadPercentile(0.99) * 100.0 << " %\n"
              << "  overruns         : " << overruns << "\n"
              << "  non-finite blocks: " << nonFinite << "\n"
              << "  rt allocations   : " << allocated << "\n"
              << "  rt frees         : " << freed << "\n";

    const bool clean = blocks > 0 && overruns == 0 && allocated == 0 && freed == 0 && nonFinite == 0;

    std::cout << "\n" << (clean ? "PASS" : "FAIL") << "\n";
    return clean ? 0 : 1;
}
