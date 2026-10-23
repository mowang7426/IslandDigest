#import <Foundation/Foundation.h>
#import <objc/runtime.h>
#import <objc/message.h>
#include <assert.h>
#include <string.h>
#include <time.h>
static double clockValue = 1;
static BOOL advancing;
static double IDMonotonic(void) { if (advancing) clockValue += 0.001; return clockValue; }
#include "../IDEnumerationAdapter.inc"
@interface IDTestRequest : NSObject
@property(nonatomic, copy) NSString *sectionIdentifier;
@end
@implementation IDTestRequest
@end
@interface IDTestGroup : NSObject
@property(nonatomic, strong) id notificationRequests;
@end
@implementation IDTestGroup
@end
@interface IDTestSection : NSObject
@property(nonatomic, strong) id allNotificationRequests;
@property(nonatomic, strong) id notificationGroups;
@property(nonatomic, strong) id filteredNotificationRequests;
@end
@implementation IDTestSection
@end
@interface IDTestThrowingSection : IDTestSection
@end
@implementation IDTestThrowingSection
- (id)allNotificationRequests { @throw [NSException exceptionWithName:@"SyntheticGetterFailure" reason:nil userInfo:nil]; }
@end
@interface IDTestMaster : NSObject
@property(nonatomic) unsigned long long notificationCount;
@property(nonatomic, strong) id sectionLists;
@property(nonatomic, strong) id incomingSectionList;
@property(nonatomic, strong) id historySectionList;
@property(nonatomic, strong) id allNotificationRequests;
@end
@implementation IDTestMaster
@end
static id annotated(id self, SEL selector) { (void)self; (void)selector; return @[]; }
static NSUInteger wrong(id self, SEL selector) { (void)self; (void)selector; abort(); }
static NSSet *snapshot(id value, BOOL *complete, NSMutableDictionary *diagnostic) {
    return IDEnumerationSnapshot(value, diagnostic, complete);
}
int main(void) { @autoreleasepool {
    assert(IDEnumerationObjectType("@")); assert(IDEnumerationObjectType("r@\"NSArray\""));
    assert(!IDEnumerationObjectType("@?")); assert(!IDEnumerationObjectType("Q"));
    Class dynamic = objc_allocateClassPair(NSObject.class, "IDTestAnnotated", 0);
    assert(class_addMethod(dynamic, sel_registerName("annotated"), (IMP)annotated, "@\"NSArray\"@:"));
    assert(class_addMethod(dynamic, sel_registerName("scalar"), (IMP)wrong, "Q@:"));
    assert(class_addMethod(dynamic, sel_registerName("argument:"), (IMP)annotated, "@@:@"));
    objc_registerClassPair(dynamic);
    id object = [dynamic new];
    assert([IDEnumerationRead(object, "annotated") isKindOfClass:NSArray.class]);
    assert(!IDEnumerationRead(object, "scalar")); assert(!IDEnumerationRead(object, "argument:"));
    NSMutableDictionary *diagnostic = [NSMutableDictionary new]; BOOL complete = NO;
    IDTestMaster *master = [IDTestMaster new];
    IDTestRequest *request = [IDTestRequest new]; request.sectionIdentifier = @"synthetic-only";
    IDTestGroup *group = [IDTestGroup new];
    group.notificationRequests = [NSOrderedSet orderedSetWithArray:@[request]];
    IDTestSection *section = [IDTestSection new];
    section.allNotificationRequests = @[]; section.notificationGroups = @[group];
    // Reproduces empty aggregate plus nested wrappers, dictionaries and deduplication.
    master.allNotificationRequests = @[];
    master.sectionLists = @{ @"synthetic": @[section] };
    master.incomingSectionList = section; master.historySectionList = section;
    NSSet *result = snapshot(master, &complete, diagnostic);
    assert(complete && result.count == 1 && [result containsObject:request]);
    assert([diagnostic[@"enumerationState"] isEqual:@"read"]);
    section.filteredNotificationRequests = @{ @"synthetic": [NSSet setWithObject:request] };
    assert(snapshot(master, &complete, diagnostic).count == 1 && complete);
    master.sectionLists = @[[NSObject new]]; master.incomingSectionList = nil; master.historySectionList = nil;
    assert(snapshot(master, &complete, diagnostic).count == 0 && !complete);
    assert([diagnostic[@"enumerationState"] isEqual:@"unread-shape"]);
    master.sectionLists = @[];
    assert(snapshot(master, &complete, diagnostic).count == 0 && complete);
    assert([diagnostic[@"enumerationState"] isEqual:@"supported-empty"]);
    master.notificationCount = 3;
    snapshot(master, &complete, diagnostic);
    assert(!complete && [diagnostic[@"enumerationState"] isEqual:@"native-count-mismatch"]);
    assert([diagnostic[@"nativeCountKnown"] boolValue] && [diagnostic[@"nativeModelCount"] unsignedIntegerValue] == 3);
    master.notificationCount = 0;
    IDTestThrowingSection *throwing = [IDTestThrowingSection new];
    throwing.filteredNotificationRequests = @[];
    master.sectionLists = @[throwing];
    snapshot(master, &complete, diagnostic);
    assert(!complete && ![diagnostic[@"enumerationSupported"] boolValue]);
    id deep = @[request];
    for (NSUInteger i = 0; i < 12; i++) deep = @[deep];
    master.sectionLists = deep;
    snapshot(master, &complete, diagnostic);
    assert(!complete && [diagnostic[@"enumerationState"] isEqual:@"bounded-partial"]);
    snapshot([NSObject new], &complete, diagnostic); assert(!complete && ![diagnostic[@"enumerationSupported"] boolValue]);
    snapshot(nil, &complete, diagnostic); assert(!complete && [diagnostic[@"enumerationState"] isEqual:@"missing-master"]);
    // Cycles deduplicate by object identity, not descriptions or notification IDs.
    group.notificationRequests = section; section.notificationGroups = @[group];
    master.sectionLists = @[section];
    snapshot(master, &complete, diagnostic); assert([diagnostic[@"enumerationVisits"] unsignedIntegerValue] < 30);
    NSMutableArray *large = [NSMutableArray new];
    for (NSUInteger i = 0; i < 5000; i++) {
        IDTestRequest *item = [IDTestRequest new]; item.sectionIdentifier = @"synthetic"; [large addObject:item];
    }
    master.sectionLists = large;
    result = snapshot(master, &complete, diagnostic);
    assert(!complete && result.count < 4096 && [diagnostic[@"enumerationVisits"] unsignedIntegerValue] <= 4097);
    assert([diagnostic[@"enumerationState"] isEqual:@"bounded-partial"]);
    advancing = YES;
    snapshot(master, &complete, diagnostic); assert(!complete && [diagnostic[@"enumerationVisits"] unsignedIntegerValue] < 20);
    advancing = NO;
    // Diagnostics contain only fixed keys/class names/counts/getter paths.
    for (id key in diagnostic) assert(![[diagnostic[key] description] containsString:@"synthetic"]);
    puts("PASS production Foundation adapter: ABI, annotated object returns, nested collection/group paths, dedup, unsupported versus empty, cycles, 4096-node/time budgets, metadata-only diagnostics");
    return 0;
} }
