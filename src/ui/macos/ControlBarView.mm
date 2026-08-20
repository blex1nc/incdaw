#include "ui/macos/ControlBarView.h"

#include "ui/macos/Theme.h"

#include "engine/core/Time.h"

#include <algorithm>
#include <cmath>

namespace theme = incdaw::ui::theme;

namespace {

using theme::Ink;

constexpr CGFloat kEdge          = 12.0;
constexpr CGFloat kButtonSize    = 30.0;
constexpr CGFloat kButtonGap     = 7.0;
constexpr CGFloat kModeWidth     = 96.0;
constexpr CGFloat kModeHeight    = 22.0;
constexpr CGFloat kTabWidth      = 78.0;
constexpr CGFloat kTabHeight     = 24.0;
constexpr CGFloat kReadoutWidth  = 78.0;
constexpr CGFloat kLcdMinWidth   = 210.0;
constexpr CGFloat kLcdMaxWidth   = 360.0;
constexpr CGFloat kLcdHeight     = 44.0;
constexpr NSInteger kEditorCount = 4;

NSString* const kEditorTitles[kEditorCount] = {@"Piano Roll", @"Playlist", @"Mixer", @"Editor"};

/// Every rectangle the bar draws or hit-tests, derived from the bounds so that
/// drawing and input can never disagree about where a button is.
struct Layout {
    NSRect rewind{};
    NSRect stop{};
    NSRect play{};
    NSRect record{};
    NSRect loop{};
    NSRect metronome{};

    NSRect modePattern{};
    NSRect modeSong{};

    NSRect lcd{};

    /// The readouts inside the display that can be edited. Empty when the
    /// display itself did not fit.
    NSRect position{};
    NSRect tempo{};
    NSRect signature{};

    NSRect tabs[kEditorCount]{};

    NSRect cpuMeter{};
    NSRect outMeter{};
};

Layout layoutFor(NSRect bounds)
{
    Layout out;

    const CGFloat midY = NSMidY(bounds);
    const CGFloat top  = midY + kButtonSize / 2.0;

    CGFloat x = kEdge;
    const auto button = [&](CGFloat extraGap) {
        const NSRect rect = NSMakeRect(x + extraGap, top - kButtonSize, kButtonSize, kButtonSize);
        x += extraGap + kButtonSize + kButtonGap;
        return rect;
    };

    out.rewind = button(0.0);
    out.stop   = button(0.0);
    out.play   = button(0.0);
    out.record = button(0.0);
    out.loop      = button(6.0);
    out.metronome = button(0.0);

    x += 6.0;
    out.modePattern = NSMakeRect(x, midY - kModeHeight / 2.0, kModeWidth / 2.0, kModeHeight);
    out.modeSong    = NSMakeRect(x + kModeWidth / 2.0, out.modePattern.origin.y,
                                 kModeWidth / 2.0, kModeHeight);
    x += kModeWidth;

    // The right-hand cluster is anchored to the trailing edge; the display then
    // takes the middle of whatever is left, so it stays centred while the
    // window is resized and only shrinks when it has to.
    const CGFloat rightEdge = NSMaxX(bounds) - kEdge;

    const CGFloat meterX = rightEdge - kReadoutWidth;
    out.cpuMeter = NSMakeRect(meterX, midY + 2.0, kReadoutWidth - 26.0, 7.0);
    out.outMeter = NSMakeRect(meterX, midY - 9.0, kReadoutWidth - 26.0, 7.0);

    CGFloat tabX = meterX - 14.0 - kTabWidth * static_cast<CGFloat>(kEditorCount);
    for (NSInteger index = 0; index < kEditorCount; ++index)
        out.tabs[index] = NSMakeRect(tabX + kTabWidth * static_cast<CGFloat>(index),
                                     midY - kTabHeight / 2.0, kTabWidth, kTabHeight);

    const CGFloat available = tabX - 14.0 - x;
    const CGFloat lcdWidth  = std::clamp(available, CGFloat{0.0}, kLcdMaxWidth);

    if (lcdWidth >= kLcdMinWidth) {
        const CGFloat lcdX = x + (available - lcdWidth) / 2.0;
        out.lcd = NSMakeRect(lcdX, midY - kLcdHeight / 2.0, lcdWidth, kLcdHeight);

        // The readouts are laid out here rather than while drawing, so that a
        // click can never land somewhere the digits are not: one function
        // decides where they are, and both drawing and hit-testing read it.
        const NSRect  inner      = NSInsetRect(out.lcd, 10.0, 6.0);
        const CGFloat tempoWidth = 84.0;

        out.position = NSMakeRect(NSMinX(inner), NSMinY(inner),
                                  std::max(CGFloat{0.0}, inner.size.width - tempoWidth - 10.0),
                                  inner.size.height);

        out.tempo     = NSMakeRect(NSMaxX(inner) - tempoWidth, NSMinY(inner) + 11.0,
                                   tempoWidth, 21.0);
        out.signature = NSMakeRect(NSMaxX(inner) - tempoWidth, NSMinY(inner) - 1.0,
                                   tempoWidth, 14.0);
    }

    return out;
}

/// Bar : beat : tick, one-based, the way every DAW counts musical position.
NSString* positionText(long long tick, int beatsPerBar, int beatValue)
{
    const long long safe    = std::max(0LL, tick);
    const long long perBeat = beatValue > 0
        ? incdaw::engine::ticksPerQuarterNote * 4 / beatValue
        : incdaw::engine::ticksPerQuarterNote;
    const long long perBar  = perBeat * beatsPerBar;

    const long long bar  = safe / perBar;
    const long long beat = (safe % perBar) / perBeat;
    const long long rest = safe % perBeat;

    return [NSString stringWithFormat:@"%03lld.%lld.%03lld", bar + 1, beat + 1, rest];
}

} // namespace

