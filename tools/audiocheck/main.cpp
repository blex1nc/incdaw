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
//                     [--amplitude A] [--silent] [--list] [--device UID] [--midi]
//                     [--play] [--record] [--input UID] [--take DIR]
//
// --record opens the input device ("default" unless --input names one) and
// records through the same RecordingSession the application uses — capture,
// the timeline anchor, and latency-compensated placement, on real hardware.
// Combine with --play to exercise placement against a rolling transport. The
// sample-accuracy exit criterion itself is asserted deterministically in
// tests/unit/AudioRecorderTests.cpp.

#include "engine/AudioEngine.h"
#include "engine/audio/AudioRecorder.h"
#include "engine/core/RealtimeGuard.h"
#include "project/RecordingSession.h"
#include "engine/dsp/GainNode.h"
#include "engine/dsp/SineOscillatorNode.h"
#include "engine/graph/RenderGraph.h"
#include "engine/instrument/InstrumentNode.h"
#include "engine/instrument/SimpleSynth.h"
#include "platform/HostTime.h"
#include "platform/MidiDevice.h"

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
    bool        midi = false;///< also open MIDI input and report what arrives
    bool        play = false;///< play a phrase through the instrument instead of a tone
    bool        record = false;   ///< capture input to a WAV while running
    std::string input = "default";///< input device uid for --record
    std::string take  = "/tmp";   ///< directory takes are written into
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
        else if (argument == "--midi")      options.midi      = true;
        else if (argument == "--play")      options.play      = true;
        else if (argument == "--record")    options.record    = true;
        else if (argument == "--input")     options.input     = next();
        else if (argument == "--take")      options.take      = next();
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

        if (const auto midi = platform::MidiDevice::create()) {
            std::cout << "\nMIDI inputs:\n";
            for (const auto& port : midi->enumerateInputs())
                std::cout << "  " << port.name << "\n      uid: " << port.identifier << "\n";

            std::cout << "MIDI outputs:\n";
            for (const auto& port : midi->enumerateOutputs())
                std::cout << "  " << port.name << "\n      uid: " << port.identifier << "\n";
        }

        return 0;
    }

    platform::AudioDeviceConfig config;
    config.outputDeviceIdentifier = options.device;
    config.sampleRate     = options.rate;
    config.bufferSize     = options.buffer;
    config.outputChannels = 2;

    if (options.record)
        config.inputDeviceIdentifier = options.input;

    std::string error;
    if (!audioEngine.start(config, error)) {
        std::cerr << "error: could not start audio: " << error << "\n";
        return 1;
    }

    // Built after the device is open, so it is compiled against the format the
    // device actually granted rather than the one we asked for.
    engine::GraphBuilder builder;
    engine::NodeIndex    source = engine::invalidNode;

    if (options.play) {
        // The full audible chain: sequenced notes -> instrument -> master.
        audioEngine.transport().tempoMapForEdit().setSampleRate(audioEngine.sampleRate());

        auto instrument = std::make_unique<engine::InstrumentNode>(
            std::make_unique<engine::SimpleSynth>(), audioEngine.transport().tempoMap());

        // A C major arpeggio in eighths, two bars, looped.
        const int  degrees[] = {0, 4, 7, 12, 16, 12, 7, 4};
        const auto step      = engine::ticksPerQuarterNote / 2;

        std::vector<engine::SequencedNote> notes;
        for (int index = 0; index < 16; ++index)
            notes.push_back({static_cast<engine::Tick>(index) * step, step - 40, 0,
                             60 + degrees[index % 8], 100});

        instrument->sequence().setNotes(std::move(notes));
        source = builder.addNode(std::move(instrument));

        const auto loopEnd = audioEngine.transport().tempoMap().frameForTick(
            engine::ticksPerQuarterNote * 8);

        audioEngine.transport().setLoopRange(0, loopEnd);
        audioEngine.transport().setLoopEnabled(true);
        audioEngine.transport().play();
    } else {
        source = builder.addNode(
            std::make_unique<engine::dsp::SineOscillatorNode>(options.frequency, options.amplitude));
    }

    const auto master = builder.addNode(std::make_unique<engine::dsp::GainNode>(1.0f));
    builder.connect(source, master);
    builder.setMaster(master);

    auto graph = builder.compile(audioEngine.sampleRate(), audioEngine.bufferSize(),
                                 audioEngine.outputChannels());
    if (graph == nullptr) {
        std::cerr << "error: graph compilation failed: " << builder.lastError() << "\n";
        return 1;
    }

    const engine::FrameCount graphLatency = graph->latencyFrames();
    audioEngine.setGraph(std::move(graph));

    // MIDI input is opened after the device, so that the first block's host
    // time is already established when messages start arriving.
    std::unique_ptr<platform::MidiDevice> midiDevice;

    if (options.midi) {
        midiDevice = platform::MidiDevice::create();
        std::string midiError;

        if (midiDevice != nullptr && !midiDevice->open({}, audioEngine.midiInput(), midiError)) {
            std::cerr << "warning: MIDI input unavailable: " << midiError << "\n";
            midiDevice.reset();
        }
    }

    const double rate = audioEngine.sampleRate();

    // Recording goes through the same RecordingSession the application uses,
    // so this exercises the whole chain on real hardware: capture, the
    // timeline anchor, and latency-compensated placement.
    project::RecordingSession recording;

    if (options.record) {
        std::string recordError;
        if (!recording.arm(audioEngine, options.take, recordError)) {
            std::cerr << "error: could not start recording: " << recordError << "\n";
            audioEngine.stop();
            return 1;
        }
    }

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
              << (options.record
                      ? "In latency  : " + std::to_string(audioEngine.totalInputLatencyFrames())
                        + " frames, " + std::to_string(audioEngine.inputChannels())
                        + " channel(s) -> " + options.take + "\n"
                      : "")
              << "Graph delay : " << graphLatency << " frames\n"
              << "Signal      : " << (options.play
                     ? std::string("sequenced arpeggio through the reference synth")
                     : std::to_string(static_cast<int>(options.frequency)) + " Hz sine")
              << "\n"
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

    if (midiDevice != nullptr)
        midiDevice->close();

    project::RecordingSession::Placement take;

    if (options.record)
        take = recording.finish(audioEngine);

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

    if (options.midi) {
        const auto& midiInput = audioEngine.midiInput();
        std::cout << "  midi received    : " << midiInput.receivedCount() << "\n"
                  << "  midi dropped     : " << midiInput.droppedCount() << "\n"
                  << "  midi late        : " << midiInput.lateCount() << "\n";
    }

    if (options.record) {
        const double expectedFrames = elapsed * rate;

        std::cout << "  recorded frames  : " << take.frameCount
                  << "  (expected ~" << static_cast<std::uint64_t>(expectedFrames) << ")\n"
                  << "  dropped frames   : " << take.droppedFrames << "\n"
                  << "  placed at frame  : " << take.startFrame
                  << (take.placedAgainstPlayback ? "  (against rolling transport)"
                                                 : "  (at the stopped playhead)")
                  << "\n"
                  << "  take             : " << (take.succeeded ? take.path.string() : take.error) << "\n";
    }

    const bool clean = blocks > 0 && overruns == 0 && allocated == 0 && freed == 0 && nonFinite == 0
                    && (!options.midi || audioEngine.midiInput().droppedCount() == 0)
                    && (!options.record || (take.succeeded && take.droppedFrames == 0
                                            && take.frameCount > 0));

    std::cout << "\n" << (clean ? "PASS" : "FAIL") << "\n";
    return clean ? 0 : 1;
}
