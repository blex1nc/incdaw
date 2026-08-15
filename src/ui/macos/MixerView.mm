#include "ui/macos/MixerView.h"

#include "app/CommandRegistry.h"
#include "app/commands/MixerCommands.h"
#include "app/commands/PluginCommands.h"
#include "engine/dsp/MixerStripNode.h"
#include "engine/dsp/effects/BuiltinEffects.h"
#include "project/Model.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <vector>

using namespace incdaw;

namespace {

constexpr CGFloat stripWidth   = 96.0;
constexpr CGFloat stripGap     = 2.0;
constexpr CGFloat headerHeight = 22.0;
constexpr CGFloat buttonHeight = 18.0;
constexpr CGFloat panHeight    = 14.0;
constexpr CGFloat padding      = 6.0;
constexpr CGFloat meterWidth   = 14.0;

NSColor* grey(CGFloat white) { return [NSColor colorWithCalibratedWhite:white alpha:1.0]; }

NSColor* colourFrom(std::uint32_t argb, CGFloat brightness = 1.0)
{
    return [NSColor colorWithCalibratedRed:static_cast<CGFloat>((argb >> 16) & 0xFFu) / 255.0 * brightness
                                     green:static_cast<CGFloat>((argb >> 8) & 0xFFu) / 255.0 * brightness
                                      blue:static_cast<CGFloat>(argb & 0xFFu) / 255.0 * brightness
                                     alpha:1.0];
}

void fillRect(NSRect rect, NSColor* colour)
{
    [colour setFill];
    NSRectFill(rect);
}

void drawText(NSString* text, NSRect rect, NSColor* colour, CGFloat size, BOOL centred = YES)
{
    NSMutableParagraphStyle* style = [[NSMutableParagraphStyle alloc] init];
    style.lineBreakMode = NSLineBreakByTruncatingTail;
    style.alignment     = centred ? NSTextAlignmentCenter : NSTextAlignmentLeft;

    [text drawInRect:rect
      withAttributes:@{NSFontAttributeName: [NSFont systemFontOfSize:size],
                       NSForegroundColorAttributeName: colour,
                       NSParagraphStyleAttributeName: style}];
}

/// Fader travel is not linear in gain: a linear fader spends most of its length
/// in the top few decibels and is unusable below -20 dB. This is the same
/// quarter-power curve most consoles use.
double positionForGain(double gain)
{
    return std::pow(std::clamp(gain, 0.0, 4.0) / 4.0, 1.0 / 3.0);
}

double gainForPosition(double position)
{
    const double clamped = std::clamp(position, 0.0, 1.0);
    return clamped * clamped * clamped * 4.0;
}

enum class MixerDrag { none, fader, pan };

} // namespace

@implementation INCDAWMixerView {
    project::Project*     _project;
    app::CommandRegistry* _registry;

    MixerDrag         _drag;
    project::EntityId _dragNode;
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
    _drag     = MixerDrag::none;

    return self;
}

- (BOOL)isFlipped { return YES; }
- (BOOL)acceptsFirstResponder { return YES; }

// ── Geometry ─────────────────────────────────────────────────────────────────

- (NSRect)stripRectAt:(std::size_t)index
{
    return NSMakeRect(static_cast<CGFloat>(index) * (stripWidth + stripGap), 0,
                      stripWidth, self.bounds.size.height);
}

- (NSRect)faderRectAt:(std::size_t)index
{
    const NSRect strip = [self stripRectAt:index];
    const CGFloat top    = headerHeight + panHeight + padding * 2.0;
    const CGFloat bottom = buttonHeight + padding * 2.0;

    return NSMakeRect(strip.origin.x + padding, top,
                      stripWidth - meterWidth - padding * 3.0,
                      std::max<CGFloat>(20.0, strip.size.height - top - bottom));
}

- (NSRect)meterRectAt:(std::size_t)index
{
    const NSRect fader = [self faderRectAt:index];
    return NSMakeRect(fader.origin.x + fader.size.width + padding, fader.origin.y,
                      meterWidth, fader.size.height);
}

- (NSRect)panRectAt:(std::size_t)index
{
    const NSRect strip = [self stripRectAt:index];
    return NSMakeRect(strip.origin.x + padding, headerHeight + padding,
                      stripWidth - padding * 2.0, panHeight);
}

