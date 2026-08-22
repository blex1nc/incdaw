#include "ui/macos/PianoRollHeaderView.h"

#include "app/MusicTheory.h"
#include "app/PianoRollHeaderModel.h"
#include "ui/macos/Theme.h"

#include <memory>

using namespace incdaw;

namespace theme = incdaw::ui::theme;

namespace {

using theme::Ink;
using Header = app::PianoRollHeaderModel;

/// The strip draws in the model's coordinates; this is the only place they turn
/// into AppKit rectangles.
NSRect box(Header::Rect rect)
{
    return NSMakeRect(rect.x, rect.y, rect.width, rect.height);
}

/// A readout: a caption of what the control is, and the value it is set to.
///
/// Both on one pill rather than a label beside a control, because the strip is
/// read at a glance while the transport runs — "SNAP 1/16" is one thing to
/// find, and a caption that can drift away from its value is two.
void drawReadout(NSRect rect, NSString* caption, NSString* value)
{
    theme::drawPanel(rect, theme::metrics::radiusControl, false, true);

    const CGFloat inset = 8.0;

    theme::drawTextCentred(caption,
                           NSMakeRect(NSMinX(rect) + inset, NSMinY(rect),
                                      rect.size.width - inset * 2.0, rect.size.height),
                           theme::ink(Ink::textDim), theme::labelFont(9.0, NSFontWeightBold));

    // The value is right-aligned against a chevron's worth of space, so that
    // "1 bar" and "Off" end where each other end and the strip does not jitter
    // as the setting changes.
    theme::drawTextCentred(value,
                           NSMakeRect(NSMinX(rect) + inset, NSMinY(rect),
                                      rect.size.width - inset * 2.0 - 9.0, rect.size.height),
                           theme::ink(Ink::textPrimary), theme::labelFont(11.0),
                           theme::Align::right);

    // The chevron: this pill opens a list. Drawn from primitives like every
    // other glyph in the shell.
    const CGFloat centreY = NSMidY(rect);
    const CGFloat right   = NSMaxX(rect) - 7.0;

    NSBezierPath* chevron = [NSBezierPath bezierPath];
    [chevron moveToPoint:NSMakePoint(right - 4.0, centreY - 1.5)];
    [chevron lineToPoint:NSMakePoint(right, centreY + 2.5)];
    [chevron lineToPoint:NSMakePoint(right + 4.0, centreY - 1.5)];
    chevron.lineWidth = 1.5;

    [theme::ink(Ink::textDim) setStroke];
    [chevron stroke];
}

} // namespace

@implementation INCDAWPianoRollHeaderView {
    std::unique_ptr<Header> _model;
}

+ (CGFloat)preferredHeight { return static_cast<CGFloat>(Header::Layout{}.height); }

- (instancetype)initWithFrame:(NSRect)frame
{
    self = [super initWithFrame:frame];
    if (self == nil)
        return nil;

    _model = std::make_unique<Header>();

    // The editor's own defaults, so the strip says what is true before anything
    // has been picked in it.
    _snapTicks           = engine::ticksPerQuarterNote / 4;
    _keyRootPitchClass   = 0;
    _scaleIndex          = 0;
    _ghostsVisible       = YES;
    _velocityLaneVisible = YES;

    return self;
}

// Flipped, so the view agrees with the model instead of converting at every
// call — the same choice the Piano Roll and the rack make.
- (BOOL)isFlipped { return YES; }

// ── Drawing ──────────────────────────────────────────────────────────────────

- (void)drawRect:(NSRect)dirtyRect
{
    (void)dirtyRect;

    theme::fillRect(self.bounds, theme::ink(Ink::panel));
    theme::drawSeparator(NSMakeRect(0.0, self.bounds.size.height - 1.0,
                                    self.bounds.size.width, 1.0));

    const double width = self.bounds.size.width;

    drawReadout(box(_model->snapRect()), @"SNAP",
                @(Header::snapName(Header::snapForTicks(self.snapTicks))));

    drawReadout(box(_model->keyRect()), @"KEY",
                @(app::music::pitchClassName(self.keyRootPitchClass)));

    drawReadout(box(_model->scaleRect()), @"SCALE",
                @(Header::scaleName(Header::scaleAt(static_cast<std::size_t>(self.scaleIndex)))));

    // The toggles carry the material colour of what they show: ghosts are other
    // channels' notes, the lane is velocity. Both are MIDI material.
    theme::drawToggle(box(_model->ghostsRect(width)), @"GHOST", self.ghostsVisible,
                      theme::ink(Ink::midi), true);

    theme::drawToggle(box(_model->velocityLaneRect(width)), @"VEL", self.velocityLaneVisible,
                      theme::ink(Ink::midi), true);
}

// ── Input ────────────────────────────────────────────────────────────────────

