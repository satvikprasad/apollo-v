#include <AppKit/AppKit.h>
#include <Cocoa/Cocoa.h>
#include <Foundation/Foundation.h>
#include <ScreenCaptureKit/ScreenCaptureKit.h>
#include <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
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
    StateSetLoopback(!StateGetLoopback());
}
@end

void MacOSCreateMenus() {
    MenuMethods *m = [MenuMethods new];

    NSMenu *menubar = [NSMenu new];
    [NSApp setMainMenu:menubar];

    NSMenuItem *appMenu = [NSMenuItem new];
    NSMenu *appDropdown = [NSMenu new];
    [appDropdown addItemWithTitle:@"Quit"
                           action:@selector(terminate:)
                    keyEquivalent:@"q"];
    [appMenu setSubmenu:appDropdown];
    [menubar addItem:appMenu];

    NSMenuItem *fileMenu = [NSMenuItem new];
    NSMenu *fileMenuDropdown = [[NSMenu alloc] initWithTitle:@"File"];
    NSMenuItem *load = [[NSMenuItem alloc] initWithTitle:@"Open"
                                                  action:@selector(load)
                                           keyEquivalent:@"o"];
    [load setTarget:m];
    [fileMenuDropdown addItem:load];
    [fileMenu setSubmenu:fileMenuDropdown];
    [menubar addItem:fileMenu];

    NSMenuItem *playbackMenu = [NSMenuItem new];
    NSMenu *playbackMenuDropdown = [[NSMenu alloc] initWithTitle:@"Playback"];
    NSMenuItem *loopback = [[NSMenuItem alloc] initWithTitle:@"Toggle Loopback"
                                                      action:@selector(loopback)
                                               keyEquivalent:@"L"];
    [loopback setTarget:m];
    [playbackMenuDropdown addItem:loopback];
    [playbackMenu setSubmenu:playbackMenuDropdown];
    [menubar addItem:playbackMenu];
}
