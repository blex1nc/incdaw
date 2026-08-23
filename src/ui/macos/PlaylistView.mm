#include "ui/macos/PlaylistView.h"

#include "app/Browser.h"
#include "app/CommandRegistry.h"
#include "app/AutomationEditorModel.h"
#include "app/PlaylistModel.h"
#include "app/commands/ClipCommands.h"
#include "app/commands/ArrangementCommands.h"
#include "app/commands/AutomationCommands.h"
#include "app/commands/ConsolidateCommands.h"
#include "app/commands/ImportCommands.h"
#include "app/commands/MarkerCommands.h"
#include "app/commands/TrackCommands.h"
#include "engine/audio/WaveformOverview.h"
#include "project/ParameterRegistry.h"
#include "project/PatternCompiler.h"
#include "project/Model.h"
#include "ui/macos/AutomationEditorView.h"
#include "ui/macos/Theme.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <filesystem>
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

/// How far one folder level moves a header in. Capped at four levels in the
/// drawing: past that the indent costs more width than the nesting is worth
/// showing, and the header would have no room left for a name.
constexpr CGFloat folderIndent = 10.0;

using theme::fillRect;

enum class PlaylistDrag { none, move, resize, boxSelect };

} // namespace

@implementation INCDAWPlaylistView {
    project::Project*     _project;
    app::CommandRegistry* _registry;

    std::unique_ptr<app::PlaylistModel> _model;

    std::vector<app::PlaylistModel::VisibleClip> _visible;

    /// Track row a dragged sample is hovering, and the tick it would land on.
    /// Drawn while the drag is in flight so the drop is aimed, not guessed.
    std::size_t   _dropTrack;
    project::Tick _dropTick;

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
    int          _dragAppliedLanes;
    BOOL         _stretchResize;   ///< Option at the handle: stretch, not trim

    /// What the last refused gesture was refused for, shown over the timeline
    /// until the timer clears it. A lock that silently swallows a drag reads
    /// as a broken playlist; the notice is the difference between "it will
    /// not" and "it does not work".
    NSString* _refusal;
    NSTimer*  _refusalTimer;

    /// The automation editor and its panel, made on first use: a project with
    /// no automation never pays for either.
    INCDAWAutomationEditorView* _automationEditor;
    NSPanel*                    _automationPanel;

    /// The row the open track menu belongs to. Set when the menu is built, so
    /// its verbs act on what was right-clicked rather than on the selection.
    project::EntityId _menuTrack;

    /// The host's parameter registry, so a consolidation renders plugin
    /// parameters the way playback does. Null is safe: builtin and stored
    /// instrument parameters are applied by the compiler either way.
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
    _dragAppliedLanes  = 0;
    _playheadTick      = -1;

    app::PlaylistModel::Viewport viewport;
    viewport.firstTick    = 0;
    viewport.visibleTicks = ticksPerQuarterNote * 4 * 16;   // sixteen bars
    viewport.width        = frame.size.width - headerWidth;
    viewport.height       = frame.size.height - rulerHeight;
    _model->setViewport(viewport);
    _model->setSnap(ticksPerQuarterNote * 4);               // one bar

    _dropTrack = app::PlaylistModel::noTrack;
    _dropTick  = 0;
    [self registerForDraggedTypes:@[ NSPasteboardTypeFileURL ]];

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

    [self drawArrangementChip];
    [self drawRuler];
    [self drawTracks];
    [self drawClips];
    [self drawPlayhead];
    [self drawRefusal];

    // Where a dragged sample would land: the lane, and the exact tick.
    if (_dropTrack != app::PlaylistModel::noTrack && _dropTrack < _project->tracks().size()) {
        const NSRect lane = NSMakeRect(headerWidth,
                                       _model->trackY(_project->tracks(), _dropTrack) + rulerHeight,
                                       self.bounds.size.width - headerWidth,
                                       app::PlaylistModel::rowHeight(_project->tracks(),
                                                                     _dropTrack));

        [theme::withAlpha(theme::ink(Ink::accent), 0.12) setFill];
        NSRectFillUsingOperation(lane, NSCompositingOperationSourceOver);

        const auto x = static_cast<CGFloat>(headerWidth + _model->tickToX(_dropTick));
        [theme::withAlpha(theme::ink(Ink::accent), 0.9) setFill];
        NSRectFill(NSMakeRect(x, lane.origin.y, 2.0, lane.size.height));
    }

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

    // Markers and regions live on the ruler: a region shades its span, a
    // marker plants a flag, and both carry their name.
    for (const project::TimelineMarker& marker : _project->markers()) {
        const CGFloat x = headerWidth + _model->tickToX(marker.tick);

        if (marker.length > 0) {
            const CGFloat right = headerWidth + _model->tickToX(marker.tick + marker.length);
            const CGFloat clippedLeft  = std::max(x, headerWidth);
            const CGFloat clippedRight = std::min(right, self.bounds.size.width);

            if (clippedRight > clippedLeft) {
                [theme::withAlpha(theme::fromArgb(marker.colour), 0.22) setFill];
                NSRectFillUsingOperation(NSMakeRect(clippedLeft, 0, clippedRight - clippedLeft,
                                                    rulerHeight),
                                         NSCompositingOperationSourceOver);
            }
        }

        if (x < headerWidth || x > self.bounds.size.width)
            continue;

        fillRect(NSMakeRect(x, 0, 2.0, rulerHeight), theme::fromArgb(marker.colour));
        theme::drawText(@(marker.name.c_str()), NSMakeRect(x + 4.0, 4.0, 90.0, 14.0),
                        theme::fromArgb(marker.colour, 1.2), theme::labelFont(9.0));
    }
}

/// The current timeline's name, in the corner the ruler and the headers leave
/// empty. The brief asks for the switcher on the toolbar; the toolbar is
/// ControlBarView, which this track does not own, so it sits here — over the
/// clips it decides the contents of, which is arguably where it belongs.
- (NSRect)arrangementChipRect
{
    return NSMakeRect(3.0, 3.0, headerWidth - 8.0, rulerHeight - 6.0);
}

- (void)drawArrangementChip
{
    const NSRect chip = [self arrangementChipRect];
    theme::drawPanel(chip, theme::metrics::radiusControl, false, true);

    const project::Arrangement* current =
        _project->findArrangement(_project->currentArrangement());

    NSString* label = current != nullptr ? @(current->name.c_str()) : @"—";

    if (_project->arrangements().size() > 1)
        label = [NSString stringWithFormat:@"%@  \u25be", label];

    theme::drawTextCentred(label, chip, theme::ink(Ink::textPrimary),
                           theme::labelFont(11.0, NSFontWeightSemibold));
}

