#import "ui/macos/AudioEditorView.h"

#include "app/CommandRegistry.h"
#include "engine/audio/WaveformOverview.h"
#include "project/Model.h"
#include "ui/macos/Theme.h"

#include <algorithm>
#include <string>

using namespace incdaw;

@implementation INCDAWAudioEditorView {
    project::Project*     _project;
    app::CommandRegistry* _registry;

    engine::WaveformOverview _overview;
    BOOL                     _loaded;

    /// View window: leftmost visible frame and zoom, in frames per point.
    double    _framesPerPoint;
    long long _firstFrame;

    /// Selection in frames, half-open; equal means none. `_dragAnchor` is
    /// where the drag started, so dragging left of the anchor works.
    long long _selectionFrom;
    long long _selectionTo;
    long long _dragAnchor;

    /// The open file's markers, re-read on every reload. Cheap: the reader
    /// seeks past the audio rather than decoding it.
    std::vector<engine::AudioMarker> _markers;

    /// Where the last click landed. A marker dropped with no selection goes
    /// here rather than at zero.
    long long _caretFrame;
}

- (instancetype)initWithFrame:(NSRect)frame
                      project:(project::Project*)project
                     registry:(app::CommandRegistry*)registry
{
    if ((self = [super initWithFrame:frame])) {
        _project        = project;
        _registry       = registry;
        _framesPerPoint = 1.0;
    }
    return self;
}

- (BOOL)isFlipped { return YES; }
- (BOOL)acceptsFirstResponder { return YES; }

- (BOOL)hasSelection { return _loaded && _selectionTo > _selectionFrom; }
- (long long)caretFrame { return _caretFrame; }
- (const std::vector<engine::AudioMarker>&)markers { return _markers; }

- (long long)markerIndexNear:(long long)frame within:(long long)tolerance
{
    long long best         = -1;
    long long bestDistance = tolerance;

    for (std::size_t index = 0; index < _markers.size(); ++index) {
        const long long distance = std::llabs(_markers[index].start - frame);
        if (distance <= bestDistance) {
            bestDistance = distance;
            best         = static_cast<long long>(index);
        }
    }

    return best;
}
- (long long)selectionFrom { return _selectionFrom; }
- (long long)selectionTo { return _selectionTo; }

- (const project::AudioAsset*)asset
{
    for (const project::AudioAsset& asset : _project->audioAssets())
        if (asset.id.value() == _assetIdValue)
            return &asset;

    return nullptr;
}

- (void)reloadWaveform
{
    _loaded = NO;

    const project::AudioAsset* asset = [self asset];
    if (asset == nullptr) {
        [self setNeedsDisplay:YES];
        return;
    }

    const std::string& path = !asset->absolutePath.empty() ? asset->absolutePath
                                                           : asset->relativePath;

    // Markers before the overview: even a file whose waveform fails to build
    // may have readable metadata, and the two are independent.
    _markers.clear();
    (void)engine::WavFile::readMarkers(path, _markers);

    if (bool(engine::WaveformOverview::build(path, _overview))) {
        _loaded = YES;

        // Fit the whole file; clamp the stale selection to the new length
        // rather than discarding it — after an edit the user's selection is
        // usually exactly what they want to keep working on.
        _firstFrame     = 0;
        _framesPerPoint = std::max(1.0, static_cast<double>(_overview.frameCount)
                                            / std::max(1.0, self.bounds.size.width));
        _selectionFrom  = std::clamp<long long>(_selectionFrom, 0, _overview.frameCount);
        _selectionTo    = std::clamp<long long>(_selectionTo, 0, _overview.frameCount);
    }

    [self setNeedsDisplay:YES];
}

- (void)setAssetIdValue:(unsigned long long)value
{
    if (_assetIdValue != value) {
        _assetIdValue  = value;
        _selectionFrom = 0;
        _selectionTo   = 0;
    }
}

// ── Geometry ─────────────────────────────────────────────────────────────────

- (long long)frameAtX:(double)x
{
    return _firstFrame + static_cast<long long>(x * _framesPerPoint);
}

- (double)xForFrame:(long long)frame
{
    return static_cast<double>(frame - _firstFrame) / _framesPerPoint;
}

// ── Drawing ──────────────────────────────────────────────────────────────────

