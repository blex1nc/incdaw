#import "ui/macos/InsertParameterPanel.h"

#include "ui/macos/Theme.h"

#import <objc/runtime.h>

#include <algorithm>
#include <cmath>

namespace theme = incdaw::ui::theme;
using incdaw::ui::theme::Ink;

@implementation INCDAWFlippedView
- (BOOL)isFlipped
{
    return YES;
}

/// The panel's own ground. A parameter list is a pane like any other, and a
/// window that arrives in AppKit's default grey reads as a different program.
- (void)drawRect:(NSRect)dirtyRect
{
    (void)dirtyRect;
    theme::fillRect(self.bounds, theme::ink(Ink::panel));
}
@end

namespace {

constexpr CGFloat rowHeight    = 28.0;
constexpr CGFloat panelWidth   = 380.0;
constexpr CGFloat labelWidth   = 120.0;
constexpr CGFloat valueWidth   = 64.0;
constexpr CGFloat margin       = 10.0;
constexpr CGFloat visibleLimit = 420.0;   ///< taller lists scroll

/// The window keeps its panel object alive through this association; the
/// sliders point their target at the panel object, so the two must share a
/// lifetime without the shell having to track both.
const void* panelOwnerKey = &panelOwnerKey;

NSString* formattedValue(double value, BOOL stepped)
{
    return stepped ? [NSString stringWithFormat:@"%.0f", value]
                   : [NSString stringWithFormat:@"%.3g", value];
}

} // namespace

@implementation INCDAWInsertParameterPanel {
    NSArray<NSDictionary*>*     _rows;
    NSArray<NSTextField*>*      _labels;
    NSArray<NSTextField*>*      _valueFields;
    NSArray<NSSlider*>*         _sliders;
    NSScrollView*               _scroll;
    void (^_onWrite)(std::uint32_t, double);
}

+ (NSWindow*)makePanelWithTitle:(NSString*)title
                           rows:(NSArray<NSDictionary*>*)rows
                        onWrite:(void (^)(std::uint32_t, double))onWrite
{
    INCDAWInsertParameterPanel* panel = [[INCDAWInsertParameterPanel alloc] init];
    panel->_rows    = rows;
    panel->_onWrite = onWrite;

    const CGFloat contentHeight = margin * 2 + rowHeight * static_cast<CGFloat>(rows.count);

    INCDAWFlippedView* document = [[INCDAWFlippedView alloc]
        initWithFrame:NSMakeRect(0, 0, panelWidth, contentHeight)];

    NSMutableArray<NSTextField*>* labels      = [NSMutableArray array];
    NSMutableArray<NSTextField*>* valueFields = [NSMutableArray array];
    NSMutableArray<NSSlider*>*    sliders     = [NSMutableArray array];

    [rows enumerateObjectsUsingBlock:^(NSDictionary* row, NSUInteger index, BOOL*) {
        const CGFloat y = margin + rowHeight * static_cast<CGFloat>(index);

        NSTextField* label  = [NSTextField labelWithString:row[@"name"]];
        label.frame         = NSMakeRect(margin, y + 4, labelWidth, 18);
        label.lineBreakMode = NSLineBreakByTruncatingTail;
        label.font          = theme::labelFont(11.0);
        label.textColor     = theme::ink(Ink::textSecondary);
        [document addSubview:label];
        [labels addObject:label];

        const double minValue = [row[@"min"] doubleValue];
        const double maxValue = [row[@"max"] doubleValue];
        const double value    = [row[@"value"] doubleValue];
        const BOOL   stepped  = [row[@"stepped"] boolValue];

        NSSlider* slider = [NSSlider sliderWithValue:value
                                            minValue:minValue
                                            maxValue:maxValue
                                              target:panel
                                              action:@selector(sliderMoved:)];
        slider.frame      = NSMakeRect(margin + labelWidth + 8, y,
                                       panelWidth - labelWidth - valueWidth - margin * 2 - 16,
                                       rowHeight - 4);
        slider.continuous = YES;
        slider.tag        = static_cast<NSInteger>(index);

        if (stepped && maxValue > minValue) {
            slider.numberOfTickMarks        = static_cast<NSInteger>(maxValue - minValue) + 1;
            slider.allowsTickMarkValuesOnly = YES;
        }

        [document addSubview:slider];
        [sliders addObject:slider];

        NSTextField* valueField = [NSTextField labelWithString:formattedValue(value, stepped)];
        valueField.frame        = NSMakeRect(panelWidth - valueWidth - margin, y + 4,
                                             valueWidth, 18);
        valueField.alignment    = NSTextAlignmentRight;
        valueField.font         = theme::numericFont(11.0, NSFontWeightRegular);
        valueField.textColor    = theme::ink(Ink::lcdText);
        [document addSubview:valueField];
        [valueFields addObject:valueField];
    }];

    panel->_labels      = labels;
    panel->_valueFields = valueFields;
    panel->_sliders     = sliders;

    const CGFloat visibleHeight = std::min(contentHeight, visibleLimit);

    NSScrollView* scroll =
        [[NSScrollView alloc] initWithFrame:NSMakeRect(0, 0, panelWidth, visibleHeight)];
    scroll.hasVerticalScroller = contentHeight > visibleLimit;
    scroll.documentView        = document;
    scroll.drawsBackground     = YES;
    scroll.backgroundColor     = theme::ink(Ink::panel);

    NSWindow* window = [[NSWindow alloc]
        initWithContentRect:NSMakeRect(0, 0, panelWidth, visibleHeight)
                  styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable
                    backing:NSBackingStoreBuffered
                      defer:NO];

    window.releasedWhenClosed = NO;
    window.title              = title;
    window.contentView        = scroll;
    window.appearance         = [NSAppearance appearanceNamed:theme::paletteIsLight()
                                                                  ? NSAppearanceNameAqua
                                                                  : NSAppearanceNameDarkAqua];
    window.backgroundColor    = theme::ink(Ink::windowBackground);

    panel->_scroll = scroll;

    objc_setAssociatedObject(window, panelOwnerKey, panel,
                             OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    return window;
}

+ (void)refreshWindow:(NSWindow*)window
               values:(NSDictionary<NSNumber*, NSNumber*>*)values
{
    INCDAWInsertParameterPanel* panel =
        objc_getAssociatedObject(window, panelOwnerKey);
    if (panel == nil)
        return;

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
            panel->_sliders[index].doubleValue      = plain;
            panel->_valueFields[index].stringValue = formattedValue(plain, stepped);
        }
    }];
}

+ (void)refreshAppearance:(NSWindow*)window
{
    INCDAWInsertParameterPanel* panel =
        objc_getAssociatedObject(window, panelOwnerKey);
    if (panel == nil)
        return;

    for (NSTextField* label in panel->_labels)
        label.textColor = theme::ink(Ink::textSecondary);

    for (NSTextField* field in panel->_valueFields)
        field.textColor = theme::ink(Ink::lcdText);

    panel->_scroll.backgroundColor = theme::ink(Ink::panel);
}

- (void)sliderMoved:(NSSlider*)slider
{
    const NSUInteger index = static_cast<NSUInteger>(slider.tag);
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
