// The test-suite's latent CLAP plugin: a true 64-frame delay that REPORTS
// its 64 frames through CLAP_EXT_LATENCY.
//
// The pair matters: delay compensation aligns paths by what plugins claim,
// and a test against a plugin that claims without delaying (or delays
// without claiming) would prove nothing. This one behaves like a real
// look-ahead processor, so the host's PDC has something honest to align.

#include <clap/clap.h>

#include <cstring>
#include <vector>

namespace {

constexpr uint32_t delayFrames = 64;

struct State {
    // One ring per channel, stereo. Zero-initialised: the first 64 frames
    // out are silence, exactly like a real look-ahead.
    std::vector<float> ring[2];
    uint32_t           position = 0;

    // What CLAP_EXT_LATENCY reports. Loading state bumps it and notifies the
    // host (clap_host_latency.changed) — the report intentionally diverges
    // from the true delay then: this half of the plugin exists to test the
    // REPORTING plumbing, and the honest-pairing PDC test never loads state.
    const clap_host_t* host     = nullptr;
    uint32_t           reported = delayFrames;
};

State* stateOf(const clap_plugin_t* plugin)
{
    return static_cast<State*>(plugin->plugin_data);
}

const char* const features[] = {CLAP_PLUGIN_FEATURE_AUDIO_EFFECT, nullptr};

const clap_plugin_descriptor_t descriptor = {
    CLAP_VERSION_INIT,
    "com.incdaw.testlatency",
    "INCDAW Test Latency",
    "INCDAW",
    "",
    "",
    "",
    "0.1.0",
    "A 64-frame delay that reports itself, for INCDAW's PDC tests.",
    features,
};

// ── The latency extension ────────────────────────────────────────────────────

uint32_t latencyGet(const clap_plugin_t* plugin) { return stateOf(plugin)->reported; }

const clap_plugin_latency_t latencyExtension = {latencyGet};

// ── The state extension: the latency-changed trigger ─────────────────────────

bool stateSave(const clap_plugin_t* plugin, const clap_ostream_t* stream)
{
    const uint32_t reported = stateOf(plugin)->reported;
    return stream->write(stream, &reported, sizeof(reported))
        == static_cast<int64_t>(sizeof(reported));
}

bool stateLoad(const clap_plugin_t* plugin, const clap_istream_t* stream)
{
    uint32_t ignored = 0;
    (void)stream->read(stream, &ignored, sizeof(ignored));

    State* state    = stateOf(plugin);
    state->reported = delayFrames * 2;

    if (state->host != nullptr && state->host->get_extension != nullptr)
        if (const auto* hostLatency = static_cast<const clap_host_latency_t*>(
                state->host->get_extension(state->host, CLAP_EXT_LATENCY)))
            if (hostLatency->changed != nullptr)
                hostLatency->changed(state->host);

    return true;
}

const clap_plugin_state_t stateExtension = {stateSave, stateLoad};

// ── The plugin ───────────────────────────────────────────────────────────────

bool pluginInit(const clap_plugin_t*) { return true; }

void pluginDestroy(const clap_plugin_t* plugin)
{
    delete stateOf(plugin);
    delete plugin;
}

bool pluginActivate(const clap_plugin_t* plugin, double, uint32_t, uint32_t)
{
    State* state = stateOf(plugin);
    state->ring[0].assign(delayFrames, 0.0f);
    state->ring[1].assign(delayFrames, 0.0f);
    state->position = 0;
    return true;
}

void pluginDeactivate(const clap_plugin_t*) {}
bool pluginStartProcessing(const clap_plugin_t*) { return true; }
void pluginStopProcessing(const clap_plugin_t*) {}
void pluginReset(const clap_plugin_t*) {}

const void* pluginGetExtension(const clap_plugin_t*, const char* id)
{
    if (id != nullptr && std::strcmp(id, CLAP_EXT_LATENCY) == 0)
        return &latencyExtension;

    if (id != nullptr && std::strcmp(id, CLAP_EXT_STATE) == 0)
        return &stateExtension;

    return nullptr;
}

void pluginOnMainThread(const clap_plugin_t*) {}

clap_process_status pluginProcess(const clap_plugin_t* plugin, const clap_process_t* process)
{
    if (process == nullptr || process->audio_outputs_count == 0)
        return CLAP_PROCESS_ERROR;

    State* state = stateOf(plugin);

    const clap_audio_buffer_t& in  = process->audio_inputs[0];
    const clap_audio_buffer_t& out = process->audio_outputs[0];

    // One shared ring position across channels, advanced after both.
    for (uint32_t frame = 0; frame < process->frames_count; ++frame) {
        for (uint32_t channel = 0; channel < out.channel_count && channel < 2; ++channel) {
            std::vector<float>& ring = state->ring[channel];

            const float incoming = channel < in.channel_count ? in.data32[channel][frame] : 0.0f;
            const float delayed  = ring[state->position];

            ring[state->position]           = incoming;
            out.data32[channel][frame]      = delayed;
        }

        state->position = (state->position + 1) % delayFrames;
    }

    return CLAP_PROCESS_CONTINUE;
}

// ── The factory ──────────────────────────────────────────────────────────────

uint32_t factoryGetPluginCount(const clap_plugin_factory_t*) { return 1; }

const clap_plugin_descriptor_t* factoryGetPluginDescriptor(const clap_plugin_factory_t*,
                                                           uint32_t index)
{
    return index == 0 ? &descriptor : nullptr;
}

const clap_plugin_t* factoryCreatePlugin(const clap_plugin_factory_t*, const clap_host_t* host,
                                         const char* pluginId)
{
    if (host == nullptr || pluginId == nullptr || std::strcmp(pluginId, descriptor.id) != 0)
        return nullptr;

    auto* state = new State{};
    state->host = host;

    auto* plugin = new clap_plugin_t{};
    plugin->desc             = &descriptor;
    plugin->plugin_data      = state;
    plugin->init             = pluginInit;
    plugin->destroy          = pluginDestroy;
    plugin->activate         = pluginActivate;
    plugin->deactivate       = pluginDeactivate;
    plugin->start_processing = pluginStartProcessing;
    plugin->stop_processing  = pluginStopProcessing;
    plugin->reset            = pluginReset;
    plugin->process          = pluginProcess;
    plugin->get_extension    = pluginGetExtension;
    plugin->on_main_thread   = pluginOnMainThread;
    return plugin;
}

const clap_plugin_factory_t factory = {
    factoryGetPluginCount,
    factoryGetPluginDescriptor,
    factoryCreatePlugin,
};

// ── The entry ────────────────────────────────────────────────────────────────

bool entryInit(const char*) { return true; }
void entryDeinit() {}

const void* entryGetFactory(const char* factoryId)
{
    return std::strcmp(factoryId, CLAP_PLUGIN_FACTORY_ID) == 0 ? &factory : nullptr;
}

} // namespace

extern "C" CLAP_EXPORT const clap_plugin_entry_t clap_entry = {
    CLAP_VERSION_INIT,
    entryInit,
    entryDeinit,
    entryGetFactory,
};