- (NSRect)muteRectAt:(std::size_t)index
{
    const NSRect strip = [self stripRectAt:index];
    const CGFloat width = (stripWidth - padding * 4.0) / 3.0;

    return NSMakeRect(strip.origin.x + padding,
                      strip.size.height - buttonHeight - padding, width, buttonHeight);
}

- (NSRect)soloRectAt:(std::size_t)index
{
    const NSRect mute = [self muteRectAt:index];
    return NSMakeRect(mute.origin.x + mute.size.width + padding, mute.origin.y,
                      mute.size.width, mute.size.height);
}

- (NSRect)polarityRectAt:(std::size_t)index
{
    const NSRect solo = [self soloRectAt:index];
    return NSMakeRect(solo.origin.x + solo.size.width + padding, solo.origin.y,
                      solo.size.width, solo.size.height);
}

- (NSRect)addStripRect
{
    return [self stripRectAt:_project->mixerNodes().size()];
}

// ── Drawing ──────────────────────────────────────────────────────────────────

- (void)drawRect:(NSRect)dirtyRect
{
    (void)dirtyRect;

    fillRect(self.bounds, grey(0.10));

    if (_project == nullptr)
        return;

    const std::vector<project::MixerNode>& nodes = _project->mixerNodes();

    for (std::size_t index = 0; index < nodes.size(); ++index)
        [self drawStrip:index node:nodes[index]];

    const NSRect add = [self addStripRect];
    fillRect(add, grey(0.125));
    drawText(@"＋", NSMakeRect(add.origin.x, add.size.height / 2.0 - 10.0, stripWidth, 20.0),
             grey(0.5), 16.0);
}

- (void)drawStrip:(std::size_t)index node:(const project::MixerNode&)node
{
    const bool isMaster = node.id == _project->masterMixerNode();
    const NSRect strip = [self stripRectAt:index];

    fillRect(strip, isMaster ? grey(0.175) : grey(0.145));

    fillRect(NSMakeRect(strip.origin.x, 0, stripWidth, headerHeight),
             colourFrom(node.colour, node.muted ? 0.5 : 1.0));

    drawText(@(node.name.c_str()),
             NSMakeRect(strip.origin.x + 2.0, 4.0, stripWidth - 4.0, 16.0),
             grey(0.92), 11.0);

    // Pan, drawn as an offset from centre rather than as a knob: the position
    // is the information, and a knob would only be a picture of one.
    const NSRect pan = [self panRectAt:index];
    fillRect(pan, grey(0.20));
    const CGFloat centre = pan.origin.x + pan.size.width / 2.0;
    const CGFloat handle = centre + static_cast<CGFloat>(node.pan) * (pan.size.width / 2.0 - 3.0);
    fillRect(NSMakeRect(centre - 0.5, pan.origin.y, 1.0, pan.size.height), grey(0.32));
    fillRect(NSMakeRect(handle - 2.0, pan.origin.y + 2.0, 4.0, pan.size.height - 4.0), grey(0.72));

    // Fader
    const NSRect fader = [self faderRectAt:index];
    fillRect(fader, grey(0.16));

    const CGFloat travel = fader.size.height - 12.0;
    const CGFloat position = fader.origin.y + travel
                           * static_cast<CGFloat>(1.0 - positionForGain(node.volume));

    fillRect(NSMakeRect(fader.origin.x + fader.size.width / 2.0 - 1.0, fader.origin.y,
                        2.0, fader.size.height), grey(0.24));
    fillRect(NSMakeRect(fader.origin.x, position, fader.size.width, 12.0), grey(0.62));

    // Meter, read from the live graph.
    const NSRect meterRect = [self meterRectAt:index];
    fillRect(meterRect, grey(0.13));

    if (self.stripLookup != nil) {
        if (engine::dsp::MixerStripNode* live = self.stripLookup(node.id.value())) {
            const CGFloat peak = static_cast<CGFloat>(std::min(1.0f, live->meter().peak()));
            const CGFloat rms  = static_cast<CGFloat>(std::min(1.0f, live->meter().rms()));

            const CGFloat rmsHeight  = meterRect.size.height * rms;
            const CGFloat peakHeight = meterRect.size.height * peak;

            fillRect(NSMakeRect(meterRect.origin.x,
                                meterRect.origin.y + meterRect.size.height - rmsHeight,
                                meterRect.size.width, rmsHeight),
                     [NSColor colorWithCalibratedRed:0.35 green:0.72 blue:0.45 alpha:1.0]);

            // The peak sits above the RMS body as a line, and turns red at the
            // point where the signal is about to clip rather than after it has.
            fillRect(NSMakeRect(meterRect.origin.x,
                                meterRect.origin.y + meterRect.size.height - peakHeight,
                                meterRect.size.width, 2.0),
                     peak > 0.98 ? [NSColor colorWithCalibratedRed:0.9 green:0.3 blue:0.25 alpha:1.0]
                                  : grey(0.85));
        }
    }

    const NSRect mute = [self muteRectAt:index];
    fillRect(mute, node.muted ? [NSColor colorWithCalibratedRed:0.75 green:0.30 blue:0.25 alpha:1.0]
                              : grey(0.22));
    drawText(@"M", NSMakeRect(mute.origin.x, mute.origin.y + 2.0, mute.size.width, mute.size.height),
             grey(0.9), 10.0);

    const NSRect solo = [self soloRectAt:index];
    fillRect(solo, node.soloed ? [NSColor colorWithCalibratedRed:0.85 green:0.70 blue:0.25 alpha:1.0]
                               : grey(0.22));
    drawText(@"S", NSMakeRect(solo.origin.x, solo.origin.y + 2.0, solo.size.width, solo.size.height),
             node.soloed ? grey(0.1) : grey(0.9), 10.0);

    const NSRect polarity = [self polarityRectAt:index];
    fillRect(polarity, node.polarityFlip ? grey(0.6) : grey(0.22));
    drawText(@"Ø", NSMakeRect(polarity.origin.x, polarity.origin.y + 2.0,
                              polarity.size.width, polarity.size.height),
             node.polarityFlip ? grey(0.1) : grey(0.9), 10.0);
}

