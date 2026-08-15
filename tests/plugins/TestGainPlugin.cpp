// The test-suite's own CLAP plugin: a gain, -6 dB by default.
//
// The plugin host cannot be tested against third-party binaries (CLAUDE.md
// §42) and must not be tested against nothing. This is a real, minimal,
// well-behaved CLAP plugin the build produces itself; the misbehaving twin
// lives in TestCrashPlugin.cpp.
//
// The gain is a CLAP parameter (id 0, plain range 0..2, default 0.5) so the
// host's discovery and event-delivery paths have something real to talk to.
// The default keeps the fixed -6 dB the earlier tests were written against.

#include <clap/clap.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {

constexpr double  defaultGain = 0.5;   // -6 dB
constexpr clap_id gainParamId = 0;

struct State {
    double gain = defaultGain;

    // Editor bookkeeping. This plugin's "editor" draws nothing: it records
    // the host's calls, which is exactly what the host's bridge tests need —
    // a real NSView would need a display and prove no more.
    bool  guiCreated = false;
    void* guiParent  = nullptr;
    bool  guiVisible = false;
};

State* stateOf(const clap_plugin_t* plugin)
{
    return static_cast<State*>(plugin->plugin_data);
}

/// Applies every CLAP_EVENT_PARAM_VALUE in `events` to the state. Shared by
/// process() and params.flush(), which is exactly the sharing the CLAP spec
/// intends.
void applyParamEvents(State* state, const clap_input_events_t* events)
{
    if (state == nullptr || events == nullptr || events->size == nullptr
        || events->get == nullptr)
        return;

    const uint32_t count = events->size(events);

    for (uint32_t index = 0; index < count; ++index) {
        const clap_event_header_t* header = events->get(events, index);

        if (header == nullptr || header->space_id != CLAP_CORE_EVENT_SPACE_ID
            || header->type != CLAP_EVENT_PARAM_VALUE)
            continue;

        const auto* event = reinterpret_cast<const clap_event_param_value_t*>(header);
        if (event->param_id == gainParamId)
            state->gain = event->value;
    }
}

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
    "A gain used by INCDAW's plugin-host tests.",
    features,
};

// ── The params extension ─────────────────────────────────────────────────────

uint32_t paramsCount(const clap_plugin_t*) { return 1; }

bool paramsGetInfo(const clap_plugin_t*, uint32_t index, clap_param_info_t* info)
{
    if (index != 0 || info == nullptr)
        return false;

    *info = {};
    info->id            = gainParamId;
    info->flags         = CLAP_PARAM_IS_AUTOMATABLE;
    info->cookie        = nullptr;
    info->min_value     = 0.0;
    info->max_value     = 2.0;
    info->default_value = defaultGain;
    std::snprintf(info->name, sizeof(info->name), "%s", "Gain");
    info->module[0] = '\0';
    return true;
}

bool paramsGetValue(const clap_plugin_t* plugin, clap_id paramId, double* out)
{
    if (paramId != gainParamId || out == nullptr)
        return false;

    *out = stateOf(plugin)->gain;
    return true;
}

bool paramsValueToText(const clap_plugin_t*, clap_id paramId, double value, char* out,
                       uint32_t capacity)
{
    if (paramId != gainParamId || out == nullptr)
        return false;

    std::snprintf(out, capacity, "%.2f", value);
    return true;
}

bool paramsTextToValue(const clap_plugin_t*, clap_id paramId, const char* text, double* out)
{
    if (paramId != gainParamId || text == nullptr || out == nullptr)
        return false;

    *out = std::strtod(text, nullptr);
    return true;
}

void paramsFlush(const clap_plugin_t* plugin, const clap_input_events_t* in,
                 const clap_output_events_t*)
{
    applyParamEvents(stateOf(plugin), in);
}

const clap_plugin_params_t paramsExtension = {
    paramsCount, paramsGetInfo, paramsGetValue, paramsValueToText, paramsTextToValue, paramsFlush,
};

// ── The state extension ──────────────────────────────────────────────────────
// The state is the gain, serialised as its 8 raw bytes. Loading demands
// exactly that shape and a sane value, so the host's "plugin rejected its
// saved state" path has something honest to trigger it.

bool stateSave(const clap_plugin_t* plugin, const clap_ostream_t* stream)
{
    if (stream == nullptr || stream->write == nullptr)
        return false;

    const double gain    = stateOf(plugin)->gain;
    const auto*  bytes   = reinterpret_cast<const uint8_t*>(&gain);
    uint64_t     written = 0;

    while (written < sizeof(gain)) {
        const int64_t count = stream->write(stream, bytes + written, sizeof(gain) - written);
        if (count <= 0)
            return false;

        written += static_cast<uint64_t>(count);
    }

    return true;
}

bool stateLoad(const clap_plugin_t* plugin, const clap_istream_t* stream)
{
    if (stream == nullptr || stream->read == nullptr)
        return false;

    double   gain = 0.0;
    auto*    bytes = reinterpret_cast<uint8_t*>(&gain);
    uint64_t total = 0;

    while (total < sizeof(gain)) {
        const int64_t count = stream->read(stream, bytes + total, sizeof(gain) - total);
        if (count < 0)
            return false;
        if (count == 0)
            return false;   // short blob: not ours

        total += static_cast<uint64_t>(count);
    }

    uint8_t       extra = 0;
    const int64_t tail  = stream->read(stream, &extra, 1);
    if (tail != 0)
        return false;   // trailing bytes: not ours either

    if (!(gain >= 0.0 && gain <= 2.0))
        return false;   // out of range or NaN

    stateOf(plugin)->gain = gain;
    return true;
}

