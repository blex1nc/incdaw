// The test-suite's own CLAP plugin: a fixed -6 dB gain.
//
// The plugin host cannot be tested against third-party binaries (CLAUDE.md
// §42) and must not be tested against nothing. This is a real, minimal,
// well-behaved CLAP plugin the build produces itself; the misbehaving twin
// lives in TestCrashPlugin.cpp.

#include <clap/clap.h>

#include <cstring>

namespace {

constexpr float testGainFactor = 0.5f;

const char* const features[] = {CLAP_PLUGIN_FEATURE_AUDIO_EFFECT, nullptr};

const clap_plugin_descriptor_t descriptor = {
    CLAP_VERSION_INIT,
    "com.incdaw.testgain",
    "INCDAW Test Gain",
    "INCDAW",
    "",
    "",
    "",
    "0.1.0",
    "A fixed gain used by INCDAW's plugin-host tests.",
    features,
};

// ── The plugin ───────────────────────────────────────────────────────────────

bool pluginInit(const clap_plugin_t*) { return true; }
void pluginDestroy(const clap_plugin_t* plugin) { delete plugin; }
bool pluginActivate(const clap_plugin_t*, double, uint32_t, uint32_t) { return true; }
void pluginDeactivate(const clap_plugin_t*) {}
bool pluginStartProcessing(const clap_plugin_t*) { return true; }
void pluginStopProcessing(const clap_plugin_t*) {}
void pluginReset(const clap_plugin_t*) {}
const void* pluginGetExtension(const clap_plugin_t*, const char*) { return nullptr; }
void pluginOnMainThread(const clap_plugin_t*) {}

clap_process_status pluginProcess(const clap_plugin_t*, const clap_process_t* process)
{
    if (process == nullptr || process->audio_outputs_count == 0)
        return CLAP_PROCESS_ERROR;

    const clap_audio_buffer_t& in  = process->audio_inputs[0];
    const clap_audio_buffer_t& out = process->audio_outputs[0];

    for (uint32_t channel = 0; channel < out.channel_count; ++channel) {
        const float* source = channel < in.channel_count ? in.data32[channel] : nullptr;

        for (uint32_t frame = 0; frame < process->frames_count; ++frame)
            out.data32[channel][frame] =
                source != nullptr ? source[frame] * testGainFactor : 0.0f;
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

    auto* plugin = new clap_plugin_t{};
    plugin->desc             = &descriptor;
    plugin->plugin_data      = nullptr;
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
