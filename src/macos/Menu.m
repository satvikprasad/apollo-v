#include <AppKit/AppKit.h>
#include <Cocoa/Cocoa.h>
#include <Foundation/Foundation.h>
#include <ScreenCaptureKit/ScreenCaptureKit.h>
#include <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#include <objc/NSObjCRuntime.h>
#include <objc/NSObject.h>
#include <objc/objc.h>
#include <stdio.h>
#include <sys/types.h>

#import "MenuBuilder.h"

#include "../state.h"

@interface MenuMethods : NSObject
@property(strong) MenuBuilder *builder;
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

                        if (StateLoadFile([[selectedURL path] cString])) {
                            StateSetCondition(StateCondition_NORMAL);
                        }
                    }
                  }];
}

- (void)loopback {
    if (StateGetLoopback()) {
        [[self builder] changeItemTitleWithTitle:@"Disable Passthrough"
                                    withNewTitle:@"Enable Passthrough"];
    } else {
        [[self builder] changeItemTitleWithTitle:@"Enable Passthrough"
                                    withNewTitle:@"Disable Passthrough"];
    }

    StateToggleLoopback();

    [[self builder] setItemEnabledWithTitle:@"Play" to:!StateGetLoopback()];
    [[self builder] setItemEnabledWithTitle:@"Pause" to:!StateGetLoopback()];

    [[self builder] changeItemTitleWithTitle:@"Play" withNewTitle:@"Pause"];
}

- (void)togglePlayPause {
    if (StateIsPaused()) {
        [[self builder] changeItemTitleWithTitle:@"Play" withNewTitle:@"Pause"];
        return;
    }

    [[self builder] changeItemTitleWithTitle:@"Pause" withNewTitle:@"Play"];

    StateTogglePlayPause();
}

- (void)toggleMenu {
    if (StateIsShowingMenu()) {
        [[self builder] changeItemTitleWithTitle:@"Hide Menu"
                                    withNewTitle:@"Show Menu"];
    } else {
        [[self builder] changeItemTitleWithTitle:@"Show Menu"
                                    withNewTitle:@"Hide Menu"];
    }

    StateToggleMenu();
}

- (void)toggleSwitch:(MenuProcedureToggle *)sender {
    StateToggleProcedure([[sender parameterName] cString]);
}
@end

typedef struct MenuData {
    MenuBuilder *builder;
} MenuData;

MenuData *MenuCreate(MemoryArena *arena) {
    MenuData *data = ArenaPushStruct(arena, MenuData);

    MenuMethods *m = [MenuMethods new];
    MenuBuilder *builder = [[MenuBuilder alloc] initWithTarget:m];
    [builder setMainMenu];

    [m setBuilder:builder];

    MenuItem *openItem = [MenuItem withName:@"Open"
                                 withMethod:@selector(load)
                          withKeyEquivalent:@"o"];

    [builder addMenuDropdown:@"File" withItems:@[ openItem ]];

    MenuItem *togglePlayPauseItem =
        [MenuItem withName:StateIsPaused() ? @"Play" : @"Pause"
                   withMethod:@selector(togglePlayPause)
            withKeyEquivalent:@" "];

    MenuItem *toggleLoopbackItem = [MenuItem withName:@"Enable Passthrough"
                                           withMethod:@selector(loopback)
                                    withKeyEquivalent:@"L"];

    [builder addMenuDropdown:@"Playback"
                   withItems:@[
                       togglePlayPauseItem, [MenuItem seperatorItem],
                       toggleLoopbackItem
                   ]];

    NSMutableArray *viewMenu = [[NSMutableArray alloc] initWithArray:@[
        [MenuItem withName:@"Show Menu"
                   withMethod:@selector(toggleMenu)
            withKeyEquivalent:@"m"],
        [MenuItem seperatorItem]
    ]];

    uint i;
    Procedure *proc;
    while (StateIterProcedures(&i, &proc)) {
        [viewMenu
            addObject:[MenuProcedureToggle
                          toggleItemWithName:[[NSString alloc]
                                                 initWithCString:proc->name]
                                  withAction:@selector(toggleSwitch:)
                                  withTarget:m
                                   withState:proc->active]];
    }

    [builder addMenuDropdown:@"View" withItems:viewMenu];

    data->builder = builder;
    return data;
}
