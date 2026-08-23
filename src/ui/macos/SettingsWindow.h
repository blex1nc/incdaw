#pragma once

#import <Cocoa/Cocoa.h>

namespace incdaw::app { struct AppSettings; }

/// Audio, MIDI, Appearance and Updates settings.
///
/// Every DAW has this window and every DAW needs it: the device that plays a
/// project is a property of the machine, not of the music, and until it can be
/// chosen the application is hardcoded to whatever the system default happens
/// to be. Choosing an interface, a sample rate and a block size is also the
/// only way latency is tunable at all (docs/AUDIO_ENGINE.md §2).
///
/// The window edits an `app::AppSettings` in place and reports through
/// `onApply`; restarting the engine, reopening MIDI and persisting the file
/// belong to the shell, which owns all three. Nothing here touches the audio
/// engine directly.
@interface INCDAWSettingsWindow : NSObject <NSWindowDelegate>

/// `themesDirectory` is "<Application Support>/INCDAW/Themes", or nil on a
/// machine with no writable support directory — in which case the Appearance
/// tab offers the built-in schemes and says why it offers nothing else.
- (instancetype)initWithSettings:(incdaw::app::AppSettings*)settings
                 themesDirectory:(NSString*)themesDirectory;

/// Called after the user applies a change, on the main thread. The settings
/// struct already carries the new values.
@property (nonatomic, copy) void (^onApply)(void);

/// Called after the theme changes, on the main thread. The palette is already
/// live and every window has been told to redraw; what remains is persisting
/// `appearance.themeName`, which belongs to whoever owns the settings file.
///
/// Deliberately not `onApply`: that restarts the audio device, and changing a
/// colour must never interrupt playback.
@property (nonatomic, copy) void (^onAppearanceChanged)(void);

/// Supplies the line describing what the device actually granted — which is
/// not always what was asked for, and is the only honest latency figure.
@property (nonatomic, copy) NSString* (^statusProvider)(void);

/// Opens the window, re-enumerating devices first. Safe to call repeatedly:
/// hardware appears and disappears while the application is running.
- (void)show;

/// Refreshes the status line from `statusProvider`, if the window is open.
- (void)refreshStatus;

/// Re-enumerates the machine's audio and MIDI hardware and rebuilds the lists.
///
/// Called by the shell when the system reports a change
/// (platform/DeviceWatcher.h), so an open window is never left describing an
/// interface that has been unplugged. A no-op before the window is built.
- (void)reloadHardware;

@end