@interface INCDAWControlBarView () <NSTextFieldDelegate>
@end

@implementation INCDAWControlBarView {
    /// A tempo drag in progress: where it started, and the tempo it started
    /// from. The gesture reports absolute tempi, never deltas, so a slow drag
    /// cannot accumulate rounding.
    BOOL         _tempoDragging;
    CGFloat      _tempoDragOriginY;
    double       _tempoAtDragStart;

    /// The typed tempo. A field editor over the readout rather than a dialog:
    /// a tempo box that interrupts the transport is a tempo box nobody uses.
    NSTextField* _tempoField;
}

- (instancetype)initWithFrame:(NSRect)frame
{
    self = [super initWithFrame:frame];
    if (self == nil)
        return nil;

    _playheadTick = -1;
    _tempo        = 120.0;
    _beatsPerBar  = 4;
    _beatValue    = 4;
    _editorIndex  = 0;
    _contextName  = @"";

    return self;
}

- (BOOL)isFlipped { return NO; }

// A redraw per property change is cheap here — the bar is a few dozen shapes —
// and it keeps the shell from having to remember which setter needs one.
- (void)setPlaying:(BOOL)value      { _playing = value;      [self setNeedsDisplay:YES]; }
- (void)setRecording:(BOOL)value    { _recording = value;    [self setNeedsDisplay:YES]; }
- (void)setLooping:(BOOL)value      { _looping = value;      [self setNeedsDisplay:YES]; }
- (void)setMetronomeOn:(BOOL)value  { _metronomeOn = value;  [self setNeedsDisplay:YES]; }
- (void)setBeatsPerBar:(NSInteger)value { _beatsPerBar = value; [self setNeedsDisplay:YES]; }
- (void)setBeatValue:(NSInteger)value   { _beatValue = value;   [self setNeedsDisplay:YES]; }
- (void)setSongMode:(BOOL)value     { _songMode = value;     [self setNeedsDisplay:YES]; }
- (void)setEditorIndex:(NSInteger)value { _editorIndex = value; [self setNeedsDisplay:YES]; }
- (void)setContextName:(NSString*)value { _contextName = [value copy]; [self setNeedsDisplay:YES]; }
- (void)setAlert:(NSString*)value   { _alert = [value copy];  [self setNeedsDisplay:YES]; }

// ── Drawing ──────────────────────────────────────────────────────────────────

