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

    MenuItem *togglePlayPauseItem = [MenuItem withToggleOn:@"Play"
                                                 toggleOff:@"Pause"
                                                withGetter:StateIsPaused
                                               withToggler:StateTogglePlayPause
                                                withMethod:nil
                                         withKeyEquivalent:@" "];

    MenuItem *toggleLoopbackItem = [MenuItem withToggleOn:@"Disable Passthrough"
                                                toggleOff:@"Enable Passthrough"
                                               withGetter:StateGetLoopback
                                              withToggler:StateToggleLoopback
                                               withMethod:nil
                                        withKeyEquivalent:@"L"];

    MenuItem *toggleMutedItem = [MenuItem withToggleOn:@"Unmute"
                                             toggleOff:@"Mute"
                                            withGetter:StateGetMuted
                                           withToggler:StateToggleMuted
                                            withMethod:nil
                                     withKeyEquivalent:@"M"];

    [builder addMenuDropdown:@"Playback"
                   withItems:@[
                       togglePlayPauseItem, toggleMutedItem,
                       [MenuItem seperatorItem], toggleLoopbackItem
                   ]];

    NSMutableArray *viewMenu = [[NSMutableArray alloc] initWithArray:@[
        [MenuItem withToggleOn:@"Hide Menu"
                     toggleOff:@"Show Menu"
                    withGetter:StateIsShowingMenu
                   withToggler:StateToggleMenu
                    withMethod:nil
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
