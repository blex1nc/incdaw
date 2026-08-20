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

    NSRect modePattern{};
    NSRect modeSong{};

    NSRect lcd{};

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
    out.loop   = button(6.0);

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
    }

    return out;
}

/// Bar : beat : tick, one-based, the way every DAW counts musical position.
NSString* positionText(long long tick, int beatsPerBar)
{
    const long long safe    = std::max(0LL, tick);
    const long long perBeat = incdaw::engine::ticksPerQuarterNote;
    const long long perBar  = perBeat * beatsPerBar;

    const long long bar  = safe / perBar;
    const long long beat = (safe % perBar) / perBeat;
    const long long rest = safe % perBeat;

    return [NSString stringWithFormat:@"%03lld.%lld.%03lld", bar + 1, beat + 1, rest];
}

} // namespace

@implementation INCDAWControlBarView

- (instancetype)initWithFrame:(NSRect)frame
{
    self = [super initWithFrame:frame];
    if (self == nil)
        return nil;

    _playheadTick = -1;
    _tempo        = 120.0;
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

    theme::drawTab(layout.modePattern, @"PAT", !self.songMode, true, false);
    theme::drawTab(layout.modeSong, @"SONG", self.songMode, false, true);

    if (layout.lcd.size.width > 0.0)
        [self drawDisplay:layout.lcd];

    for (NSInteger index = 0; index < kEditorCount; ++index)
        theme::drawTab(layout.tabs[index], kEditorTitles[index], index == self.editorIndex,
                       index == 0, index == kEditorCount - 1);

    [self drawReadouts:layout];
}

/// The centre display: position and tempo in lit digits, with the mode and the
/// material's name underneath in the same panel.
- (void)drawDisplay:(NSRect)rect
{
    theme::drawLcd(rect);

    const NSRect inner = NSInsetRect(rect, 10.0, 6.0);
    const CGFloat tempoWidth = 84.0;

    const NSRect positionArea = NSMakeRect(NSMinX(inner), NSMinY(inner),
                                           std::max(CGFloat{0.0},
                                                    inner.size.width - tempoWidth - 10.0),
                                           inner.size.height);

    const NSRect tempoArea = NSMakeRect(NSMaxX(inner) - tempoWidth, NSMinY(inner),
                                        tempoWidth, inner.size.height);

    // A stopped transport shows the head of the material rather than a blank,
    // because "where would playing start" is the question the display answers.
    NSString* position = positionText(self.playheadTick < 0 ? 0 : self.playheadTick, 4);

    theme::drawText(position,
                    NSMakeRect(NSMinX(positionArea), NSMinY(positionArea) + 12.0,
                               positionArea.size.width, 20.0),
                    self.playing ? theme::ink(Ink::lcdText)
                                 : theme::darken(theme::ink(Ink::lcdText), 0.25),
                    theme::numericFont(17.0, NSFontWeightSemibold));

    NSString* caption = self.alert != nil
        ? self.alert
        : [NSString stringWithFormat:@"%@ · %@", self.songMode ? @"SONG" : @"PATTERN",
                                     self.contextName.length > 0 ? self.contextName : @"—"];

    theme::drawText(caption,
                    NSMakeRect(NSMinX(positionArea), NSMinY(positionArea) - 1.0,
                               positionArea.size.width, 13.0),
                    self.alert != nil ? theme::ink(Ink::record) : theme::ink(Ink::lcdTextDim),
                    theme::labelFont(9.5, NSFontWeightSemibold));

    theme::drawText([NSString stringWithFormat:@"%.2f", self.tempo],
                    NSMakeRect(NSMinX(tempoArea), NSMinY(tempoArea) + 12.0,
                               tempoArea.size.width, 20.0),
                    theme::ink(Ink::lcdText), theme::numericFont(17.0, NSFontWeightSemibold),
                    theme::Align::right);

    theme::drawText(@"BPM · 4/4",
                    NSMakeRect(NSMinX(tempoArea), NSMinY(tempoArea) - 1.0,
                               tempoArea.size.width, 13.0),
                    theme::ink(Ink::lcdTextDim), theme::labelFont(9.5, NSFontWeightSemibold),
                    theme::Align::right);

    // The recording lamp lives on the display, where it is impossible to miss.
    if (self.recording) {
        const NSRect lamp = NSMakeRect(NSMaxX(rect) - 13.0, NSMaxY(rect) - 13.0, 6.0, 6.0);
        theme::fillRounded(lamp, 3.0, theme::ink(Ink::record));
    }
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
