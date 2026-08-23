#include "ui/macos/AutomationEditorView.h"

#include "app/AutomationEditorModel.h"
#include "app/CommandRegistry.h"
#include "app/PianoRollHeaderModel.h"
#include "app/commands/AutomationCommands.h"
#include "project/Model.h"
#include "ui/macos/Theme.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <vector>

using namespace incdaw;
using incdaw::engine::Tick;
using incdaw::engine::ticksPerQuarterNote;

namespace theme = incdaw::ui::theme;

namespace {

using theme::Ink;
using theme::fillRect;

/// The band above the curve where bars are numbered, and the strip below it
/// where the grid and the lane's name live.
constexpr CGFloat rulerHeight  = theme::metrics::rulerHeight;
constexpr CGFloat headerHeight = 26.0;

constexpr Tick bar = ticksPerQuarterNote * 4;

enum class EditorDrag { none, point, box };

} // namespace

@implementation INCDAWAutomationEditorView {
    project::Project*     _project;
    app::CommandRegistry* _registry;

    std::unique_ptr<app::AutomationEditorModel> _model;

    /// The clipboard, rebased to zero — shared across lanes on purpose: a lane
    /// is a list of normalised points and nothing else, so a filter sweep can
    /// be pasted onto a send level.
    std::vector<project::AutomationPoint> _clipboard;

    /// Sampled curve and box-select scratch, reused across frames so a steady
    /// state allocates nothing.
    std::vector<double>      _curve;
    std::vector<std::size_t> _boxed;

    EditorDrag _drag;
    NSPoint    _dragOrigin;
    NSPoint    _dragCurrent;
    Tick       _dragAppliedTicks;
    double     _dragAppliedValue;

    app::PianoRollHeaderModel::Snap _snap;
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
    _model    = std::make_unique<app::AutomationEditorModel>();

    _drag             = EditorDrag::none;
    _dragAppliedTicks = 0;
    _dragAppliedValue = 0.0;
    _playheadTick     = -1;
    _statusText       = @"No lane";

    // The Piano Roll's own grid vocabulary rather than a second one: the two
    // editors snap to the same divisions and say so with the same words.
    _snap = app::PianoRollHeaderModel::Snap::sixteenth;
    _model->setSnap(app::PianoRollHeaderModel::ticksFor(_snap));

    [self refreshViewport];
    return self;
}

- (BOOL)isFlipped { return YES; }
- (BOOL)acceptsFirstResponder { return YES; }

- (void)setFrameSize:(NSSize)size
{
    [super setFrameSize:size];
    [self refreshViewport];
}

- (void)refreshViewport
{
    app::AutomationEditorModel::Viewport viewport = _model->viewport();
    viewport.width       = self.bounds.size.width;
    viewport.height      = std::max(0.0, self.bounds.size.height - headerHeight);
    viewport.rulerHeight = rulerHeight;
    _model->setViewport(viewport);
}

// ── The lane ─────────────────────────────────────────────────────────────────

- (project::AutomationLane*)lane
{
    if (_project == nullptr || _laneIdValue == 0)
        return nullptr;

    const project::EntityId id{_laneIdValue};

    for (project::AutomationLane& lane : _project->automation())
        if (lane.id == id)
            return &lane;

    return nullptr;
}

- (void)setLaneIdValue:(unsigned long long)value
{
    _laneIdValue = value;
    _model->clearSelection();
    [self describe:nil];
    [self setNeedsDisplay:YES];
}

/// The lane's own line: what it automates, and what just happened to it. This
/// is also the answer to "which parameter is this?", which a lane in the
/// playlist could not previously be asked.
- (void)describe:(NSString*)action
{
    const project::AutomationLane* lane = [self lane];
    if (lane == nullptr) {
        _statusText = @"No lane";
        return;
    }

    NSString* target = @"—";
    if (const project::MixerNode* node = _project->findMixerNode(lane->targetEntity))
        target = @(node->name.c_str());
    else if (const project::Channel* channel = _project->findChannel(lane->targetEntity))
        target = @(channel->name.c_str());

    NSString* head = [NSString stringWithFormat:@"%@ · %s · %lu points",
                                                target, lane->parameterKey.c_str(),
                                                static_cast<unsigned long>(lane->points.size())];

    _statusText = action == nil ? head : [NSString stringWithFormat:@"%@ · %@", head, action];
}

