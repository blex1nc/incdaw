#pragma once

// INCDAW — the shared visual language of the macOS shell.
//
// Every pane used to invent its own greys inline. That made the window look
// like six programs sharing a title bar, and it made a palette change a
// six-file edit. This header is the single place where INCDAW decides what a
// surface, a pad, a fader, a region or a readout looks like; the views draw
// rectangles through it and own no colours of their own.
//
// The design language is a deliberate hybrid of the two workflows INCDAW is
// measured against (docs/DECISIONS.md D-035):
//
//   * FL Studio contributes the density — a dark, low-glare ground, step pads
//     grouped by beat, channel colour used as the primary identifier, and
//     readouts that stay legible at a glance while the transport runs.
//   * GarageBand contributes the calm — rounded geometry, soft vertical
//     gradients with a single hairline highlight, round transport buttons
//     around a centre display, and regions drawn as translucent, named,
//     rounded blocks rather than flat rectangles.
//
// Nothing here reproduces either program's artwork: there are no imported
// assets, icons, gradients or measurements. Every shape below is drawn from
// primitives by this file (CLAUDE.md §43).

#import <Cocoa/Cocoa.h>

// The roles themselves, the values behind them, and the file they are read
// from live in a header with no AppKit in it, so that a theme can be parsed,
// edited and tested without a window server (ui/ThemePalette.h).
#include "ui/ThemePalette.h"

#include <cstdint>