- (void)drawRect:(NSRect)dirtyRect
{
    (void)dirtyRect;

    const NSRect bounds = self.bounds;

    theme::fillGradient(bounds, 0.0, theme::ink(Ink::chromeTop), theme::ink(Ink::chromeBottom),
                        false);
    theme::drawSeparator(NSMakeRect(0.0, 0.0, bounds.size.width, 1.0));

    const Layout layout = layoutFor(bounds);

    theme::drawTransportButton(layout.rewind, theme::Transport::rewind, false);
    theme::drawTransportButton(layout.stop, theme::Transport::stop, false);
    theme::drawTransportButton(layout.play,
                               self.playing ? theme::Transport::pause : theme::Transport::play,
                               self.playing);
    theme::drawTransportButton(layout.record, theme::Transport::record, self.recording);
    theme::drawTransportButton(layout.loop, theme::Transport::loop, self.looping);
    theme::drawTransportButton(layout.metronome, theme::Transport::metronome, self.metronomeOn);

    theme::drawTab(layout.modePattern, @"PAT", !self.songMode, true, false);
    theme::drawTab(layout.modeSong, @"SONG", self.songMode, false, true);

    if (layout.lcd.size.width > 0.0)
        [self drawDisplay:layout];

    for (NSInteger index = 0; index < kEditorCount; ++index)
        theme::drawTab(layout.tabs[index], kEditorTitles[index], index == self.editorIndex,
                       index == 0, index == kEditorCount - 1);

    [self drawReadouts:layout];
}

/// The centre display: position and tempo in lit digits, with the mode and the
/// material's name underneath in the same panel. Tempo and signature are live
/// controls, not labels — drag, double-click or pick.
- (void)drawDisplay:(const Layout&)layout
{
    const NSRect rect = layout.lcd;

    theme::drawLcd(rect);

    // A stopped transport shows the head of the material rather than a blank,
    // because "where would playing start" is the question the display answers.
    NSString* position = positionText(self.playheadTick < 0 ? 0 : self.playheadTick,
                                      static_cast<int>(self.beatsPerBar),
                                      static_cast<int>(self.beatValue));

    theme::drawText(position,
                    NSMakeRect(NSMinX(layout.position), NSMinY(layout.position) + 12.0,
                               layout.position.size.width, 20.0),
                    self.playing ? theme::ink(Ink::lcdText)
                                 : theme::darken(theme::ink(Ink::lcdText), 0.25),
                    theme::numericFont(17.0, NSFontWeightSemibold));

    NSString* caption = self.alert != nil
        ? self.alert
        : [NSString stringWithFormat:@"%@ · %@", self.songMode ? @"SONG" : @"PATTERN",
                                     self.contextName.length > 0 ? self.contextName : @"—"];

    theme::drawText(caption,
                    NSMakeRect(NSMinX(layout.position), NSMinY(layout.position) - 1.0,
                               layout.position.size.width, 13.0),
                    self.alert != nil ? theme::ink(Ink::record) : theme::ink(Ink::lcdTextDim),
                    theme::labelFont(9.5, NSFontWeightSemibold));

    // The two editable readouts carry a faint well behind them: the only cue
    // in the display that says "this number is a control".
    if (_tempoField == nil)
        theme::fillRounded(NSInsetRect(layout.tempo, -3.0, -1.0), theme::metrics::radiusPad,
                           theme::withAlpha(theme::ink(Ink::lcdText), 0.06));

    theme::fillRounded(NSInsetRect(layout.signature, -3.0, -1.0), theme::metrics::radiusPad,
                       theme::withAlpha(theme::ink(Ink::lcdText), 0.06));

    if (_tempoField == nil)
        theme::drawText([NSString stringWithFormat:@"%.2f", self.tempo],
                        NSMakeRect(NSMinX(layout.tempo), NSMinY(layout.tempo) + 1.0,
                                   layout.tempo.size.width, 20.0),
                        theme::ink(Ink::lcdText), theme::numericFont(17.0, NSFontWeightSemibold),
                        theme::Align::right);

    theme::drawText([NSString stringWithFormat:@"BPM · %ld/%ld",
                                               (long)self.beatsPerBar, (long)self.beatValue],
                    NSMakeRect(NSMinX(layout.signature), NSMinY(layout.signature),
                               layout.signature.size.width, 13.0),
                    theme::ink(Ink::lcdTextDim), theme::labelFont(9.5, NSFontWeightSemibold),
                    theme::Align::right);

    // The recording lamp lives on the display, where it is impossible to miss.
    if (self.recording) {
        const NSRect lamp = NSMakeRect(NSMaxX(rect) - 13.0, NSMaxY(rect) - 13.0, 6.0, 6.0);
        theme::fillRounded(lamp, 3.0, theme::ink(Ink::record));
    }
}