/// Hands the lane its next point vector. Mergeable when a gesture is in
/// flight, so a drag is one undo entry.
- (void)commitPoints:(std::vector<project::AutomationPoint>)points
                name:(NSString*)gesture
             merging:(BOOL)merging
{
    const project::AutomationLane* lane = [self lane];
    if (lane == nullptr)
        return;

    auto command = std::make_unique<app::SetAutomationPointsCommand>(
        lane->id, std::move(points), gesture.UTF8String);

    const bool applied = merging ? _registry->executeMerging(std::move(command))
                                 : _registry->execute(std::move(command));

    if (!applied)
        return;

    if (const project::AutomationLane* after = [self lane])
        _model->pruneSelection(after->points.size());

    [self describe:gesture];
    [self changed];
}

- (void)changed
{
    [self setNeedsDisplay:YES];

    if (self.onChange != nil)
        self.onChange();
}

// ── Drawing ──────────────────────────────────────────────────────────────────

- (void)drawRect:(NSRect)dirtyRect
{
    (void)dirtyRect;

    fillRect(self.bounds, theme::ink(Ink::panel));

    [self drawHeader];

    const project::AutomationLane* lane = [self lane];
    if (lane == nullptr) {
        theme::drawTextCentred(@"Double-click an automation clip to edit its lane",
                               self.bounds, theme::ink(Ink::textDim),
                               theme::labelFont(12.0));
        return;
    }

    [self drawGrid];
    [self drawCurve:lane->points];
    [self drawPoints:lane->points];
    [self drawPlayhead];

    if (_drag == EditorDrag::box) {
        const NSRect box = NSMakeRect(std::min(_dragOrigin.x, _dragCurrent.x),
                                      std::min(_dragOrigin.y, _dragCurrent.y) + headerHeight,
                                      std::abs(_dragCurrent.x - _dragOrigin.x),
                                      std::abs(_dragCurrent.y - _dragOrigin.y));

        fillRect(box, theme::ink(Ink::selectionFill));
        theme::strokeRounded(box, 2.0, theme::ink(Ink::selectionStroke));
    }
}

- (void)drawHeader
{
    const NSRect header = NSMakeRect(0, 0, self.bounds.size.width, headerHeight);
    theme::drawPanel(header, 0.0, false, true);

    theme::drawText([NSString stringWithFormat:@"Grid  %s",
                                               app::PianoRollHeaderModel::snapName(_snap)],
                    NSMakeRect(10.0, 6.0, 140.0, 14.0),
                    theme::ink(Ink::textSecondary), theme::labelFont(11.0));

    theme::drawText(_statusText,
                    NSMakeRect(160.0, 6.0, self.bounds.size.width - 170.0, 14.0),
                    theme::ink(Ink::textPrimary), theme::labelFont(11.0));
}

- (void)drawGrid
{
    const CGFloat top    = headerHeight + rulerHeight;
    const CGFloat bottom = self.bounds.size.height;

    fillRect(NSMakeRect(0, top, self.bounds.size.width, bottom - top),
             theme::ink(Ink::panelSunken));

    // Half, then full, so a fader's own midpoint is readable at a glance.
    for (const double value : {0.5, 0.0, 1.0}) {
        const CGFloat y = headerHeight + static_cast<CGFloat>(_model->valueToY(value));
        fillRect(NSMakeRect(0, y, self.bounds.size.width, 1.0),
                 theme::withAlpha(theme::ink(Ink::gridLine), value == 0.5 ? 0.9 : 0.5));
    }

    const auto& viewport = _model->viewport();
    const Tick  step     = _model->snap() > 0 ? _model->snap() : ticksPerQuarterNote;

    const Tick from = (viewport.firstTick / step) * step;
    const Tick to   = viewport.firstTick + viewport.visibleTicks;

    for (Tick tick = from; tick <= to; tick += step) {
        const auto x = static_cast<CGFloat>(_model->tickToX(tick));
        if (x < 0.0 || x > self.bounds.size.width)
            continue;

        const bool onBar = tick % bar == 0;

        fillRect(NSMakeRect(x, top, 1.0, bottom - top),
                 onBar ? theme::ink(Ink::gridLineStrong) : theme::ink(Ink::gridLine));

        if (onBar)
            theme::drawText([NSString stringWithFormat:@"%lld", tick / bar + 1],
                            NSMakeRect(x + 3.0, headerHeight + 3.0, 40.0, 12.0),
                            theme::ink(Ink::textDim), theme::labelFont(9.0));
    }
}

