#include "plugins/clap/ClapLibrary.h"

#include <array>
#include <cmath>
#include <cstring>

namespace incdaw::plugins {
namespace {

// ── The host the plugin sees ─────────────────────────────────────────────────
// Deliberately minimal: every callback is a safe no-op. Extensions arrive
// with the parameter and editor bridges; a plugin asking for one it needs to
// merely EXIST gets a truthful "not provided" rather than a lying stub.

const void* hostGetExtension(const clap_host_t*, const char*) { return nullptr; }
void        hostRequestRestart(const clap_host_t*) {}
void        hostRequestProcess(const clap_host_t*) {}
void        hostRequestCallback(const clap_host_t*) {}

// Event lists: process() must hand the plugin valid lists even when there are
// no events, or a well-behaved plugin dereferences null. The input list is
// backed by the events drained from the instance's queue onto the caller's
// stack; the output list stays a truthful "no room" until plugin-originated
// changes are hosted (docs/PLUGIN_HOST.md §5).

/// One block's parameter events. Lives on process()'s stack: bounded, and
/// nothing the audio thread allocates. Leftover queue entries beyond the
/// bound stay queued and ride the next block.
struct PendingParamEvents {
    std::array<clap_event_param_value_t, 64> events{};
    uint32_t                                 count = 0;
};

uint32_t pendingInSize(const clap_input_events_t* list)
{
    return static_cast<const PendingParamEvents*>(list->ctx)->count;
}

const clap_event_header_t* pendingInGet(const clap_input_events_t* list, uint32_t index)
{
    const auto* pending = static_cast<const PendingParamEvents*>(list->ctx);
    return index < pending->count ? &pending->events[index].header : nullptr;
}

bool emptyOutTryPush(const clap_output_events_t*, const clap_event_header_t*) { return false; }

/// A macOS .clap is a bundle directory; the loadable binary lives inside.
/// A flat dylib named .clap (our test plugins) loads as-is.
std::filesystem::path resolveBinary(const std::filesystem::path& path)
{
    if (std::filesystem::is_directory(path)) {
        const std::filesystem::path inner = path / "Contents" / "MacOS" / path.stem();
        if (std::filesystem::exists(inner))
            return inner;
    }

    return path;
}

} // namespace

// ── ClapLibrary ──────────────────────────────────────────────────────────────

ClapLibrary::~ClapLibrary()
{
    close();
}

bool ClapLibrary::open(const std::filesystem::path& path, std::string& error)
{
    close();

    if (!library_.open(resolveBinary(path), error))
        return false;

    entry_ = static_cast<const clap_plugin_entry_t*>(library_.symbol("clap_entry"));
    if (entry_ == nullptr) {
        error = "no clap_entry symbol: " + path.string();
        close();
        return false;
    }

    if (!clap_version_is_compatible(entry_->clap_version)) {
        error = "incompatible CLAP version: " + path.string();
        entry_ = nullptr;
        close();
        return false;
    }

    if (entry_->init == nullptr || !entry_->init(path.string().c_str())) {
        error = "clap entry init failed: " + path.string();
        entry_ = nullptr;
        close();
        return false;
    }

    factory_ = static_cast<const clap_plugin_factory_t*>(
        entry_->get_factory(CLAP_PLUGIN_FACTORY_ID));

    if (factory_ == nullptr) {
        error = "no plugin factory: " + path.string();
        if (entry_->deinit != nullptr)
            entry_->deinit();
        entry_ = nullptr;
        close();
        return false;
    }

    return true;
}

void ClapLibrary::close()
{
    factory_ = nullptr;

    if (entry_ != nullptr) {
        if (entry_->deinit != nullptr)
            entry_->deinit();
        entry_ = nullptr;
    }

    library_.close();
}

std::vector<ClapDescriptor> ClapLibrary::descriptors() const
{
    std::vector<ClapDescriptor> results;

    if (factory_ == nullptr)
        return results;

    const uint32_t count = factory_->get_plugin_count(factory_);

    for (uint32_t index = 0; index < count; ++index) {
        const clap_plugin_descriptor_t* descriptor =
            factory_->get_plugin_descriptor(factory_, index);

        if (descriptor == nullptr || descriptor->id == nullptr)
            continue;   // hostile input: a null descriptor is skipped, not trusted

        ClapDescriptor entry;
        entry.id      = descriptor->id;
        entry.name    = descriptor->name != nullptr ? descriptor->name : "";
        entry.vendor  = descriptor->vendor != nullptr ? descriptor->vendor : "";
        entry.version = descriptor->version != nullptr ? descriptor->version : "";
        results.push_back(std::move(entry));
    }

    return results;
}

std::unique_ptr<ClapInstance> ClapLibrary::create(const std::string& pluginId,
                                                  double sampleRate, std::uint32_t maxFrames,
                                                  std::string& error)
{
    if (factory_ == nullptr) {
        error = "library is not open";
        return nullptr;
    }

    auto instance = std::unique_ptr<ClapInstance>(new ClapInstance());

    instance->host_.clap_version     = CLAP_VERSION;
    instance->host_.host_data        = instance.get();
    instance->host_.name             = "INCDAW";
    instance->host_.vendor           = "INCDAW";
    instance->host_.url              = "";
    instance->host_.version          = "0.1.0";
    instance->host_.get_extension    = hostGetExtension;
    instance->host_.request_restart  = hostRequestRestart;
    instance->host_.request_process  = hostRequestProcess;
    instance->host_.request_callback = hostRequestCallback;

    instance->plugin_ = factory_->create_plugin(factory_, &instance->host_, pluginId.c_str());
    if (instance->plugin_ == nullptr) {
        error = "create_plugin failed: " + pluginId;
        return nullptr;
    }

    if (!instance->plugin_->init(instance->plugin_)) {
        error = "plugin init failed: " + pluginId;
        instance->plugin_->destroy(instance->plugin_);
        instance->plugin_ = nullptr;
        return nullptr;
    }

    // Parameter discovery happens here, once, on this thread: get_info is
    // main-thread-only, and everything downstream — the registry, automation —
    // works from this snapshot (docs/PLUGIN_HOST.md §5).
    const auto* params = static_cast<const clap_plugin_params_t*>(
        instance->plugin_->get_extension(instance->plugin_, CLAP_EXT_PARAMS));

    if (params != nullptr && params->count != nullptr && params->get_info != nullptr) {
        const uint32_t parameterCount = params->count(instance->plugin_);

        for (uint32_t index = 0; index < parameterCount; ++index) {
            clap_param_info_t info{};
            if (!params->get_info(instance->plugin_, index, &info))
                continue;   // hostile input: a parameter that will not describe itself

            // Hostile input, continued: a range that is not a range would turn
            // the registry's normalised mapping into NaN or nonsense.
            if (!std::isfinite(info.min_value) || !std::isfinite(info.max_value)
                || info.min_value > info.max_value)
                continue;

            if ((info.flags & CLAP_PARAM_IS_AUTOMATABLE) == 0)
                continue;

            info.name[sizeof(info.name) - 1] = '\0';

            PluginParameterInfo parameter;
            parameter.id           = info.id;
            parameter.name         = info.name;
            parameter.minValue     = info.min_value;
            parameter.maxValue     = info.max_value;
            parameter.defaultValue = info.default_value;
            parameter.stepped      = (info.flags & CLAP_PARAM_IS_STEPPED) != 0;
            instance->parameters_.push_back(std::move(parameter));
        }
    }

    if (!instance->plugin_->activate(instance->plugin_, sampleRate, 1, maxFrames)) {
        error = "plugin activate failed: " + pluginId;
        instance->plugin_->destroy(instance->plugin_);
        instance->plugin_ = nullptr;
        return nullptr;
    }

    if (!instance->plugin_->start_processing(instance->plugin_)) {
        error = "start_processing failed: " + pluginId;
        instance->plugin_->deactivate(instance->plugin_);
        instance->plugin_->destroy(instance->plugin_);
        instance->plugin_ = nullptr;
        return nullptr;
    }

    instance->processing_ = true;
    return instance;
}

// ── ClapInstance ─────────────────────────────────────────────────────────────

ClapInstance::~ClapInstance()
{
    if (plugin_ != nullptr) {
        if (processing_)
            plugin_->stop_processing(plugin_);

        plugin_->deactivate(plugin_);
        plugin_->destroy(plugin_);
        plugin_ = nullptr;
    }
}

void ClapInstance::setParameter(std::uint32_t parameterId, double plainValue) noexcept
{
    // A failed push is a full queue; dropping is safe because automation
    // writes every block, so the value arrives one block late at worst.
    (void)paramEvents_.push({parameterId, plainValue});
}

bool ClapInstance::process(float* left, float* right, std::uint32_t frames) noexcept
{
    if (plugin_ == nullptr || !processing_)
        return false;

    float* channels[2] = {left, right};

    clap_audio_buffer_t buffer{};
    buffer.data32        = channels;
    buffer.channel_count = 2;

    // Values queued since the last block become this block's input events.
    // time = 0 for all of them: the AutomationNode evaluates once per block,
    // so block start IS the automation grain today.
    PendingParamEvents pending;

    ParamEvent queued;
    while (pending.count < pending.events.size() && paramEvents_.pop(queued)) {
        clap_event_param_value_t& event = pending.events[pending.count++];

        event.header.size     = sizeof(clap_event_param_value_t);
        event.header.time     = 0;
        event.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
        event.header.type     = CLAP_EVENT_PARAM_VALUE;
        event.header.flags    = 0;
        event.param_id        = queued.id;
        event.cookie          = nullptr;   // plugins must handle a null cookie
        event.note_id         = -1;
        event.port_index      = -1;
        event.channel         = -1;
        event.key             = -1;
        event.value           = queued.value;
    }

    clap_input_events_t inEvents{};
    inEvents.ctx  = &pending;
    inEvents.size = pendingInSize;
    inEvents.get  = pendingInGet;

    clap_output_events_t outEvents{};
    outEvents.ctx      = nullptr;
    outEvents.try_push = emptyOutTryPush;

    clap_process_t processData{};
    processData.steady_time         = steadyTime_;
    processData.frames_count        = frames;
    processData.transport           = nullptr;
    processData.audio_inputs        = &buffer;
    processData.audio_inputs_count  = 1;
    processData.audio_outputs       = &buffer;
    processData.audio_outputs_count = 1;
    processData.in_events           = &inEvents;
    processData.out_events          = &outEvents;

    const clap_process_status status = plugin_->process(plugin_, &processData);
    steadyTime_ += frames;

    return status != CLAP_PROCESS_ERROR;
}

} // namespace incdaw::plugins
