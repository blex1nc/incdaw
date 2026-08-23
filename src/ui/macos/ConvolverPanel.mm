#import "ui/macos/ConvolverPanel.h"

#include "engine/dsp/effects/ConvolutionReverb.h"
#include "ui/macos/InsertParameterPanel.h"
#include "ui/macos/Theme.h"

#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#import <objc/runtime.h>

#include <algorithm>
#include <cmath>

namespace theme = incdaw::ui::theme;
namespace dsp   = incdaw::engine::dsp;
using incdaw::ui::theme::Ink;

namespace {

constexpr CGFloat panelWidth = 400.0;
constexpr CGFloat rowHeight  = 28.0;
constexpr CGFloat margin     = 12.0;
constexpr CGFloat fileHeight = 56.0;

constexpr const char* generatedName = "Generated hall";

const void* panelOwnerKey = &panelOwnerKey;

NSString* formattedValue(double value, BOOL stepped)
{
    return stepped ? [NSString stringWithFormat:@"%.0f", value]
                   : [NSString stringWithFormat:@"%.4g", value];
}

} // namespace

@implementation INCDAWConvolverPanel {
    NSArray<NSDictionary*>* _rows;
    NSArray<NSSlider*>*     _sliders;
    NSArray<NSTextField*>*  _labels;
    NSArray<NSTextField*>*  _valueFields;
    NSTextField*            _fileField;
    void (^_onWrite)(std::uint32_t, double);
    void (^_onImpulse)(NSString* _Nullable);
}