- (void)drawCurve:(const std::vector<project::AutomationPoint>&)points
{
    constexpr double step = 2.0;
    _model->collectCurve(points, _curve, step);

    if (_curve.size() < 2)
        return;

    NSBezierPath* path = [NSBezierPath bezierPath];
    NSBezierPath* fill = [NSBezierPath bezierPath];

    const CGFloat floorY = headerHeight + static_cast<CGFloat>(_model->valueToY(0.0));

    for (std::size_t index = 0; index < _curve.size(); ++index) {
        const auto x = static_cast<CGFloat>(static_cast<double>(index) * step);
        const auto y = headerHeight + static_cast<CGFloat>(_model->valueToY(_curve[index]));

        if (index == 0) {
            [path moveToPoint:NSMakePoint(x, y)];
            [fill moveToPoint:NSMakePoint(x, floorY)];
            [fill lineToPoint:NSMakePoint(x, y)];
        } else {
            [path lineToPoint:NSMakePoint(x, y)];
            [fill lineToPoint:NSMakePoint(x, y)];
        }
    }

    [fill lineToPoint:NSMakePoint(static_cast<CGFloat>(static_cast<double>(_curve.size() - 1) * step),
                                  floorY)];
    [fill closePath];

    [theme::withAlpha(theme::ink(Ink::automation), 0.18) setFill];
    [fill fill];

    path.lineWidth = 1.6;
    [theme::ink(Ink::automation) setStroke];
    [path stroke];
}

- (void)drawPoints:(const std::vector<project::AutomationPoint>&)points
{
    for (std::size_t index = 0; index < points.size(); ++index) {
        const auto rect = _model->pointRect(points[index]);

        const NSRect handle = NSMakeRect(static_cast<CGFloat>(rect.x),
                                         headerHeight + static_cast<CGFloat>(rect.y),
                                         static_cast<CGFloat>(rect.width),
                                         static_cast<CGFloat>(rect.height));

        if (NSMaxX(handle) < 0.0 || NSMinX(handle) > self.bounds.size.width)
            continue;

        const bool selected = _model->isSelected(index);

        NSBezierPath* dot = [NSBezierPath bezierPathWithOvalInRect:NSInsetRect(handle, 1.5, 1.5)];

        [(selected ? theme::ink(Ink::accent) : theme::ink(Ink::panelRaised)) setFill];
        [dot fill];

        [(selected ? theme::ink(Ink::textOnAccent) : theme::ink(Ink::automation)) setStroke];
        dot.lineWidth = 1.4;
        [dot stroke];

        // A held segment gets a tick on its point, so the shape of the
        // envelope is readable without clicking anything.
        if (points[index].curve == project::AutomationCurve::hold)
            fillRect(NSMakeRect(NSMidX(handle) - 0.5, NSMinY(handle) - 4.0, 1.0, 3.0),
                     theme::ink(Ink::automation));
    }
}

- (void)drawPlayhead
{
    if (_playheadTick < 0)
        return;

    const auto x = static_cast<CGFloat>(_model->tickToX(static_cast<Tick>(_playheadTick)));
    if (x < 0.0 || x > self.bounds.size.width)
        return;

    fillRect(NSMakeRect(x, headerHeight, 1.0, self.bounds.size.height - headerHeight),
             theme::ink(Ink::playhead));
}

// ── Mouse ────────────────────────────────────────────────────────────────────

/// View coordinates with the header taken off, which is what the model works
/// in: the model knows about a ruler and a curve band, not about chrome.
- (NSPoint)editorPointFor:(NSEvent*)event
{
    const NSPoint view = [self convertPoint:event.locationInWindow fromView:nil];
    return NSMakePoint(view.x, view.y - headerHeight);
}

