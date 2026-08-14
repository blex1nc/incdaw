#include "plugins/clap/ClapLibrary.h"

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

// Empty event queues: process() must hand the plugin valid lists even when
// there are no events, or a well-behaved plugin dereferences null.

uint32_t emptyInSize(const clap_input_events_t*) { return 0; }
const clap_event_header_t* emptyInGet(const clap_input_events_t*, uint32_t) { return nullptr; }
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

bool ClapInstance::process(float* left, float* right, std::uint32_t frames) noexcept
{
    if (plugin_ == nullptr || !processing_)
        return false;

    float* channels[2] = {left, right};

    clap_audio_buffer_t buffer{};
    buffer.data32        = channels;
    buffer.channel_count = 2;

    clap_input_events_t inEvents{};
    inEvents.ctx  = nullptr;
    inEvents.size = emptyInSize;
    inEvents.get  = emptyInGet;

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