// ── Input ────────────────────────────────────────────────────────────────────

- (std::size_t)stripIndexAtX:(CGFloat)x
{
    if (x < 0.0)
        return static_cast<std::size_t>(-1);

    return static_cast<std::size_t>(x / (stripWidth + stripGap));
}

- (void)mouseDown:(NSEvent*)event
{
    const NSPoint point = [self convertPoint:event.locationInWindow fromView:nil];
    const std::size_t index = [self stripIndexAtX:point.x];

    _drag = MixerDrag::none;

    if (index == static_cast<std::size_t>(-1))
        return;

    if (index >= _project->mixerNodes().size()) {
        if (index == _project->mixerNodes().size())
            [self addMixerTrack];

        return;
    }

    const project::MixerNode& node = _project->mixerNodes()[index];
    const project::EntityId nodeId = node.id;

    if (NSPointInRect(point, [self muteRectAt:index])) {
        [self commitStructural:std::make_unique<app::SetMixerMutedCommand>(nodeId, !node.muted)];
        return;
    }

    if (NSPointInRect(point, [self soloRectAt:index])) {
        [self commitStructural:std::make_unique<app::SetMixerSoloedCommand>(nodeId, !node.soloed)];
        return;
    }

    if (NSPointInRect(point, [self polarityRectAt:index])) {
        [self commitStructural:std::make_unique<app::SetMixerPolarityCommand>(nodeId, !node.polarityFlip)];
        return;
    }

    if (NSPointInRect(point, [self panRectAt:index])) {
        _drag     = MixerDrag::pan;
        _dragNode = nodeId;
        [self applyPanAt:point index:index];
        return;
    }

    if (NSPointInRect(point, [self faderRectAt:index])) {
        _drag     = MixerDrag::fader;
        _dragNode = nodeId;
        [self applyFaderAt:point index:index];
        return;
    }

    if (point.y < headerHeight && event.clickCount == 2)
        [self renameNode:nodeId];
}

- (void)mouseDragged:(NSEvent*)event
{
    if (_drag == MixerDrag::none)
        return;

    const std::size_t index = _project->indexOfMixerNode(_dragNode);
    if (index == project::Project::notFound)
        return;

    const NSPoint point = [self convertPoint:event.locationInWindow fromView:nil];

    if (_drag == MixerDrag::fader)
        [self applyFaderAt:point index:index];
    else
        [self applyPanAt:point index:index];
}

- (void)mouseUp:(NSEvent*)event
{
    (void)event;
    _drag = MixerDrag::none;
}

