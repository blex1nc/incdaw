#include "doctest.h"

#include "engine/core/RealtimeGuard.h"
#include "engine/midi/MidiBuffer.h"
#include "app/AppSettings.h"
#include "engine/midi/MidiOutput.h"
#include "platform/HostTime.h"
#include "platform/MidiDevice.h"

#include <string>
#include <vector>

using namespace incdaw;
using namespace incdaw::engine;

namespace {

constexpr double nanosPerSecond = 1'000'000'000.0;

/// A destination that records instead of playing.
///
/// The point of the platform interface is that the engine side is testable
/// without hardware: every timing claim below is checked against what the
/// device was actually handed, on a machine with nothing plugged in.
class RecordingMidiDevice final : public platform::MidiDevice {
public:
    std::vector<platform::MidiDeviceInfo> enumerateInputs()  const override { return {}; }
    std::vector<platform::MidiDeviceInfo> enumerateOutputs() const override { return {}; }

    bool open(const std::vector<std::string>&, platform::MidiInputCallback&, std::string&) override
    {
        open_ = true;
        return true;
    }

    void close() override { open_ = false; }
    [[nodiscard]] bool isOpen() const noexcept override { return open_; }

    bool selectOutput(const std::string& identifier, std::string&) override
    {
        selected_ = identifier;
        return true;
    }

    [[nodiscard]] std::string selectedOutput() const override { return selected_; }

    void sendMessage(const platform::TimestampedMidiMessage& message) noexcept override
    {
        sent.push_back(message);
    }

    std::vector<platform::TimestampedMidiMessage> sent;

private:
    bool        open_ = false;
    std::string selected_;
};

MidiBuffer bufferOf(std::initializer_list<MidiMessage> messages)
{
    MidiBuffer buffer;
    for (const MidiMessage& message : messages)
        (void)buffer.insert(message);
    return buffer;
}

} // namespace

// ── The clock conversion ─────────────────────────────────────────────────────

TEST_CASE("host time converts to nanoseconds and back")
{
    // The conversion the output path depends on. On Apple silicon the tick and
    // the nanosecond are different units, and a send that skips this lands at
    // the wrong moment rather than failing loudly.
    for (std::uint64_t nanos : {std::uint64_t{0}, std::uint64_t{1'000'000},
                                std::uint64_t{1'000'000'000}, std::uint64_t{123'456'789'012}}) {
        const std::uint64_t ticks = platform::nanosToHostTime(nanos);
        const std::uint64_t back  = platform::hostTimeToNanos(ticks);

        // A tick is coarser than a nanosecond on some machines, so the round
        // trip is exact to within one tick rather than bit-identical.
        const std::uint64_t tolerance = platform::hostTimeToNanos(1) + 1;
        CHECK(back + tolerance >= nanos);
        CHECK(nanos + tolerance >= back);
    }
}

TEST_CASE("nanosToHostTime is monotonic")
{
    std::uint64_t previous = 0;
    for (std::uint64_t nanos = 0; nanos < 10'000'000; nanos += 997) {
        const std::uint64_t ticks = platform::nanosToHostTime(nanos);
        CHECK(ticks >= previous);
        previous = ticks;
    }
}

// ── Placement ────────────────────────────────────────────────────────────────

TEST_CASE("a frame offset becomes the host time that frame is heard")
{
    RecordingMidiDevice device;
    MidiOutput          output;
    output.setDevice(&device);

    constexpr SampleRate    rate       = 48000.0;
    constexpr std::uint64_t blockNanos = 1'000'000'000;

    output.sendForBlock(bufferOf({MidiMessage::noteOn(0, 60, 100, 0),
                                  MidiMessage::noteOn(0, 64, 100, 240),
                                  MidiMessage::noteOff(0, 60, 64, 480)}),
                        blockNanos, rate);

    CHECK(output.drainPending() == 3);
    REQUIRE(device.sent.size() == 3);

    // 240 frames at 48 kHz is exactly 5 ms; 480 is 10 ms. Sending all three at
    // the block's own time would collapse those five milliseconds to zero,
    // which is the block-quantised output this class exists to avoid.
    CHECK(device.sent[0].hostTimeNanos == blockNanos);
    CHECK(device.sent[1].hostTimeNanos == blockNanos + 5'000'000);
    CHECK(device.sent[2].hostTimeNanos == blockNanos + 10'000'000);

    CHECK(device.sent[0].status == 0x90);
    CHECK(device.sent[2].status == 0x80);
}

TEST_CASE("messages reach the device in the order the block held them")
{
    RecordingMidiDevice device;
    MidiOutput          output;
    output.setDevice(&device);

    MidiBuffer buffer;
    for (int index = 0; index < 64; ++index)
        (void)buffer.insert(MidiMessage::noteOn(0, 36 + index, 100, index * 4));

    output.sendForBlock(buffer, 0, 48000.0);
    CHECK(output.drainPending() == 64);

    REQUIRE(device.sent.size() == 64);
    for (std::size_t index = 1; index < device.sent.size(); ++index) {
        CHECK(device.sent[index].data1 > device.sent[index - 1].data1);
        CHECK(device.sent[index].hostTimeNanos >= device.sent[index - 1].hostTimeNanos);
    }
}

// ── No destination ───────────────────────────────────────────────────────────

TEST_CASE("without a device nothing is queued and nothing is dropped")
{
    MidiOutput output;
    CHECK_FALSE(output.hasDevice());

    output.sendForBlock(bufferOf({MidiMessage::noteOn(0, 60, 100, 0)}), 0, 48000.0);
    output.sendAllNotesOff();

    // Not a silent drop: the messages were never queued, so a session with no
    // external gear does not report a growing dropped count.
    CHECK(output.droppedCount() == 0);
    CHECK(output.drainPending() == 0);
    CHECK(output.sentCount() == 0);
}

