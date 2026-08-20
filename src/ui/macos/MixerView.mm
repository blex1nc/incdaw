#include "ui/macos/MixerView.h"

#include "app/CommandRegistry.h"
#include "app/commands/MixerCommands.h"
#include "app/commands/PluginCommands.h"
#include "engine/dsp/MixerStripNode.h"
#include "engine/dsp/effects/BuiltinEffects.h"
#include "project/Model.h"
#include "ui/macos/Theme.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <vector>

using namespace incdaw;

namespace theme = incdaw::ui::theme;

namespace {

using theme::Ink;

constexpr CGFloat stripWidth   = 104.0;
constexpr CGFloat stripGap     = 4.0;
constexpr CGFloat headerHeight = 24.0;
constexpr CGFloat buttonHeight = 20.0;
constexpr CGFloat panHeight    = 34.0;
constexpr CGFloat padding      = 7.0;
constexpr CGFloat meterWidth   = 14.0;

using theme::fillRect;

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
    const CGFloat bottom = buttonHeight + padding * 3.0 + 12.0;

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
                      strip.size.height - buttonHeight - padding * 2.0, width, buttonHeight);
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

    fillRect(self.bounds, theme::ink(Ink::panel));

    if (_project == nullptr)
        return;

    const std::vector<project::MixerNode>& nodes = _project->mixerNodes();

    for (std::size_t index = 0; index < nodes.size(); ++index)
        [self drawStrip:index node:nodes[index]];

    const NSRect add = NSInsetRect([self addStripRect], 3.0, 6.0);
    theme::strokeRounded(add, theme::metrics::radiusPanel,
                         theme::withAlpha(theme::ink(Ink::textDim), 0.35));

    theme::drawTextCentred(@"＋", add, theme::ink(Ink::textSecondary),
                           theme::labelFont(16.0), theme::Align::centre);
}

