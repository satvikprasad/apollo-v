#include <AppKit/AppKit.h>
#include <Foundation/Foundation.h>

#include "MenuBuilder.h"

@implementation MenuItemToggle
+ (MenuItemToggle *)withToggleOn:(NSString *)on
                         withOff:(NSString *)off
                      withGetter:(ToggleGetter)g
                     withToggler:(ToggleToggler)t {
    MenuItemToggle *toggle = [MenuItemToggle alloc];
    [toggle setOn:on];
    [toggle setOff:off];
    [toggle setG:g];
    [toggle setT:t];

    return toggle;
}

- (void)toggle:(id)sender {
    if ([self g]()) {
        [[self builder] changeItemTitleWithTitle:[self on]
                                    withNewTitle:[self off]];
    } else {
        [[self builder] changeItemTitleWithTitle:[self off]
                                    withNewTitle:[self on]];
    }

    [self t]();

    if ([self method]) {
        [[self target] performSelector:[[self method] pointerValue]];
    }
}
@end

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

+ (id)withToggleOn:(NSString *)on
            toggleOff:(NSString *)off
           withGetter:(ToggleGetter)g
          withToggler:(ToggleToggler)t
           withMethod:(SEL)method
    withKeyEquivalent:(NSString *)keyEquivalent {
    MenuItem *item = [MenuItem withName:g() ? on : off
                             withMethod:method
                      withKeyEquivalent:keyEquivalent];

    [item setToggle:[MenuItemToggle withToggleOn:on
                                         withOff:off
                                      withGetter:g
                                     withToggler:t]];

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

    if (method) {
        [self setMethod:[NSValue valueWithPointer:method]];
    }

    [self setKeyEquivalent:keyEquivalent];
    [self setToggle:nil];
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

        NSMenuItem *newItem;

        if ([item toggle]) {
            newItem = [[NSMenuItem alloc] initWithTitle:[item name]
                                                 action:@selector(toggle:)
                                          keyEquivalent:[item keyEquivalent]];
            [[item toggle] setBuilder:self];
            [[item toggle] setTarget:[self target]];
            [[item toggle] setMethod:[item method]];
            [newItem setTarget:[item toggle]];
        } else {
            SEL method = [[item method] pointerValue];
            newItem = [[NSMenuItem alloc] initWithTitle:[item name]
                                                 action:method
                                          keyEquivalent:[item keyEquivalent]];
            [newItem setTarget:[self target]];
        }

        [item setItem:newItem];
        [item setIndex:[dropdown numberOfItems]];

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