TEST_CASE("clearing the device stops anything further reaching it")
{
    RecordingMidiDevice device;
    MidiOutput          output;

    output.setDevice(&device);
    output.sendForBlock(bufferOf({MidiMessage::noteOn(0, 60, 100, 0)}), 0, 48000.0);

    // Queued but not yet drained when the device goes away — the case a
    // settings change creates, and the one that would hand a dangling pointer
    // to the sender.
    output.setDevice(nullptr);
    CHECK(output.drainPending() == 0);
    CHECK(device.sent.empty());
}

// ── Panic ────────────────────────────────────────────────────────────────────

TEST_CASE("all notes off releases sustain then notes on every channel")
{
    RecordingMidiDevice device;
    MidiOutput          output;
    output.setDevice(&device);

    output.sendAllNotesOff();
    CHECK(output.drainPending() == 32);

    REQUIRE(device.sent.size() == 32);

    for (int channel = 0; channel < 16; ++channel) {
        const platform::TimestampedMidiMessage& sustain = device.sent[static_cast<std::size_t>(channel * 2)];
        const platform::TimestampedMidiMessage& notes   = device.sent[static_cast<std::size_t>(channel * 2 + 1)];

        CHECK(sustain.status == (0xB0 | channel));
        CHECK(sustain.data1 == 64);
        CHECK(sustain.data2 == 0);

        // Sustain first. Sending all-notes-off while the pedal is still down
        // leaves the notes sounding on any synthesiser that honours it.
        CHECK(notes.status == (0xB0 | channel));
        CHECK(notes.data1 == 123);

        CHECK(sustain.hostTimeNanos == 0);   // a rescue, not a performance
    }
}

// ── Realtime contract ────────────────────────────────────────────────────────

TEST_CASE("queueing a block allocates nothing on the audio thread")
{
    RecordingMidiDevice device;
    MidiOutput          output;
    output.setDevice(&device);

    MidiBuffer buffer;
    for (int index = 0; index < 128; ++index)
        (void)buffer.insert(MidiMessage::noteOn(0, 60, 100, index));

    rt::resetViolations();
    {
        const rt::ScopedRealtimeContext scope;
        output.sendForBlock(buffer, 1'000'000, 48000.0);
        (void)output.send(platform::TimestampedMidiMessage{2'000'000, 0xB0, 7, 100});
        output.sendAllNotesOff();
    }

    CHECK(rt::allocationViolations() == 0);
    CHECK(rt::deallocationViolations() == 0);

    if (!rt::guardEnabled())
        MESSAGE("realtime guard disabled in this build — allocation not verified");
}

TEST_CASE("a full queue is counted rather than silently discarded")
{
    RecordingMidiDevice device;
    MidiOutput          output;
    output.setDevice(&device);

    // One more than the queue can hold, sent without draining.
    for (std::size_t index = 0; index <= MidiOutput::queueCapacity; ++index)
        (void)output.send(platform::TimestampedMidiMessage{index, 0x90, 60, 100});

    CHECK(output.droppedCount() > 0);
    CHECK(output.drainPending() == MidiOutput::queueCapacity - 1);
}

// ── The sender thread ────────────────────────────────────────────────────────

TEST_CASE("the sender thread delivers what the audio thread queued")
{
    RecordingMidiDevice device;
    MidiOutput          output;
    output.setDevice(&device);
    output.start();
    CHECK(output.isRunning());

    output.sendForBlock(bufferOf({MidiMessage::noteOn(0, 60, 100, 0),
                                  MidiMessage::noteOff(0, 60, 64, 96)}),
                        static_cast<std::uint64_t>(nanosPerSecond), 48000.0);

    // stop() drains once more on the way out, so the thread cannot leave a
    // note-off behind with nothing left running to send it.
    output.stop();
    CHECK_FALSE(output.isRunning());

    REQUIRE(device.sent.size() == 2);
    CHECK(output.sentCount() == 2);
    CHECK(device.sent[1].hostTimeNanos == device.sent[0].hostTimeNanos + 2'000'000);
}

TEST_CASE("starting and stopping the sender twice is harmless")
{
    MidiOutput output;
    output.start();
    output.start();
    output.stop();
    output.stop();
    CHECK_FALSE(output.isRunning());
}

// ── The stored preference ────────────────────────────────────────────────────

TEST_CASE("the chosen MIDI destination survives a settings round trip")
{
    app::AppSettings settings;
    CHECK(settings.midiOutputIdentifier.empty());   // none, until asked

    settings.midiOutputIdentifier = "-1234567";
    settings.midiInputIdentifiers = {"77"};

    const app::AppSettings reloaded = app::AppSettings::fromJson(settings.toJson());
    CHECK(reloaded.midiOutputIdentifier == "-1234567");
    REQUIRE(reloaded.midiInputIdentifiers.size() == 1);
    CHECK(reloaded.midiInputIdentifiers[0] == "77");
}

TEST_CASE("a settings file written before MIDI output existed reads as none")
{
    // The field was added, not changed, so the version does not move and an
    // older file stays valid — it simply has no destination.
    const app::AppSettings reloaded =
        app::AppSettings::fromJson(R"({"version":1,"midi":{"inputs":["77"]}})");

    CHECK(reloaded.midiOutputIdentifier.empty());
    REQUIRE(reloaded.midiInputIdentifiers.size() == 1);
}
