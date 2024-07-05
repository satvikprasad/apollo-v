#include <AppKit/AppKit.h>
#import <Foundation/Foundation.h>
#include <objc/NSObject.h>

typedef bool (*ToggleGetter)(void);
typedef void (*ToggleToggler)(void);

@class MenuBuilder;

@interface MenuItemToggle : NSObject
@property(strong) MenuBuilder *builder;
@property(strong) NSString *on;
@property(strong) NSString *off;
@property ToggleGetter g;
@property ToggleToggler t;

@property(strong) NSValue * method;
@property(strong) id target;

- (void)toggle:(id)sender;
@end

@interface MenuItem : NSObject
@property bool seperator;
@property(strong) NSString *name;
@property(strong) NSValue *method;
@property(strong) NSString *keyEquivalent;
@property(strong) NSMenuItem *item;
@property(strong) NSView *view;
@property(strong) MenuItemToggle *toggle;
@property NSInteger index;

+ (id)withName:(NSString *)name
           withMethod:(SEL)method
    withKeyEquivalent:(NSString *)keyEquivalent;

+ (id)withName:(NSString *)name
           withMethod:(SEL)method
    withKeyEquivalent:(NSString *)keyEquivalent
    withView:(NSView *)view;

+ (id)withToggleOn:(NSString *)on
             toggleOff:(NSString *)off
           withGetter:(ToggleGetter)g
           withToggler:(ToggleToggler)s
           withMethod:(SEL)method
    withKeyEquivalent:(NSString *)keyEquivalent;

+ (id)seperatorItem;

- (id)initWithName:(NSString *)name
           withMethod:(SEL)method
    withKeyEquivalent:(NSString *)keyEquivalent;

- (SEL)getMethod;

@end

@interface MenuBuilder : NSObject

@property(strong) NSMenu *menubar;
@property(strong) NSMutableDictionary<NSString *, MenuItem *> *items;
@property(strong) id target;

- (id)initWithTarget:(id)target;
- (void)setMainMenu;
- (void)addMenuDropdown:(NSString *)name
              withItems:(NSArray<MenuItem *> *)items;

- (void)changeItemTitle:(MenuItem *)item withNewTitle:(NSString *)newTitle;

- (void)changeItemTitleWithTitle:(NSString *)title
                    withNewTitle:(NSString *)newTitle;

- (void)setItemEnabled:(MenuItem *)item to:(bool)enabled;
- (void)setItemEnabledWithTitle:(NSString *)title to:(bool)enabled;
@end

@interface MenuProcedureToggle : NSButton
@property(strong) NSString *parameterName;

+ (MenuItem *)toggleItemWithName:(NSString *)name
              withAction:(SEL)action
              withTarget:(id)target
               withState:(bool)state; 
@end
