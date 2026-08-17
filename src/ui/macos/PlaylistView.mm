#include "ui/macos/PlaylistView.h"

#include "app/CommandRegistry.h"
#include "app/PlaylistModel.h"
#include "app/commands/ClipCommands.h"
#include "app/commands/TrackCommands.h"
#include "engine/audio/WaveformOverview.h"
#include "project/PatternCompiler.h"
#include "project/Model.h"
#include "ui/macos/Theme.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

using namespace incdaw;
using incdaw::engine::Tick;
using incdaw::engine::ticksPerQuarterNote;

namespace theme = incdaw::ui::theme;

namespace {

using theme::Ink;

/// Track headers down the left edge, and the ruler across the top. Both are
/// pinned: the grid scrolls under them.
constexpr CGFloat headerWidth = 176.0;
constexpr CGFloat rulerHeight = theme::metrics::rulerHeight;

/// The row below the last track, which creates one.
constexpr CGFloat addRowHeight = 28.0;

constexpr CGFloat buttonWidth = 20.0;
constexpr CGFloat padding     = 7.0;

using theme::fillRect;

enum class PlaylistDrag { none, move, resize, boxSelect };

} // namespace

@implementation INCDAWPlaylistView {
    project::Project*     _project;
    app::CommandRegistry* _registry;

    std::unique_ptr<app::PlaylistModel> _model;

    std::vector<app::PlaylistModel::VisibleClip> _visible;

    /// Waveform overviews per asset, built lazily on first draw. Invalidated
    /// by the host after any edit that may have rewritten an asset's file —
    /// the view cannot see a file change, only the host knows one happened.
    std::unordered_map<unsigned long long, engine::WaveformOverview> _waveforms;
    std::vector<project::EntityId>               _boxed;

    PlaylistDrag _drag;
    NSPoint      _dragOrigin;
    NSPoint      _dragCurrent;
    Tick         _dragAppliedTicks;
    int          _dragAppliedTracks;
    Tick         _dragAppliedLength;
}

- (instancetype)initWithFrame:(NSRect)frame
                      project:(project::Project*)project
                     registry:(app::CommandRegistry*)registry
{
    self = [super initWithFrame:frame];
    if (self == nil)
        return nil;

    _project  = project;
    _registry = registry;
    _model    = std::make_unique<app::PlaylistModel>();

    _drag              = PlaylistDrag::none;
    _dragAppliedTicks  = 0;
    _dragAppliedTracks = 0;
    _dragAppliedLength = 0;
    _playheadTick      = -1;

    app::PlaylistModel::Viewport viewport;
    viewport.firstTick    = 0;
    viewport.visibleTicks = ticksPerQuarterNote * 4 * 16;   // sixteen bars
    viewport.width        = frame.size.width - headerWidth;
    viewport.height       = frame.size.height - rulerHeight;
    _model->setViewport(viewport);
    _model->setSnap(ticksPerQuarterNote * 4);               // one bar

    return self;
}

- (BOOL)isFlipped { return YES; }
- (BOOL)acceptsFirstResponder { return YES; }

- (void)setFrameSize:(NSSize)size
{
    [super setFrameSize:size];

    auto viewport = _model->viewport();
    viewport.width  = size.width - headerWidth;
    viewport.height = size.height - rulerHeight;
    _model->setViewport(viewport);

    [self setNeedsDisplay:YES];
}

/// Point in the grid's own coordinates: the ruler and the track headers are
/// pinned, so everything else works relative to the corner where they meet.
- (NSPoint)gridPointFor:(NSEvent*)event
{
    const NSPoint point = [self convertPoint:event.locationInWindow fromView:nil];
    return NSMakePoint(point.x - headerWidth, point.y - rulerHeight);
}

// ── Drawing ──────────────────────────────────────────────────────────────────

- (void)drawRect:(NSRect)dirtyRect
{
    (void)dirtyRect;

    fillRect(self.bounds, theme::ink(Ink::panel));

    if (_project == nullptr)
        return;

    [self drawRuler];
    [self drawTracks];
    [self drawClips];
    [self drawPlayhead];

    if (_drag == PlaylistDrag::boxSelect) {
        const NSRect box = NSMakeRect(std::min(_dragOrigin.x, _dragCurrent.x) + headerWidth,
                                      std::min(_dragOrigin.y, _dragCurrent.y) + rulerHeight,
                                      std::abs(_dragCurrent.x - _dragOrigin.x),
                                      std::abs(_dragCurrent.y - _dragOrigin.y));

        theme::fillRounded(box, 3.0, theme::ink(Ink::selectionFill));
        theme::strokeRounded(box, 3.0, theme::ink(Ink::selectionStroke));
    }
}