- (void)mouseDown:(NSEvent*)event
{
    const NSPoint point = [self convertPoint:event.locationInWindow fromView:nil];
    const Header::Zone zone = _model->hitTest(point.x, point.y, self.bounds.size.width);

    switch (zone) {
        case Header::Zone::snap:
            [self showSnapMenuUnder:box(_model->snapRect())];
            return;

        case Header::Zone::key:
            [self showKeyMenuUnder:box(_model->keyRect())];
            return;

        case Header::Zone::scale:
            [self showScaleMenuUnder:box(_model->scaleRect())];
            return;

        case Header::Zone::ghosts:
            if (self.onToggleGhosts != nil)
                self.onToggleGhosts();
            return;

        case Header::Zone::velocityLane:
            if (self.onToggleVelocityLane != nil)
                self.onToggleVelocityLane();
            return;

        case Header::Zone::none:
            return;
    }
}

/// Opens a list under the pill that owns it, so the chosen value appears where
/// the value already was.
- (void)popUp:(NSMenu*)menu under:(NSRect)rect
{
    [menu popUpMenuPositioningItem:nil
                        atLocation:NSMakePoint(NSMinX(rect), NSMaxY(rect) + 2.0)
                            inView:self];
}

- (void)showSnapMenuUnder:(NSRect)rect
{
    NSMenu* menu = [[NSMenu alloc] init];
    const Header::Snap current = Header::snapForTicks(self.snapTicks);

    for (std::size_t index = 0; index < Header::snapCount; ++index) {
        const Header::Snap snap = Header::snapAt(index);

        NSMenuItem* item = [menu addItemWithTitle:@(Header::snapName(snap))
                                           action:@selector(snapPicked:)
                                    keyEquivalent:@""];
        item.target           = self;
        item.tag              = static_cast<NSInteger>(index);
        item.state            = snap == current ? NSControlStateValueOn : NSControlStateValueOff;
    }

    [self popUp:menu under:rect];
}

- (void)showKeyMenuUnder:(NSRect)rect
{
    NSMenu* menu = [[NSMenu alloc] init];

    for (int pitchClass = 0; pitchClass < 12; ++pitchClass) {
        NSMenuItem* item = [menu addItemWithTitle:@(app::music::pitchClassName(pitchClass))
                                           action:@selector(keyPicked:)
                                    keyEquivalent:@""];
        item.target = self;
        item.tag    = pitchClass;
        item.state  = pitchClass == self.keyRootPitchClass ? NSControlStateValueOn
                                                           : NSControlStateValueOff;
    }

    [self popUp:menu under:rect];
}

- (void)showScaleMenuUnder:(NSRect)rect
{
    NSMenu* menu = [[NSMenu alloc] init];

    for (std::size_t index = 0; index < Header::scaleCount; ++index) {
        NSMenuItem* item =
            [menu addItemWithTitle:@(Header::scaleName(Header::scaleAt(index)))
                            action:@selector(scalePicked:)
                     keyEquivalent:@""];
        item.target = self;
        item.tag    = static_cast<NSInteger>(index);
        item.state  = static_cast<int>(index) == self.scaleIndex ? NSControlStateValueOn
                                                                 : NSControlStateValueOff;
    }

    [self popUp:menu under:rect];
}

- (void)snapPicked:(NSMenuItem*)item
{
    if (self.onSnapPicked != nil)
        self.onSnapPicked(static_cast<long long>(
            Header::ticksFor(Header::snapAt(static_cast<std::size_t>(item.tag)))));
}

- (void)keyPicked:(NSMenuItem*)item
{
    if (self.onKeyPicked != nil)
        self.onKeyPicked(static_cast<int>(item.tag));
}

- (void)scalePicked:(NSMenuItem*)item
{
    if (self.onScalePicked != nil)
        self.onScalePicked(static_cast<int>(item.tag));
}

// ── State ────────────────────────────────────────────────────────────────────

- (void)setSnapTicks:(long long)ticks
{
    if (_snapTicks == ticks)
        return;

    _snapTicks = ticks;
    [self setNeedsDisplay:YES];
}

- (void)setKeyRootPitchClass:(int)pitchClass
{
    if (_keyRootPitchClass == pitchClass)
        return;

    _keyRootPitchClass = pitchClass;
    [self setNeedsDisplay:YES];
}

- (void)setScaleIndex:(int)index
{
    if (_scaleIndex == index)
        return;

    _scaleIndex = index;
    [self setNeedsDisplay:YES];
}

- (void)setGhostsVisible:(BOOL)visible
{
    if (_ghostsVisible == visible)
        return;

    _ghostsVisible = visible;
    [self setNeedsDisplay:YES];
}

- (void)setVelocityLaneVisible:(BOOL)visible
{
    if (_velocityLaneVisible == visible)
        return;

    _velocityLaneVisible = visible;
    [self setNeedsDisplay:YES];
}

@end