- (void)mouseDown:(NSEvent*)event
{
    project::AutomationLane* lane = [self lane];
    if (lane == nullptr)
        return;

    const NSPoint point = [self editorPointFor:event];

    _drag             = EditorDrag::none;
    _dragOrigin       = point;
    _dragCurrent      = point;
    _dragAppliedTicks = 0;
    _dragAppliedValue = 0.0;

    if (point.y < 0.0)
        return;   // in the header

    const std::size_t hit = _model->pointAt(lane->points, point.x, point.y);

    if (hit == app::AutomationEditorModel::noPoint) {
        if ((event.modifierFlags & NSEventModifierFlagShift) != 0) {
            _drag = EditorDrag::box;
            [self setNeedsDisplay:YES];
            return;
        }

        // Clicking empty space draws a point there — the fastest way to shape
        // an envelope, and the reason the grid has a resolution.
        const Tick   tick  = _model->snapTick(_model->xToTick(point.x));
        const double value = _model->yToValue(point.y);

        [self commitPoints:app::AutomationEditorModel::withPointAdded(lane->points, tick, value)
                      name:@"Draw Automation"
                   merging:NO];

        if (const project::AutomationLane* after = [self lane]) {
            const std::size_t added = _model->pointAt(after->points,
                                                      _model->tickToX(tick),
                                                      _model->valueToY(value));
            if (added != app::AutomationEditorModel::noPoint) {
                _model->setSelection({added});
                _drag = EditorDrag::point;
            }
        }

        [self setNeedsDisplay:YES];
        return;
    }

    // Option-click erases: the same "the modifier is the eraser" the playlist
    // uses for its slice gesture.
    if ((event.modifierFlags & NSEventModifierFlagOption) != 0) {
        [self commitPoints:app::AutomationEditorModel::withPointsRemoved(lane->points, {hit})
                      name:@"Erase Automation"
                   merging:NO];

        _model->clearSelection();
        [self setNeedsDisplay:YES];
        return;
    }

    if ((event.modifierFlags & NSEventModifierFlagShift) != 0)
        _model->toggleSelection(hit);
    else if (!_model->isSelected(hit))
        _model->setSelection({hit});

    _drag = EditorDrag::point;
    [self setNeedsDisplay:YES];
}

- (void)mouseDragged:(NSEvent*)event
{
    if (_drag == EditorDrag::none)
        return;

    const project::AutomationLane* lane = [self lane];
    if (lane == nullptr)
        return;

    _dragCurrent = [self editorPointFor:event];

    if (_drag == EditorDrag::box) {
        [self setNeedsDisplay:YES];
        return;
    }

    if (_model->selection().empty())
        return;

    // Deltas against what has already been applied, so the gesture produces
    // one merged command rather than a fight between absolute and relative.
    const Tick wantedTicks = _model->snapTick(_model->xToTick(_dragCurrent.x)
                                              - _model->xToTick(_dragOrigin.x));
    const double wantedValue = _model->yToValue(_dragCurrent.y)
                             - _model->yToValue(_dragOrigin.y);

    const Tick   tickDelta  = wantedTicks - _dragAppliedTicks;
    const double valueDelta = wantedValue - _dragAppliedValue;

    if (tickDelta == 0 && std::abs(valueDelta) < 1.0e-6)
        return;

    const auto selection = _model->selection();

    [self commitPoints:app::AutomationEditorModel::withPointsMoved(lane->points, selection,
                                                                   tickDelta, valueDelta)
                  name:@"Move Automation"
               merging:YES];

    _dragAppliedTicks = wantedTicks;
    _dragAppliedValue = wantedValue;
}

- (void)mouseUp:(NSEvent*)event
{
    (void)event;

    if (_drag == EditorDrag::box) {
        if (const project::AutomationLane* lane = [self lane]) {
            _model->pointsInRectangle(lane->points,
                                      std::min(_dragOrigin.x, _dragCurrent.x),
                                      std::min(_dragOrigin.y, _dragCurrent.y),
                                      std::abs(_dragCurrent.x - _dragOrigin.x),
                                      std::abs(_dragCurrent.y - _dragOrigin.y),
                                      _boxed);
            _model->setSelection(_boxed);
        }
    }

    _drag = EditorDrag::none;
    [self setNeedsDisplay:YES];
}