- (void)drawStrip:(std::size_t)index node:(const project::MixerNode&)node
{
    const bool isMaster   = node.id == _project->masterMixerNode();
    const bool isSelected = node.id.value() == _selectedChannelIdValue;

    const NSRect strip = NSInsetRect([self stripRectAt:index], 3.0, 6.0);

    theme::drawPanel(strip, theme::metrics::radiusPanel, isSelected || isMaster, true);

    if (isSelected)
        theme::strokeRounded(strip, theme::metrics::radiusPanel, theme::ink(Ink::accent));

    // ── Name plate ───────────────────────────────────────────────────────────
    NSColor* colour = theme::fromArgb(node.colour, node.muted ? 0.6 : 1.0);

    const NSRect header = NSMakeRect(NSMinX(strip) + 4.0, NSMinY(strip) + 4.0,
                                     strip.size.width - 8.0, headerHeight - 6.0);

    theme::fillGradient(header, theme::metrics::radiusPad, theme::lighten(colour, 0.22),
                        theme::darken(colour, 0.18), true);
    theme::strokeRounded(header, theme::metrics::radiusPad, theme::darken(colour, 0.5));

    theme::drawTextCentred(@(node.name.c_str()), NSInsetRect(header, 4.0, 0.0),
                           theme::labelOn(colour),
                           theme::labelFont(11.0, NSFontWeightSemibold), theme::Align::centre);

    // ── Pan ──────────────────────────────────────────────────────────────────
    const NSRect pan = [self panRectAt:index];
    const CGFloat knobSize = std::min(pan.size.height, CGFloat{28.0});

    theme::drawKnob(NSMakeRect(NSMidX(pan) - knobSize / 2.0, NSMinY(pan), knobSize, knobSize),
                    (node.pan + 1.0) / 2.0, theme::ink(Ink::accent), true);

    // ── Fader and meter ──────────────────────────────────────────────────────
    const NSRect fader = [self faderRectAt:index];

    theme::drawFader(fader, positionForGain(node.volume),
                     node.muted ? theme::ink(Ink::textDim) : theme::ink(Ink::accent),
                     isSelected, true);

    const NSRect meterRect = [self meterRectAt:index];

    double peak = 0.0;
    double rms  = 0.0;

    if (self.stripLookup != nil) {
        if (engine::dsp::MixerStripNode* live = self.stripLookup(node.id.value())) {
            peak = std::min(1.0, static_cast<double>(live->meter().peak()));
            rms  = std::min(1.0, static_cast<double>(live->meter().rms()));
        }
    }

    theme::drawMeter(meterRect, rms, peak, true, true);

    // The number the fader is worth, in the unit a mixer is discussed in.
    const double db = node.volume > 0.0001 ? 20.0 * std::log10(node.volume) : -96.0;

    theme::drawTextCentred(db <= -95.0 ? @"-∞"
                                       : [NSString stringWithFormat:@"%+.1f", db],
                           NSMakeRect(NSMinX(strip) + 4.0, NSMaxY(fader) + 2.0,
                                      strip.size.width - 8.0, 13.0),
                           theme::ink(Ink::textSecondary), theme::numericFont(9.5),
                           theme::Align::centre);

    // ── Switches ─────────────────────────────────────────────────────────────
    theme::drawToggle([self muteRectAt:index], @"M", node.muted, theme::ink(Ink::mute), true);
    theme::drawToggle([self soloRectAt:index], @"S", node.soloed, theme::ink(Ink::solo), true);
    theme::drawToggle([self polarityRectAt:index], @"Ø", node.polarityFlip,
                      theme::ink(Ink::automation), true);
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

    if (_drag != MixerDrag::none && self.onParameterGestureEnded != nil)
        self.onParameterGestureEnded(_dragNode.value(),
                                     _drag == MixerDrag::fader ? "volume" : "pan");

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

    NSMenuItem* preSend = [menu addItemWithTitle:@"Send To Master (Pre-Fader, 25%)"
                                          action:@selector(addPreFaderSendFromMenu:)
                                   keyEquivalent:@""];
    preSend.target = self;
    preSend.representedObject = @(nodeId.value());

    // Stereo separation presets; the strip applies them mid/side, before pan.
    NSMenuItem* widthItem = [menu addItemWithTitle:@"Stereo Separation"
                                            action:nil
                                     keyEquivalent:@""];
    NSMenu* widthMenu = [[NSMenu alloc] init];

    const struct { const char* title; double value; } widths[] = {
        { "Mono (-100%)", -1.0 }, { "Narrow (-50%)", -0.5 }, { "Normal (0%)", 0.0 },
        { "Wide (+50%)", 0.5 },   { "Widest (+100%)", 1.0 },
    };
    for (const auto& width : widths) {
        NSMenuItem* item = [widthMenu addItemWithTitle:@(width.title)
                                                action:@selector(setSeparationFromMenu:)
                                         keyEquivalent:@""];
        item.target            = self;
        item.representedObject = @{@"node": @(nodeId.value()), @"separation": @(width.value)};
    }
    widthItem.submenu = widthMenu;

    // Sidechain: this strip becomes the key of a compressor on the chosen
    // strip. The compile step warns if the destination has none to key.
    NSMenuItem* sidechainItem = [menu addItemWithTitle:@"Sidechain Into"
                                                action:nil
                                         keyEquivalent:@""];
    NSMenu* sidechainMenu = [[NSMenu alloc] init];

    for (const project::MixerNode& node : _project->mixerNodes()) {
        if (node.id.value() == nodeId.value() || node.id == _project->masterMixerNode())
            continue;

        NSMenuItem* target = [sidechainMenu addItemWithTitle:@(node.name.c_str())
                                                      action:@selector(addSidechainFromMenu:)
                                               keyEquivalent:@""];
        target.target            = self;
        target.representedObject = @{@"source": @(nodeId.value()),
                                     @"destination": @(node.id.value())};
    }
    sidechainItem.submenu = sidechainMenu;

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
        item.representedObject = @{@"node": @(nodeId.value()), @"id": plugin[@"id"]};
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

    // ── MIDI learn ───────────────────────────────────────────────────────
    [menu addItem:[NSMenuItem separatorItem]];

    NSMenuItem* learnVolume = [menu addItemWithTitle:@"MIDI Learn Volume"
                                              action:@selector(midiLearnVolumeFromMenu:)
                                       keyEquivalent:@""];
    learnVolume.target            = self;
    learnVolume.representedObject = @(nodeId.value());

    NSMenuItem* learnPan = [menu addItemWithTitle:@"MIDI Learn Pan"
                                           action:@selector(midiLearnPanFromMenu:)
                                    keyEquivalent:@""];
    learnPan.target            = self;
    learnPan.representedObject = @(nodeId.value());

    bool anyMappingHere = false;
    for (const project::MidiMapping& mapping : _project->midiMappings())
        anyMappingHere = anyMappingHere || mapping.targetEntity == nodeId;

    if (anyMappingHere) {
        NSMenuItem* forget = [menu addItemWithTitle:@"Forget MIDI Mappings"
                                             action:@selector(midiForgetFromMenu:)
                                      keyEquivalent:@""];
        forget.target            = self;
        forget.representedObject = @(nodeId.value());
    }

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

            NSMenuItem* moveUp = [slotMenu addItemWithTitle:@"Move Up"
                                                     action:@selector(moveInsertUpFromMenu:)
                                              keyEquivalent:@""];
            moveUp.target            = self;
            moveUp.representedObject = address;

            NSMenuItem* moveDown = [slotMenu addItemWithTitle:@"Move Down"
                                                       action:@selector(moveInsertDownFromMenu:)
                                                keyEquivalent:@""];
            moveDown.target            = self;
            moveDown.representedObject = address;

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

    // "clap:com.acme.reverb", "au:aufx:dely:appl" — the round-trippable form
    // the project file stores. The menu no longer assumes a format.
    if (!plugins::PluginIdentifier::fromString([info[@"id"] UTF8String], plugin))
        return;

    if (_registry->execute(std::make_unique<app::AddInsertCommand>(
            project::EntityId{[info[@"node"] unsignedLongLongValue]}, std::move(plugin))))
        [self structuralChange];
}