- (void)drawRuler
{
    const NSRect ruler = NSMakeRect(0, 0, self.bounds.size.width, rulerHeight);

    theme::fillGradient(ruler, 0.0, theme::ink(Ink::panelRaisedTop),
                        theme::ink(Ink::panelRaised), true);
    theme::drawSeparator(NSMakeRect(0, rulerHeight - 1.0, self.bounds.size.width, 1.0));

    const auto& viewport = _model->viewport();
    const Tick barTicks  = ticksPerQuarterNote * 4;

    const Tick first = (viewport.firstTick / barTicks) * barTicks;
    const Tick last  = viewport.firstTick + viewport.visibleTicks;

    for (Tick tick = first; tick <= last; tick += barTicks) {
        const CGFloat x = headerWidth + _model->tickToX(tick);
        if (x < headerWidth)
            continue;

        const bool fourBar = (tick / barTicks) % 4 == 0;

        // Every fourth bar gets a full-height tick and the rest a stub, so the
        // ruler can be counted without reading each number.
        fillRect(NSMakeRect(x, fourBar ? 4.0 : rulerHeight - 8.0, 1.0,
                            fourBar ? rulerHeight - 5.0 : 7.0),
                 fourBar ? theme::ink(Ink::textDim) : theme::ink(Ink::gridLineStrong));

        if (fourBar || _model->pointsPerTick() * static_cast<double>(barTicks) > 42.0)
            theme::drawText([NSString stringWithFormat:@"%lld",
                                                       static_cast<long long>(tick / barTicks) + 1],
                            NSMakeRect(x + 4.0, 5.0, 40.0, 13.0),
                            fourBar ? theme::ink(Ink::textSecondary) : theme::ink(Ink::textDim),
                            theme::numericFont(9.5, NSFontWeightSemibold));
    }
}

- (void)drawTracks
{
    const std::vector<project::Track>& tracks = _project->tracks();

    for (std::size_t row = 0; row < tracks.size(); ++row) {
        const project::Track& track = tracks[row];

        const CGFloat y      = rulerHeight + _model->trackY(tracks, row);
        const CGFloat height = app::PlaylistModel::trackHeight(track);

        if (y + height < rulerHeight || y > self.bounds.size.height)
            continue;

        NSColor* colour = theme::fromArgb(track.colour);

        // The lane behind the clips, so an empty track is still a place rather
        // than a gap.
        fillRect(NSMakeRect(headerWidth, y, self.bounds.size.width - headerWidth, height - 1.0),
                 (row % 2) == 0 ? theme::ink(Ink::rowEven) : theme::ink(Ink::rowOdd));

        [self drawBarLinesInLaneAt:y height:height - 1.0];

        // ── The track's header ───────────────────────────────────────────────
        const NSRect header = NSMakeRect(2.0, y + 1.0, headerWidth - 5.0, height - 4.0);
        theme::drawPanel(header, theme::metrics::radiusControl, false, true);

        theme::fillGradient(NSMakeRect(NSMinX(header) + 5.0, NSMinY(header) + 5.0, 8.0,
                                       header.size.height - 10.0),
                            2.5, theme::lighten(colour, 0.25),
                            theme::darken(colour, track.muted ? 0.65 : 0.15), true);

        theme::drawText(@(track.name.c_str()),
                        NSMakeRect(NSMinX(header) + 20.0, NSMinY(header) + 6.0,
                                   header.size.width - 2.0 * buttonWidth - 3.0 * padding, 16.0),
                        track.muted ? theme::ink(Ink::textDim) : theme::ink(Ink::textPrimary),
                        theme::labelFont(12.0, NSFontWeightMedium));

        theme::drawToggle([self muteRectForRow:row], @"M", track.muted, theme::ink(Ink::mute), true);
        theme::drawToggle([self soloRectForRow:row], @"S", track.soloed, theme::ink(Ink::solo), true);
    }

    const NSRect addRow = [self addTrackRect];
    if (addRow.origin.y < self.bounds.size.height) {
        const NSRect rect = NSMakeRect(2.0, addRow.origin.y + 2.0, headerWidth - 5.0,
                                       addRow.size.height - 6.0);

        theme::strokeRounded(rect, theme::metrics::radiusControl,
                             theme::withAlpha(theme::ink(Ink::textDim), 0.35));

        theme::drawTextCentred(@"＋  Add track",
                               NSMakeRect(NSMinX(rect) + 10.0, NSMinY(rect), rect.size.width,
                                          rect.size.height),
                               theme::ink(Ink::textSecondary), theme::labelFont(12.0));
    }
}