- (void)rightMouseDown:(NSEvent*)event
{
    const NSPoint point = [self convertPoint:event.locationInWindow fromView:nil];
    const std::size_t index = [self stripIndexAtX:point.x];

    if (index >= _project->mixerNodes().size())
        return;

    const project::EntityId nodeId = _project->mixerNodes()[index].id;

    NSMenu* menu = [[NSMenu alloc] init];

    NSMenuItem* rename = [menu addItemWithTitle:@"Rename…"
                                         action:@selector(renameFromMenu:)
                                  keyEquivalent:@""];
    rename.target = self;
    rename.representedObject = @(nodeId.value());

    NSMenuItem* route = [menu addItemWithTitle:@"Route Selected Channel Here"
                                        action:@selector(routeChannelFromMenu:)
                                 keyEquivalent:@""];
    route.target = self;
    route.representedObject = @(nodeId.value());

    NSMenuItem* send = [menu addItemWithTitle:@"Send To Master (25%)"
                                       action:@selector(addSendFromMenu:)
                                keyEquivalent:@""];
    send.target = self;
    send.representedObject = @(nodeId.value());

    // ── The insert chain ─────────────────────────────────────────────────
    [menu addItem:[NSMenuItem separatorItem]];

    NSMenuItem* addInsertItem = [menu addItemWithTitle:@"Add Insert"
                                                action:nil
                                         keyEquivalent:@""];
    NSMenu* addInsertMenu = [[NSMenu alloc] init];

    for (NSDictionary* plugin in self.availableInserts) {
        NSMenuItem* item = [addInsertMenu addItemWithTitle:plugin[@"name"]
                                                    action:@selector(addInsertFromMenu:)
                                             keyEquivalent:@""];
        item.target            = self;
        item.representedObject = @{@"node": @(nodeId.value()), @"uid": plugin[@"uid"]};
    }

    if (self.availableInserts.count == 0)
        [addInsertMenu addItemWithTitle:@"No plugins scanned" action:nil keyEquivalent:@""];

    addInsertItem.submenu = addInsertMenu;

    // Builtin effects, from the engine's own catalogue — always present,
    // never scanned.
    NSMenuItem* addBuiltinItem = [menu addItemWithTitle:@"Add Built-in Effect"
                                                 action:nil
                                          keyEquivalent:@""];
    NSMenu* addBuiltinMenu = [[NSMenu alloc] init];

    for (const engine::dsp::BuiltinEffectInfo& info : engine::dsp::builtinEffects()) {
        NSMenuItem* item = [addBuiltinMenu addItemWithTitle:@(info.displayName)
                                                     action:@selector(addBuiltinInsertFromMenu:)
                                              keyEquivalent:@""];
        item.target            = self;
        item.representedObject = @{@"node": @(nodeId.value()), @"uid": @(info.uid)};
    }

    addBuiltinItem.submenu = addBuiltinMenu;

    // One submenu per slot: Bypass (checkable) and Remove, in chain order —
    // the order is the audible order.
    const project::MixerNode* node = _project->findMixerNode(nodeId);

    if (node != nullptr) {
        for (const project::PluginSlot& slot : node->inserts) {
            NSString* title = [NSString stringWithFormat:@"%s", slot.plugin.uid.c_str()];

            if (const auto* builtin = engine::dsp::findBuiltinEffect(slot.plugin.uid))
                title = @(builtin->displayName);

            for (NSDictionary* plugin in self.availableInserts)
                if ([plugin[@"uid"] isEqualToString:@(slot.plugin.uid.c_str())])
                    title = plugin[@"name"];

            NSMenuItem* slotItem = [menu addItemWithTitle:title action:nil keyEquivalent:@""];
            NSMenu*     slotMenu = [[NSMenu alloc] init];

            NSDictionary* address = @{@"node": @(nodeId.value()), @"slot": @(slot.id.value())};

            NSMenuItem* editor = [slotMenu addItemWithTitle:@"Open Editor"
                                                     action:@selector(openInsertEditorFromMenu:)
                                              keyEquivalent:@""];
            editor.target            = self;
            editor.representedObject = address;

            NSMenuItem* bypass = [slotMenu addItemWithTitle:@"Bypass"
                                                     action:@selector(toggleInsertBypassFromMenu:)
                                              keyEquivalent:@""];
            bypass.target            = self;
            bypass.representedObject = address;
            bypass.state = slot.bypassed ? NSControlStateValueOn : NSControlStateValueOff;

            NSMenuItem* removeSlot = [slotMenu addItemWithTitle:@"Remove"
                                                         action:@selector(removeInsertFromMenu:)
                                                  keyEquivalent:@""];
            removeSlot.target            = self;
            removeSlot.representedObject = address;

            slotItem.submenu = slotMenu;
        }
    }

    if (nodeId != _project->masterMixerNode()) {
        [menu addItem:[NSMenuItem separatorItem]];

        NSMenuItem* remove = [menu addItemWithTitle:@"Remove Mixer Track"
                                             action:@selector(removeFromMenu:)
                                      keyEquivalent:@""];
        remove.target = self;
        remove.representedObject = @(nodeId.value());
    }

    [NSMenu popUpContextMenu:menu withEvent:event forView:self];
}