- (void)rightMouseDown:(NSEvent*)event
{
    const project::AutomationLane* lane = [self lane];
    if (lane == nullptr)
        return;

    const NSPoint point = [self editorPointFor:event];

    // A right-click acts on the selection, or on what is under the pointer
    // when nothing is selected — the segment included, since curve and tension
    // belong to the point that starts one.
    if (_model->selection().empty()) {
        const std::size_t hit = _model->pointAt(lane->points, point.x, point.y);

        if (hit != app::AutomationEditorModel::noPoint) {
            _model->setSelection({hit});
        } else {
            const std::size_t segment = _model->segmentAt(lane->points, point.x);
            if (segment != app::AutomationEditorModel::noPoint)
                _model->setSelection({segment});
        }

        [self setNeedsDisplay:YES];
    }

    if (_model->selection().empty())
        return;

    NSMenu* menu = [[NSMenu alloc] init];

    const struct { const char* title; int curve; } curves[] = {
        {"Linear", 0}, {"Hold", 1}, {"Smooth", 2}, {"Exponential", 3},
    };

    for (const auto& entry : curves) {
        NSMenuItem* item = [menu addItemWithTitle:@(entry.title)
                                           action:@selector(setCurveFromMenu:)
                                    keyEquivalent:@""];
        item.target            = self;
        item.representedObject = @(entry.curve);
    }

    [menu addItem:[NSMenuItem separatorItem]];

    NSMenuItem* tension = [menu addItemWithTitle:@"Tension" action:nil keyEquivalent:@""];
    tension.submenu = [self tensionMenu];

    NSMenuItem* scaleTime = [menu addItemWithTitle:@"Scale in Time" action:nil keyEquivalent:@""];
    scaleTime.submenu = [self scaleMenuForTime:YES];

    NSMenuItem* scaleValue = [menu addItemWithTitle:@"Scale in Value" action:nil keyEquivalent:@""];
    scaleValue.submenu = [self scaleMenuForTime:NO];

    [menu addItem:[NSMenuItem separatorItem]];

    NSMenuItem* erase = [menu addItemWithTitle:@"Erase"
                                        action:@selector(eraseFromMenu:)
                                 keyEquivalent:@""];
    erase.target = self;

    [NSMenu popUpContextMenu:menu withEvent:event forView:self];
}

- (NSMenu*)tensionMenu
{
    NSMenu* menu = [[NSMenu alloc] init];

    for (const int percent : {-100, -50, 0, 50, 100}) {
        NSMenuItem* item =
            [menu addItemWithTitle:[NSString stringWithFormat:@"%+d%%", percent]
                            action:@selector(setTensionFromMenu:)
                     keyEquivalent:@""];
        item.target            = self;
        item.representedObject = @(percent);
    }

    return menu;
}

- (NSMenu*)scaleMenuForTime:(BOOL)inTime
{
    NSMenu* menu = [[NSMenu alloc] init];

    for (const int percent : {25, 50, 200, 400}) {
        NSMenuItem* item =
            [menu addItemWithTitle:[NSString stringWithFormat:@"%d%%", percent]
                            action:inTime ? @selector(scaleTimeFromMenu:)
                                          : @selector(scaleValueFromMenu:)
                     keyEquivalent:@""];
        item.target            = self;
        item.representedObject = @(percent);
    }

    return menu;
}

- (void)setCurveFromMenu:(NSMenuItem*)item
{
    const project::AutomationLane* lane = [self lane];
    if (lane == nullptr)
        return;

    const auto curve = static_cast<project::AutomationCurve>([item.representedObject intValue]);

    [self commitPoints:app::AutomationEditorModel::withCurve(lane->points,
                                                              _model->selection(), curve)
                  name:@"Set Automation Curve"
               merging:NO];
}

- (void)setTensionFromMenu:(NSMenuItem*)item
{
    const project::AutomationLane* lane = [self lane];
    if (lane == nullptr)
        return;

    [self commitPoints:app::AutomationEditorModel::withTension(
                           lane->points, _model->selection(),
                           [item.representedObject doubleValue] / 100.0)
                  name:@"Set Automation Tension"
               merging:NO];
}

/// The earliest selected point anchors a time scale, and the mean of the
/// selection anchors a value scale: both keep the gesture where the user is
/// looking rather than dragging it back to the origin.
- (void)scaleTimeFromMenu:(NSMenuItem*)item
{
    const project::AutomationLane* lane = [self lane];
    if (lane == nullptr || _model->selection().empty())
        return;

    const Tick anchor = lane->points[_model->selection().front()].tick;

    [self commitPoints:app::AutomationEditorModel::withTimeScaled(
                           lane->points, _model->selection(),
                           [item.representedObject doubleValue] / 100.0, anchor)
                  name:@"Scale Automation"
               merging:NO];
}

- (void)scaleValueFromMenu:(NSMenuItem*)item
{
    const project::AutomationLane* lane = [self lane];
    if (lane == nullptr || _model->selection().empty())
        return;

    double sum = 0.0;
    for (const std::size_t index : _model->selection())
        sum += lane->points[index].value;

    const double anchor = sum / static_cast<double>(_model->selection().size());

    [self commitPoints:app::AutomationEditorModel::withValueScaled(
                           lane->points, _model->selection(),
                           [item.representedObject doubleValue] / 100.0, anchor)
                  name:@"Scale Automation"
               merging:NO];
}

- (void)eraseFromMenu:(id)sender { (void)sender; [self eraseSelection]; }