- (void)showArrangementMenuWithEvent:(NSEvent*)event
{
    NSMenu* menu = [[NSMenu alloc] init];

    for (const project::Arrangement& arrangement : _project->arrangements()) {
        NSMenuItem* item = [menu addItemWithTitle:@(arrangement.name.c_str())
                                           action:@selector(switchArrangementFromMenu:)
                                    keyEquivalent:@""];
        item.target            = self;
        item.representedObject = @(arrangement.id.value());

        if (arrangement.id == _project->currentArrangement())
            item.state = NSControlStateValueOn;
    }

    [menu addItem:[NSMenuItem separatorItem]];

    NSMenuItem* add = [menu addItemWithTitle:@"New Arrangement"
                                      action:@selector(addArrangementFromMenu:)
                               keyEquivalent:@""];
    add.target = self;

    NSMenuItem* duplicate = [menu addItemWithTitle:@"Duplicate Arrangement"
                                            action:@selector(duplicateArrangementFromMenu:)
                                     keyEquivalent:@""];
    duplicate.target = self;

    NSMenuItem* rename = [menu addItemWithTitle:@"Rename Arrangement\u2026"
                                         action:@selector(renameArrangementFromMenu:)
                                  keyEquivalent:@""];
    rename.target = self;

    if (_project->arrangements().size() > 1) {
        [menu addItem:[NSMenuItem separatorItem]];

        NSMenuItem* remove = [menu addItemWithTitle:@"Remove Arrangement"
                                             action:@selector(removeArrangementFromMenu:)
                                      keyEquivalent:@""];
        remove.target = self;
    }

    [NSMenu popUpContextMenu:menu withEvent:event forView:self];
}

/// Every switch goes through the command stack, which is what keeps one undo
/// history honest across two timelines: an edit, a switch and an undo walk
/// back through the switch before reaching the edit.
- (void)switchArrangementFromMenu:(NSMenuItem*)item
{
    const project::EntityId id{[item.representedObject unsignedLongLongValue]};

    if (_registry->execute(std::make_unique<app::SetCurrentArrangementCommand>(id))) {
        _model->clearSelection();
        [self invalidateWaveformCache];
        [self changed];
    }
}

- (void)addArrangementFromMenu:(id)sender
{
    (void)sender;

    const std::string name = "Arrangement "
                           + std::to_string(_project->arrangements().size() + 1);

    if (_registry->execute(std::make_unique<app::AddArrangementCommand>(name))) {
        _model->clearSelection();
        [self changed];
    }
}

- (void)duplicateArrangementFromMenu:(id)sender
{
    (void)sender;

    const project::Arrangement* current =
        _project->findArrangement(_project->currentArrangement());
    if (current == nullptr)
        return;

    if (_registry->execute(std::make_unique<app::AddArrangementCommand>(
            current->name + " copy", current->id))) {
        _model->clearSelection();
        [self changed];
    }
}

- (void)renameArrangementFromMenu:(id)sender
{
    (void)sender;

    const project::Arrangement* current =
        _project->findArrangement(_project->currentArrangement());
    if (current == nullptr)
        return;

    NSAlert* alert = [[NSAlert alloc] init];
    alert.messageText = @"Rename arrangement";
    [alert addButtonWithTitle:@"Rename"];
    [alert addButtonWithTitle:@"Cancel"];

    NSTextField* field = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 220, 24)];
    field.stringValue = @(current->name.c_str());
    alert.accessoryView = field;

    if ([alert runModal] != NSAlertFirstButtonReturn)
        return;

    [self commit:std::make_unique<app::RenameArrangementCommand>(
                     current->id, field.stringValue.UTF8String)];
}

- (void)removeArrangementFromMenu:(id)sender
{
    (void)sender;

    if (_registry->execute(std::make_unique<app::RemoveArrangementCommand>(
            _project->currentArrangement()))) {
        _model->clearSelection();
        [self invalidateWaveformCache];
        [self changed];
    } else {
        [self refuse:@"A project keeps at least one arrangement."];
    }
}

- (void)drawTracks
{
    const std::vector<project::Track>& tracks = _project->tracks();

    for (std::size_t row = 0; row < tracks.size(); ++row) {
        const project::Track& track = tracks[row];

        const CGFloat y      = rulerHeight + _model->trackY(tracks, row);
        const CGFloat height = app::PlaylistModel::rowHeight(tracks, row);

        if (height <= 0.0)
            continue;   // a collapsed folder above it

        if (y + height < rulerHeight || y > self.bounds.size.height)
            continue;

        NSColor* colour = theme::fromArgb(track.colour);

        // One step of indent per folder above, so the tree is legible from
        // the shape of the column rather than from reading every name.
        const CGFloat indent = static_cast<CGFloat>(
            std::min<std::size_t>(project::trackDepth(tracks, track), 4)) * folderIndent;

        // The lane behind the clips, so an empty track is still a place rather
        // than a gap.
        fillRect(NSMakeRect(headerWidth, y, self.bounds.size.width - headerWidth, height - 1.0),
                 (row % 2) == 0 ? theme::ink(Ink::rowEven) : theme::ink(Ink::rowOdd));

        [self drawBarLinesInLaneAt:y height:height - 1.0];

        // A hairline between lanes, so a two-deep track reads as two places
        // rather than as one row with clips at odd heights.
        const int lanes = project::trackLaneCount(*_project, track.id);
        if (lanes > 1) {
            const CGFloat band = (height - 1.0) / static_cast<CGFloat>(lanes);

            for (int lane = 1; lane < lanes; ++lane)
                fillRect(NSMakeRect(headerWidth, y + band * static_cast<CGFloat>(lane),
                                    self.bounds.size.width - headerWidth, 1.0),
                         theme::withAlpha(theme::ink(Ink::separator), 0.7));
        }

        // ── The track's header ───────────────────────────────────────────────
        const NSRect header = NSMakeRect(2.0 + indent, y + 1.0, headerWidth - 5.0 - indent,
                                         height - 4.0);
        theme::drawPanel(header, theme::metrics::radiusControl, false, true);

        theme::fillGradient(NSMakeRect(NSMinX(header) + 5.0, NSMinY(header) + 5.0, 8.0,
                                       header.size.height - 10.0),
                            2.5, theme::lighten(colour, 0.25),
                            theme::darken(colour, track.muted ? 0.65 : 0.15), true);

        // A folder gets a disclosure triangle where a track has nothing, and
        // its name starts after it.
        const CGFloat nameLeft = track.type == project::TrackType::folder ? 32.0 : 20.0;

        if (track.type == project::TrackType::folder)
            [self drawDisclosure:[self disclosureRectForRow:row] open:!track.collapsed];

        theme::drawText(@(track.name.c_str()),
                        NSMakeRect(NSMinX(header) + nameLeft, NSMinY(header) + 6.0,
                                   header.size.width - 2.0 * buttonWidth - 3.0 * padding, 16.0),
                        track.muted ? theme::ink(Ink::textDim) : theme::ink(Ink::textPrimary),
                        theme::labelFont(12.0,
                                         track.type == project::TrackType::folder
                                             ? NSFontWeightBold
                                             : NSFontWeightMedium));

        if (lanes > 1)
            theme::drawText([NSString stringWithFormat:@"%d lanes", lanes],
                            NSMakeRect(NSMinX(header) + nameLeft, NSMinY(header) + 20.0,
                                       header.size.width - nameLeft - 4.0, 12.0),
                            theme::ink(Ink::textDim), theme::labelFont(9.0));

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

        // An automation clip is named for what it rides, so a lane can be told
        // apart in the arrangement without opening it.
        const std::string label = model.type == project::ClipType::automation
                                      ? [self automationLabelFor:model]
                                      : model.name;

        // One region shape for every kind of clip; what is drawn inside it is
        // what says whether it holds notes, audio or an automation ride.
        const NSRect body = theme::drawRegion(rect, theme::fromArgb(clip.colour),
                                              @(label.c_str()), clip.selected, clip.muted,
                                              true);

        if (model.type == project::ClipType::audio)
            [self drawWaveformFor:model inBody:body];
        else if (model.type == project::ClipType::automation)
            [self drawAutomationCurveFor:model inBody:body];
        else
            [self drawPatternPreviewFor:model inBody:body];

        if (model.type == project::ClipType::audio)
            [self drawFadesFor:model inBody:body];

        if (model.group.isValid())
            [self drawGroupTieIn:body forGroup:model.group];

        if (model.locked)
            [self drawLockMarkIn:body];
    }
}