- (void)drawBarLinesInLaneAt:(CGFloat)y height:(CGFloat)height
{
    const auto& viewport = _model->viewport();
    const Tick  barTicks = ticksPerQuarterNote * 4;

    const Tick first = (viewport.firstTick / barTicks) * barTicks;
    const Tick last  = viewport.firstTick + viewport.visibleTicks;

    for (Tick tick = first; tick <= last; tick += barTicks) {
        const CGFloat x = headerWidth + _model->tickToX(tick);
        if (x < headerWidth)
            continue;

        const bool fourBar = (tick / barTicks) % 4 == 0;
        fillRect(NSMakeRect(x, y, 1.0, height),
                 fourBar ? theme::ink(Ink::gridLineStrong) : theme::ink(Ink::gridLine));
    }
}

- (void)drawClips
{
    _model->collectVisibleClips(*_project, _visible);

    for (const auto& clip : _visible) {
        const NSRect rect = NSMakeRect(clip.rect.x + headerWidth, clip.rect.y + rulerHeight + 1.0,
                                       std::max(2.0, clip.rect.width),
                                       std::max(2.0, clip.rect.height - 3.0));

        const project::Clip& model = _project->clips()[clip.index];

        // One region shape for every kind of clip; what is drawn inside it is
        // what says whether it holds notes, audio or an automation ride.
        const NSRect body = theme::drawRegion(rect, theme::fromArgb(clip.colour),
                                              @(model.name.c_str()), clip.selected, clip.muted,
                                              true);

        if (model.type == project::ClipType::audio)
            [self drawWaveformFor:model inBody:body];
        else if (model.type == project::ClipType::automation)
            [self drawAutomationCurveFor:model inBody:body];
        else
            [self drawPatternPreviewFor:model inBody:body];
    }
}

- (const engine::WaveformOverview*)overviewForAsset:(unsigned long long)assetId
{
    if (const auto found = _waveforms.find(assetId); found != _waveforms.end())
        return &found->second;

    // A failed build inserts the empty overview too, so a missing file costs
    // one attempt rather than one per frame.
    engine::WaveformOverview& slot = _waveforms[assetId];

    for (const project::AudioAsset& asset : _project->audioAssets()) {
        if (asset.id.value() != assetId)
            continue;

        const std::string& path = !asset.absolutePath.empty() ? asset.absolutePath
                                                              : asset.relativePath;
        (void)engine::WaveformOverview::build(path, slot);
        break;
    }

    return &slot;
}