- (void)drawRect:(NSRect)dirty
{
    (void)dirty;

    namespace theme = incdaw::ui::theme;
    using theme::Ink;

    theme::fillRect(self.bounds, theme::ink(Ink::panel));

    if (!_loaded) {
        theme::drawTextCentred(@"Audio editor — double-click an audio clip in the Playlist to open it",
                               self.bounds, theme::ink(Ink::textDim), theme::labelFont(12.0),
                               theme::Align::centre);
        return;
    }

    // The waveform lives in a well, the way every other grid in the window does.
    const NSRect canvas = NSInsetRect(self.bounds, 8.0, 8.0);
    theme::drawWell(NSMakeRect(NSMinX(canvas), NSMinY(canvas) + 22.0, canvas.size.width,
                               std::max(CGFloat{0.0}, canvas.size.height - 22.0)),
                    theme::metrics::radiusControl, true);

    const double width      = self.bounds.size.width;
    const double laneHeight = self.bounds.size.height / static_cast<double>(_overview.channelCount);

    // Selection first, under the waveform.
    if (self.hasSelection) {
        const double left  = std::max(0.0, [self xForFrame:_selectionFrom]);
        const double right = std::min(width, [self xForFrame:_selectionTo]);

        if (right > left) {
            const NSRect selection = NSMakeRect(left, 0, right - left, self.bounds.size.height);
            theme::fillRect(selection, theme::ink(Ink::selectionFill));

            theme::fillRect(NSMakeRect(NSMinX(selection), 0, 1.0, selection.size.height),
                            theme::ink(Ink::selectionStroke));
            theme::fillRect(NSMakeRect(NSMaxX(selection) - 1.0, 0, 1.0, selection.size.height),
                            theme::ink(Ink::selectionStroke));
        }
    }

    NSColor* wave   = theme::ink(Ink::audio);
    NSColor* centre = theme::ink(Ink::gridLineStrong);

    for (std::size_t channel = 0; channel < _overview.channelCount; ++channel) {
        const double top    = static_cast<double>(channel) * laneHeight;
        const double middle = top + laneHeight / 2.0;
        const double scale  = laneHeight * 0.45;

        [centre setFill];
        NSRectFill(NSMakeRect(0, middle, width, 1.0));

        [wave setFill];

        // One min/max column per point: aggregate every bucket the column's
        // frame range touches.
        for (double x = 0.0; x < width; x += 1.0) {
            const long long from = [self frameAtX:x];
            const long long to   = [self frameAtX:x + 1.0];

            if (to <= 0 || from >= _overview.frameCount)
                continue;

            const auto firstBucket = static_cast<std::size_t>(
                std::max<long long>(0, from) / _overview.framesPerBucket);
            const auto lastBucket = static_cast<std::size_t>(
                std::max<long long>(0, to - 1) / _overview.framesPerBucket);

            float low = 0.0f, high = 0.0f;

            for (std::size_t bucket = firstBucket;
                 bucket <= lastBucket && bucket < _overview.bucketCount(); ++bucket) {
                low  = std::min(low, _overview.channels[channel][bucket].low);
                high = std::max(high, _overview.channels[channel][bucket].high);
            }

            const double columnTop    = middle - static_cast<double>(high) * scale;
            const double columnHeight = std::max(1.0, static_cast<double>(high - low) * scale);

            NSRectFill(NSMakeRect(x, columnTop, 1.0, columnHeight));
        }
    }

    // Markers, over the waveform. Regions are a band so the span reads at a
    // glance; points are a line with a flag, because a bare line at the top of
    // a busy waveform is indistinguishable from a transient.
    if (!_markers.empty()) {
        NSColor* markerInk = theme::ink(Ink::accent);
        const double laneTop = 22.0;

        for (const engine::AudioMarker& marker : _markers) {
            const double left = [self xForFrame:marker.start];

            if (marker.isRegion()) {
                const double right = [self xForFrame:marker.end()];
                if (right > 0.0 && left < width) {
                    const NSRect band = NSMakeRect(std::max(0.0, left), laneTop,
                                                   std::min(width, right) - std::max(0.0, left),
                                                   self.bounds.size.height - laneTop);
                    theme::fillRect(band, [markerInk colorWithAlphaComponent:0.10]);
                }
            }

            if (left < -1.0 || left > width)
                continue;

            [markerInk setFill];
            NSRectFill(NSMakeRect(left, laneTop, 1.0, self.bounds.size.height - laneTop));

            // The flag, and the name beside it. Drawn last so a dense cluster
            // of markers still reads.
            NSRectFill(NSMakeRect(left, laneTop, 7.0, 7.0));

            if (!marker.name.empty()) {
                const NSRect label = NSMakeRect(left + 10.0, laneTop - 1.0, 160.0, 12.0);
                theme::drawTextCentred(@(marker.name.c_str()), label, markerInk,
                                       theme::labelFont(9.0), theme::Align::left);
            }
        }
    }

    // The header line: what is open, how long, what is selected.
    const project::AudioAsset* asset = [self asset];
    if (asset != nullptr && _overview.sampleRate > 0.0) {
        const double seconds = static_cast<double>(_overview.frameCount) / _overview.sampleRate;

        NSString* info;
        if (self.hasSelection) {
            const double selectionSeconds =
                static_cast<double>(_selectionTo - _selectionFrom) / _overview.sampleRate;
            info = [NSString stringWithFormat:@"%s  ·  %.2fs  ·  selection %.2fs  (%lld..%lld)",
                    asset->absolutePath.c_str(), seconds, selectionSeconds,
                    _selectionFrom, _selectionTo];
        } else {
            info = [NSString stringWithFormat:@"%s  ·  %.2fs  ·  drag to select",
                    asset->absolutePath.c_str(), seconds];
        }

        const NSRect bar = NSMakeRect(0, 0, self.bounds.size.width, 22.0);
        theme::fillGradient(bar, 0.0, theme::ink(Ink::panelRaisedTop),
                            theme::ink(Ink::panelRaised), true);
        theme::drawSeparator(NSMakeRect(0, 21.0, self.bounds.size.width, 1.0));

        theme::drawTextCentred(info, NSInsetRect(bar, 10.0, 0.0), theme::ink(Ink::textSecondary),
                               theme::numericFont(10.0, NSFontWeightRegular));
    }
}

