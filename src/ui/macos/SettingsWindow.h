#pragma once

#import <Cocoa/Cocoa.h>

namespace incdaw::app { struct AppSettings; }

/// Audio and MIDI settings.
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

- (instancetype)initWithSettings:(incdaw::app::AppSettings*)settings;

/// Called after the user applies a change, on the main thread. The settings
/// struct already carries the new values.
@property (nonatomic, copy) void (^onApply)(void);

/// Supplies the line describing what the device actually granted — which is
/// not always what was asked for, and is the only honest latency figure.
@property (nonatomic, copy) NSString* (^statusProvider)(void);

/// Opens the window, re-enumerating devices first. Safe to call repeatedly:
/// hardware appears and disappears while the application is running.
- (void)show;

/// Refreshes the status line from `statusProvider`, if the window is open.
- (void)refreshStatus;

@end