- (void)drawWaveformFor:(const project::Clip&)model inBody:(NSRect)body
{
    const engine::WaveformOverview* overview = [self overviewForAsset:model.source.value()];
    if (overview->bucketCount() == 0 || overview->channelCount == 0 || model.length <= 0)
        return;

    // Folded to mono — a clip body is a reminder of what the audio looks like,
    // not the editor.
    if (body.size.height < 6.0 || body.size.width < 2.0)
        return;

    const double middle = NSMidY(body);
    const double scale  = body.size.height * 0.45;
    const double framesPerPoint = static_cast<double>(model.length) / body.size.width;

    // Light on the region's own colour, the way a waveform reads inside a
    // coloured block rather than as a hole cut through it.
    [[NSColor colorWithSRGBRed:1 green:1 blue:1 alpha:0.62] setFill];

    for (double x = 0.0; x < body.size.width; x += 1.0) {
        const auto from = model.sourceOffset
                        + static_cast<engine::FrameCount>(x * framesPerPoint);
        const auto to   = model.sourceOffset
                        + static_cast<engine::FrameCount>((x + 1.0) * framesPerPoint);

        if (from >= overview->frameCount)
            break;   // the clip extends past the audio: the rest is silence

        const auto firstBucket = static_cast<std::size_t>(from / overview->framesPerBucket);
        const auto lastBucket  = static_cast<std::size_t>(
            std::max<engine::FrameCount>(from, to - 1) / overview->framesPerBucket);

        float low = 0.0f, high = 0.0f;

        for (std::size_t channel = 0; channel < overview->channelCount; ++channel)
            for (std::size_t bucket = firstBucket;
                 bucket <= lastBucket && bucket < overview->bucketCount(); ++bucket) {
                low  = std::min(low, overview->channels[channel][bucket].low);
                high = std::max(high, overview->channels[channel][bucket].high);
            }

        NSRectFillUsingOperation(
            NSMakeRect(body.origin.x + x, middle - static_cast<double>(high) * scale, 1.0,
                       std::max(1.0, static_cast<double>(high - low) * scale)),
            NSCompositingOperationSourceOver);
    }
}

- (void)invalidateWaveformCache
{
    _waveforms.clear();
    [self setNeedsDisplay:YES];
}

/// The lane's envelope across the clip's window, as a thumbnail polyline.
/// Linear between points — curve shapes are an editing-surface concern; the
/// clip body only has to say "this is the shape of the ride".
- (void)drawAutomationCurveFor:(const project::Clip&)model inBody:(NSRect)body
{
    const project::AutomationLane* lane = nullptr;
    for (const project::AutomationLane& candidate : _project->automation())
        if (candidate.id == model.source)
            lane = &candidate;

    if (lane == nullptr || lane->points.empty() || model.lengthTicks <= 0)
        return;

    if (body.size.height < 6.0 || body.size.width < 2.0)
        return;

    // Lane ticks are absolute; the clip shows [sourceOffsetTicks, +length).
    const auto valueAtTick = [lane](Tick tick) -> double {
        if (tick <= lane->points.front().tick)
            return lane->points.front().value;
        if (tick >= lane->points.back().tick)
            return lane->points.back().value;

        for (std::size_t index = 1; index < lane->points.size(); ++index) {
            const auto& before = lane->points[index - 1];
            const auto& after  = lane->points[index];

            if (tick < after.tick) {
                const double span = static_cast<double>(after.tick - before.tick);
                const double mix  = span > 0.0 ? static_cast<double>(tick - before.tick) / span
                                               : 1.0;
                return before.value + (after.value - before.value) * mix;
            }
        }

        return lane->points.back().value;
    };

    NSBezierPath* path = [NSBezierPath bezierPath];
    path.lineWidth = 1.5;

    const double ticksPerPoint =
        static_cast<double>(model.lengthTicks) / body.size.width;

    for (double x = 0.0; x <= body.size.width; x += 2.0) {
        const Tick tick = model.sourceOffsetTicks
                        + static_cast<Tick>(x * ticksPerPoint);
        const double value = std::clamp(valueAtTick(tick), 0.0, 1.0);

        const NSPoint point = NSMakePoint(
            body.origin.x + x,
            body.origin.y + body.size.height * (1.0 - value));

        if (x == 0.0)
            [path moveToPoint:point];
        else
            [path lineToPoint:point];
    }

    [[NSColor colorWithSRGBRed:1 green:1 blue:1 alpha:0.75] setStroke];
    [path stroke];
}

