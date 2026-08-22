#include "ui/macos/Theme.h"

#include <algorithm>
#include <cmath>

namespace incdaw::ui::theme {
namespace {

NSColor* rgb(unsigned red, unsigned green, unsigned blue, CGFloat alpha = 1.0)
{
    return [NSColor colorWithSRGBRed:static_cast<CGFloat>(red) / 255.0
                               green:static_cast<CGFloat>(green) / 255.0
                                blue:static_cast<CGFloat>(blue) / 255.0
                               alpha:alpha];
}

/// Clamped 0..1, so callers may hand over raw meter or parameter values.
CGFloat unit(double value)
{
    return static_cast<CGFloat>(std::clamp(value, 0.0, 1.0));
}

NSColor* srgb(NSColor* colour)
{
    NSColor* converted = [colour colorUsingColorSpace:NSColorSpace.sRGBColorSpace];
    return converted != nil ? converted : colour;
}

/// The lit edge of a raised surface and the dark edge under it. Both are drawn
/// as hairlines rather than as gradients, which is what keeps the chrome from
/// looking plastic at small sizes.
void drawBevel(NSRect rect, CGFloat radius, bool flipped)
{
    NSBezierPath* path = roundedPath(NSInsetRect(rect, 0.5, 0.5), radius);
    path.lineWidth = 1.0;

    [ink(Ink::shadow) setStroke];
    [path stroke];

    const CGFloat topY = flipped ? NSMinY(rect) + 1.0 : NSMaxY(rect) - 1.0;

    NSBezierPath* lip = [NSBezierPath bezierPath];
    [lip moveToPoint:NSMakePoint(NSMinX(rect) + radius, topY)];
    [lip lineToPoint:NSMakePoint(NSMaxX(rect) - radius, topY)];
    lip.lineWidth = 1.0;

    [ink(Ink::highlight) setStroke];
    [lip stroke];
}

} // namespace

// ── Palette ──────────────────────────────────────────────────────────────────

NSColor* ink(Ink which)
{
    switch (which) {
        case Ink::windowBackground: return rgb(0x0F, 0x11, 0x15);
        case Ink::chromeTop:        return rgb(0x2C, 0x31, 0x3A);
        case Ink::chromeBottom:     return rgb(0x1B, 0x1F, 0x26);
        case Ink::panel:            return rgb(0x18, 0x1B, 0x21);
        case Ink::panelRaised:      return rgb(0x24, 0x29, 0x31);
        case Ink::panelRaisedTop:   return rgb(0x2E, 0x34, 0x3E);
        case Ink::panelSunken:      return rgb(0x0B, 0x0D, 0x11);
        case Ink::rowEven:          return rgb(0x1C, 0x20, 0x27);
        case Ink::rowOdd:           return rgb(0x18, 0x1C, 0x22);
        case Ink::rowSelected:      return rgb(0x2A, 0x31, 0x3C);
        case Ink::gridLine:         return rgb(0x22, 0x26, 0x2E);
        case Ink::gridLineStrong:   return rgb(0x33, 0x39, 0x44);
        case Ink::separator:        return rgb(0x07, 0x08, 0x0B);
        case Ink::highlight:        return [NSColor colorWithSRGBRed:1 green:1 blue:1 alpha:0.07];
        case Ink::shadow:           return [NSColor colorWithSRGBRed:0 green:0 blue:0 alpha:0.45];
        case Ink::textPrimary:      return rgb(0xE9, 0xEC, 0xF1);
        case Ink::textSecondary:    return rgb(0x99, 0xA2, 0xB0);
        case Ink::textDim:          return rgb(0x63, 0x6C, 0x7A);
        case Ink::textOnAccent:     return rgb(0x0B, 0x0D, 0x11);
        case Ink::accent:           return rgb(0x2E, 0x8F, 0xFF);
        case Ink::accentDim:        return rgb(0x1C, 0x53, 0x92);
        case Ink::record:           return rgb(0xFF, 0x45, 0x3A);
        case Ink::solo:             return rgb(0xFF, 0xD4, 0x26);
        case Ink::mute:             return rgb(0xFF, 0x6B, 0x5E);
        case Ink::midi:             return rgb(0x46, 0xD9, 0x7F);
        case Ink::audio:            return rgb(0x3A, 0xA9, 0xFF);
        case Ink::automation:       return rgb(0xC0, 0x7B, 0xFF);
        case Ink::playhead:         return rgb(0xFF, 0xC2, 0x4A);
        case Ink::lcdBackground:    return rgb(0x0A, 0x0E, 0x12);
        case Ink::lcdBezel:         return rgb(0x3A, 0x41, 0x4C);
        case Ink::lcdText:          return rgb(0xFF, 0xC9, 0x6B);
        case Ink::lcdTextDim:       return rgb(0x6D, 0x78, 0x86);
        case Ink::meterLow:         return rgb(0x35, 0xD0, 0x6E);
        case Ink::meterMid:         return rgb(0xFF, 0xD4, 0x26);
        case Ink::meterHigh:        return rgb(0xFF, 0x45, 0x3A);
        case Ink::selectionFill:    return [NSColor colorWithSRGBRed:0.18 green:0.56 blue:1.0 alpha:0.22];
        case Ink::selectionStroke:  return [NSColor colorWithSRGBRed:0.35 green:0.68 blue:1.0 alpha:0.85];
    }

    return rgb(0xFF, 0x00, 0xFF);   // unreachable; loud if it ever is not
}

NSColor* fromArgb(std::uint32_t argb, CGFloat brightness)
{
    const CGFloat red   = static_cast<CGFloat>((argb >> 16) & 0xFFu) / 255.0;
    const CGFloat green = static_cast<CGFloat>((argb >> 8) & 0xFFu) / 255.0;
    const CGFloat blue  = static_cast<CGFloat>(argb & 0xFFu) / 255.0;

    return [NSColor colorWithSRGBRed:std::min(red * brightness, CGFloat{1.0})
                               green:std::min(green * brightness, CGFloat{1.0})
                                blue:std::min(blue * brightness, CGFloat{1.0})
                               alpha:1.0];
}

NSColor* withAlpha(NSColor* colour, CGFloat alpha)
{
    return [colour colorWithAlphaComponent:alpha];
}

NSColor* lighten(NSColor* colour, CGFloat amount)
{
    return mix(colour, [NSColor whiteColor], amount);
}

NSColor* darken(NSColor* colour, CGFloat amount)
{
    return mix(colour, [NSColor blackColor], amount);
}

NSColor* mix(NSColor* a, NSColor* b, CGFloat t)
{
    NSColor* left  = srgb(a);
    NSColor* right = srgb(b);
    const CGFloat amount = std::clamp(t, CGFloat{0.0}, CGFloat{1.0});

    return [NSColor colorWithSRGBRed:left.redComponent * (1 - amount) + right.redComponent * amount
                               green:left.greenComponent * (1 - amount) + right.greenComponent * amount
                                blue:left.blueComponent * (1 - amount) + right.blueComponent * amount
                               alpha:left.alphaComponent * (1 - amount) + right.alphaComponent * amount];
}

NSColor* labelOn(NSColor* background)
{
    NSColor* colour = srgb(background);

    // Rec. 601 luma: close enough to perceived brightness for a two-way choice,
    // and it needs no colour-appearance model to explain.
    const CGFloat luma = 0.299 * colour.redComponent
                       + 0.587 * colour.greenComponent
                       + 0.114 * colour.blueComponent;

    return luma > 0.55 ? ink(Ink::textOnAccent) : ink(Ink::textPrimary);
}

// ── Type ─────────────────────────────────────────────────────────────────────

NSFont* labelFont(CGFloat size, NSFontWeight weight)
{
    return [NSFont systemFontOfSize:size weight:weight];
}

NSFont* numericFont(CGFloat size, NSFontWeight weight)
{
    return [NSFont monospacedDigitSystemFontOfSize:size weight:weight];
}

namespace {

NSDictionary* textAttributes(NSColor* colour, NSFont* font, Align align)
{
    NSMutableParagraphStyle* style = [[NSMutableParagraphStyle alloc] init];
    style.lineBreakMode = NSLineBreakByTruncatingTail;
    style.alignment     = align == Align::centre  ? NSTextAlignmentCenter
                        : align == Align::right   ? NSTextAlignmentRight
                                                  : NSTextAlignmentLeft;

    return @{NSFontAttributeName: font,
             NSForegroundColorAttributeName: colour,
             NSParagraphStyleAttributeName: style};
}

} // namespace

void drawText(NSString* text, NSRect rect, NSColor* colour, NSFont* font, Align align)
{
    if (text == nil || rect.size.width <= 0.0 || rect.size.height <= 0.0)
        return;

    [text drawInRect:rect withAttributes:textAttributes(colour, font, align)];
}

void drawTextCentred(NSString* text, NSRect rect, NSColor* colour, NSFont* font, Align align)
{
    if (text == nil || rect.size.width <= 0.0 || rect.size.height <= 0.0)
        return;

    NSDictionary* attributes = textAttributes(colour, font, align);
    const NSSize size = [text sizeWithAttributes:attributes];
    const CGFloat inset = std::max(CGFloat{0.0}, (rect.size.height - size.height) / 2.0);

    [text drawInRect:NSMakeRect(rect.origin.x, rect.origin.y + inset,
                                rect.size.width, rect.size.height - inset)
      withAttributes:attributes];
}

// ── Shape primitives ─────────────────────────────────────────────────────────

NSBezierPath* roundedPath(NSRect rect, CGFloat radius)
{
    const CGFloat limit = std::min(rect.size.width, rect.size.height) / 2.0;
    const CGFloat r     = std::max(CGFloat{0.0}, std::min(radius, limit));

    return [NSBezierPath bezierPathWithRoundedRect:rect xRadius:r yRadius:r];
}

void fillRect(NSRect rect, NSColor* colour)
{
    [colour setFill];
    NSRectFill(rect);
}

void fillRounded(NSRect rect, CGFloat radius, NSColor* colour)
{
    if (rect.size.width <= 0.0 || rect.size.height <= 0.0)
        return;

    [colour setFill];
    [roundedPath(rect, radius) fill];
}

void strokeRounded(NSRect rect, CGFloat radius, NSColor* colour, CGFloat width)
{
    if (rect.size.width <= 0.0 || rect.size.height <= 0.0)
        return;

    NSBezierPath* path = roundedPath(NSInsetRect(rect, width / 2.0, width / 2.0), radius);
    path.lineWidth = width;

    [colour setStroke];
    [path stroke];
}

void fillGradient(NSRect rect, CGFloat radius, NSColor* top, NSColor* bottom, bool flipped)
{
    if (rect.size.width <= 0.0 || rect.size.height <= 0.0)
        return;

    // The gradient runs along +y of the *current* space. In a flipped view +y
    // points down the screen, so the two stops swap to keep `top` at the edge
    // the user sees as the top.
    NSGradient* gradient = flipped
        ? [[NSGradient alloc] initWithStartingColor:top endingColor:bottom]
        : [[NSGradient alloc] initWithStartingColor:bottom endingColor:top];

    [gradient drawInBezierPath:roundedPath(rect, radius) angle:90.0];
}

// ── Composite surfaces ───────────────────────────────────────────────────────

void drawPanel(NSRect rect, CGFloat radius, bool selected, bool flipped)
{
    if (rect.size.width <= 0.0 || rect.size.height <= 0.0)
        return;

    NSColor* top    = selected ? mix(ink(Ink::panelRaisedTop), ink(Ink::accent), 0.22)
                               : ink(Ink::panelRaisedTop);
    NSColor* bottom = selected ? mix(ink(Ink::panelRaised), ink(Ink::accent), 0.14)
                               : ink(Ink::panelRaised);

    fillGradient(rect, radius, top, bottom, flipped);
    drawBevel(rect, radius, flipped);
}

void drawWell(NSRect rect, CGFloat radius, bool flipped)
{
    if (rect.size.width <= 0.0 || rect.size.height <= 0.0)
        return;

    fillRounded(rect, radius, ink(Ink::panelSunken));

    // An inner shadow along the top edge is what reads as "recessed"; the rest
    // of the well is left flat so content drawn in it stays honest.
    const CGFloat topY = flipped ? NSMinY(rect) + 0.5 : NSMaxY(rect) - 0.5;

    NSBezierPath* lip = [NSBezierPath bezierPath];
    [lip moveToPoint:NSMakePoint(NSMinX(rect) + radius, topY)];
    [lip lineToPoint:NSMakePoint(NSMaxX(rect) - radius, topY)];
    lip.lineWidth = 1.0;

    [[NSColor colorWithSRGBRed:0 green:0 blue:0 alpha:0.55] setStroke];
    [lip stroke];

    strokeRounded(rect, radius, [NSColor colorWithSRGBRed:1 green:1 blue:1 alpha:0.04]);
}

void drawSeparator(NSRect rect)
{
    fillRect(rect, ink(Ink::separator));
}

// ── Controls ─────────────────────────────────────────────────────────────────

void drawStepPad(NSRect rect, NSColor* colour, bool on, bool downbeat, bool underPlayhead,
                 bool flipped, double level)
{
    if (rect.size.width <= 0.0 || rect.size.height <= 0.0)
        return;

    if (on) {
        const CGFloat amount = unit(level);

        // A quiet step is a darker step. The floor is deliberately not black:
        // the pad must still read as lit and as this channel's colour at the
        // lowest velocity, or a soft step and a cleared one look alike.
        NSColor* lit = darken(colour, 0.5 * (1.0 - amount));

        // Lit pads carry the channel colour, brightest at the top edge, with a
        // ring that separates neighbouring lit steps.
        fillGradient(rect, metrics::radiusPad, lighten(lit, 0.32), darken(lit, 0.12), flipped);
        strokeRounded(rect, metrics::radiusPad, darken(colour, 0.45));

        const NSRect gloss = flipped
            ? NSMakeRect(NSMinX(rect) + 1.0, NSMinY(rect) + 1.0, rect.size.width - 2.0,
                         rect.size.height * 0.35)
            : NSMakeRect(NSMinX(rect) + 1.0, NSMaxY(rect) - 1.0 - rect.size.height * 0.35,
                         rect.size.width - 2.0, rect.size.height * 0.35);

        fillRounded(gloss, metrics::radiusPad - 1.0,
                    [NSColor colorWithSRGBRed:1 green:1 blue:1 alpha:0.16]);

        // The foot: how far up the pad the level has filled. Brightness alone
        // is hard to compare across two pads that are not side by side; a
        // measured edge is not.
        const CGFloat inset      = 2.0;
        const CGFloat footHeight = (rect.size.height - 2.0 * inset) * amount;

        if (footHeight >= 1.0 && rect.size.width > 2.0 * inset) {
            const NSRect foot = flipped
                ? NSMakeRect(NSMinX(rect) + inset, NSMaxY(rect) - inset - footHeight,
                             rect.size.width - 2.0 * inset, footHeight)
                : NSMakeRect(NSMinX(rect) + inset, NSMinY(rect) + inset,
                             rect.size.width - 2.0 * inset, footHeight);

            fillRounded(foot, metrics::radiusPad - 1.0,
                        [NSColor colorWithSRGBRed:1 green:1 blue:1 alpha:0.13]);
        }
    } else {
        NSColor* base = downbeat ? mix(ink(Ink::panelRaised), ink(Ink::panelSunken), 0.25)
                                 : mix(ink(Ink::panelSunken), ink(Ink::panelRaised), 0.35);

        fillRounded(rect, metrics::radiusPad, base);
        strokeRounded(rect, metrics::radiusPad,
                      [NSColor colorWithSRGBRed:1 green:1 blue:1 alpha:downbeat ? 0.06 : 0.03]);
    }

    if (underPlayhead)
        fillRounded(rect, metrics::radiusPad,
                    [NSColor colorWithSRGBRed:1 green:1 blue:1 alpha:0.20]);
}

NSRect drawRegion(NSRect rect, NSColor* colour, NSString* name, bool selected, bool muted,
                  bool flipped)
{
    if (rect.size.width <= 0.0 || rect.size.height <= 0.0)
        return NSZeroRect;

    NSColor* base = muted ? mix(colour, ink(Ink::panel), 0.62) : colour;

    fillGradient(rect, metrics::radiusControl, lighten(base, 0.24), darken(base, 0.30), flipped);

    // The name band: a darker strip along the top, the way a region in a
    // track-based arrangement labels itself without covering its content.
    const CGFloat bandHeight = std::min(CGFloat{14.0}, rect.size.height * 0.42);
    const NSRect band = flipped
        ? NSMakeRect(NSMinX(rect), NSMinY(rect), rect.size.width, bandHeight)
        : NSMakeRect(NSMinX(rect), NSMaxY(rect) - bandHeight, rect.size.width, bandHeight);

    if (bandHeight >= 9.0) {
        NSGraphicsContext* context = [NSGraphicsContext currentContext];
        [context saveGraphicsState];
        [roundedPath(rect, metrics::radiusControl) addClip];
        fillRect(band, [NSColor colorWithSRGBRed:0 green:0 blue:0 alpha:0.30]);
        [context restoreGraphicsState];

        if (name != nil)
            drawTextCentred(name, NSInsetRect(band, 5.0, 0.0),
                            muted ? withAlpha(labelOn(darken(base, 0.35)), 0.5)
                                  : labelOn(darken(base, 0.35)),
                            labelFont(10.0, NSFontWeightSemibold));
    }

    strokeRounded(rect, metrics::radiusControl,
                  selected ? ink(Ink::selectionStroke) : darken(base, 0.55),
                  selected ? 2.0 : 1.0);

    const CGFloat contentTop = flipped ? NSMinY(rect) + bandHeight : NSMinY(rect);
    return NSMakeRect(NSMinX(rect) + 2.0, contentTop + 1.0,
                      std::max(CGFloat{0.0}, rect.size.width - 4.0),
                      std::max(CGFloat{0.0}, rect.size.height - bandHeight - 2.0));
}

namespace {

/// Transport glyphs are geometry, not glyphs from a font: a triangle and a
/// square survive any scale factor, and INCDAW ships no icon assets.
void drawTriangle(NSPoint centre, CGFloat size, bool pointsRight, NSColor* colour)
{
    const CGFloat half = size / 2.0;
    const CGFloat sign = pointsRight ? 1.0 : -1.0;

    NSBezierPath* path = [NSBezierPath bezierPath];
    [path moveToPoint:NSMakePoint(centre.x + sign * half, centre.y)];
    [path lineToPoint:NSMakePoint(centre.x - sign * half, centre.y + half)];
    [path lineToPoint:NSMakePoint(centre.x - sign * half, centre.y - half)];
    [path closePath];

    [colour setFill];
    [path fill];
}

} // namespace

void drawTransportButton(NSRect rect, Transport kind, bool active, bool enabled)
{
    if (rect.size.width <= 0.0 || rect.size.height <= 0.0)
        return;

    const CGFloat radius = std::min(rect.size.width, rect.size.height) / 2.0;
    const NSPoint centre = NSMakePoint(NSMidX(rect), NSMidY(rect));

    NSColor* lit = ink(Ink::accent);
    if (kind == Transport::record)
        lit = ink(Ink::record);
    else if (kind == Transport::play)
        lit = ink(Ink::midi);
    else if (kind == Transport::loop || kind == Transport::metronome)
        lit = ink(Ink::solo);

    NSColor* top    = active ? lighten(lit, 0.30) : ink(Ink::panelRaisedTop);
    NSColor* bottom = active ? darken(lit, 0.18)  : ink(Ink::panelRaised);

    fillGradient(rect, radius, top, bottom, false);
    strokeRounded(rect, radius, active ? darken(lit, 0.55) : ink(Ink::shadow));

    NSColor* glyph = !enabled                 ? ink(Ink::textDim)
                   : active                   ? ink(Ink::textOnAccent)
                   : kind == Transport::record ? ink(Ink::record)
                                               : ink(Ink::textPrimary);

    const CGFloat size = std::min(rect.size.width, rect.size.height) * 0.42;

    switch (kind) {
        case Transport::play:
            drawTriangle(NSMakePoint(centre.x + size * 0.08, centre.y), size, true, glyph);
            break;

        case Transport::pause: {
            const CGFloat barWidth = size * 0.30;
            fillRounded(NSMakeRect(centre.x - size * 0.42, centre.y - size / 2.0,
                                   barWidth, size), 1.0, glyph);
            fillRounded(NSMakeRect(centre.x + size * 0.12, centre.y - size / 2.0,
                                   barWidth, size), 1.0, glyph);
            break;
        }

        case Transport::stop:
            fillRounded(NSMakeRect(centre.x - size / 2.0, centre.y - size / 2.0, size, size),
                        1.5, glyph);
            break;

        case Transport::record:
            fillRounded(NSMakeRect(centre.x - size / 2.0, centre.y - size / 2.0, size, size),
                        size / 2.0, glyph);
            break;

        case Transport::rewind:
            drawTriangle(NSMakePoint(centre.x - size * 0.22, centre.y), size * 0.8, false, glyph);
            drawTriangle(NSMakePoint(centre.x + size * 0.32, centre.y), size * 0.8, false, glyph);
            break;

        case Transport::forward:
            drawTriangle(NSMakePoint(centre.x + size * 0.22, centre.y), size * 0.8, true, glyph);
            drawTriangle(NSMakePoint(centre.x - size * 0.32, centre.y), size * 0.8, true, glyph);
            break;

        case Transport::loop: {
            NSBezierPath* arc = [NSBezierPath bezierPath];
            [arc appendBezierPathWithArcWithCenter:centre
                                            radius:size * 0.62
                                        startAngle:35.0
                                          endAngle:305.0];
            arc.lineWidth = std::max(CGFloat{1.5}, size * 0.20);
            [glyph setStroke];
            [arc stroke];

            drawTriangle(NSMakePoint(centre.x + size * 0.62, centre.y + size * 0.34),
                         size * 0.55, true, glyph);
            break;
        }

        case Transport::metronome: {
            NSBezierPath* body = [NSBezierPath bezierPath];
            [body moveToPoint:NSMakePoint(centre.x - size * 0.55, centre.y - size * 0.55)];
            [body lineToPoint:NSMakePoint(centre.x + size * 0.55, centre.y - size * 0.55)];
            [body lineToPoint:NSMakePoint(centre.x + size * 0.22, centre.y + size * 0.60)];
            [body lineToPoint:NSMakePoint(centre.x - size * 0.22, centre.y + size * 0.60)];
            [body closePath];
            body.lineWidth = std::max(CGFloat{1.2}, size * 0.16);
            [glyph setStroke];
            [body stroke];

            NSBezierPath* arm = [NSBezierPath bezierPath];
            [arm moveToPoint:NSMakePoint(centre.x - size * 0.30, centre.y - size * 0.45)];
            [arm lineToPoint:NSMakePoint(centre.x + size * 0.30, centre.y + size * 0.50)];
            arm.lineWidth = std::max(CGFloat{1.2}, size * 0.16);
            [arm stroke];
            break;
        }
    }
}

void drawKnob(NSRect rect, double normalised, NSColor* accent, bool bipolar)
{
    const CGFloat diameter = std::min(rect.size.width, rect.size.height);
    if (diameter <= 4.0)
        return;

    const NSRect square = NSMakeRect(NSMidX(rect) - diameter / 2.0,
                                     NSMidY(rect) - diameter / 2.0, diameter, diameter);
    const NSPoint centre = NSMakePoint(NSMidX(square), NSMidY(square));
    const CGFloat radius = diameter / 2.0;

    // The travel arc: 270° with the dead zone at the bottom, the way a rotary
    // that has to show its value at a glance is drawn.
    const CGFloat start = 225.0;
    const CGFloat sweep = -270.0;
    const CGFloat value = unit(normalised);

    NSBezierPath* track = [NSBezierPath bezierPath];
    [track appendBezierPathWithArcWithCenter:centre
                                      radius:radius - 1.5
                                  startAngle:start
                                    endAngle:start + sweep
                                   clockwise:YES];
    track.lineWidth = std::max(CGFloat{2.0}, diameter * 0.13);
    [ink(Ink::panelSunken) setStroke];
    [track stroke];

    const CGFloat from = bipolar ? start + sweep * 0.5 : start;
    const CGFloat to   = start + sweep * value;

    if (std::abs(to - from) > 0.5) {
        NSBezierPath* fill = [NSBezierPath bezierPath];
        [fill appendBezierPathWithArcWithCenter:centre
                                         radius:radius - 1.5
                                     startAngle:from
                                       endAngle:to
                                      clockwise:to < from];
        fill.lineWidth = std::max(CGFloat{2.0}, diameter * 0.13);
        [accent setStroke];
        [fill stroke];
    }

    // The cap.
    const NSRect cap = NSInsetRect(square, diameter * 0.24, diameter * 0.24);
    fillGradient(cap, cap.size.width / 2.0, ink(Ink::panelRaisedTop), ink(Ink::panelRaised), false);
    strokeRounded(cap, cap.size.width / 2.0, ink(Ink::shadow));

    // The pointer.
    const CGFloat angle = (start + sweep * value) * static_cast<CGFloat>(M_PI) / 180.0;
    NSBezierPath* pointer = [NSBezierPath bezierPath];
    [pointer moveToPoint:NSMakePoint(centre.x + std::cos(angle) * radius * 0.32,
                                     centre.y + std::sin(angle) * radius * 0.32)];
    [pointer lineToPoint:NSMakePoint(centre.x + std::cos(angle) * radius * 0.66,
                                     centre.y + std::sin(angle) * radius * 0.66)];
    pointer.lineWidth = std::max(CGFloat{1.0}, diameter * 0.09);
    [ink(Ink::textPrimary) setStroke];
    [pointer stroke];
}

void drawSlider(NSRect rect, double normalised, NSColor* accent, bool flipped)
{
    if (rect.size.width <= 2.0 || rect.size.height <= 2.0)
        return;

    const CGFloat trackHeight = std::min(rect.size.height, CGFloat{6.0});
    const NSRect track = NSMakeRect(NSMinX(rect), NSMidY(rect) - trackHeight / 2.0,
                                    rect.size.width, trackHeight);

    drawWell(track, trackHeight / 2.0, flipped);

    const CGFloat value = unit(normalised);
    const NSRect filled = NSMakeRect(NSMinX(track), NSMinY(track),
                                     track.size.width * value, track.size.height);

    if (filled.size.width > 1.0)
        fillGradient(filled, trackHeight / 2.0, lighten(accent, 0.25), accent, flipped);

    // The cap: a rounded handle, large enough to grab, small enough to leave
    // the value visible behind it.
    const CGFloat capWidth = 9.0;
    const CGFloat capX = std::clamp(NSMinX(rect) + rect.size.width * value - capWidth / 2.0,
                                    NSMinX(rect), NSMaxX(rect) - capWidth);
    const NSRect cap = NSMakeRect(capX, NSMidY(rect) - rect.size.height / 2.0 + 1.0,
                                  capWidth, rect.size.height - 2.0);

    fillGradient(cap, 3.0, ink(Ink::panelRaisedTop), ink(Ink::panelRaised), flipped);
    strokeRounded(cap, 3.0, ink(Ink::shadow));
}

void drawFader(NSRect rect, double normalised, NSColor* accent, bool selected, bool flipped)
{
    if (rect.size.width <= 4.0 || rect.size.height <= 8.0)
        return;

    const CGFloat trackWidth = 6.0;
    const NSRect track = NSMakeRect(NSMidX(rect) - trackWidth / 2.0, NSMinY(rect),
                                    trackWidth, rect.size.height);

    drawWell(track, trackWidth / 2.0, flipped);

    const CGFloat value  = unit(normalised);
    const CGFloat capHeight = 16.0;
    const CGFloat travel = rect.size.height - capHeight;
    const CGFloat capY   = NSMinY(rect) + travel * (flipped ? 1.0 - value : value);

    // The lit part of the track always runs from the quiet end to the cap.
    const NSRect filled = flipped
        ? NSMakeRect(NSMinX(track), capY + capHeight / 2.0, trackWidth,
                     NSMaxY(track) - capY - capHeight / 2.0)
        : NSMakeRect(NSMinX(track), NSMinY(track), trackWidth,
                     capY + capHeight / 2.0 - NSMinY(track));

    if (filled.size.height > 1.0)
        fillGradient(filled, trackWidth / 2.0, accent, darken(accent, 0.35), flipped);

    const NSRect cap = NSMakeRect(NSMinX(rect), capY, rect.size.width, capHeight);
    fillGradient(cap, 4.0, ink(Ink::panelRaisedTop), ink(Ink::panelRaised), flipped);
    strokeRounded(cap, 4.0, selected ? ink(Ink::accent) : ink(Ink::shadow));

    // A grip line across the cap, so the handle reads as a fader rather than
    // as a block sitting on a slot.
    NSBezierPath* grip = [NSBezierPath bezierPath];
    [grip moveToPoint:NSMakePoint(NSMinX(cap) + 3.0, NSMidY(cap))];
    [grip lineToPoint:NSMakePoint(NSMaxX(cap) - 3.0, NSMidY(cap))];
    grip.lineWidth = 1.0;
    [withAlpha(ink(Ink::textDim), 0.8) setStroke];
    [grip stroke];
}

void drawMeter(NSRect rect, double level, double peak, bool vertical, bool flipped)
{
    if (rect.size.width <= 0.0 || rect.size.height <= 0.0)
        return;

    drawWell(rect, 2.0, flipped);

    const CGFloat value = unit(level);
    if (value > 0.001) {
        const NSRect body = vertical
            ? NSMakeRect(NSMinX(rect) + 1.0,
                         flipped ? NSMaxY(rect) - 1.0 - (rect.size.height - 2.0) * value
                                 : NSMinY(rect) + 1.0,
                         rect.size.width - 2.0, (rect.size.height - 2.0) * value)
            : NSMakeRect(NSMinX(rect) + 1.0, NSMinY(rect) + 1.0,
                         (rect.size.width - 2.0) * value, rect.size.height - 2.0);

        static const CGFloat stops[] = {0.0, 0.62, 0.85, 1.0};

        NSGradient* gradient =
            [[NSGradient alloc] initWithColors:@[ink(Ink::meterLow), ink(Ink::meterLow),
                                                 ink(Ink::meterMid), ink(Ink::meterHigh)]
                                   atLocations:stops
                                    colorSpace:NSColorSpace.sRGBColorSpace];

        // The gradient spans the housing, not the bar: a quiet signal must show
        // green, not a scaled copy of the whole green-to-red ramp.
        NSGraphicsContext* context = [NSGraphicsContext currentContext];
        [context saveGraphicsState];
        [roundedPath(body, 1.5) addClip];
        [gradient drawInRect:rect angle:vertical ? (flipped ? -90.0 : 90.0) : 0.0];
        [context restoreGraphicsState];
    }

    const CGFloat held = unit(peak);
    if (held > 0.001) {
        const NSRect line = vertical
            ? NSMakeRect(NSMinX(rect) + 1.0,
                         flipped ? NSMaxY(rect) - 1.0 - (rect.size.height - 2.0) * held
                                 : NSMinY(rect) + (rect.size.height - 2.0) * held,
                         rect.size.width - 2.0, 1.5)
            : NSMakeRect(NSMinX(rect) + (rect.size.width - 2.0) * held, NSMinY(rect) + 1.0,
                         1.5, rect.size.height - 2.0);

        fillRect(line, held > 0.985 ? ink(Ink::meterHigh) : withAlpha(ink(Ink::textPrimary), 0.85));
    }
}

void drawToggle(NSRect rect, NSString* glyph, bool on, NSColor* onColour, bool flipped)
{
    if (rect.size.width <= 0.0 || rect.size.height <= 0.0)
        return;

    const CGFloat radius = std::min(metrics::radiusControl,
                                    std::min(rect.size.width, rect.size.height) / 2.0);

    if (on) {
        fillGradient(rect, radius, lighten(onColour, 0.28), onColour, flipped);
        strokeRounded(rect, radius, darken(onColour, 0.5));
    } else {
        fillGradient(rect, radius, ink(Ink::panelRaisedTop), ink(Ink::panelRaised), flipped);
        strokeRounded(rect, radius, ink(Ink::shadow));
    }

    if (glyph != nil)
        drawTextCentred(glyph, rect, on ? labelOn(onColour) : ink(Ink::textSecondary),
                        labelFont(std::min(CGFloat{11.0}, rect.size.height * 0.62),
                                  NSFontWeightBold),
                        Align::centre);
}

void drawLcd(NSRect rect)
{
    if (rect.size.width <= 0.0 || rect.size.height <= 0.0)
        return;

    fillRounded(rect, metrics::radiusPanel, ink(Ink::lcdBackground));

    // Two strokes: a dark seat under the readout and a lit bezel around it.
    strokeRounded(NSInsetRect(rect, -1.0, -1.0), metrics::radiusPanel + 1.0, ink(Ink::shadow));
    strokeRounded(rect, metrics::radiusPanel, withAlpha(ink(Ink::lcdBezel), 0.9));

    // A faint sheen across the upper half, which is what makes a flat dark
    // rectangle read as glass.
    const NSRect sheen = NSMakeRect(NSMinX(rect) + 2.0, NSMidY(rect),
                                    rect.size.width - 4.0, rect.size.height / 2.0 - 2.0);
    fillRounded(sheen, metrics::radiusPanel - 3.0,
                [NSColor colorWithSRGBRed:1 green:1 blue:1 alpha:0.025]);
}

void drawTab(NSRect rect, NSString* title, bool selected, bool first, bool last)
{
    if (rect.size.width <= 0.0 || rect.size.height <= 0.0)
        return;

    const CGFloat radius = metrics::radiusControl;

    // Only the outer corners of a segmented run are rounded; the joins are not.
    NSGraphicsContext* context = [NSGraphicsContext currentContext];
    [context saveGraphicsState];

    NSRect clip = rect;
    if (!first)
        clip = NSMakeRect(NSMinX(rect) - radius, NSMinY(rect), rect.size.width + radius,
                          rect.size.height);
    if (!last)
        clip.size.width += radius;

    [NSBezierPath clipRect:rect];

    if (selected) {
        fillGradient(clip, radius, lighten(ink(Ink::accent), 0.18), ink(Ink::accent), false);
        strokeRounded(clip, radius, darken(ink(Ink::accent), 0.5));
    } else {
        fillGradient(clip, radius, ink(Ink::panelRaisedTop), ink(Ink::panelRaised), false);
        strokeRounded(clip, radius, ink(Ink::shadow));
    }

    [context restoreGraphicsState];

    drawTextCentred(title, rect, selected ? ink(Ink::textOnAccent) : ink(Ink::textSecondary),
                    labelFont(11.0, selected ? NSFontWeightSemibold : NSFontWeightMedium),
                    Align::centre);
}

void drawPlayhead(CGFloat x, NSRect bounds, CGFloat headY, bool flipped)
{
    fillRect(NSMakeRect(x - 0.5, NSMinY(bounds), 1.5, bounds.size.height), ink(Ink::playhead));

    // The head: a small triangle pointing into the content, so the line has a
    // grabbable end rather than running off the edge of the ruler.
    const CGFloat size = 5.0;
    const CGFloat sign = flipped ? 1.0 : -1.0;

    NSBezierPath* head = [NSBezierPath bezierPath];
    [head moveToPoint:NSMakePoint(x - size, headY)];
    [head lineToPoint:NSMakePoint(x + size, headY)];
    [head lineToPoint:NSMakePoint(x, headY + sign * size * 1.4)];
    [head closePath];

    [ink(Ink::playhead) setFill];
    [head fill];
}

} // namespace incdaw::ui::theme