- (void)eraseSelection
{
    const project::AutomationLane* lane = [self lane];
    if (lane == nullptr || _model->selection().empty())
        return;

    [self commitPoints:app::AutomationEditorModel::withPointsRemoved(lane->points,
                                                                      _model->selection())
                  name:@"Erase Automation"
               merging:NO];

    _model->clearSelection();
    [self setNeedsDisplay:YES];
}

// ── Keyboard ─────────────────────────────────────────────────────────────────

- (void)keyDown:(NSEvent*)event
{
    project::AutomationLane* lane = [self lane];
    if (lane == nullptr) {
        [super keyDown:event];
        return;
    }

    NSString* characters = event.charactersIgnoringModifiers;
    if (characters.length == 0) {
        [super keyDown:event];
        return;
    }

    const unichar character = [characters characterAtIndex:0];
    const bool    command   = (event.modifierFlags & NSEventModifierFlagCommand) != 0;
    const bool    shift     = (event.modifierFlags & NSEventModifierFlagShift) != 0;

    if (command && (character == 'z' || character == 'Z')) {
        if (shift)
            (void)_registry->redo();
        else
            (void)_registry->undo();

        if (const project::AutomationLane* after = [self lane])
            _model->pruneSelection(after->points.size());

        [self describe:@"Undo"];
        [self changed];
        return;
    }

    if (character == NSDeleteCharacter || character == NSBackspaceCharacter
        || character == NSDeleteFunctionKey) {
        [self eraseSelection];
        return;
    }

    if (command && (character == 'a' || character == 'A')) {
        std::vector<std::size_t> all(lane->points.size());
        for (std::size_t index = 0; index < all.size(); ++index)
            all[index] = index;

        _model->setSelection(std::move(all));
        [self setNeedsDisplay:YES];
        return;
    }

    if (command && (character == 'c' || character == 'C')) {
        _clipboard = app::AutomationEditorModel::copyOf(lane->points, _model->selection());
        [self describe:[NSString stringWithFormat:@"Copied %lu",
                                                  static_cast<unsigned long>(_clipboard.size())]];
        [self setNeedsDisplay:YES];
        return;
    }

    if (command && (character == 'v' || character == 'V')) {
        if (_clipboard.empty())
            return;

        // At the playhead when there is one, so a paste lands where the ear
        // is; otherwise at the left edge of what is on screen.
        const Tick at = _playheadTick >= 0 ? static_cast<Tick>(_playheadTick)
                                           : _model->viewport().firstTick;

        [self commitPoints:app::AutomationEditorModel::withPasted(lane->points, _clipboard, at)
                      name:@"Paste Automation"
                   merging:NO];

        if (const project::AutomationLane* after = [self lane])
            _model->setSelection(app::AutomationEditorModel::pastedIndices(after->points,
                                                                           _clipboard, at));

        [self setNeedsDisplay:YES];
        return;
    }

    // The grid, in the Piano Roll's own words and divisions.
    if (!command && (character == 'g' || character == 'G')) {
        const auto next = static_cast<std::size_t>(_snap) + 1;
        _snap = app::PianoRollHeaderModel::snapAt(next % app::PianoRollHeaderModel::snapCount);
        _model->setSnap(app::PianoRollHeaderModel::ticksFor(_snap));

        [self describe:@"Grid"];
        [self setNeedsDisplay:YES];
        return;
    }

    // Nudging: the grid horizontally, one per cent vertically.
    if (!_model->selection().empty()
        && (character == NSLeftArrowFunctionKey || character == NSRightArrowFunctionKey
            || character == NSUpArrowFunctionKey || character == NSDownArrowFunctionKey)) {
        const Tick step = _model->snap() > 0 ? _model->snap() : ticksPerQuarterNote / 4;

        Tick   tickDelta  = 0;
        double valueDelta = 0.0;

        if (character == NSLeftArrowFunctionKey)  tickDelta  = -step;
        if (character == NSRightArrowFunctionKey) tickDelta  = step;
        if (character == NSUpArrowFunctionKey)    valueDelta = shift ? 0.1 : 0.01;
        if (character == NSDownArrowFunctionKey)  valueDelta = shift ? -0.1 : -0.01;

        [self commitPoints:app::AutomationEditorModel::withPointsMoved(
                               lane->points, _model->selection(), tickDelta, valueDelta)
                      name:@"Nudge Automation"
                   merging:YES];
        return;
    }

    [super keyDown:event];
}

- (void)setPlayheadTick:(long long)tick
{
    if (_playheadTick == tick)
        return;

    _playheadTick = tick;
    [self setNeedsDisplay:YES];
}

@end