/// A pattern clip shows the notes it will play, at the scale of the clip. It is
/// the same information the Piano Roll draws, reduced to what fits: without it
/// every pattern clip in an arrangement looks identical.
- (void)drawPatternPreviewFor:(const project::Clip&)model inBody:(NSRect)body
{
    if (body.size.height < 8.0 || body.size.width < 6.0 || model.lengthTicks <= 0)
        return;

    const project::Pattern* pattern = _project->findPattern(model.source);
    if (pattern == nullptr)
        return;

    int lowest  = 127;
    int highest = 0;

    for (const auto& content : pattern->channels)
        for (const project::MidiEvent& event : content.events) {
            if (event.type != project::MidiEventType::note)
                continue;

            lowest  = std::min(lowest, static_cast<int>(event.key));
            highest = std::max(highest, static_cast<int>(event.key));
        }

    if (lowest > highest)
        return;

    const double span   = std::max(1, highest - lowest + 1);
    const double scale  = body.size.width / static_cast<double>(model.lengthTicks);
    const double height = std::max(1.0, std::min(3.0, body.size.height / span));

    [[NSColor colorWithSRGBRed:1 green:1 blue:1 alpha:0.55] setFill];

    for (const auto& content : pattern->channels)
        for (const project::MidiEvent& event : content.events) {
            if (event.type != project::MidiEventType::note)
                continue;

            const Tick from = event.tick - model.sourceOffsetTicks;
            if (from < 0 || from >= model.lengthTicks)
                continue;

            const double x = body.origin.x + static_cast<double>(from) * scale;
            const double width = std::max(1.0, static_cast<double>(event.duration) * scale);
            const double y = body.origin.y + body.size.height
                           - (static_cast<double>(event.key - lowest) + 1.0)
                                 * (body.size.height / span);

            NSRectFillUsingOperation(NSMakeRect(x, y, std::min(width, NSMaxX(body) - x), height),
                                     NSCompositingOperationSourceOver);
        }
}

- (void)drawPlayhead
{
    if (_playheadTick < 0)
        return;

    const CGFloat x = headerWidth + _model->tickToX(static_cast<Tick>(_playheadTick));
    if (x < headerWidth || x > self.bounds.size.width)
        return;

    theme::drawPlayhead(x, self.bounds, rulerHeight - 6.0, true);
}

// ── Header geometry ──────────────────────────────────────────────────────────

- (NSRect)muteRectForRow:(std::size_t)row
{
    const CGFloat y = rulerHeight + _model->trackY(_project->tracks(), row);
    return NSMakeRect(headerWidth - 2.0 * buttonWidth - 2.0 * padding - 3.0, y + 6.0,
                      buttonWidth, buttonWidth);
}

- (NSRect)soloRectForRow:(std::size_t)row
{
    const NSRect mute = [self muteRectForRow:row];
    return NSMakeRect(mute.origin.x + buttonWidth + padding, mute.origin.y, buttonWidth,
                      buttonWidth);
}

- (NSRect)addTrackRect
{
    const CGFloat y = rulerHeight
                    + app::PlaylistModel::tracksHeight(_project->tracks())
                    - _model->viewport().firstTrackY;

    return NSMakeRect(0, y, self.bounds.size.width, addRowHeight);
}

// ── Input ────────────────────────────────────────────────────────────────────

- (void)mouseDown:(NSEvent*)event
{
    const NSPoint view = [self convertPoint:event.locationInWindow fromView:nil];
    const NSPoint grid = [self gridPointFor:event];

    _drag              = PlaylistDrag::none;
    _dragOrigin        = grid;
    _dragCurrent       = grid;
    _dragAppliedTicks  = 0;
    _dragAppliedTracks = 0;
    _dragAppliedLength = 0;

    if (view.y < rulerHeight) {
        if (self.onSeekTick != nil && view.x >= headerWidth)
            self.onSeekTick(static_cast<long long>(std::max<Tick>(0, _model->xToTick(grid.x))));

        return;
    }

    if (view.x < headerWidth) {
        [self headerClickAt:view event:event];
        return;
    }

    const std::size_t index = _model->clipAtPoint(*_project, grid.x, grid.y);

    if (index == app::PlaylistModel::noClip) {
        if ((event.modifierFlags & NSEventModifierFlagShift) != 0) {
            _drag = PlaylistDrag::boxSelect;
            return;
        }

        _model->clearSelection();
        [self placeClipAt:grid];
        return;
    }

    const project::Clip&    clicked = _project->clips()[index];
    const project::EntityId clipId  = clicked.id;

    // Double-clicking an audio clip opens its asset in the audio editor —
    // the clip is the handle, the recording is the thing being edited.
    if (event.clickCount == 2 && clicked.type == project::ClipType::audio) {
        if (self.onOpenAudioAsset != nil)
            self.onOpenAudioAsset(clicked.source.value());
        return;
    }

    if ((event.modifierFlags & NSEventModifierFlagShift) != 0)
        _model->toggleSelection(clipId);
    else if (!_model->isSelected(clipId))
        _model->setSelection({clipId});

    _drag = _model->isOverResizeHandle(*_project, index, grid.x, grid.y) ? PlaylistDrag::resize
                                                                        : PlaylistDrag::move;

    [self setNeedsDisplay:YES];
}

