#pragma once

#import <Cocoa/Cocoa.h>

/// The Piano Roll's control strip: snap, key, scale, and the two view toggles.
///
/// A separate view from the editor beneath it, and deliberately so. The Piano
/// Roll draws through Metal, where the only primitive is a rectangle and text
/// costs a layer per label — a budget that pays for ten thousand notes in one
/// draw call (docs/DECISIONS.md D-006). Chrome with five labels on it does not
/// belong in that budget, so it is drawn with CoreGraphics through the theme,
/// like every other control surface in the shell.
///
/// It owns no state: it shows what it is told and reports what was clicked.
/// Geometry and hit testing come from app::PianoRollHeaderModel.
@interface INCDAWPianoRollHeaderView : NSView

/// The height the strip wants. Read by whoever lays the editor pane out.
+ (CGFloat)preferredHeight;

/// What the editor is currently set to. Setting any of these redraws.
@property (nonatomic, assign) long long snapTicks;          ///< 0 = no snapping
@property (nonatomic, assign) int       keyRootPitchClass;  ///< 0 = C
@property (nonatomic, assign) int       scaleIndex;         ///< app::music::Scale
@property (nonatomic, assign) BOOL      ghostsVisible;
@property (nonatomic, assign) BOOL      velocityLaneVisible;

/// Picked from the strip. The header changes nothing itself — the editor owns
/// the state, so it is the editor that is told.
@property (nonatomic, copy) void (^onSnapPicked)(long long ticks);
@property (nonatomic, copy) void (^onKeyPicked)(int rootPitchClass);
@property (nonatomic, copy) void (^onScalePicked)(int scaleIndex);
@property (nonatomic, copy) void (^onToggleGhosts)(void);
@property (nonatomic, copy) void (^onToggleVelocityLane)(void);

@end