const clap_plugin_state_t stateExtension = {stateSave, stateLoad};

// ── The gui extension ────────────────────────────────────────────────────────

constexpr uint32_t editorWidth  = 300;
constexpr uint32_t editorHeight = 200;

bool guiIsApiSupported(const clap_plugin_t*, const char* api, bool isFloating)
{
    return !isFloating && api != nullptr && std::strcmp(api, CLAP_WINDOW_API_COCOA) == 0;
}

bool guiGetPreferredApi(const clap_plugin_t*, const char** api, bool* isFloating)
{
    if (api == nullptr || isFloating == nullptr)
        return false;

    *api        = CLAP_WINDOW_API_COCOA;
    *isFloating = false;
    return true;
}

bool guiCreate(const clap_plugin_t* plugin, const char* api, bool isFloating)
{
    if (!guiIsApiSupported(plugin, api, isFloating))
        return false;

    stateOf(plugin)->guiCreated = true;
    return true;
}

void guiDestroy(const clap_plugin_t* plugin)
{
    State* state     = stateOf(plugin);
    state->guiCreated = false;
    state->guiParent  = nullptr;
    state->guiVisible = false;
}

bool guiSetScale(const clap_plugin_t*, double) { return true; }

bool guiGetSize(const clap_plugin_t* plugin, uint32_t* width, uint32_t* height)
{
    if (!stateOf(plugin)->guiCreated || width == nullptr || height == nullptr)
        return false;

    *width  = editorWidth;
    *height = editorHeight;
    return true;
}

bool guiCanResize(const clap_plugin_t*) { return false; }
bool guiGetResizeHints(const clap_plugin_t*, clap_gui_resize_hints_t*) { return false; }
bool guiAdjustSize(const clap_plugin_t*, uint32_t*, uint32_t*) { return false; }
bool guiSetSize(const clap_plugin_t*, uint32_t, uint32_t) { return false; }

bool guiSetParent(const clap_plugin_t* plugin, const clap_window_t* window)
{
    if (window == nullptr || window->cocoa == nullptr)
        return false;

    stateOf(plugin)->guiParent = window->cocoa;
    return true;
}

bool guiSetTransient(const clap_plugin_t*, const clap_window_t*) { return false; }
void guiSuggestTitle(const clap_plugin_t*, const char*) {}

bool guiShow(const clap_plugin_t* plugin)
{
    State* state = stateOf(plugin);
    if (!state->guiCreated || state->guiParent == nullptr)
        return false;

    state->guiVisible = true;
    return true;
}

bool guiHide(const clap_plugin_t* plugin)
{
    stateOf(plugin)->guiVisible = false;
    return true;
}

const clap_plugin_gui_t guiExtension = {
    guiIsApiSupported, guiGetPreferredApi, guiCreate,      guiDestroy,
    guiSetScale,       guiGetSize,         guiCanResize,   guiGetResizeHints,
    guiAdjustSize,     guiSetSize,         guiSetParent,   guiSetTransient,
    guiSuggestTitle,   guiShow,            guiHide,
};

// ── The plugin ───────────────────────────────────────────────────────────────

bool pluginInit(const clap_plugin_t*) { return true; }

void pluginDestroy(const clap_plugin_t* plugin)
{
    delete stateOf(plugin);
    delete plugin;
}

bool pluginActivate(const clap_plugin_t*, double, uint32_t, uint32_t) { return true; }
void pluginDeactivate(const clap_plugin_t*) {}
bool pluginStartProcessing(const clap_plugin_t*) { return true; }
void pluginStopProcessing(const clap_plugin_t*) {}
void pluginReset(const clap_plugin_t*) {}

const void* pluginGetExtension(const clap_plugin_t*, const char* id)
{
    if (id != nullptr && std::strcmp(id, CLAP_EXT_PARAMS) == 0)
        return &paramsExtension;

    if (id != nullptr && std::strcmp(id, CLAP_EXT_STATE) == 0)
        return &stateExtension;

    if (id != nullptr && std::strcmp(id, CLAP_EXT_GUI) == 0)
        return &guiExtension;

    return nullptr;
}

void pluginOnMainThread(const clap_plugin_t*) {}

clap_process_status pluginProcess(const clap_plugin_t* plugin, const clap_process_t* process)
{
    if (process == nullptr || process->audio_outputs_count == 0)
        return CLAP_PROCESS_ERROR;

    State* state = stateOf(plugin);
    applyParamEvents(state, process->in_events);

    const float gain = static_cast<float>(state->gain);

    const clap_audio_buffer_t& in  = process->audio_inputs[0];
    const clap_audio_buffer_t& out = process->audio_outputs[0];

    for (uint32_t channel = 0; channel < out.channel_count; ++channel) {
        const float* source = channel < in.channel_count ? in.data32[channel] : nullptr;

        for (uint32_t frame = 0; frame < process->frames_count; ++frame)
            out.data32[channel][frame] = source != nullptr ? source[frame] * gain : 0.0f;
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
    plugin->plugin_data      = new State{};
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