- (void)mouseDragged:(NSEvent*)event
{
    if (_drag == PlaylistDrag::none)
        return;

    const NSPoint grid = [self gridPointFor:event];
    _dragCurrent = grid;

    if (_drag == PlaylistDrag::boxSelect) {
        [self setNeedsDisplay:YES];
        return;
    }

    if (_model->selection().empty())
        return;

    // Deltas are computed against what has already been applied, so a drag
    // produces one merged command rather than a fight between absolute and
    // relative positions.
    const Tick wanted = _model->snapTick(_model->xToTick(grid.x) - _model->xToTick(_dragOrigin.x));

    if (_drag == PlaylistDrag::resize) {
        const Tick delta = wanted - _dragAppliedLength;
        if (delta == 0)
            return;

        if (_registry->executeMerging(std::make_unique<app::ResizeClipsCommand>(
                _model->selection(), delta))) {
            _dragAppliedLength = wanted;
            [self changed];
        }

        return;
    }

    const std::size_t fromRow = _model->trackAtY(_project->tracks(), _dragOrigin.y);
    const std::size_t toRow   = _model->trackAtY(_project->tracks(), grid.y);

    int trackDelta = 0;
    if (fromRow != app::PlaylistModel::noTrack && toRow != app::PlaylistModel::noTrack)
        trackDelta = static_cast<int>(toRow) - static_cast<int>(fromRow) - _dragAppliedTracks;

    const Tick tickDelta = wanted - _dragAppliedTicks;

    if (tickDelta == 0 && trackDelta == 0)
        return;

    if (_registry->executeMerging(std::make_unique<app::MoveClipsCommand>(
            _model->selection(), tickDelta, trackDelta))) {
        _dragAppliedTicks  = wanted;
        _dragAppliedTracks += trackDelta;
        [self changed];
    }
}

- (void)mouseUp:(NSEvent*)event
{
    (void)event;

    if (_drag == PlaylistDrag::boxSelect) {
        _model->clipsInRectangle(*_project,
                                 std::min(_dragOrigin.x, _dragCurrent.x),
                                 std::min(_dragOrigin.y, _dragCurrent.y),
                                 std::abs(_dragCurrent.x - _dragOrigin.x),
                                 std::abs(_dragCurrent.y - _dragOrigin.y),
                                 _boxed);

        _model->setSelection(_boxed);
    }

    _drag = PlaylistDrag::none;
    [self setNeedsDisplay:YES];
}

- (void)rightMouseDown:(NSEvent*)event
{
    const NSPoint grid = [self gridPointFor:event];
    const NSPoint view = [self convertPoint:event.locationInWindow fromView:nil];

    if (view.x < headerWidth) {
        const std::size_t row = _model->trackAtY(_project->tracks(), grid.y);
        if (row == app::PlaylistModel::noTrack)
            return;

        [self showTrackMenuFor:_project->tracks()[row].id event:event];
        return;
    }

    const std::size_t index = _model->clipAtPoint(*_project, grid.x, grid.y);
    if (index == app::PlaylistModel::noClip)
        return;

    const project::EntityId clipId = _project->clips()[index].id;
    if (!_model->isSelected(clipId))
        _model->setSelection({clipId});

    [self showClipMenuWithEvent:event];
}

- (void)keyDown:(NSEvent*)event
{
    const unichar character = event.charactersIgnoringModifiers.length > 0
                                  ? [event.charactersIgnoringModifiers characterAtIndex:0]
                                  : 0;

    const bool command = (event.modifierFlags & NSEventModifierFlagCommand) != 0;
    const bool shift   = (event.modifierFlags & NSEventModifierFlagShift) != 0;

    if (command && (character == 'z' || character == 'Z')) {
        if (shift)
            (void)_registry->redo();
        else
            (void)_registry->undo();

        _model->pruneSelection(*_project);
        [self changed];
        return;
    }

    if (character == NSDeleteCharacter || character == NSBackspaceCharacter
        || character == NSDeleteFunctionKey) {
        if (!_model->selection().empty()
            && _registry->execute(std::make_unique<app::RemoveClipsCommand>(_model->selection()))) {
            _model->clearSelection();
            [self changed];
        }

        return;
    }

    if (character == ' ') {
        if (self.onTransportToggle != nil)
            self.onTransportToggle();

        return;
    }

    if (command && character == 'd' && !_model->selection().empty()) {
        [self duplicateSelection];
        return;
    }

    [super keyDown:event];
}

