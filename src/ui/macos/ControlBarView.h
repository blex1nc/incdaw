#pragma once

// INCDAW — the control bar and the hint bar.
//
// The window used to carry two stock NSSegmentedControls and a text field. That
// worked, but it put the transport in a menu and left the most-read numbers in
// a project — position, tempo, mode, load — inside a status string.
//
// This is the shell's chrome instead: a centre display flanked by round
// transport buttons (the arrangement GarageBand made standard), carrying the
// readouts and the pattern/song switch a pattern-based DAW needs on screen at
// all times (what FL Studio's toolbar exists for). It owns no state of its own
// beyond what it is told, and turns clicks into blocks — the shell still
// decides what a transport action means.

#import <Cocoa/Cocoa.h>

typedef NS_ENUM(NSInteger, INCDAWTransportAction) {
    INCDAWTransportRewind,
    INCDAWTransportStop,
    INCDAWTransportPlay,
    INCDAWTransportRecord,
    INCDAWTransportLoop,
};

@interface INCDAWControlBarView : NSView

// ── What the bar shows ───────────────────────────────────────────────────────

@property (nonatomic) BOOL      playing;
@property (nonatomic) BOOL      recording;
@property (nonatomic) BOOL      looping;
@property (nonatomic) BOOL      songMode;
@property (nonatomic) NSInteger editorIndex;      ///< 0 roll, 1 playlist, 2 mixer, 3 editor

/// -1 while the transport is stopped, which the display shows as the start of
/// the material rather than as a blank.
@property (nonatomic) long long playheadTick;

@property (nonatomic) double tempo;
@property (nonatomic) double cpuLoad;             ///< 0..1, peak callback load
@property (nonatomic) double masterPeak;          ///< 0..1
@property (nonatomic) double masterRms;           ///< 0..1

/// The pattern or arrangement the transport is pointed at.
@property (nonatomic, copy) NSString* contextName;

/// A one-line warning shown in place of the context name; nil when healthy.
@property (nonatomic, copy) NSString* alert;

// ── What the bar reports ─────────────────────────────────────────────────────

@property (nonatomic, copy) void (^onTransport)(INCDAWTransportAction action);
@property (nonatomic, copy) void (^onSelectEditor)(NSInteger index);
@property (nonatomic, copy) void (^onSelectMode)(BOOL songMode);

@end

/// The hint bar along the bottom edge: one line of monospaced state, drawn in
/// the same language as the rest of the chrome instead of as a bare label.
@interface INCDAWStatusBarView : NSView

@property (nonatomic, copy) NSString* text;

@end
