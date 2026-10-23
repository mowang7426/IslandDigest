#import <Foundation/Foundation.h>
#import <objc/runtime.h>
#import <objc/message.h>
#include <assert.h>
#include <string.h>
static double clockValue = 1;
static BOOL advancing;
static double IDMonotonic(void) { if (advancing) clockValue += 0.001; return clockValue; }
#include "../IDEnumerationAdapter.inc"
#include "../IDNotificationIndex.inc"
@interface IDTestRequest : NSObject
@property(nonatomic, copy) NSString *sectionIdentifier;
@property(nonatomic, copy) NSString *notificationIdentifier;
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
@interface IDTestUnknownCountMaster : NSObject
@property(nonatomic, strong) id allNotificationRequests;
@property(nonatomic, strong) id sectionLists;
@end
@implementation IDTestUnknownCountMaster
@end
static id annotated(id self, SEL selector) { (void)self; (void)selector; return @[]; }
static NSUInteger wrong(id self, SEL selector) { (void)self; (void)selector; abort(); }
static IDEnumerationCursor *drain(IDEnumerationCursor *cursor, NSMutableDictionary *diagnostic) {
    NSUInteger attempts = 0;
    do { IDEnumerationAdvance(cursor, diagnostic); assert(++attempts < 100000); } while (!cursor.done);
    return cursor;
}
static NSArray *snapshot(id value, BOOL *complete, NSMutableDictionary *diagnostic) {
    IDEnumerationCursor *cursor = drain(IDEnumerationBegin(value), diagnostic);
    *complete = cursor.reliable; return cursor.requests;
}
static NSMutableArray *requests(NSUInteger count) {
    NSMutableArray *items = [NSMutableArray new];
    for (NSUInteger i = 0; i < count; i++) {
        IDTestRequest *item = [IDTestRequest new];
        item.sectionIdentifier = [NSString stringWithFormat:@"synthetic-app-%lu", (unsigned long)(i % 5)];
        item.notificationIdentifier = [NSString stringWithFormat:@"synthetic-id-%lu", (unsigned long)i];
        [items addObject:item];
    }
    return items;
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
    IDTestRequest *request = requests(1).firstObject;
    IDTestGroup *group = [IDTestGroup new]; group.notificationRequests = [NSOrderedSet orderedSetWithArray:@[request]];
    IDTestSection *section = [IDTestSection new]; section.allNotificationRequests = @[]; section.notificationGroups = @[group];
    master.allNotificationRequests = @[]; master.sectionLists = @{@"synthetic": @[section]};
    master.incomingSectionList = section; master.historySectionList = section;
    NSArray *result = snapshot(master, &complete, diagnostic);
    assert(complete && result.count == 1 && [result containsObject:request]);
    section.filteredNotificationRequests = @{@"synthetic": [NSSet setWithObject:request]};
    assert(snapshot(master, &complete, diagnostic).count == 1 && complete);
    master.sectionLists = @[[NSObject new]]; master.incomingSectionList = nil; master.historySectionList = nil;
    assert(snapshot(master, &complete, diagnostic).count == 0 && !complete);
    assert([diagnostic[@"enumerationState"] isEqual:@"unread-shape"]);
    master.sectionLists = @[];
    assert(snapshot(master, &complete, diagnostic).count == 0 && complete);
    assert([diagnostic[@"enumerationState"] isEqual:@"supported-empty"]);
    master.notificationCount = 3; snapshot(master, &complete, diagnostic);
    assert(!complete && [diagnostic[@"enumerationState"] isEqual:@"native-count-mismatch"]);
    master.notificationCount = 0;
    IDTestThrowingSection *throwing = [IDTestThrowingSection new]; throwing.filteredNotificationRequests = @[];
    master.sectionLists = @[throwing]; snapshot(master, &complete, diagnostic);
    assert(!complete && ![diagnostic[@"enumerationSupported"] boolValue]);
    id deep = @[request]; for (NSUInteger i = 0; i < 35; i++) deep = @[deep];
    master.sectionLists = deep; snapshot(master, &complete, diagnostic);
    assert(!complete && [diagnostic[@"enumerationState"] isEqual:@"capacity-fallback"]);
    snapshot([NSObject new], &complete, diagnostic); assert(!complete);
    snapshot(nil, &complete, diagnostic); assert(!complete && [diagnostic[@"enumerationState"] isEqual:@"missing-master"]);
    group.notificationRequests = section; section.notificationGroups = @[group]; master.sectionLists = @[section];
    snapshot(master, &complete, diagnostic); assert([diagnostic[@"enumerationVisits"] unsignedIntegerValue] < 30);
    // Real adapter, not a parallel toy model: every formerly capped boundary.
    for (NSNumber *size in @[@159, @160, @161, @500, @5000]) {
        NSUInteger count = size.unsignedIntegerValue;
        NSMutableArray *items = requests(count);
        master.notificationCount = count; master.allNotificationRequests = items;
        master.sectionLists = @[[NSObject new]]; // unrelated unsupported alternate path
        result = snapshot(master, &complete, diagnostic);
        assert(complete && result.count == count);
        assert([diagnostic[@"enumerationCaptured"] unsignedIntegerValue] == count);
        assert([diagnostic[@"enumerationSlices"] unsignedIntegerValue] > 1);
        // Feed actual captured objects through the exact production index/page
        // adapter. All pages and every app must preserve identity and coverage.
        NSMutableArray *records = [NSMutableArray new];
        for (IDTestRequest *item in result) [records addObject:@{@"app":item.sectionIdentifier, @"request":item}];
        NSArray *all = nil; NSDictionary *byApp = IDBuildNotificationIndex(records, &all);
        assert(all.count == count && byApp.count == 5);
        for (id scope in @[[NSNull null], @"synthetic-app-0", @"synthetic-app-1", @"synthetic-app-2", @"synthetic-app-3", @"synthetic-app-4"]) {
            NSString *selected = scope == NSNull.null ? nil : scope;
            NSArray *expected = selected ? byApp[selected] : all;
            NSMutableArray *visited = [NSMutableArray new];
            NSUInteger pages = IDContainedPageCount(expected.count), page = 0, selectedCount = 0;
            for (NSUInteger p = 0; p < pages; ++p) {
                NSArray *slice = IDNotificationPageRequests(all, byApp, selected, &page, &selectedCount);
                assert(page == p && selectedCount == expected.count && slice.count <= 160);
                assert(IDContainedSnapshotCoverage(count, count, selectedCount, page, slice.count, YES));
                if (selectedCount > 160) assert(!IDContainedSnapshotCoverage(count,count,selectedCount,page,slice.count,NO));
                [visited addObjectsFromArray:slice]; page = IDContainedPageMove(page,selectedCount,1);
            }
            assert([visited isEqual:expected]);
            while (page) { page = IDContainedPageMove(page,selectedCount,-1); IDNotificationPageRequests(all,byApp,selected,&page,&selectedCount); }
            assert(page == 0);
        }
        NSUInteger page = 99, selectedCount = 9;
        assert(IDNotificationPageRequests(all,byApp,@"absent-app",&page,&selectedCount).count == 0);
        assert(!page && !selectedCount);
    }
    // Unknown native count + a valid-looking aggregate is NOT sufficient to
    // discard an unsupported secondary path and claim complete coverage.
    IDTestUnknownCountMaster *unknown = [IDTestUnknownCountMaster new];
    unknown.allNotificationRequests = requests(500); unknown.sectionLists = @[[NSObject new]];
    snapshot(unknown, &complete, diagnostic); assert(!complete);
    // Empty primary aggregate must not suppress incoming/history union. Stable
    // IDs deduplicate distinct wrapper instances without leaking IDs to diagnostics.
    master.allNotificationRequests = @[]; master.sectionLists = nil; master.notificationCount = 500;
    master.incomingSectionList = requests(500); master.historySectionList = requests(500);
    result = snapshot(master, &complete, diagnostic); assert(complete && result.count == 500);
    master.incomingSectionList = nil; master.historySectionList = nil;
    // Crossing the time budget resumes to completion rather than permanent failure.
    advancing = YES; master.allNotificationRequests = requests(500);
    IDEnumerationCursor *cursor = IDEnumerationBegin(master);
    assert(!IDEnumerationAdvance(cursor, diagnostic) && !cursor.reliable);
    assert(cursor.visits < 20);
    drain(cursor, diagnostic); assert(cursor.reliable && cursor.requests.count == 500 && cursor.slices > 100);
    advancing = NO;
    // Cancellation cannot publish an old revision; a fresh generation includes
    // the incoming request, even when count is changed during pending traversal.
    cursor = IDEnumerationBegin(master); IDEnumerationAdvance(cursor, diagnostic);
    IDEnumerationCancel(cursor); drain(cursor, diagnostic);
    assert(!cursor.reliable && !cursor.requests.count && [diagnostic[@"enumerationState"] isEqual:@"cancelled"]);
    cursor = IDEnumerationBegin(master); IDEnumerationAdvance(cursor, diagnostic);
    master.notificationCount = 501;
    drain(cursor, diagnostic); assert(!cursor.reliable && [cursor.failure isEqual:@"model-changed"]);
    master.allNotificationRequests = requests(501);
    cursor = drain(IDEnumerationBegin(master), diagnostic); assert(cursor.reliable && cursor.requests.count == 501);
    // Real safety capacity remains explicit and fail-closed, not a hidden UI cap.
    master.notificationCount = 20001; master.allNotificationRequests = requests(20001);
    cursor = drain(IDEnumerationBegin(master), diagnostic);
    assert(!cursor.reliable && cursor.requests.count == 20000);
    for (id key in diagnostic) assert(![[diagnostic[key] description] containsString:@"synthetic"]);
    puts("PASS production Foundation adapter: exact160/161/500/5000 multi-app capture, incoming/history stable-ID union, resumable deadline, cancellation/new revision, authoritative coverage, unsupported/count/capacity fail-closed, ABI/privacy");
    return 0;
} }