- (void)scrollWheel:(NSEvent*)event
{
    auto viewport = _model->viewport();

    if ((event.modifierFlags & NSEventModifierFlagCommand) != 0) {
        const double factor = event.scrollingDeltaY > 0 ? 0.9 : 1.1;
        const auto   ticks  = static_cast<Tick>(static_cast<double>(viewport.visibleTicks) * factor);

        viewport.visibleTicks = std::clamp<Tick>(ticks, ticksPerQuarterNote * 4,
                                                 ticksPerQuarterNote * 4 * 512);
    } else {
        const double scale = _model->pointsPerTick();
        if (scale > 0.0)
            viewport.firstTick -= static_cast<Tick>(event.scrollingDeltaX / scale);

        viewport.firstTrackY -= event.scrollingDeltaY;
    }

    viewport.firstTick   = std::max<Tick>(0, viewport.firstTick);
    viewport.firstTrackY = std::max(0.0, viewport.firstTrackY);

    _model->setViewport(viewport);
    [self setNeedsDisplay:YES];
}

// ── Edits ────────────────────────────────────────────────────────────────────

- (void)headerClickAt:(NSPoint)view event:(NSEvent*)event
{
    const NSRect addRow = [self addTrackRect];
    if (view.y >= addRow.origin.y && view.y < addRow.origin.y + addRow.size.height) {
        [self addTrack];
        return;
    }

    const NSPoint grid = NSMakePoint(view.x - headerWidth, view.y - rulerHeight);
    const std::size_t row = _model->trackAtY(_project->tracks(), grid.y);
    if (row == app::PlaylistModel::noTrack)
        return;

    const project::Track& track = _project->tracks()[row];
    const project::EntityId trackId = track.id;

    if (NSPointInRect(view, [self muteRectForRow:row])) {
        [self commit:std::make_unique<app::SetTrackMutedCommand>(trackId, !track.muted)];
        return;
    }

    if (NSPointInRect(view, [self soloRectForRow:row])) {
        [self commit:std::make_unique<app::SetTrackSoloedCommand>(trackId, !track.soloed)];
        return;
    }

    if (event.clickCount == 2)
        [self renameTrack:trackId];
}

/// Clicking empty timeline places the current pattern there — the fastest way
/// to build an arrangement, and the reason the pattern list has a selection.
- (void)placeClipAt:(NSPoint)grid
{
    const std::size_t row = _model->trackAtY(_project->tracks(), grid.y);
    if (row == app::PlaylistModel::noTrack)
        return;

    const project::EntityId pattern{_patternIdValue};
    if (_project->findPattern(pattern) == nullptr)
        return;

    const Tick start = _model->snapTick(std::max<Tick>(0, _model->xToTick(grid.x)));

    auto command = std::make_unique<app::AddPatternClipCommand>(_project->tracks()[row].id,
                                                                pattern, start);
    app::AddPatternClipCommand* raw = command.get();

    if (!_registry->execute(std::move(command)))
        return;

    _model->setSelection({raw->clipId()});
    [self changed];
}

- (void)duplicateSelection
{
    // Offset by the width of the widest selected clip, so the copy lands after
    // the original rather than on top of it.
    Tick offset = 0;
    for (const project::EntityId id : _model->selection())
        if (const project::Clip* clip = _project->findClip(id))
            offset = std::max(offset, clip->lengthTicks);

    auto command = std::make_unique<app::DuplicateClipsCommand>(_model->selection(), offset);
    app::DuplicateClipsCommand* raw = command.get();

    if (!_registry->execute(std::move(command)))
        return;

    _model->setSelection(raw->createdClips());
    [self changed];
}

