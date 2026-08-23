#import "ui/macos/CompingView.h"

#include "app/CommandRegistry.h"
#include "app/commands/RecordingCommands.h"
#include "engine/audio/WaveformOverview.h"
#include "project/Model.h"
#include "ui/macos/Theme.h"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

using namespace incdaw;

namespace {

constexpr CGFloat headerHeight = 24.0;
constexpr CGFloat laneGap      = 4.0;
constexpr CGFloat minLane      = 44.0;

} // namespace

@implementation INCDAWCompingView {
    project::Project*     _project;
    app::CommandRegistry* _registry;

    std::vector<app::comping::Take> _takes;

    /// One overview per distinct source asset in the stack. Loop recording
    /// gives every pass the same file, so this is usually one entry doing the
    /// work of every lane.
    std::vector<std::pair<unsigned long long, engine::WaveformOverview>> _overviews;

    /// The drag in progress: which lane, and the frames it has covered.
    NSInteger _dragLane;
    long long _dragFrom;
    long long _dragTo;
    BOOL      _dragging;
}

- (instancetype)initWithFrame:(NSRect)frame
                      project:(project::Project*)project
                     registry:(app::CommandRegistry*)registry
{
    if ((self = [super initWithFrame:frame])) {
        _project  = project;
        _registry = registry;
        _dragLane = -1;
    }
    return self;
}

- (BOOL)isFlipped { return YES; }
- (BOOL)acceptsFirstResponder { return YES; }

- (NSInteger)laneCount { return static_cast<NSInteger>(_takes.size()); }

- (void)setTrackIdValue:(unsigned long long)value
{
    if (_trackIdValue != value) {
        _trackIdValue = value;
        [self reload];
    }
}

- (void)setSpanFrom:(long long)value
{
    if (_spanFrom != value) {
        _spanFrom = value;
        [self reload];
    }
}

- (void)setSpanTo:(long long)value
{
    if (_spanTo != value) {
        _spanTo = value;
        [self reload];
    }
}

- (void)reload
{
    _takes.clear();

    if (_project != nullptr && _trackIdValue != 0 && _spanTo > _spanFrom) {
        _takes = app::comping::takesOver(*_project, project::EntityId{_trackIdValue},
                                         _spanFrom, _spanTo);
    }

    // One overview per distinct source. Built lazily and kept, because a comp
    // is a great many redraws over the same few files.
    for (const app::comping::Take& take : _takes) {
        const unsigned long long source = take.source.value();

        bool have = false;
        for (const auto& entry : _overviews)
            if (entry.first == source)
                have = true;

        if (have)
            continue;

        for (const project::AudioAsset& asset : _project->audioAssets()) {
            if (asset.id != take.source)
                continue;

            const std::string& path =
                !asset.absolutePath.empty() ? asset.absolutePath : asset.relativePath;

            engine::WaveformOverview overview;
            if (bool(engine::WaveformOverview::build(path, overview)))
                _overviews.emplace_back(source, std::move(overview));
        }
    }

    [self setNeedsDisplay:YES];
}

// ── Geometry ─────────────────────────────────────────────────────────────────

- (double)framesPerPoint
{
    const double width = std::max(1.0, self.bounds.size.width - 16.0);
    return static_cast<double>(std::max<long long>(1, _spanTo - _spanFrom)) / width;
}

- (long long)frameAtX:(double)x
{
    return _spanFrom + static_cast<long long>((x - 8.0) * [self framesPerPoint]);
}

- (double)xForFrame:(long long)frame
{
    return 8.0 + static_cast<double>(frame - _spanFrom) / [self framesPerPoint];
}

- (CGFloat)laneHeight
{
    if (_takes.empty())
        return minLane;

    const CGFloat available = self.bounds.size.height - headerHeight
                            - laneGap * static_cast<CGFloat>(_takes.size() + 1);

    return std::max(minLane, available / static_cast<CGFloat>(_takes.size()));
}