- (void)midiLearnVolumeFromMenu:(NSMenuItem*)item
{
    if (self.onMidiLearn != nil)
        self.onMidiLearn(@"volume", [item.representedObject unsignedLongLongValue]);
}

- (void)midiLearnPanFromMenu:(NSMenuItem*)item
{
    if (self.onMidiLearn != nil)
        self.onMidiLearn(@"pan", [item.representedObject unsignedLongLongValue]);
}

- (void)midiForgetFromMenu:(NSMenuItem*)item
{
    if (self.onMidiForget != nil)
        self.onMidiForget([item.representedObject unsignedLongLongValue]);
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

- (void)moveInsert:(NSMenuItem*)item direction:(int)direction
{
    NSDictionary* info = item.representedObject;

    if (_registry->execute(std::make_unique<app::MoveInsertCommand>(
            project::EntityId{[info[@"node"] unsignedLongLongValue]},
            project::EntityId{[info[@"slot"] unsignedLongLongValue]}, direction)))
        [self structuralChange];
}

- (void)moveInsertUpFromMenu:(NSMenuItem*)item
{
    [self moveInsert:item direction:-1];
}

- (void)moveInsertDownFromMenu:(NSMenuItem*)item
{
    [self moveInsert:item direction:1];
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

- (void)addPreFaderSendFromMenu:(NSMenuItem*)item
{
    const project::EntityId nodeId{[item.representedObject unsignedLongLongValue]};

    if (nodeId == _project->masterMixerNode())
        return;

    [self commitStructural:std::make_unique<app::ConnectMixerCommand>(
        nodeId, _project->masterMixerNode(), true, 0.25, true)];
}

- (void)setSeparationFromMenu:(NSMenuItem*)item
{
    NSDictionary* info = item.representedObject;

    [self commitStructural:std::make_unique<app::SetMixerStereoSeparationCommand>(
        project::EntityId{[info[@"node"] unsignedLongLongValue]},
        [info[@"separation"] doubleValue])];
}

- (void)addSidechainFromMenu:(NSMenuItem*)item
{
    NSDictionary* info = item.representedObject;
    const project::EntityId source{[info[@"source"] unsignedLongLongValue]};
    const project::EntityId destination{[info[@"destination"] unsignedLongLongValue]};

    [self commitStructural:std::make_unique<app::ConnectMixerCommand>(
        source, destination, false, 1.0, false, true)];
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