- (void)showClipMenuWithEvent:(NSEvent*)event
{
    NSMenu* menu = [[NSMenu alloc] init];

    NSMenuItem* duplicate = [menu addItemWithTitle:@"Duplicate"
                                            action:@selector(duplicateFromMenu:)
                                     keyEquivalent:@""];
    duplicate.target = self;

    NSMenuItem* mute = [menu addItemWithTitle:@"Mute"
                                       action:@selector(muteFromMenu:)
                                keyEquivalent:@""];
    mute.target = self;

    NSMenuItem* unmute = [menu addItemWithTitle:@"Unmute"
                                         action:@selector(unmuteFromMenu:)
                                  keyEquivalent:@""];
    unmute.target = self;

    [menu addItem:[NSMenuItem separatorItem]];

    NSMenuItem* remove = [menu addItemWithTitle:@"Remove"
                                         action:@selector(removeFromMenu:)
                                  keyEquivalent:@""];
    remove.target = self;

    [NSMenu popUpContextMenu:menu withEvent:event forView:self];
}

- (void)showTrackMenuFor:(project::EntityId)trackId event:(NSEvent*)event
{
    NSMenu* menu = [[NSMenu alloc] init];

    NSMenuItem* rename = [menu addItemWithTitle:@"Rename Track…"
                                         action:@selector(renameTrackFromMenu:)
                                  keyEquivalent:@""];
    rename.target = self;
    rename.representedObject = @(trackId.value());

    NSMenuItem* remove = [menu addItemWithTitle:@"Remove Track"
                                         action:@selector(removeTrackFromMenu:)
                                  keyEquivalent:@""];
    remove.target = self;
    remove.representedObject = @(trackId.value());

    [NSMenu popUpContextMenu:menu withEvent:event forView:self];
}

- (void)duplicateFromMenu:(id)sender { (void)sender; [self duplicateSelection]; }

- (void)muteFromMenu:(id)sender
{
    (void)sender;
    [self commit:std::make_unique<app::SetClipMutedCommand>(_model->selection(), true)];
}

- (void)unmuteFromMenu:(id)sender
{
    (void)sender;
    [self commit:std::make_unique<app::SetClipMutedCommand>(_model->selection(), false)];
}

- (void)removeFromMenu:(id)sender
{
    (void)sender;

    if (_registry->execute(std::make_unique<app::RemoveClipsCommand>(_model->selection()))) {
        _model->clearSelection();
        [self changed];
    }
}

- (void)renameTrackFromMenu:(NSMenuItem*)item
{
    [self renameTrack:project::EntityId{[item.representedObject unsignedLongLongValue]}];
}

- (void)removeTrackFromMenu:(NSMenuItem*)item
{
    const project::EntityId trackId{[item.representedObject unsignedLongLongValue]};

    [self commit:std::make_unique<app::RemoveTrackCommand>(trackId)];
    _model->pruneSelection(*_project);
}

- (void)addTrack
{
    auto command = std::make_unique<app::AddTrackCommand>(
        "Track " + std::to_string(_project->tracks().size() + 1));

    [self commit:std::move(command)];
}

- (void)renameTrack:(project::EntityId)trackId
{
    const project::Track* track = _project->findTrack(trackId);
    if (track == nullptr)
        return;

    NSAlert* alert = [[NSAlert alloc] init];
    alert.messageText = @"Rename track";
    [alert addButtonWithTitle:@"Rename"];
    [alert addButtonWithTitle:@"Cancel"];

    NSTextField* field = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 220, 24)];
    field.stringValue = @(track->name.c_str());
    alert.accessoryView = field;

    if ([alert runModal] != NSAlertFirstButtonReturn)
        return;

    [self commit:std::make_unique<app::RenameTrackCommand>(trackId, field.stringValue.UTF8String)];
}

- (void)commit:(app::CommandPtr)command
{
    if (_registry->execute(std::move(command)))
        [self changed];
}

- (void)changed
{
    [self setNeedsDisplay:YES];

    if (self.onChange != nil)
        self.onChange();
}

- (void)setPatternIdValue:(unsigned long long)value
{
    _patternIdValue = value;
    [self setNeedsDisplay:YES];
}

- (void)setPlayheadTick:(long long)tick
{
    if (_playheadTick == tick)
        return;

    _playheadTick = tick;
    [self setNeedsDisplay:YES];
}

@end