// ── Insert-chain edits ───────────────────────────────────────────────────────

- (void)openInsertEditorFromMenu:(NSMenuItem*)item
{
    NSDictionary* info = item.representedObject;

    if (self.onOpenInsertEditor != nil)
        self.onOpenInsertEditor([info[@"slot"] unsignedLongLongValue]);
}

- (void)addInsertFromMenu:(NSMenuItem*)item
{
    NSDictionary* info = item.representedObject;

    plugins::PluginIdentifier plugin;
    plugin.format = plugins::Format::clap;
    plugin.uid    = [info[@"uid"] UTF8String];

    if (_registry->execute(std::make_unique<app::AddInsertCommand>(
            project::EntityId{[info[@"node"] unsignedLongLongValue]}, std::move(plugin))))
        [self structuralChange];
}

- (void)addBuiltinInsertFromMenu:(NSMenuItem*)item
{
    NSDictionary* info = item.representedObject;

    plugins::PluginIdentifier plugin;
    plugin.format = plugins::Format::builtin;
    plugin.uid    = [info[@"uid"] UTF8String];

    if (_registry->execute(std::make_unique<app::AddInsertCommand>(
            project::EntityId{[info[@"node"] unsignedLongLongValue]}, std::move(plugin))))
        [self structuralChange];
}

- (void)toggleInsertBypassFromMenu:(NSMenuItem*)item
{
    NSDictionary* info = item.representedObject;

    const project::EntityId nodeId{[info[@"node"] unsignedLongLongValue]};
    const project::EntityId slotId{[info[@"slot"] unsignedLongLongValue]};

    const project::MixerNode* node = _project->findMixerNode(nodeId);
    if (node == nullptr)
        return;

    for (const project::PluginSlot& slot : node->inserts)
        if (slot.id == slotId)
            if (_registry->execute(std::make_unique<app::SetInsertBypassedCommand>(
                    nodeId, slotId, !slot.bypassed)))
                [self structuralChange];
}

- (void)removeInsertFromMenu:(NSMenuItem*)item
{
    NSDictionary* info = item.representedObject;

    if (_registry->execute(std::make_unique<app::RemoveInsertCommand>(
            project::EntityId{[info[@"node"] unsignedLongLongValue]},
            project::EntityId{[info[@"slot"] unsignedLongLongValue]})))
        [self structuralChange];
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

        [self structuralChange];
        return;
    }

    if (character == ' ') {
        if (self.onTransportToggle != nil)
            self.onTransportToggle();

        return;
    }

    [super keyDown:event];
}

// ── Edits ────────────────────────────────────────────────────────────────────

- (void)applyFaderAt:(NSPoint)point index:(std::size_t)index
{
    const NSRect fader = [self faderRectAt:index];
    const CGFloat travel = std::max<CGFloat>(1.0, fader.size.height - 12.0);

    const double position = 1.0 - std::clamp((point.y - fader.origin.y) / travel, 0.0, 1.0);
    const double gain     = gainForPosition(position);

    const project::EntityId nodeId = _project->mixerNodes()[index].id;

    if (_registry->executeMerging(std::make_unique<app::SetMixerVolumeCommand>(nodeId, gain))) {
        [self parameterChangedFor:nodeId];

        // `position` is already the registry's normalised volume: both sides
        // share the cubic fader law, so a recorded pass replays identically.
        if (self.onParameterEdited != nil)
            self.onParameterEdited(nodeId.value(), "volume", position);
    }
}