// ── The tempo readout as a control ───────────────────────────────────────────

- (void)mouseDragged:(NSEvent*)event
{
    if (!_tempoDragging)
        return;

    const NSPoint point = [self convertPoint:event.locationInWindow fromView:nil];

    // Up is faster. A quarter of a BPM per point is fine enough to land on a
    // whole number without effort and coarse enough to cross a genre in one
    // gesture; Option makes it a hundredth for the last decimal.
    const CGFloat  travel = point.y - _tempoDragOriginY;
    const double   step   = (event.modifierFlags & NSEventModifierFlagOption) != 0 ? 0.01 : 0.25;
    const double   tempo  = _tempoAtDragStart + static_cast<double>(travel) * step;

    if (self.onTempoChange != nil)
        self.onTempoChange(tempo, NO);
}

- (void)mouseUp:(NSEvent*)event
{
    (void)event;

    if (!_tempoDragging)
        return;

    _tempoDragging = NO;

    // The gesture's last word: the shell closes the merged undo entry and
    // rebuilds the graph on this one.
    if (self.onTempoChange != nil)
        self.onTempoChange(self.tempo, YES);
}

/// A field editor placed exactly over the digits it replaces.
- (void)beginTypingTempo:(NSRect)rect
{
    if (_tempoField != nil)
        return;

    _tempoField = [[NSTextField alloc] initWithFrame:NSInsetRect(rect, -3.0, -1.0)];
    _tempoField.stringValue     = [NSString stringWithFormat:@"%.2f", self.tempo];
    _tempoField.font            = theme::numericFont(15.0, NSFontWeightSemibold);
    _tempoField.alignment       = NSTextAlignmentRight;
    _tempoField.textColor       = theme::ink(Ink::lcdText);
    _tempoField.backgroundColor = theme::ink(Ink::lcdBackground);
    _tempoField.bezeled         = NO;
    _tempoField.focusRingType   = NSFocusRingTypeNone;
    _tempoField.delegate        = self;

    [self addSubview:_tempoField];
    [self.window makeFirstResponder:_tempoField];
    [self setNeedsDisplay:YES];
}

- (void)endTypingTempoCommitting:(BOOL)commit
{
    if (_tempoField == nil)
        return;

    const double typed = _tempoField.doubleValue;

    [_tempoField removeFromSuperview];
    _tempoField = nil;

    if (commit && typed > 0.0 && self.onTempoChange != nil)
        self.onTempoChange(typed, YES);

    [self.window makeFirstResponder:self];
    [self setNeedsDisplay:YES];
}

- (BOOL)control:(NSControl*)control textView:(NSTextView*)view doCommandBySelector:(SEL)selector
{
    (void)control;
    (void)view;

    if (selector == @selector(insertNewline:)) {
        [self endTypingTempoCommitting:YES];
        return YES;
    }

    if (selector == @selector(cancelOperation:)) {
        [self endTypingTempoCommitting:NO];
        return YES;
    }

    return NO;
}

/// Clicking away commits, the way every other numeric field in a DAW behaves.
- (void)controlTextDidEndEditing:(NSNotification*)notification
{
    (void)notification;
    [self endTypingTempoCommitting:YES];
}

/// The signatures a session actually uses, offered as a menu rather than as
/// two more fields to type into.
- (void)showSignatureMenuAt:(NSRect)rect withEvent:(NSEvent*)event
{
    (void)event;

    if (self.onTimeSignature == nil)
        return;

    static const NSInteger choices[][2] = {
        {2, 4}, {3, 4}, {4, 4}, {5, 4}, {6, 4}, {6, 8}, {7, 8}, {9, 8}, {12, 8},
    };

    NSMenu* menu = [[NSMenu alloc] init];

    for (const auto& choice : choices) {
        NSString* title = [NSString stringWithFormat:@"%ld/%ld", (long)choice[0], (long)choice[1]];

        NSMenuItem* item = [menu addItemWithTitle:title action:@selector(signatureChosen:)
                                    keyEquivalent:@""];
        item.target      = self;
        item.tag         = choice[0] * 100 + choice[1];
        item.state       = (choice[0] == self.beatsPerBar && choice[1] == self.beatValue)
                               ? NSControlStateValueOn
                               : NSControlStateValueOff;
    }

    [menu popUpMenuPositioningItem:nil
                        atLocation:NSMakePoint(NSMinX(rect), NSMinY(rect) - 4.0)
                            inView:self];
}

