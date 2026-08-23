#pragma once

#include "engine/audio/WavFile.h"
#include "engine/instrument/Instrument.h"
#include "engine/instrument/SamplerStream.h"
#include "project/Model.h"

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace incdaw::project {

// ── The hosted seam ──────────────────────────────────────────────────────────

/// Builds the instrument for a channel whose instrument is NOT a builtin.
///
/// Injected rather than hardcoded so that plugin hosting (Phase 13) becomes a
/// different factory instead of a change to the compiler (docs/DECISIONS.md
/// D-028). Returning nullptr leaves the channel silent, which is the correct
/// behaviour for a channel whose plugin is missing.
using InstrumentFactory = std::function<std::unique_ptr<engine::Instrument>(const Channel&)>;

/// The fallback factory: every channel gets a SimpleSynth — what a channel
/// that has not chosen an instrument sounds like.
[[nodiscard]] InstrumentFactory defaultInstrumentFactory();

// ── The builtin registry ─────────────────────────────────────────────────────

/// What an instrument factory may ask of the compile it runs inside: the
/// project's audio assets, decoded off the audio thread, memoised per compile
/// and shared across rebuilds through the SampleCache (docs/DECISIONS.md
/// D-032). Implemented by the graph compiler; tests implement it over
/// fixture audio. An instrument never opens a file itself.
class AssetResolver {
public:
    virtual ~AssetResolver() = default;

    /// The path of the project's audio asset `asset`, or nullptr when the
    /// project has no such asset.
    [[nodiscard]] virtual const std::string* assetPath(EntityId asset) const = 0;

    /// The whole of `asset`, decoded. Null when the asset is missing or
    /// unreadable — warned once per compile, so the caller just skips it.
    [[nodiscard]] virtual std::shared_ptr<const engine::AudioFileData> loadAsset(EntityId asset) = 0;

    /// A disk stream for `asset` when the compile's streaming policy admits
    /// it (a streamer exists, the file is long enough, its channel count is
    /// streamable); nullptr otherwise, and the caller preloads through
    /// `loadAsset` instead. Whether a *zone* may stream at all (it must be
    /// forward and unlooped) is the instrument's own decision.
    [[nodiscard]] virtual std::shared_ptr<engine::SamplerZoneStream> streamAsset(EntityId asset) = 0;

    /// Records a compile warning: something was skipped, not something that
    /// failed. A channel whose sample is missing is silent, not absent.
    virtual void warn(std::string message) = 0;
};

/// Everything a builtin instrument's factory receives.
struct InstrumentBuildContext {
    const Channel&     channel;
    engine::SampleRate sampleRate;
    AssetResolver&     assets;
};

/// One builtin instrument the compiler can construct.
///
/// Construction only: the parameter table stays in
/// `engine::BuiltinInstrumentInfo`, where the registry and the panel read it.
/// `uid` matches both that table's entry and the channel's PluginIdentifier.
struct BuiltinInstrumentEntry {
    const char* uid;
    std::function<std::unique_ptr<engine::Instrument>(const InstrumentBuildContext&)> make;
};

/// A family's entry point: appends one entry per instrument it owns. Called
/// once from `InstrumentFactory.cpp` (docs/plugin-archive/00-CONTRACTS.md §3.2).
using InstrumentRegistrar = void (*)(std::vector<BuiltinInstrumentEntry>&);

/// Every registered builtin instrument, in registration order.
[[nodiscard]] const std::vector<BuiltinInstrumentEntry>& builtinInstrumentEntries();

/// The entry for `uid`, or nullptr for an unknown one.
[[nodiscard]] const BuiltinInstrumentEntry* findBuiltinInstrumentEntry(std::string_view uid);

/// Builds the channel's builtin instrument, or returns nullptr when the
/// channel names a uid no family registered. The caller decides what an
/// unknown uid means (the compiler warns and leaves the channel silent).
[[nodiscard]] std::unique_ptr<engine::Instrument>
makeBuiltinInstrument(const InstrumentBuildContext& context);

} // namespace incdaw::project