+ (nullable NSWindow*)makePanelWithTitle:(NSString*)title
                                    rows:(NSArray<NSDictionary*>*)rows
                                 impulse:(nullable NSString*)impulse
                                 onWrite:(void (^)(std::uint32_t, double))onWrite
                               onImpulse:(void (^)(NSString* _Nullable))onImpulse
{
    if (rows.count == 0)
        return nil;

    INCDAWConvolverPanel* panel = [[INCDAWConvolverPanel alloc] init];
    panel->_rows      = rows;
    panel->_onWrite   = onWrite;
    panel->_onImpulse = onImpulse;

    const CGFloat height =
        margin * 2 + fileHeight + rowHeight * static_cast<CGFloat>(rows.count);

    INCDAWFlippedView* content =
        [[INCDAWFlippedView alloc] initWithFrame:NSMakeRect(0, 0, panelWidth, height)];

    // ── The file field, at the top, because it is the parameter that
    //    matters most and is not a number.
    NSTextField* caption  = [NSTextField labelWithString:@"Impulse"];
    caption.frame         = NSMakeRect(margin, margin, 60, 18);
    caption.font          = theme::labelFont(11.0);
    caption.textColor     = theme::ink(Ink::textSecondary);
    [content addSubview:caption];

    NSTextField* file = [NSTextField labelWithString:impulse != nil
                                                         ? impulse.lastPathComponent
                                                         : @(generatedName)];
    file.frame         = NSMakeRect(margin + 64, margin, panelWidth - margin * 2 - 64, 18);
    file.lineBreakMode = NSLineBreakByTruncatingHead;
    file.font          = theme::labelFont(11.0);
    file.textColor     = theme::ink(Ink::lcdText);
    file.toolTip       = impulse;
    [content addSubview:file];

    NSButton* choose = [NSButton buttonWithTitle:@"Load Impulse…"
                                          target:panel
                                          action:@selector(chooseImpulse:)];
    choose.frame     = NSMakeRect(margin, margin + 24, 140, 24);
    choose.bezelStyle = NSBezelStyleRounded;
    [content addSubview:choose];

    NSButton* clear = [NSButton buttonWithTitle:@"Use Generated"
                                         target:panel
                                         action:@selector(clearImpulse:)];
    clear.frame      = NSMakeRect(margin + 148, margin + 24, 140, 24);
    clear.bezelStyle = NSBezelStyleRounded;
    [content addSubview:clear];

    NSMutableArray<NSSlider*>*    sliders     = [NSMutableArray array];
    NSMutableArray<NSTextField*>* labels      = [NSMutableArray array];
    NSMutableArray<NSTextField*>* valueFields = [NSMutableArray array];

    [rows enumerateObjectsUsingBlock:^(NSDictionary* row, NSUInteger index, BOOL*) {
        const CGFloat y = margin + fileHeight + rowHeight * static_cast<CGFloat>(index);

        NSTextField* label  = [NSTextField labelWithString:row[@"name"]];
        label.frame         = NSMakeRect(margin, y + 4, 90, 18);
        label.font          = theme::labelFont(11.0);
        label.textColor     = theme::ink(Ink::textSecondary);
        [content addSubview:label];
        [labels addObject:label];

        const double value   = [row[@"value"] doubleValue];
        const BOOL   stepped = [row[@"stepped"] boolValue];

        NSSlider* slider = [NSSlider sliderWithValue:value
                                            minValue:[row[@"min"] doubleValue]
                                            maxValue:[row[@"max"] doubleValue]
                                              target:panel
                                              action:@selector(sliderMoved:)];
        slider.frame      = NSMakeRect(margin + 96, y, panelWidth - margin * 2 - 96 - 64,
                                       rowHeight - 4);
        slider.continuous = YES;
        slider.tag        = static_cast<NSInteger>(index);

        if (stepped) {
            const double span =
                [row[@"max"] doubleValue] - [row[@"min"] doubleValue];
            slider.numberOfTickMarks        = static_cast<NSInteger>(span) + 1;
            slider.allowsTickMarkValuesOnly = YES;
        }

        [content addSubview:slider];
        [sliders addObject:slider];

        NSTextField* readout = [NSTextField labelWithString:formattedValue(value, stepped)];
        readout.frame        = NSMakeRect(panelWidth - margin - 58, y + 4, 58, 18);
        readout.alignment    = NSTextAlignmentRight;
        readout.font         = theme::numericFont(11.0, NSFontWeightRegular);
        readout.textColor    = theme::ink(Ink::lcdText);
        [content addSubview:readout];
        [valueFields addObject:readout];
    }];

    panel->_sliders     = sliders;
    panel->_labels      = labels;
    panel->_valueFields = valueFields;
    panel->_fileField   = file;

    NSWindow* window = [[NSWindow alloc]
        initWithContentRect:NSMakeRect(0, 0, panelWidth, height)
                  styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable
                    backing:NSBackingStoreBuffered
                      defer:NO];

    window.releasedWhenClosed = NO;
    window.title              = title;
    window.contentView        = content;
    window.appearance         = [NSAppearance appearanceNamed:theme::paletteIsLight()
                                                                  ? NSAppearanceNameAqua
                                                                  : NSAppearanceNameDarkAqua];
    window.backgroundColor    = theme::ink(Ink::windowBackground);

    objc_setAssociatedObject(window, panelOwnerKey, panel, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    return window;
}

+ (void)refreshWindow:(NSWindow*)window
               values:(NSDictionary<NSNumber*, NSNumber*>*)values
              impulse:(nullable NSString*)impulse
{
    INCDAWConvolverPanel* panel = objc_getAssociatedObject(window, panelOwnerKey);
    if (panel == nil)
        return;

    NSString* wanted = impulse != nil ? impulse.lastPathComponent : @(generatedName);
    if (![panel->_fileField.stringValue isEqualToString:wanted]) {
        panel->_fileField.stringValue = wanted;
        panel->_fileField.toolTip     = impulse;
    }

    // A drag in progress owns the sliders; the next tick catches up.
    if ((NSEvent.pressedMouseButtons & 1) != 0)
        return;

    [panel->_rows enumerateObjectsUsingBlock:^(NSDictionary* row, NSUInteger index, BOOL*) {
        NSNumber* value = values[row[@"id"]];
        if (value == nil || index >= panel->_sliders.count)
            return;

        const BOOL   stepped = [row[@"stepped"] boolValue];
        const double plain   = stepped ? std::round(value.doubleValue) : value.doubleValue;

        if (panel->_sliders[index].doubleValue != plain) {
            panel->_sliders[index].doubleValue     = plain;
            panel->_valueFields[index].stringValue = formattedValue(plain, stepped);
        }
    }];
}

+ (void)refreshAppearance:(NSWindow*)window
{
    INCDAWConvolverPanel* panel = objc_getAssociatedObject(window, panelOwnerKey);
    if (panel == nil)
        return;

    for (NSTextField* label in panel->_labels)
        label.textColor = theme::ink(Ink::textSecondary);

    for (NSTextField* field in panel->_valueFields)
        field.textColor = theme::ink(Ink::lcdText);

    panel->_fileField.textColor = theme::ink(Ink::lcdText);
}

- (void)chooseImpulse:(id)sender
{
    (void)sender;

    NSOpenPanel* open = [NSOpenPanel openPanel];
    open.canChooseFiles          = YES;
    open.canChooseDirectories    = NO;
    open.allowsMultipleSelection = NO;
    open.message                 = @"Choose an impulse response";

    if (@available(macOS 11.0, *))
        open.allowedContentTypes = @[UTTypeWAV];

    if ([open runModal] != NSModalResponseOK || open.URL == nil)
        return;

    if (_onImpulse != nil)
        _onImpulse(open.URL.path);
}

- (void)clearImpulse:(id)sender
{
    (void)sender;

    if (_onImpulse != nil)
        _onImpulse(nil);
}

- (void)sliderMoved:(NSSlider*)slider
{
    const auto index = static_cast<NSUInteger>(slider.tag);
    if (index >= _rows.count)
        return;

    NSDictionary* row     = _rows[index];
    const BOOL    stepped = [row[@"stepped"] boolValue];
    const double  value   = stepped ? std::round(slider.doubleValue) : slider.doubleValue;

    _valueFields[index].stringValue = formattedValue(value, stepped);

    if (_onWrite != nil)
        _onWrite([row[@"id"] unsignedIntValue], value);
}

@end
