#include <AppKit/AppKit.h>
#import <Foundation/Foundation.h>

@interface MenuItem : NSObject

@property bool seperator;
@property(strong) NSString *name;
@property(strong) NSValue *method;
@property(strong) NSString *keyEquivalent;
@property(strong) NSMenuItem *item;
@property(strong) NSView *view;
@property NSInteger index;

+ (id)withName:(NSString *)name
           withMethod:(SEL)method
    withKeyEquivalent:(NSString *)keyEquivalent;

+ (id)withName:(NSString *)name
           withMethod:(SEL)method
    withKeyEquivalent:(NSString *)keyEquivalent
    withView:(NSView *)view;

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