/// The fade ramps, drawn from the same arithmetic the compiler plays: a
/// crossfaded edge shows the whole overlap, so what the eye reads as the
/// crossover is where it actually is.
- (void)drawFadesFor:(const project::Clip&)clip inBody:(NSRect)body
{
    const project::ClipFades fades = project::clipFades(*_project, clip);
    if ((fades.in == 0 && fades.out == 0) || clip.length == 0 || body.size.width < 2.0)
        return;

    const CGFloat perFrame = body.size.width / static_cast<CGFloat>(clip.length);

    [theme::withAlpha(theme::ink(Ink::panel), 0.55) setFill];

    if (fades.in > 0) {
        const CGFloat width = std::min(body.size.width,
                                       static_cast<CGFloat>(fades.in) * perFrame);

        NSBezierPath* wedge = [NSBezierPath bezierPath];
        [wedge moveToPoint:NSMakePoint(NSMinX(body), NSMinY(body))];
        [wedge lineToPoint:NSMakePoint(NSMinX(body) + width, NSMaxY(body))];
        [wedge lineToPoint:NSMakePoint(NSMinX(body), NSMaxY(body))];
        [wedge closePath];
        [wedge fill];
    }

    if (fades.out > 0) {
        const CGFloat width = std::min(body.size.width,
                                       static_cast<CGFloat>(fades.out) * perFrame);

        NSBezierPath* wedge = [NSBezierPath bezierPath];
        [wedge moveToPoint:NSMakePoint(NSMaxX(body), NSMinY(body))];
        [wedge lineToPoint:NSMakePoint(NSMaxX(body) - width, NSMaxY(body))];
        [wedge lineToPoint:NSMakePoint(NSMaxX(body), NSMaxY(body))];
        [wedge closePath];
        [wedge fill];
    }
}

/// A tinted strip along the region's top edge, its hue derived from the group
/// id so that every member of one group carries the same one and two adjacent
/// groups almost never share it. Cheaper to read than a badge and it survives
/// down to a clip two points wide.
- (void)drawGroupTieIn:(NSRect)body forGroup:(project::EntityId)group
{
    if (body.size.height < 6.0 || body.size.width < 4.0)
        return;

    // The golden-ratio step spreads consecutive ids across the wheel instead
    // of leaving neighbours a shade apart.
    const CGFloat hue = std::fmod(static_cast<CGFloat>(group.value()) * 0.6180339887, 1.0);

    NSColor* tint = [NSColor colorWithCalibratedHue:hue
                                         saturation:0.55
                                         brightness:0.95
                                              alpha:0.85];

    fillRect(NSMakeRect(NSMinX(body), NSMaxY(body) - 3.0, body.size.width, 3.0), tint);
}

/// A bar-and-shackle glyph in the region's bottom-right corner. Drawn rather
/// than set as text so it stays legible at the two or three pixels a clip has
/// to spare, and placed opposite the name so it never covers one.
- (void)drawLockMarkIn:(NSRect)body
{
    constexpr CGFloat size = 7.0;
    if (body.size.width < size + 6.0 || body.size.height < size + 4.0)
        return;

    const NSRect shackle = NSMakeRect(NSMaxX(body) - size - 3.0,
                                      NSMinY(body) + 2.0 + size * 0.45,
                                      size, size * 0.55);
    const NSRect bar     = NSMakeRect(NSMaxX(body) - size - 3.0,
                                      NSMinY(body) + 2.0,
                                      size, size * 0.5);

    NSBezierPath* hoop = [NSBezierPath bezierPath];
    [hoop appendBezierPathWithArcWithCenter:NSMakePoint(NSMidX(shackle), NSMinY(shackle))
                                     radius:shackle.size.width * 0.32
                                 startAngle:0.0
                                   endAngle:180.0];
    hoop.lineWidth = 1.0;

    [theme::withAlpha(theme::ink(Ink::textPrimary), 0.85) setStroke];
    [hoop stroke];

    [theme::withAlpha(theme::ink(Ink::textPrimary), 0.85) setFill];
    NSRectFillUsingOperation(bar, NSCompositingOperationSourceOver);
}

/// Puts a sentence over the timeline for a moment. Replacing an unexpired
/// notice restarts the clock rather than queueing, so hammering a locked clip
/// keeps one message on screen instead of a backlog.
/// How many of `clips` a lock is protecting. The commands enforce the rule; a
/// UI that has to explain the refusal still has to be able to see it.
- (std::size_t)lockedCountIn:(const app::ClipIds&)clips
{
    std::size_t locked = 0;
    for (const project::EntityId id : clips)
        if (const project::Clip* clip = _project->findClip(id); clip != nullptr && clip->locked)
            ++locked;

    return locked;
}

- (void)refuse:(NSString*)reason
{
    _refusal = [reason copy];

    [_refusalTimer invalidate];
    _refusalTimer = [NSTimer scheduledTimerWithTimeInterval:2.0
                                                    repeats:NO
                                                      block:^(NSTimer* timer) {
        (void)timer;
        self->_refusal = nil;
        [self setNeedsDisplay:YES];
    }];

    [self setNeedsDisplay:YES];
}