- (NSRect)rectForLane:(std::size_t)lane
{
    const CGFloat height = [self laneHeight];
    const CGFloat top    = headerHeight + laneGap
                         + static_cast<CGFloat>(lane) * (height + laneGap);

    return NSMakeRect(8.0, top, std::max(CGFloat{1.0}, self.bounds.size.width - 16.0), height);
}

- (NSInteger)laneAtY:(CGFloat)y
{
    for (std::size_t lane = 0; lane < _takes.size(); ++lane) {
        const NSRect rect = [self rectForLane:lane];
        if (y >= NSMinY(rect) && y < NSMaxY(rect))
            return static_cast<NSInteger>(lane);
    }

    return -1;
}

// ── Drawing ──────────────────────────────────────────────────────────────────

/// The clips of one take that are audible, as timeline spans.
- (std::vector<std::pair<long long, long long>>)audibleSpansOfTake:(const app::comping::Take&)take
{
    std::vector<std::pair<long long, long long>> spans;

    for (const project::Clip& clip : _project->clips()) {
        if (clip.track.value() != _trackIdValue || clip.type != project::ClipType::audio)
            continue;
        if (clip.muted || clip.source != take.source)
            continue;

        const engine::FrameCount anchor =
            clip.sourceOffset - static_cast<engine::FrameCount>(clip.start);

        if (anchor != take.anchor)
            continue;

        spans.emplace_back(clip.start, clip.start + clip.length);
    }

    return spans;
}

- (void)drawWaveformOfTake:(const app::comping::Take&)take inRect:(NSRect)rect
{
    const engine::WaveformOverview* overview = nullptr;
    for (const auto& entry : _overviews)
        if (entry.first == take.source.value())
            overview = &entry.second;

    if (overview == nullptr || overview->channelCount == 0 || overview->framesPerBucket <= 0)
        return;

    const double middle = NSMinY(rect) + rect.size.height / 2.0;
    const double scale  = rect.size.height * 0.42;

    for (double x = NSMinX(rect); x < NSMaxX(rect); x += 1.0) {
        // The lane shows the SOURCE window this take maps onto the span, which
        // is what makes three passes of the same file look different.
        const long long timelineFrame = [self frameAtX:x];
        const long long sourceFrame   = timelineFrame + take.anchor;

        if (sourceFrame < 0 || sourceFrame >= overview->frameCount)
            continue;

        const auto bucket = static_cast<std::size_t>(sourceFrame / overview->framesPerBucket);
        if (bucket >= overview->bucketCount())
            continue;

        float low = 0.0f, high = 0.0f;
        for (std::size_t channel = 0; channel < overview->channelCount; ++channel) {
            low  = std::min(low, overview->channels[channel][bucket].low);
            high = std::max(high, overview->channels[channel][bucket].high);
        }

        const double top    = middle - static_cast<double>(high) * scale;
        const double height = std::max(1.0, static_cast<double>(high - low) * scale);

        NSRectFill(NSMakeRect(x, top, 1.0, height));
    }
}