// ── Interaction ──────────────────────────────────────────────────────────────

- (void)mouseDown:(NSEvent*)event
{
    if (!_loaded)
        return;

    [self.window makeFirstResponder:self];

    if (event.clickCount == 2) {
        _selectionFrom = 0;
        _selectionTo   = _overview.frameCount;
        [self setNeedsDisplay:YES];
        return;
    }

    const NSPoint point = [self convertPoint:event.locationInWindow fromView:nil];
    _dragAnchor    = std::clamp<long long>([self frameAtX:point.x], 0, _overview.frameCount);
    _caretFrame    = _dragAnchor;
    _selectionFrom = _dragAnchor;
    _selectionTo   = _dragAnchor;
    [self setNeedsDisplay:YES];
}

- (void)mouseDragged:(NSEvent*)event
{
    if (!_loaded)
        return;

    const NSPoint point = [self convertPoint:event.locationInWindow fromView:nil];
    const long long frame =
        std::clamp<long long>([self frameAtX:point.x], 0, _overview.frameCount);

    _selectionFrom = std::min(_dragAnchor, frame);
    _selectionTo   = std::max(_dragAnchor, frame);
    [self setNeedsDisplay:YES];
}

- (void)scrollWheel:(NSEvent*)event
{
    if (!_loaded)
        return;

    if ((event.modifierFlags & NSEventModifierFlagCommand) != 0) {
        // Zoom around the cursor, so the frame under it stays put.
        const NSPoint point  = [self convertPoint:event.locationInWindow fromView:nil];
        const long long fixed = [self frameAtX:point.x];

        const double factor = event.scrollingDeltaY > 0 ? 1.0 / 1.15 : 1.15;
        _framesPerPoint = std::clamp(_framesPerPoint * factor, 0.05,
                                     static_cast<double>(std::max<long long>(1, _overview.frameCount))
                                         / std::max(1.0, self.bounds.size.width) * 4.0);

        _firstFrame = fixed - static_cast<long long>(point.x * _framesPerPoint);
    } else {
        _firstFrame -= static_cast<long long>(event.scrollingDeltaX * _framesPerPoint);
        _firstFrame -= static_cast<long long>(event.scrollingDeltaY * _framesPerPoint);
    }

    _firstFrame = std::clamp<long long>(
        _firstFrame,
        -static_cast<long long>(self.bounds.size.width * _framesPerPoint) / 4,
        _overview.frameCount);

    [self setNeedsDisplay:YES];
}

@end