- (void)drawRefusal
{
    if (_refusal == nil)
        return;

    NSDictionary* attributes = @{
        NSFontAttributeName            : [NSFont systemFontOfSize:11.0
                                                           weight:NSFontWeightMedium],
        NSForegroundColorAttributeName : theme::ink(Ink::textPrimary),
    };

    const NSSize text = [_refusal sizeWithAttributes:attributes];
    const NSRect box  = NSMakeRect(headerWidth + 12.0,
                                   NSMaxY(self.bounds) - rulerHeight - text.height - 18.0,
                                   text.width + 16.0, text.height + 8.0);

    NSBezierPath* plate = [NSBezierPath bezierPathWithRoundedRect:box xRadius:4.0 yRadius:4.0];
    [theme::withAlpha(theme::ink(Ink::panelRaised), 0.94) setFill];
    [plate fill];
    [theme::withAlpha(theme::ink(Ink::accent), 0.55) setStroke];
    [plate stroke];

    [_refusal drawAtPoint:NSMakePoint(NSMinX(box) + 8.0, NSMinY(box) + 4.0)
           withAttributes:attributes];
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
    [theme::withAlpha(theme::ink(Ink::textOnAccent), 0.62) setFill];

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
/// "Master · volume" rather than a lane id: the answer to "which parameter is
/// this?" belongs on the clip, not behind a double-click.
- (std::string)automationLabelFor:(const project::Clip&)model
{
    const project::AutomationLane* lane = nullptr;
    for (const project::AutomationLane& candidate : _project->automation())
        if (candidate.id == model.source)
            lane = &candidate;

    if (lane == nullptr)
        return model.name;

    std::string target;
    if (const project::MixerNode* node = _project->findMixerNode(lane->targetEntity))
        target = node->name;
    else if (const project::Channel* channel = _project->findChannel(lane->targetEntity))
        target = channel->name;

    return target.empty() ? lane->parameterKey : target + " \u00b7 " + lane->parameterKey;
}

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
    //
    // Evaluated by the engine's own class rather than by a straight line
    // between points: a held or shaped segment drawn as a ramp would be a
    // picture of a curve nobody is playing, and since the automation editor
    // arrived those segments are something the user actually sets.
    const auto valueAtTick = [lane](Tick tick) -> double {
        return app::AutomationEditorModel::valueAt(lane->points, tick);
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

    [theme::withAlpha(theme::ink(Ink::textOnAccent), 0.75) setStroke];
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

    [theme::withAlpha(theme::ink(Ink::textOnAccent), 0.55) setFill];

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

/// The triangle that opens and closes a folder. Sized for the pointer rather
/// than for the glyph, which is small.
- (NSRect)disclosureRectForRow:(std::size_t)row
{
    const std::vector<project::Track>& tracks = _project->tracks();
    if (row >= tracks.size())
        return NSZeroRect;

    const CGFloat y = rulerHeight + _model->trackY(tracks, row);
    const CGFloat indent = static_cast<CGFloat>(
        std::min<std::size_t>(project::trackDepth(tracks, tracks[row]), 4)) * folderIndent;

    return NSMakeRect(2.0 + indent + 16.0, y + 5.0, 16.0, 16.0);
}

- (void)drawDisclosure:(NSRect)rect open:(BOOL)open
{
    const CGFloat size = 5.0;
    const NSPoint centre = NSMakePoint(NSMidX(rect), NSMidY(rect));

    NSBezierPath* arrow = [NSBezierPath bezierPath];

    if (open) {
        // Pointing down: the folder's contents are below it.
        [arrow moveToPoint:NSMakePoint(centre.x - size, centre.y + size * 0.6)];
        [arrow lineToPoint:NSMakePoint(centre.x + size, centre.y + size * 0.6)];
        [arrow lineToPoint:NSMakePoint(centre.x, centre.y - size * 0.7)];
    } else {
        [arrow moveToPoint:NSMakePoint(centre.x - size * 0.6, centre.y - size)];
        [arrow lineToPoint:NSMakePoint(centre.x - size * 0.6, centre.y + size)];
        [arrow lineToPoint:NSMakePoint(centre.x + size * 0.7, centre.y)];
    }

    [arrow closePath];
    [theme::withAlpha(theme::ink(Ink::textSecondary), 0.9) setFill];
    [arrow fill];
}

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

// ── Dropping a sample ───────────────────────────────────────────────────────

/// The dragged file, if INCDAW can read it. The same answer the Channel Rack
/// gives, from the same place: what is on the pasteboard, not who started the
/// drag — a drop from Finder behaves exactly like one from the Browser.
static NSString* droppedAudioPath(id<NSDraggingInfo> info)
{
    NSArray<NSURL*>* urls = [info.draggingPasteboard
        readObjectsForClasses:@[ [NSURL class] ]
                      options:@{NSPasteboardURLReadingFileURLsOnlyKey: @YES}];

    if (urls.count != 1)
        return nil;

    NSString* path = urls.firstObject.path;

    return path != nil && incdaw::app::Browser::canDecodeAudio(std::filesystem::path{path.UTF8String})
               ? path
               : nil;
}

- (project::Tick)dropTickForGrid:(NSPoint)grid
{
    const project::Tick tick    = _model->xToTick(grid.x > 0.0 ? grid.x : 0.0);
    const project::Tick snapped = _model->snapTick(tick > 0 ? tick : 0);
    return snapped > 0 ? snapped : 0;
}

- (NSDragOperation)draggingEntered:(id<NSDraggingInfo>)info
{
    return [self draggingUpdated:info];
}

- (NSDragOperation)draggingUpdated:(id<NSDraggingInfo>)info
{
    if (droppedAudioPath(info) == nil || _project == nullptr) {
        _dropTrack = app::PlaylistModel::noTrack;
        return NSDragOperationNone;
    }

    const NSPoint point = [self convertPoint:info.draggingLocation fromView:nil];
    const NSPoint grid  = NSMakePoint(point.x - headerWidth, point.y - rulerHeight);

    _dropTrack = _model->trackAtY(_project->tracks(), grid.y);
    _dropTick  = [self dropTickForGrid:grid];

    [self setNeedsDisplay:YES];

    return _dropTrack == app::PlaylistModel::noTrack ? NSDragOperationNone : NSDragOperationCopy;
}

- (void)draggingExited:(id<NSDraggingInfo>)info
{
    (void)info;
    _dropTrack = app::PlaylistModel::noTrack;
    [self setNeedsDisplay:YES];
}

/// A sample dropped on a lane becomes an audio clip there, snapped like every
/// other placement in this view, as long as the file itself. It is the clip a
/// recording would have produced — arrived at by dragging.
- (BOOL)performDragOperation:(id<NSDraggingInfo>)info
{
    NSString* path = droppedAudioPath(info);

    const std::size_t   row  = _dropTrack;
    const project::Tick tick = _dropTick;

    _dropTrack = app::PlaylistModel::noTrack;
    [self setNeedsDisplay:YES];

    if (path == nil || _project == nullptr || row == app::PlaylistModel::noTrack
        || row >= _project->tracks().size())
        return NO;

    [self commit:std::make_unique<app::ImportAudioClipCommand>(_project->tracks()[row].id,
                                                               path.UTF8String, tick)];
    return YES;
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
    _dragAppliedLanes  = 0;

    if (view.y < rulerHeight) {
        if (NSPointInRect(view, [self arrangementChipRect])) {
            [self showArrangementMenuWithEvent:event];
            return;
        }

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

    // Option-click cuts the clip where the mouse is — the slice gesture.
    if ((event.modifierFlags & NSEventModifierFlagOption) != 0) {
        if (clicked.locked) {
            [self refuse:@"This clip is locked \u2014 unlock it to split it."];
            return;
        }

        const Tick cut = _model->snapTick(_model->xToTick(grid.x));
        if (_registry->execute(std::make_unique<app::SplitClipCommand>(clipId, cut)))
            _model->setSelection({clipId});

        [self changed];
        return;
    }

    // Double-clicking an audio clip opens its asset in the audio editor —
    // the clip is the handle, the recording is the thing being edited.
    if (event.clickCount == 2 && clicked.type == project::ClipType::audio) {
        if (self.onOpenAudioAsset != nil)
            self.onOpenAudioAsset(clicked.source.value());
        return;
    }

    // The same idea one clip type over: an automation clip is the handle, the
    // lane is the thing being edited.
    if (event.clickCount == 2 && clicked.type == project::ClipType::automation) {
        [self openAutomationLane:clicked.source];
        return;
    }

    if ((event.modifierFlags & NSEventModifierFlagShift) != 0)
        _model->toggleSelection(clipId);
    else if (!_model->isSelected(clipId))
        _model->setSelection({clipId});

    // A locked clip refuses the whole gesture rather than letting the drag
    // begin and quietly moving everything else in the selection: the clip
    // under the pointer is the one the user is holding.
    if (clicked.locked) {
        [self refuse:@"This clip is locked \u2014 unlock it to move or resize it."];
        [self setNeedsDisplay:YES];
        return;
    }

    _drag = _model->isOverResizeHandle(*_project, index, grid.x, grid.y) ? PlaylistDrag::resize
                                                                        : PlaylistDrag::move;

    // Option at the resize handle stretches the audio to the new length
    // instead of trimming it — FL Studio 2026's resize-vs-stretch split.
    _stretchResize = _drag == PlaylistDrag::resize
                  && (event.modifierFlags & NSEventModifierFlagOption) != 0;

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

        const bool applied =
            _stretchResize
                ? _registry->executeMerging(std::make_unique<app::StretchClipsCommand>(
                      _model->selection(), delta))
                : _registry->executeMerging(std::make_unique<app::ResizeClipsCommand>(
                      _model->selection(), delta));

        if (applied) {
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

    // Vertical motion inside one row is a lane change; crossing the row's edge
    // is a track change. Measured in lanes rather than in points so a tall
    // track and a short one feel the same, and only while the pointer stays on
    // the row the drag began on — mixing the two on one gesture would make a
    // diagonal drag land somewhere nobody aimed at.
    int laneDelta = 0;
    if (trackDelta == 0 && _dragAppliedTracks == 0
        && fromRow != app::PlaylistModel::noTrack && fromRow == toRow) {
        const int wantedLane = [self laneOffsetFrom:_dragOrigin.y to:grid.y row:fromRow];
        laneDelta            = wantedLane - _dragAppliedLanes;
    }

    const Tick tickDelta = wanted - _dragAppliedTicks;

    if (tickDelta == 0 && trackDelta == 0 && laneDelta == 0)
        return;

    if (_registry->executeMerging(std::make_unique<app::MoveClipsCommand>(
            _model->selection(), tickDelta, trackDelta, laneDelta))) {
        _dragAppliedTicks  = wanted;
        _dragAppliedTracks += trackDelta;
        _dragAppliedLanes  += laneDelta;
        [self changed];
    }
}

/// How many lane bands the pointer has travelled within one row.
- (int)laneOffsetFrom:(CGFloat)fromY to:(CGFloat)toY row:(std::size_t)row
{
    const double band = _model->laneHeight(*_project, row);
    if (band <= 0.0)
        return 0;

    return static_cast<int>(std::floor((toY - fromY) / band + 0.5));
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
        const std::size_t locked = [self lockedCountIn:_model->selection()];

        if (!_model->selection().empty()
            && _registry->execute(std::make_unique<app::RemoveClipsCommand>(_model->selection()))) {
            _model->clearSelection();
            [self changed];
        }

        if (locked > 0)
            [self refuse:locked == 1
                             ? @"A locked clip was kept \u2014 unlock it to delete it."
                             : [NSString stringWithFormat:
                                   @"%lu locked clips were kept \u2014 unlock them to delete them.",
                                   static_cast<unsigned long>(locked)]];

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

    if (command && (character == 'g' || character == 'G')
        && !_model->selection().empty()) {
        if (shift) {
            [self commit:std::make_unique<app::UngroupClipsCommand>(_model->selection())];
        } else if (!_registry->execute(
                       std::make_unique<app::GroupClipsCommand>(_model->selection()))) {
            [self refuse:@"Select two or more clips to group them."];
        } else {
            [self changed];
        }

        return;
    }

    // M drops a marker at the playhead (or the viewport's left edge when the
    // transport has never rolled); Shift+M makes it a one-bar region.
    if (!command && (character == 'm' || character == 'M')) {
        const Tick tick   = _playheadTick >= 0 ? static_cast<Tick>(_playheadTick)
                                               : _model->viewport().firstTick;
        const Tick length = shift ? ticksPerQuarterNote * 4 : 0;

        const std::size_t count = _project->markers().size() + 1;
        NSString* name = [NSString stringWithFormat:@"%s %lu", shift ? "Region" : "Marker",
                                                    static_cast<unsigned long>(count)];

        if (_registry->execute(std::make_unique<app::AddMarkerCommand>(
                tick, std::string(name.UTF8String), length)))
            [self changed];

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

    if (track.type == project::TrackType::folder
        && NSPointInRect(view, [self disclosureRectForRow:row])) {
        [self commit:std::make_unique<app::SetTrackCollapsedCommand>(trackId, !track.collapsed)];
        _model->pruneSelection(*_project);
        return;
    }

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

    if (_project->tracks()[row].type == project::TrackType::folder) {
        [self refuse:@"A folder groups tracks \u2014 clips go on the rows inside it."];
        return;
    }

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

    NSMenuItem* pan = [menu addItemWithTitle:@"Pan\u2026"
                                      action:@selector(panFromMenu:)
                               keyEquivalent:@""];
    pan.target = self;

    NSMenuItem* reverse = [menu addItemWithTitle:@"Reverse"
                                          action:@selector(reverseFromMenu:)
                                   keyEquivalent:@""];
    reverse.target = self;

    NSMenuItem* unreverse = [menu addItemWithTitle:@"Play Forwards"
                                            action:@selector(unreverseFromMenu:)
                                     keyEquivalent:@""];
    unreverse.target = self;

    [menu addItem:[NSMenuItem separatorItem]];

    NSMenuItem* editLane = [menu addItemWithTitle:@"Edit Automation Lane"
                                           action:@selector(editLaneFromMenu:)
                                    keyEquivalent:@""];
    editLane.target = self;

    NSMenuItem* unique = [menu addItemWithTitle:@"Make Automation Unique"
                                         action:@selector(makeLaneUniqueFromMenu:)
                                  keyEquivalent:@""];
    unique.target = self;

    NSMenuItem* clear = [menu addItemWithTitle:@"Clear Automation"
                                        action:@selector(clearLaneFromMenu:)
                                 keyEquivalent:@""];
    clear.target = self;

    [menu addItem:[NSMenuItem separatorItem]];

    NSMenuItem* consolidate = [menu addItemWithTitle:@"Consolidate"
                                              action:@selector(consolidateFromMenu:)
                                       keyEquivalent:@""];
    consolidate.target = self;

    [menu addItem:[NSMenuItem separatorItem]];

    NSMenuItem* fadeIn = [menu addItemWithTitle:@"Fade In\u2026"
                                         action:@selector(fadeInFromMenu:)
                                  keyEquivalent:@""];
    fadeIn.target = self;

    NSMenuItem* fadeOut = [menu addItemWithTitle:@"Fade Out\u2026"
                                          action:@selector(fadeOutFromMenu:)
                                   keyEquivalent:@""];
    fadeOut.target = self;

    NSMenuItem* crossfade = [menu addItemWithTitle:@"Crossfade"
                                            action:@selector(crossfadeFromMenu:)
                                     keyEquivalent:@""];
    crossfade.target = self;

    NSMenuItem* uncrossfade = [menu addItemWithTitle:@"Remove Crossfade"
                                              action:@selector(removeCrossfadeFromMenu:)
                                       keyEquivalent:@""];
    uncrossfade.target = self;

    [menu addItem:[NSMenuItem separatorItem]];

    NSMenuItem* group = [menu addItemWithTitle:@"Group"
                                        action:@selector(groupFromMenu:)
                                 keyEquivalent:@""];
    group.target = self;

    NSMenuItem* ungroup = [menu addItemWithTitle:@"Ungroup"
                                          action:@selector(ungroupFromMenu:)
                                   keyEquivalent:@""];
    ungroup.target = self;

    NSMenuItem* colour = [menu addItemWithTitle:@"Colour" action:nil keyEquivalent:@""];
    colour.submenu = [self clipColourMenu];

    [menu addItem:[NSMenuItem separatorItem]];

    NSMenuItem* lock = [menu addItemWithTitle:@"Lock"
                                       action:@selector(lockFromMenu:)
                                keyEquivalent:@""];
    lock.target = self;

    NSMenuItem* unlock = [menu addItemWithTitle:@"Unlock"
                                         action:@selector(unlockFromMenu:)
                                  keyEquivalent:@""];
    unlock.target = self;

    [menu addItem:[NSMenuItem separatorItem]];

    NSMenuItem* remove = [menu addItemWithTitle:@"Remove"
                                         action:@selector(removeFromMenu:)
                                  keyEquivalent:@""];
    remove.target = self;

    [NSMenu popUpContextMenu:menu withEvent:event forView:self];
}

- (void)showTrackMenuFor:(project::EntityId)trackId event:(NSEvent*)event
{
    const project::Track* track = _project->findTrack(trackId);
    if (track == nullptr)
        return;

    // The menu's verbs all act on this row; the id rides on the item so a
    // right-click never depends on what happens to be selected.
    _menuTrack = trackId;

    NSMenu* menu = [[NSMenu alloc] init];

    NSMenuItem* rename = [menu addItemWithTitle:@"Rename Track…"
                                         action:@selector(renameTrackFromMenu:)
                                  keyEquivalent:@""];
    rename.target = self;
    rename.representedObject = @(trackId.value());

    NSMenuItem* colour = [menu addItemWithTitle:@"Colour" action:nil keyEquivalent:@""];
    colour.submenu = [self colourMenu];

    [menu addItem:[NSMenuItem separatorItem]];

    NSMenuItem* folder = [menu addItemWithTitle:@"New Folder"
                                         action:@selector(addFolderFromMenu:)
                                  keyEquivalent:@""];
    folder.target = self;

    NSMenuItem* automate = [menu addItemWithTitle:@"New Automation Clip"
                                           action:nil
                                    keyEquivalent:@""];
    automate.submenu = [self automationTargetMenu];

    if (track->type == project::TrackType::folder) {
        NSMenuItem* toggle =
            [menu addItemWithTitle:track->collapsed ? @"Expand" : @"Collapse"
                            action:@selector(toggleCollapseFromMenu:)
                     keyEquivalent:@""];
        toggle.target = self;
        toggle.representedObject = @(trackId.value());
    }

    NSMenu* into = [self folderMenuExcluding:trackId];
    if (into != nil) {
        NSMenuItem* move = [menu addItemWithTitle:@"Move Into Folder" action:nil keyEquivalent:@""];
        move.submenu = into;
    }

    if (track->parent.isValid()) {
        NSMenuItem* out = [menu addItemWithTitle:@"Move Out Of Folder"
                                          action:@selector(moveOutOfFolderFromMenu:)
                                   keyEquivalent:@""];
        out.target = self;
        out.representedObject = @(trackId.value());
    }

    [menu addItem:[NSMenuItem separatorItem]];

    NSMenuItem* remove =
        [menu addItemWithTitle:track->type == project::TrackType::folder ? @"Remove Folder"
                                                                         : @"Remove Track"
                        action:@selector(removeTrackFromMenu:)
                 keyEquivalent:@""];
    remove.target = self;
    remove.representedObject = @(trackId.value());

    [NSMenu popUpContextMenu:menu withEvent:event forView:self];
}

/// Every folder the row could be filed under, minus the ones that would close
/// a loop. Nil when the project has no folder to offer.
- (NSMenu*)folderMenuExcluding:(project::EntityId)trackId
{
    NSMenu* menu   = [[NSMenu alloc] init];
    bool    anyRow = false;

    for (const project::Track& candidate : _project->tracks()) {
        if (candidate.type != project::TrackType::folder)
            continue;
        if (candidate.id == trackId)
            continue;
        if (project::trackWouldCycle(*_project, trackId, candidate.id))
            continue;

        const project::Track* track = _project->findTrack(trackId);
        if (track != nullptr && track->parent == candidate.id)
            continue;   // already there

        NSMenuItem* item = [menu addItemWithTitle:@(candidate.name.c_str())
                                           action:@selector(moveIntoFolderFromMenu:)
                                    keyEquivalent:@""];
        item.target           = self;
        item.representedObject = @(candidate.id.value());
        anyRow                = true;
    }

    return anyRow ? menu : nil;
}

/// INCDAW's own track hues, the same set `Project::addTrack` rotates through.
/// A named list rather than the system colour panel: picking a track colour is
/// a one-click decision, and a modal panel makes it three.
- (NSMenu*)colourMenu
{
    static const struct { const char* name; unsigned int argb; } swatches[] = {
        {"Blue",   0xFF4A78C8u}, {"Teal",   0xFF3E9E96u}, {"Green",  0xFF5C9E4Au},
        {"Amber",  0xFFC8963Cu}, {"Orange", 0xFFC86E3Cu}, {"Red",    0xFFBE4A4Au},
        {"Pink",   0xFFB4569Eu}, {"Violet", 0xFF7E5CBEu}, {"Grey",   0xFF6E6E78u},
    };

    NSMenu* menu = [[NSMenu alloc] init];

    for (const auto& swatch : swatches) {
        NSMenuItem* item = [menu addItemWithTitle:@(swatch.name)
                                           action:@selector(setColourFromMenu:)
                                    keyEquivalent:@""];
        item.target            = self;
        item.representedObject = @(swatch.argb);
    }

    return menu;
}

/// Every mixer strip and channel, each offering the parameters a strip has.
///
/// The entry point the brief asks for is a control's own context menu in the
/// mixer; that lives in MixerView, which this track does not own, so the same
/// verb is offered from the arrangement side instead — which is where an
/// automation clip ends up anyway.
- (NSMenu*)automationTargetMenu
{
    static const struct { const char* title; const char* key; } parameters[] = {
        {"Volume", "volume"}, {"Pan", "pan"}, {"Stereo Width", "stereoSeparation"},
    };

    NSMenu* menu = [[NSMenu alloc] init];

    const auto addTarget = [&](project::EntityId target, const std::string& name) {
        NSMenuItem* entry = [menu addItemWithTitle:@(name.c_str()) action:nil keyEquivalent:@""];

        NSMenu* keys = [[NSMenu alloc] init];

        for (const auto& parameter : parameters) {
            NSMenuItem* item = [keys addItemWithTitle:@(parameter.title)
                                               action:@selector(createAutomationFromMenu:)
                                        keyEquivalent:@""];
            item.target = self;
            item.representedObject =
                @[@(target.value()), @(parameter.key)];
        }

        entry.submenu = keys;
    };

    for (const project::MixerNode& node : _project->mixerNodes())
        addTarget(node.id, node.name);

    for (const project::Channel& channel : _project->channels())
        addTarget(channel.id, channel.name);

    return menu;
}

- (void)createAutomationFromMenu:(NSMenuItem*)item
{
    NSArray* pair = item.representedObject;
    if (pair.count != 2)
        return;

    const project::EntityId target{[pair[0] unsignedLongLongValue]};
    const std::string       key = [pair[1] UTF8String];

    // Four bars from the playhead, or from the left edge of the viewport when
    // the transport has never rolled.
    const Tick start = _playheadTick >= 0 ? static_cast<Tick>(_playheadTick)
                                          : _model->viewport().firstTick;

    // Seeded at the control's own value, so the clip opens on the line the
    // parameter is already sitting at.
    double value = 0.5;
    if (const project::MixerNode* node = _project->findMixerNode(target)) {
        if (key == "volume")
            value = std::clamp(node->volume, 0.0, 1.0);
        else if (key == "pan")
            value = std::clamp((node->pan + 1.0) * 0.5, 0.0, 1.0);
        else if (key == "stereoSeparation")
            value = std::clamp((node->stereoSeparation + 1.0) * 0.5, 0.0, 1.0);
    } else if (const project::Channel* channel = _project->findChannel(target)) {
        if (key == "volume")
            value = std::clamp(channel->volume, 0.0, 1.0);
        else if (key == "pan")
            value = std::clamp((channel->pan + 1.0) * 0.5, 0.0, 1.0);
    }

    auto command = std::make_unique<app::CreateAutomationClipCommand>(
        target, key, _model->snapTick(start), ticksPerQuarterNote * 4 * 4, value);

    app::CreateAutomationClipCommand* raw = command.get();

    if (!_registry->execute(std::move(command))) {
        [self refuse:@"That parameter already has a clip here."];
        return;
    }

    _model->setSelection({raw->clipId()});
    [self changed];
    [self openAutomationLane:raw->laneId()];
}

- (void)addFolderFromMenu:(id)sender
{
    (void)sender;

    [self commit:std::make_unique<app::AddTrackCommand>(
                     "Folder " + std::to_string(_project->tracks().size() + 1),
                     project::TrackType::folder)];
}

- (void)toggleCollapseFromMenu:(NSMenuItem*)item
{
    const project::EntityId trackId{[item.representedObject unsignedLongLongValue]};

    const project::Track* track = _project->findTrack(trackId);
    if (track == nullptr)
        return;

    [self commit:std::make_unique<app::SetTrackCollapsedCommand>(trackId, !track->collapsed)];
    _model->pruneSelection(*_project);
}

- (void)moveIntoFolderFromMenu:(NSMenuItem*)item
{
    const project::EntityId folder{[item.representedObject unsignedLongLongValue]};

    if (!_registry->execute(std::make_unique<app::SetTrackParentCommand>(_menuTrack, folder))) {
        [self refuse:@"That folder cannot hold this track."];
        return;
    }

    [self changed];
}

- (void)moveOutOfFolderFromMenu:(NSMenuItem*)item
{
    const project::EntityId trackId{[item.representedObject unsignedLongLongValue]};
    [self commit:std::make_unique<app::SetTrackParentCommand>(trackId, project::EntityId{})];
}

- (void)setColourFromMenu:(NSMenuItem*)item
{
    const auto argb = static_cast<std::uint32_t>([item.representedObject unsignedIntValue]);
    [self commit:std::make_unique<app::SetTrackColourCommand>(_menuTrack, argb)];
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

/// The first automation clip in the selection, or an invalid id.
- (project::EntityId)selectedAutomationClip
{
    for (const project::EntityId id : _model->selection())
        if (const project::Clip* clip = _project->findClip(id);
            clip != nullptr && clip->type == project::ClipType::automation)
            return clip->id;

    return {};
}

- (void)editLaneFromMenu:(id)sender
{
    (void)sender;

    const project::EntityId id = [self selectedAutomationClip];
    if (const project::Clip* clip = _project->findClip(id))
        [self openAutomationLane:clip->source];
    else
        [self refuse:@"Select an automation clip."];
}

- (void)makeLaneUniqueFromMenu:(id)sender
{
    (void)sender;

    const project::EntityId id = [self selectedAutomationClip];
    if (!id.isValid()) {
        [self refuse:@"Select an automation clip."];
        return;
    }

    if (!_registry->execute(std::make_unique<app::MakeAutomationClipUniqueCommand>(id))) {
        [self refuse:@"This clip is already the only one on its lane."];
        return;
    }

    [self changed];
}

- (void)clearLaneFromMenu:(id)sender
{
    (void)sender;

    const project::EntityId id = [self selectedAutomationClip];
    const project::Clip* clip  = _project->findClip(id);

    if (clip == nullptr) {
        [self refuse:@"Select an automation clip."];
        return;
    }

    [self commit:std::make_unique<app::SetAutomationPointsCommand>(
                     clip->source, std::vector<project::AutomationPoint>{},
                     "Clear Automation")];
}

- (void)consolidateFromMenu:(id)sender
{
    (void)sender;

    // Asked before the command is built: a command the registry refuses is
    // destroyed on the way out, so there is nothing left to ask afterwards.
    const std::string refusal =
        app::consolidationRefusal(*_project, _model->selection());

    if (!refusal.empty()) {
        [self refuse:@(refusal.c_str())];
        return;
    }

    NSString* music = [NSSearchPathForDirectoriesInDomains(NSMusicDirectory,
                                                           NSUserDomainMask, YES) firstObject];
    if (music == nil) {
        [self refuse:@"No Music folder to write the render into."];
        return;
    }

    const auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm parts{};
    localtime_r(&now, &parts);

    char stamp[48];
    std::snprintf(stamp, sizeof(stamp), "consolidated-%04d%02d%02d-%02d%02d%02d.wav",
                  parts.tm_year + 1900, parts.tm_mon + 1, parts.tm_mday,
                  parts.tm_hour, parts.tm_min, parts.tm_sec);

    const std::filesystem::path output =
        std::filesystem::path{music.UTF8String} / "INCDAW" / "Consolidated" / stamp;

    auto command = std::make_unique<app::ConsolidateClipsCommand>(_model->selection(), output,
                                                                   self.parameterRegistry);
    app::ConsolidateClipsCommand* raw = command.get();

    // The render runs here, on this thread. A long selection will make the
    // window wait; a background render with progress is the next increment,
    // not a silent one that leaves the project half-edited.
    if (!_registry->execute(std::move(command))) {
        [self refuse:@"The selection did not render."];
        return;
    }

    _model->setSelection({raw->consolidatedClip()});
    [self invalidateWaveformCache];
    [self changed];
}

- (void)crossfadeFromMenu:(id)sender
{
    (void)sender;

    if (!_registry->execute(
            std::make_unique<app::CrossfadeClipsCommand>(_model->selection(), true))) {
        [self refuse:@"Select two audio clips that overlap on one lane."];
        return;
    }

    [self changed];
}

- (void)removeCrossfadeFromMenu:(id)sender
{
    (void)sender;
    [self commit:std::make_unique<app::CrossfadeClipsCommand>(_model->selection(), false)];
}

- (void)fadeInFromMenu:(id)sender { (void)sender; [self editFadeIncoming:YES]; }
- (void)fadeOutFromMenu:(id)sender { (void)sender; [self editFadeIncoming:NO]; }

/// The fade length in milliseconds, not frames: a fade is a duration the ear
/// judges, and the sample rate is not the user's problem.
- (void)editFadeIncoming:(BOOL)incoming
{
    const app::ClipIds& selection = _model->selection();
    if (selection.empty())
        return;

    const project::Clip* first = nullptr;
    for (const project::EntityId id : selection) {
        const project::Clip* clip = _project->findClip(id);
        if (clip != nullptr && clip->type == project::ClipType::audio) {
            first = clip;
            break;
        }
    }

    if (first == nullptr) {
        [self refuse:@"Clip fades apply to audio clips."];
        return;
    }

    const double rate = _project->tempoMap().sampleRate() > 0.0
                            ? _project->tempoMap().sampleRate()
                            : 48000.0;

    const auto current = static_cast<double>(incoming ? first->fadeInFrames
                                                      : first->fadeOutFrames);

    NSAlert* alert = [[NSAlert alloc] init];
    alert.messageText = incoming ? @"Fade in" : @"Fade out";
    alert.informativeText = @"Length in milliseconds.";
    [alert addButtonWithTitle:@"Set"];
    [alert addButtonWithTitle:@"Cancel"];

    NSTextField* field = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 160, 24)];
    field.stringValue = [NSString stringWithFormat:@"%.0f", current / rate * 1000.0];
    alert.accessoryView = field;

    if ([alert runModal] != NSAlertFirstButtonReturn)
        return;

    const auto frames = static_cast<long long>(
        std::max(0.0, field.doubleValue) / 1000.0 * rate);

    [self commit:std::make_unique<app::SetClipFadesCommand>(selection,
                                                            incoming ? frames : -1,
                                                            incoming ? -1 : frames)];
}

- (void)groupFromMenu:(id)sender
{
    (void)sender;

    if (!_registry->execute(std::make_unique<app::GroupClipsCommand>(_model->selection()))) {
        [self refuse:@"Select two or more clips to group them."];
        return;
    }

    [self changed];
}

- (void)ungroupFromMenu:(id)sender
{
    (void)sender;
    [self commit:std::make_unique<app::UngroupClipsCommand>(_model->selection())];
}

/// The clip palette, matching the track menu's: a colour is a one-click
/// decision, and the group takes it together.
- (NSMenu*)clipColourMenu
{
    static const struct { const char* name; unsigned int argb; } swatches[] = {
        {"Blue",   0xFF6699CCu}, {"Teal",   0xFF3E9E96u}, {"Green",  0xFF5C9E4Au},
        {"Amber",  0xFFC8963Cu}, {"Orange", 0xFFC86E3Cu}, {"Red",    0xFFBE4A4Au},
        {"Pink",   0xFFB4569Eu}, {"Violet", 0xFF7E5CBEu}, {"Grey",   0xFF6E6E78u},
    };

    NSMenu* menu = [[NSMenu alloc] init];

    for (const auto& swatch : swatches) {
        NSMenuItem* item = [menu addItemWithTitle:@(swatch.name)
                                           action:@selector(setClipColourFromMenu:)
                                    keyEquivalent:@""];
        item.target            = self;
        item.representedObject = @(swatch.argb);
    }

    return menu;
}

- (void)setClipColourFromMenu:(NSMenuItem*)item
{
    const auto argb = static_cast<std::uint32_t>([item.representedObject unsignedIntValue]);
    [self commit:std::make_unique<app::SetClipColourCommand>(_model->selection(), argb)];
}

- (void)lockFromMenu:(id)sender
{
    (void)sender;
    [self commit:std::make_unique<app::SetClipLockedCommand>(_model->selection(), true)];
}

- (void)unlockFromMenu:(id)sender
{
    (void)sender;
    [self commit:std::make_unique<app::SetClipLockedCommand>(_model->selection(), false)];
}

- (void)reverseFromMenu:(id)sender
{
    (void)sender;
    [self commit:std::make_unique<app::SetClipReversedCommand>(_model->selection(), true)];
}

- (void)unreverseFromMenu:(id)sender
{
    (void)sender;
    [self commit:std::make_unique<app::SetClipReversedCommand>(_model->selection(), false)];
}

- (void)panFromMenu:(id)sender
{
    (void)sender;
    [self editPan];
}

/// One dialog for the whole selection, seeded from the first clip in it: a
/// multi-clip edit sets them all to one value, which is what a single control
/// over several clips can honestly mean.
- (void)editPan
{
    const app::ClipIds& selection = _model->selection();
    if (selection.empty())
        return;

    double current = 0.0;
    for (const project::EntityId id : selection) {
        if (const project::Clip* clip = _project->findClip(id)) {
            current = clip->pan;
            break;
        }
    }

    NSAlert* alert = [[NSAlert alloc] init];
    alert.messageText = @"Clip pan";
    alert.informativeText = @"Left \u2013 centre \u2013 right.";
    [alert addButtonWithTitle:@"Set"];
    [alert addButtonWithTitle:@"Cancel"];

    NSSlider* slider = [NSSlider sliderWithValue:current * 100.0
                                        minValue:-100.0
                                        maxValue:100.0
                                          target:nil
                                          action:nil];
    slider.frame = NSMakeRect(0, 0, 220, 24);
    slider.numberOfTickMarks = 3;
    slider.allowsTickMarkValuesOnly = NO;
    alert.accessoryView = slider;

    if ([alert runModal] != NSAlertFirstButtonReturn)
        return;

    [self commit:std::make_unique<app::SetClipPanCommand>(selection, slider.doubleValue / 100.0)];
}

- (void)removeFromMenu:(id)sender
{
    (void)sender;

    const std::size_t locked = [self lockedCountIn:_model->selection()];

    if (_registry->execute(std::make_unique<app::RemoveClipsCommand>(_model->selection()))) {
        _model->clearSelection();
        [self changed];
    }

    if (locked > 0)
        [self refuse:@"A locked clip was kept \u2014 unlock it to delete it."];
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

/// Opens the lane in its own panel.
///
/// A panel rather than a fifth tab in the editor switcher: the switcher's
/// segments belong to the control bar, and an automation lane is edited
/// ALONGSIDE the arrangement it sits in rather than instead of it.
- (void)openAutomationLane:(project::EntityId)lane
{
    if (_automationEditor == nil) {
        const NSRect frame = NSMakeRect(0, 0, 720, 260);

        _automationEditor = [[INCDAWAutomationEditorView alloc]
            initWithFrame:frame
                  project:_project
                 registry:_registry];

        __weak INCDAWPlaylistView* weakSelf = self;
        _automationEditor.onChange = ^{
            [weakSelf changed];
        };

        _automationPanel = [[NSPanel alloc]
            initWithContentRect:frame
                      styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable
                               | NSWindowStyleMaskResizable | NSWindowStyleMaskUtilityWindow
                        backing:NSBackingStoreBuffered
                          defer:NO];

        _automationPanel.releasedWhenClosed = NO;
        _automationPanel.contentView        = _automationEditor;
        _automationEditor.autoresizingMask  = NSViewWidthSizable | NSViewHeightSizable;
    }

    _automationEditor.laneIdValue = lane.value();
    _automationEditor.playheadTick = _playheadTick;
    _automationPanel.title         = @"Automation";

    [_automationPanel makeKeyAndOrderFront:nil];
    [_automationPanel makeFirstResponder:_automationEditor];
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
    _automationEditor.playheadTick = tick;
    [self setNeedsDisplay:YES];
}

@end
