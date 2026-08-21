#pragma once

#include "platform/AudioDevice.h"

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace incdaw::app {

/// What the shell asks a device for before anyone has expressed a preference.
///
/// 512 frames rather than the smallest the hardware allows: the block size is
/// set on the SHARED device, and Bluetooth outputs crackle below it. Someone
/// who wants 128 frames on an interface says so in Audio Settings, and that
/// preference is what this file exists to remember.
inline platform::AudioDeviceConfig defaultAudioConfig()
{
    platform::AudioDeviceConfig config;
    config.sampleRate     = 48000.0;
    config.bufferSize     = 512;
    config.outputChannels = 2;
    return config;
}

/// Preferences that belong to the installation rather than to a project.
///
/// A project stores what the music is; this stores what the machine is — which
/// interface plays it, at which rate and block size, which MIDI keyboards are
/// connected, and where the window was left. Putting either set in the other
/// file is the mistake that makes a project unopenable on a second Mac, so the
/// two formats are versioned and stored separately (docs/PROJECT_FORMAT.md §1).
///
/// Deliberately narrow: state that already has an owner does not move here.
/// The Browser keeps its own libraries, favourites and recents
/// (`app::Browser::save`), and the recent-projects menu is user defaults —
/// this file carries only what nothing else owns.
///
/// Every field degrades to a default. A settings file is a cache of a
/// preference, never a precondition for launching: a corrupt one, a file from
/// a newer build, or a device that has since been unplugged all resolve to
/// "use the system default" rather than to an error.
struct AppSettings {
    /// Bumped when a field's meaning changes, never when one is added — the
    /// reader tolerates unknown keys and missing keys alike.
    static constexpr int currentVersion = 1;

    /// Defaults deliberately match what the shell used to hardcode: an empty
    /// output identifier is the system default device, and 512 frames is the
    /// block size a shared Bluetooth output can actually sustain.
    platform::AudioDeviceConfig audio = defaultAudioConfig();

    /// Whether the input device opens at launch.
    ///
    /// Off by default, and that is a privacy decision rather than a
    /// performance one: opening the microphone unasked is a permission prompt
    /// the user did not ask for. Arming a recording opens it on demand.
    bool openInputAtLaunch = false;

    /// MIDI sources to connect. Empty means every available source, which is
    /// what someone who plugs in a keyboard and plays a note expects.
    std::vector<std::string> midiInputIdentifiers;

    /// Where the window was, and what it was showing.
    struct Workspace {
        /// All four zero means "never placed" — the window centres itself.
        double windowX      = 0.0;
        double windowY      = 0.0;
        double windowWidth  = 0.0;
        double windowHeight = 0.0;

        /// Index into the editor selector: piano roll, playlist, mixer, editor.
        int  activeEditor = 0;
        bool songMode     = false;
    } workspace;

    [[nodiscard]] std::string toJson() const;

    /// Never fails. Anything unparseable or unexpected yields defaults for the
    /// fields it could not read, and keeps the ones it could.
    [[nodiscard]] static AppSettings fromJson(const std::string& text);

    /// Writes atomically enough for a preferences file: a failed write leaves
    /// the previous settings in place and returns false.
    [[nodiscard]] bool save(const std::filesystem::path& file) const;

    /// A missing file is the normal first-run state and yields defaults.
    [[nodiscard]] static AppSettings load(const std::filesystem::path& file);
};

} // namespace incdaw::app
