#pragma once

#import <Cocoa/Cocoa.h>

/// The pasteboard type a picker row carries: one string, the
/// PluginIdentifier's own round-trippable form ("builtin:incdaw.eq").
///
/// Private to INCDAW on purpose. A plugin choice means nothing outside this
/// application, and a drag that left the window carrying a file URL would
/// promise something it cannot deliver.
extern NSPasteboardType const INCDAWPluginPasteboardType;

/// The mixer's plugin picker: search at the top, the catalogue below, grouped.
///
/// A row is chosen by double-click or Return (it goes into the selected
/// strip's first free slot), or dragged onto the slot it should occupy. The
/// view owns no catalogue of its own: builtin effects come from the engine,
/// scanned plugins are handed over by the shell, exactly as the insert menu
/// already receives them.
@interface INCDAWPluginPickerView : NSView

- (instancetype)initWithFrame:(NSRect)frame;

/// Scanned plugins, as the shell lists them: @{@"id": ..., @"name": ...}.
@property (nonatomic, copy) NSArray<NSDictionary*>* hostedPlugins;

/// A row was chosen. `identifier` is a PluginIdentifier string.
@property (nonatomic, copy) void (^onChoose)(NSString* identifier);

/// Puts the caret in the search field — what clicking an empty insert slot
/// does, so the next thing typed is the search for what goes in it.
- (void)focusSearch;

@end