- (void)signatureChosen:(NSMenuItem*)sender
{
    if (self.onTimeSignature != nil)
        self.onTimeSignature(sender.tag / 100, sender.tag % 100);
}

/// Load and output level. Both are read every housekeeping tick, so they are
/// the two numbers that tell a user whether the engine is healthy.
- (void)drawReadouts:(const Layout&)layout
{
    theme::drawMeter(layout.cpuMeter, self.cpuLoad, 0.0, false, false);
    theme::drawMeter(layout.outMeter, self.masterRms, self.masterPeak, false, false);

    const NSRect cpuLabel = NSMakeRect(NSMaxX(layout.cpuMeter) + 5.0,
                                       NSMinY(layout.cpuMeter) - 3.0, 22.0, 12.0);
    const NSRect outLabel = NSMakeRect(NSMaxX(layout.outMeter) + 5.0,
                                       NSMinY(layout.outMeter) - 3.0, 22.0, 12.0);

    theme::drawText(@"CPU", cpuLabel, theme::ink(Ink::textDim), theme::labelFont(8.5,
                    NSFontWeightSemibold));
    theme::drawText(@"OUT", outLabel, theme::ink(Ink::textDim), theme::labelFont(8.5,
                    NSFontWeightSemibold));
}

// ── Input ────────────────────────────────────────────────────────────────────

- (void)mouseDown:(NSEvent*)event
{
    const NSPoint point = [self convertPoint:event.locationInWindow fromView:nil];
    const Layout layout = layoutFor(self.bounds);

    if (self.onTransport != nil) {
        if (NSPointInRect(point, layout.rewind))      { self.onTransport(INCDAWTransportRewind); return; }
        if (NSPointInRect(point, layout.stop))        { self.onTransport(INCDAWTransportStop);   return; }
        if (NSPointInRect(point, layout.play))        { self.onTransport(INCDAWTransportPlay);   return; }
        if (NSPointInRect(point, layout.record))      { self.onTransport(INCDAWTransportRecord); return; }
        if (NSPointInRect(point, layout.loop))        { self.onTransport(INCDAWTransportLoop);   return; }
        if (NSPointInRect(point, layout.metronome))   { self.onTransport(INCDAWTransportMetronome); return; }
    }

    if (layout.tempo.size.width > 0.0 && NSPointInRect(point, layout.tempo)) {
        if (event.clickCount >= 2) {
            [self beginTypingTempo:layout.tempo];
            return;
        }

        _tempoDragging     = YES;
        _tempoDragOriginY  = point.y;
        _tempoAtDragStart  = self.tempo;
        return;
    }

    if (layout.signature.size.width > 0.0 && NSPointInRect(point, layout.signature)) {
        [self showSignatureMenuAt:layout.signature withEvent:event];
        return;
    }

    if (self.onSelectMode != nil) {
        if (NSPointInRect(point, layout.modePattern)) { self.onSelectMode(NO);  return; }
        if (NSPointInRect(point, layout.modeSong))    { self.onSelectMode(YES); return; }
    }

    if (self.onSelectEditor != nil) {
        for (NSInteger index = 0; index < kEditorCount; ++index) {
            if (NSPointInRect(point, layout.tabs[index])) {
                self.onSelectEditor(index);
                return;
            }
        }
    }
}

@end

@implementation INCDAWStatusBarView

- (BOOL)isFlipped { return NO; }

- (void)setText:(NSString*)value
{
    _text = [value copy];
    [self setNeedsDisplay:YES];
}

- (void)drawRect:(NSRect)dirtyRect
{
    (void)dirtyRect;

    const NSRect bounds = self.bounds;

    theme::fillRect(bounds, theme::ink(Ink::panel));
    theme::drawSeparator(NSMakeRect(0.0, NSMaxY(bounds) - 1.0, bounds.size.width, 1.0));

    theme::drawTextCentred(self.text, NSInsetRect(bounds, 12.0, 2.0),
                           theme::ink(Ink::textSecondary), theme::numericFont(10.5,
                           NSFontWeightRegular));
}

@end
