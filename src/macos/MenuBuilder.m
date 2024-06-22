#include <AppKit/AppKit.h>
#include <Foundation/Foundation.h>

#include "MenuBuilder.h"

@implementation MenuProcedureToggle
- (instancetype)initWithFrame:(NSRect)frameRect {
    MenuProcedureToggle *tog = [super initWithFrame:frameRect];
    [tog setButtonType:NSButtonTypeSwitch];
    return tog;
}

+ (MenuItem *)toggleItemWithName:(NSString *)name
                      withAction:(SEL)action
                      withTarget:(id)target
                       withState:(bool)state {
    NSView *toggleView =
        [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 100, 30)];

    MenuProcedureToggle *sw =
        [[MenuProcedureToggle alloc] initWithFrame:NSMakeRect(10, 0, 100, 30)];

    [sw setAction:action];
    [sw setTarget:target];
    [sw setParameterName:name];
    [sw setState:state];
    [sw setTitle:[NSString stringWithFormat:@"Proc: %@", name]];
    [toggleView addSubview:sw];

    return [MenuItem withName:name
                   withMethod:nil
            withKeyEquivalent:@""
                     withView:toggleView];
}
@end

@implementation MenuItem
+ (id)withName:(NSString *)name
           withMethod:(SEL)method
    withKeyEquivalent:(NSString *)keyEquivalent {
    return [[MenuItem alloc] initWithName:name
                               withMethod:method
                        withKeyEquivalent:keyEquivalent];
}
+ (id)withName:(NSString *)name
           withMethod:(SEL)method
    withKeyEquivalent:(NSString *)keyEquivalent
             withView:(NSView *)view {
    MenuItem *item = [MenuItem withName:name
                             withMethod:method
                      withKeyEquivalent:keyEquivalent];
    [item setView:view];
    return item;
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

@implementation MenuBuilder
- (id)initWithTarget:(id)target {
    [self setMenubar:[NSMenu new]];
    [self setTarget:target];

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
              withItems:(NSArray<MenuItem *> *)items {
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
        [newItem setTarget:[self target]];

        if ([item view]) {
            [newItem setView:[item view]];
        }

        [dropdown addItem:newItem];
        [[self items] setObject:item forKey:[item name]];
    }

    [menu setSubmenu:dropdown];
    [[self menubar] addItem:menu];
}

- (void)changeItemTitle:(MenuItem *)item withNewTitle:(NSString *)newTitle {
    [[item item] setTitle:newTitle];

    // Update items dictionary
    [[self items] removeObjectForKey:[item name]];
    [[self items] setObject:item forKey:newTitle];

    [item setName:newTitle];
}

- (void)changeItemTitleWithTitle:(NSString *)title
                    withNewTitle:(NSString *)newTitle {
    MenuItem *item = [[self items] objectForKey:title];

    if (item) {
        [self changeItemTitle:item withNewTitle:newTitle];
    }
}

- (void)setItemEnabled:(MenuItem *)item to:(bool)enabled {
    [[item item] setTarget:enabled ? [self target] : nil];
}

- (void)setItemEnabledWithTitle:(NSString *)title to:(bool)enabled {
    [self setItemEnabled:[[self items] objectForKey:title] to:enabled];
}
@end