namespace incdaw::ui::theme {

// ── Palette ──────────────────────────────────────────────────────────────────

/// The scheme every drawing call below reads from. Set once at launch from the
/// user's preference, and again whenever the Appearance tab changes a colour.
///
/// Main thread only, and that is not a restriction so much as a description:
/// AppKit draws on the main thread and nothing else ever asks for a colour, so
/// the palette needs no lock and takes none.
void setPalette(const ThemePalette& palette);

[[nodiscard]] const ThemePalette& palette();

/// Posted after `setPalette`. Views that draw themselves need no observer —
/// they are invalidated by `refreshViewTree` — but anything that handed a
/// colour to an AppKit control holds a snapshot, and snapshots must be
/// reassigned by whoever made them.
extern NSString* const kPaletteChangedNotification;

/// Recursively marks `root` and every view under it as needing display.
void refreshViewTree(NSView* root);

/// True when the scheme's ground is brighter than its text — which is what the
/// shell needs in order to hand AppKit the matching NSAppearance. INCDAW draws
/// its own surfaces, but scrollers, sheets, menus and text fields are AppKit's,
/// and a dark scroller over a light pane is the one thing a theme cannot fix.
[[nodiscard]] BOOL paletteIsLight();

[[nodiscard]] NSColor* ink(Ink which);

/// Project colours arrive as 0xAARRGGBB from project::Model.
[[nodiscard]] NSColor* fromArgb(std::uint32_t argb, CGFloat brightness = 1.0);

[[nodiscard]] NSColor* withAlpha(NSColor* colour, CGFloat alpha);
[[nodiscard]] NSColor* lighten(NSColor* colour, CGFloat amount);
[[nodiscard]] NSColor* darken(NSColor* colour, CGFloat amount);
/// `t` = 0 gives `a`, `t` = 1 gives `b`.
[[nodiscard]] NSColor* mix(NSColor* a, NSColor* b, CGFloat t);

/// Text that stays readable on `background`. Project colours are the user's to
/// choose, so a name plate cannot assume its own label is dark or light.
[[nodiscard]] NSColor* labelOn(NSColor* background);

// ── Metrics ──────────────────────────────────────────────────────────────────

namespace metrics {

inline constexpr CGFloat radiusPad     = 3.0;   ///< step pads, small toggles
inline constexpr CGFloat radiusControl = 5.0;   ///< buttons, regions, faders
inline constexpr CGFloat radiusPanel   = 8.0;   ///< headers, strips, displays

inline constexpr CGFloat controlBarHeight = 64.0;
inline constexpr CGFloat statusBarHeight  = 24.0;
inline constexpr CGFloat rulerHeight      = 26.0;

} // namespace metrics

// ── Type ─────────────────────────────────────────────────────────────────────

/// UI text: the system face, so INCDAW reads like a macOS application.
[[nodiscard]] NSFont* labelFont(CGFloat size, NSFontWeight weight = NSFontWeightMedium);

/// Numbers that change while the transport runs. Monospaced digits, because a
/// position readout that reflows as it counts is unreadable.
[[nodiscard]] NSFont* numericFont(CGFloat size, NSFontWeight weight = NSFontWeightSemibold);

enum class Align { left, centre, right };

/// Draws inside `rect` from its top edge, truncating with an ellipsis.
void drawText(NSString* text, NSRect rect, NSColor* colour, NSFont* font,
              Align align = Align::left);

/// The same, vertically centred on `rect` — what almost every control wants.
void drawTextCentred(NSString* text, NSRect rect, NSColor* colour, NSFont* font,
                     Align align = Align::left);

// ── Shape primitives ─────────────────────────────────────────────────────────

[[nodiscard]] NSBezierPath* roundedPath(NSRect rect, CGFloat radius);

void fillRect(NSRect rect, NSColor* colour);
void fillRounded(NSRect rect, CGFloat radius, NSColor* colour);
void strokeRounded(NSRect rect, CGFloat radius, NSColor* colour, CGFloat width = 1.0);

/// A vertical gradient clipped to a rounded rectangle. `top` is the lit edge;
/// flipped views get the same visual result because the gradient is drawn in
/// the view's own coordinate space and callers pass what they mean.
void fillGradient(NSRect rect, CGFloat radius, NSColor* top, NSColor* bottom,
                  bool flipped = false);

// ── Composite surfaces ───────────────────────────────────────────────────────

/// A raised chrome surface with a hairline top highlight and bottom shadow:
/// control bar, track header, mixer strip, pane header.
void drawPanel(NSRect rect, CGFloat radius = metrics::radiusPanel, bool selected = false,
               bool flipped = false);

/// A recessed area content sits inside: step grid, timeline, meter housing.
void drawWell(NSRect rect, CGFloat radius = metrics::radiusPad, bool flipped = false);

/// The horizontal divider between panes.
void drawSeparator(NSRect rect);

// ── Controls ─────────────────────────────────────────────────────────────────

/// FL's step button under GarageBand's rounding: unlit pads carry the beat
/// grouping, lit pads carry the channel colour and a soft glow.
///
/// `level` is how hard the step hits, 0..1. It dims the lit colour and raises a
/// foot from the pad's low edge, so a pattern's dynamics are readable by
/// scanning the grid rather than by opening an editor for each step. It is the
/// same velocity-to-brightness rule the Piano Roll draws its notes with;
/// unlit pads ignore it.
void drawStepPad(NSRect rect, NSColor* colour, bool on, bool downbeat, bool underPlayhead,
                 bool flipped = false, double level = 1.0);

/// A GarageBand-style region: rounded, gradient-filled in the material colour,
/// with a darker name band along its top edge. `contentInset` reports where a
/// caller may draw notes or a waveform without colliding with the band.
NSRect drawRegion(NSRect rect, NSColor* colour, NSString* name, bool selected, bool muted,
                  bool flipped = false);

enum class Transport { play, pause, stop, record, loop, rewind, forward, metronome };

/// A round control-bar button. The glyph is drawn from primitives, never from
/// a font or an image, so it stays crisp at any size.
void drawTransportButton(NSRect rect, Transport kind, bool active, bool enabled = true);

/// FL's rotary, drawn as an arc around a dot. `normalised` is 0..1; `bipolar`
/// draws the arc outward from twelve o'clock, which is what pan wants.
void drawKnob(NSRect rect, double normalised, NSColor* accent, bool bipolar = false);

/// A horizontal level control — the rack row's volume, a track header's slider.
void drawSlider(NSRect rect, double normalised, NSColor* accent, bool flipped = false);

/// The mixer's vertical fader: a recessed track with a rounded cap. Unity is
/// at the top of the travel in both orientations; only the direction of `y`
/// changes with `flipped`.
void drawFader(NSRect rect, double normalised, NSColor* accent, bool selected,
               bool flipped = false);

/// Green → amber → red level meter with a peak-hold line. `level` and `peak`
/// are 0..1 display positions, already scaled by the caller.
void drawMeter(NSRect rect, double level, double peak, bool vertical, bool flipped = false);

/// M / S / ● toggles. `glyph` may be nil for a plain lamp.
void drawToggle(NSRect rect, NSString* glyph, bool on, NSColor* onColour, bool flipped = false);

/// The centre display: a dark recessed readout with a lit bezel.
void drawLcd(NSRect rect);

/// A pill in a segmented selector (the editor tabs, the pattern/song switch).
void drawTab(NSRect rect, NSString* title, bool selected, bool first, bool last);

/// The vertical playhead line with a small triangular head at `y`.
void drawPlayhead(CGFloat x, NSRect bounds, CGFloat headY, bool flipped = false);

} // namespace incdaw::ui::theme