- (void)applyPanAt:(NSPoint)point index:(std::size_t)index
{
    const NSRect rect = [self panRectAt:index];
    const double pan = std::clamp((point.x - rect.origin.x) / rect.size.width * 2.0 - 1.0, -1.0, 1.0);

    const project::EntityId nodeId = _project->mixerNodes()[index].id;

    if (_registry->executeMerging(std::make_unique<app::SetMixerPanCommand>(nodeId, pan))) {
        [self parameterChangedFor:nodeId];

        if (self.onParameterEdited != nil)
            self.onParameterEdited(nodeId.value(), "pan", (pan + 1.0) / 2.0);
    }
}

/// Writes a parameter straight to the strip that is rendering, instead of
/// recompiling the graph for it. The value is already in the model; this is the
/// same value reaching the audio thread the way it will when automation writes
/// it (Phase 11).
- (void)parameterChangedFor:(project::EntityId)nodeId
{
    if (self.stripLookup != nil) {
        if (engine::dsp::MixerStripNode* strip = self.stripLookup(nodeId.value())) {
            const project::MixerNode* node = _project->findMixerNode(nodeId);
            if (node != nullptr) {
                const bool isMaster = nodeId == _project->masterMixerNode();

                strip->setGain(static_cast<engine::Sample>(node->volume)
                               * (isMaster ? engine::Sample{0.8f} : engine::Sample{1}));
                strip->setPan(node->pan);
            }
        }
    }

    [self setNeedsDisplay:YES];

    if (self.onParameterChange != nil)
        self.onParameterChange();
}

- (void)addMixerTrack
{
    auto command = std::make_unique<app::AddMixerNodeCommand>(
        project::MixerNodeType::track,
        "Insert " + std::to_string(_project->mixerNodes().size()));

    app::AddMixerNodeCommand* raw = command.get();

    if (!_registry->execute(std::move(command)))
        return;

    // A new track routed nowhere would be a strip that cannot be heard.
    (void)_registry->execute(std::make_unique<app::ConnectMixerCommand>(
        raw->mixerNodeId(), _project->masterMixerNode()));

    [self structuralChange];
}

- (void)renameFromMenu:(NSMenuItem*)item
{
    [self renameNode:project::EntityId{[item.representedObject unsignedLongLongValue]}];
}

- (void)routeChannelFromMenu:(NSMenuItem*)item
{
    const project::EntityId nodeId{[item.representedObject unsignedLongLongValue]};

    [self commitStructural:std::make_unique<app::SetChannelOutputCommand>(
        project::EntityId{_selectedChannelIdValue}, nodeId)];
}

- (void)addSendFromMenu:(NSMenuItem*)item
{
    const project::EntityId nodeId{[item.representedObject unsignedLongLongValue]};

    if (nodeId == _project->masterMixerNode())
        return;   // the master sending to itself is a cycle

    [self commitStructural:std::make_unique<app::ConnectMixerCommand>(
        nodeId, _project->masterMixerNode(), true, 0.25)];
}

- (void)removeFromMenu:(NSMenuItem*)item
{
    [self commitStructural:std::make_unique<app::RemoveMixerNodeCommand>(
        project::EntityId{[item.representedObject unsignedLongLongValue]})];
}

- (void)renameNode:(project::EntityId)nodeId
{
    const project::MixerNode* node = _project->findMixerNode(nodeId);
    if (node == nullptr)
        return;

    NSAlert* alert = [[NSAlert alloc] init];
    alert.messageText = @"Rename mixer track";
    [alert addButtonWithTitle:@"Rename"];
    [alert addButtonWithTitle:@"Cancel"];

    NSTextField* field = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 220, 24)];
    field.stringValue = @(node->name.c_str());
    alert.accessoryView = field;

    if ([alert runModal] != NSAlertFirstButtonReturn)
        return;

    [self commitStructural:std::make_unique<app::RenameMixerNodeCommand>(
        nodeId, field.stringValue.UTF8String)];
}

- (void)commitStructural:(app::CommandPtr)command
{
    if (_registry->execute(std::move(command)))
        [self structuralChange];
}

- (void)structuralChange
{
    [self setNeedsDisplay:YES];

    if (self.onChange != nil)
        self.onChange();
}

@end