- (void)drawRect:(NSRect)dirty
{
    (void)dirty;

    namespace theme = incdaw::ui::theme;
    using theme::Ink;

    theme::fillRect(self.bounds, theme::ink(Ink::panel));

    if (_takes.empty()) {
        theme::drawTextCentred(
            @"No stacked takes here — record over a loop, then comp the passes.",
            self.bounds, theme::ink(Ink::textDim), theme::labelFont(12.0), theme::Align::centre);
        return;
    }

    const NSRect header = NSMakeRect(0, 0, self.bounds.size.width, headerHeight);
    theme::fillGradient(header, 0.0, theme::ink(Ink::panelRaisedTop), theme::ink(Ink::panelRaised),
                        true);
    theme::drawSeparator(NSMakeRect(0, headerHeight - 1.0, self.bounds.size.width, 1.0));

    theme::drawTextCentred(
        [NSString stringWithFormat:@"%lu takes  ·  drag across a lane to use it there",
                                   static_cast<unsigned long>(_takes.size())],
        NSInsetRect(header, 10.0, 0.0), theme::ink(Ink::textSecondary),
        theme::numericFont(10.0, NSFontWeightRegular));

    for (std::size_t lane = 0; lane < _takes.size(); ++lane) {
        const app::comping::Take& take = _takes[lane];
        const NSRect              rect = [self rectForLane:lane];

        theme::drawWell(rect, theme::metrics::radiusControl, true);

        // The audible parts first, as a wash: what this lane contributes to
        // the comp is the only thing on screen that is not decoration.
        for (const auto& [from, to] : [self audibleSpansOfTake:take]) {
            const double left  = std::max(NSMinX(rect), [self xForFrame:from]);
            const double right = std::min(NSMaxX(rect), [self xForFrame:to]);

            if (right > left)
                theme::fillRect(NSMakeRect(left, NSMinY(rect), right - left, rect.size.height),
                                [theme::ink(Ink::accent) colorWithAlphaComponent:0.22]);
        }

        [[theme::ink(Ink::audio) colorWithAlphaComponent:0.85] setFill];
        [self drawWaveformOfTake:take inRect:rect];

        theme::drawTextCentred([NSString stringWithFormat:@"Take %lu",
                                                          static_cast<unsigned long>(lane + 1)],
                               NSMakeRect(NSMinX(rect) + 6.0, NSMinY(rect) + 2.0, 90.0, 14.0),
                               theme::ink(Ink::textDim), theme::labelFont(9.0));
    }

    // The drag, drawn over everything so the range being chosen is legible
    // against whatever it is being chosen from.
    if (_dragging && _dragLane >= 0) {
        const NSRect rect  = [self rectForLane:static_cast<std::size_t>(_dragLane)];
        const double left  = [self xForFrame:std::min(_dragFrom, _dragTo)];
        const double right = [self xForFrame:std::max(_dragFrom, _dragTo)];

        const NSRect band = NSMakeRect(std::max(NSMinX(rect), left), NSMinY(rect),
                                       std::max(1.0, std::min(NSMaxX(rect), right)
                                                         - std::max(NSMinX(rect), left)),
                                       rect.size.height);

        theme::fillRect(band, theme::ink(Ink::selectionFill));
        theme::strokeRounded(band, 2.0, theme::ink(Ink::selectionStroke));
    }
}

// ── Interaction ──────────────────────────────────────────────────────────────

- (void)mouseDown:(NSEvent*)event
{
    [self.window makeFirstResponder:self];

    const NSPoint point = [self convertPoint:event.locationInWindow fromView:nil];

    _dragLane = [self laneAtY:point.y];
    if (_dragLane < 0)
        return;

    _dragFrom = std::clamp<long long>([self frameAtX:point.x], _spanFrom, _spanTo);
    _dragTo   = _dragFrom;
    _dragging = YES;

    [self setNeedsDisplay:YES];
}

- (void)mouseDragged:(NSEvent*)event
{
    if (!_dragging)
        return;

    const NSPoint point = [self convertPoint:event.locationInWindow fromView:nil];
    _dragTo = std::clamp<long long>([self frameAtX:point.x], _spanFrom, _spanTo);

    [self setNeedsDisplay:YES];
}

- (void)mouseUp:(NSEvent*)event
{
    (void)event;

    if (!_dragging)
        return;

    _dragging = NO;

    const long long from = std::min(_dragFrom, _dragTo);
    const long long to   = std::max(_dragFrom, _dragTo);

    // A click is not a drag. Assigning a zero-width range would be an undo
    // entry for having touched the window.
    if (to <= from || _dragLane < 0) {
        [self setNeedsDisplay:YES];
        return;
    }

    const bool changed = _registry->execute(std::make_unique<app::AssignCompRangeCommand>(
        project::EntityId{_trackIdValue}, from, to, static_cast<std::size_t>(_dragLane)));

    [self reload];

    // The composite is what plays: the host rebuilds the graph, and the next
    // pass over the span is the comp rather than the pile.
    if (changed && self.onCompChanged != nil)
        self.onCompChanged();
}

@end
