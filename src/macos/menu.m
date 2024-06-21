#include <AppKit/AppKit.h>
#include <Cocoa/Cocoa.h>
#include <Foundation/Foundation.h>
#include <ScreenCaptureKit/ScreenCaptureKit.h>
#include <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#include <objc/NSObjCRuntime.h>
#include <objc/NSObject.h>
#include <objc/objc.h>
#include <stdio.h>

#include "../state.h"

@interface MenuMethods : NSObject
@end

@implementation MenuMethods
- (void)load {
    NSArray *filetypes = [[NSArray alloc]
        initWithObjects:[UTType typeWithFilenameExtension:@"mp3"],
                        [UTType typeWithFilenameExtension:@"wav"], nil];

    NSOpenPanel *panel = [NSOpenPanel openPanel];
    [panel setCanChooseFiles:YES];
    [panel setCanChooseDirectories:NO];
    [panel setAllowsMultipleSelection:NO];
    [panel setAllowedContentTypes:filetypes];

    [panel beginSheetModalForWindow:nil
                  completionHandler:^(NSInteger result) {
                    if (result == NSFileHandlingPanelOKButton) {
                        NSURL *selectedURL = [[panel URLs] objectAtIndex:0];

                        StateLoadFile([[selectedURL path] cString]);
                    }
                  }];
}

- (void)loopback {
    StateToggleLoopback();
}

- (void)togglePlayPause {
    StateTogglePlayPause();
}
@end

@interface MenuItem : NSObject
@property bool seperator;
@property(strong) NSString *name;
@property(strong) NSValue *method;
@property(strong) NSString *keyEquivalent;
@property(strong) NSMenuItem *item;
@property NSInteger index;
@end

@implementation MenuItem
+ (id)withName:(NSString *)name
           withMethod:(SEL)method
    withKeyEquivalent:(NSString *)keyEquivalent {
    return [[MenuItem alloc] initWithName:name
                               withMethod:method
                        withKeyEquivalent:keyEquivalent];
}

+ (id)seperatorItem {
    MenuItem *m = [MenuItem alloc];
    [m setSeperator:true];
    return m;
}

- (id)initWithName:(NSString *)name
           withMethod:(SEL)method
    withKeyEquivalent:(NSString *)keyEquivalent {
    [self setName:name];
    [self setMethod:[NSValue valueWithPointer:method]];
    [self setKeyEquivalent:keyEquivalent];
    return self;
}

- (SEL)getMethod {
    return [[self method] pointerValue];
}
@end

@interface MenuBuilder : NSObject
@property(strong) NSMenu *menubar;
@property(strong) NSMutableDictionary<NSString *, MenuItem *> *items;

@end

@implementation MenuBuilder
- (id)init {
    [self setMenubar:[NSMenu new]];

    NSMenuItem *appMenu = [NSMenuItem new];
    NSMenu *appDropdown = [NSMenu new];
    [appDropdown addItemWithTitle:@"Quit"
                           action:@selector(terminate:)
                    keyEquivalent:@"q"];
    [appMenu setSubmenu:appDropdown];
    [[self menubar] addItem:appMenu];
    [self setItems:[[NSMutableDictionary alloc] init]];

    return self;
}

- (void)setMainMenu {
    [NSApp setMainMenu:[self menubar]];
}

- (void)addMenuDropdown:(NSString *)name
              withItems:(NSArray<MenuItem *> *)items
             withTarget:(id)target {
    NSMenuItem *menu = [NSMenuItem new];
    NSMenu *dropdown = [[NSMenu alloc] initWithTitle:name];

    for (id item in items) {
        if ([item seperator]) {
            [dropdown addItem:[NSMenuItem separatorItem]];
            continue;
        }

        SEL method = [[item method] pointerValue];
        NSMenuItem *newItem =
            [[NSMenuItem alloc] initWithTitle:[item name]
                                       action:method
                                keyEquivalent:[item keyEquivalent]];

        [item setItem:newItem];
        [item setIndex:[dropdown numberOfItems]];
        [newItem setTarget:target];
        [dropdown addItem:newItem];
        [[self items] setObject:item forKey:[item name]];
    }

    [menu setSubmenu:dropdown];
    [[self menubar] addItem:menu];
}

- (void)changeItemTitle:(MenuItem *)item withNewTitle:(NSString *)newTitle {
    [[item item] setTitle:newTitle];
}

- (void)changeItemTitleWithTitle:(NSString *)title
                    withNewTitle:(NSString *)newTitle {
    MenuItem *item = [[self items] objectForKey:title];

    [[item item] setTitle:newTitle];

    // Update items dictionary
    [[self items] removeObjectForKey:title];
    [[self items] setObject:item forKey:newTitle];
}
@end

typedef struct MenuData {
    MenuBuilder *builder;
} MenuData;

void MenuTogglePlayPause(MenuData *data) {
    if (StateIsPaused()) {
        [data->builder changeItemTitleWithTitle:@"Play" withNewTitle:@"Pause"];
        return;
    }

    [data->builder changeItemTitleWithTitle:@"Pause" withNewTitle:@"Play"];
}

MenuData *MenuCreate(MemoryArena *arena) {
    MenuData *data = ArenaPushStruct(arena, MenuData);

    MenuMethods *m = [MenuMethods new];

    MenuBuilder *builder = [[MenuBuilder alloc] init];
    [builder setMainMenu];

    MenuItem *openItem = [MenuItem withName:@"Open"
                                 withMethod:@selector(load)
                          withKeyEquivalent:@"o"];

    [builder addMenuDropdown:@"File" withItems:@[ openItem ] withTarget:m];

    MenuItem *togglePlayPauseItem =
        [MenuItem withName:StateIsPaused() ? @"Play" : @"Pause"
                   withMethod:@selector(togglePlayPause)
            withKeyEquivalent:@" "];

    MenuItem *toggleLoopbackItem = [MenuItem withName:@"Toggle Loopback"
                                           withMethod:@selector(loopback)
                                    withKeyEquivalent:@"L"];

    [builder addMenuDropdown:@"Playback"
                   withItems:@[
                       togglePlayPauseItem, [MenuItem seperatorItem],
                       toggleLoopbackItem
                   ]
                  withTarget:m];

    data->builder = builder;

    return data;
}
