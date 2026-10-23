/* Main-thread, resumable, read-only native model traversal. No UIKit/model getter
 * is dispatched to a worker. A deadline yields a cursor, never a false final verdict. */
#include "IDEnumerationABI.h"
static id IDEnumerationTryRead(id object, const char *name, BOOL *failed) {
    if (!object) return nil;
    Method method = class_getInstanceMethod(object_getClass(object), sel_registerName(name));
    if (!method || !IDEnumerationGetterABI(method_getTypeEncoding(method))) return nil;
    @try { return ((id (*)(id, SEL))objc_msgSend)(object, sel_registerName(name)); }
    @catch (__unused NSException *exception) { if (failed) *failed = YES; return nil; }
}
static id IDEnumerationRead(id object, const char *name) { return IDEnumerationTryRead(object, name, NULL); }
static BOOL IDEnumerationCollection(id value) {
    return [value isKindOfClass:NSArray.class] || [value isKindOfClass:NSDictionary.class]
        || [value isKindOfClass:NSSet.class] || [value isKindOfClass:NSOrderedSet.class];
}
static BOOL IDEnumerationNativeCount(id object, NSUInteger *count) {
    Method method = class_getInstanceMethod(object_getClass(object), sel_registerName("notificationCount"));
    if (!method || method_getNumberOfArguments(method) != 2) return NO;
    char *type = method_copyReturnType(method);
    BOOL valid = type && strcmp(type, "Q") == 0; free(type);
    if (!valid) return NO;
    @try { *count = (NSUInteger)((unsigned long long (*)(id, SEL))objc_msgSend)(object, sel_registerName("notificationCount")); return YES; }
    @catch (__unused NSException *exception) { return NO; }
}
@interface IDEnumerationFrame : NSObject
@property(nonatomic, strong) id value;
@property(nonatomic, strong) NSEnumerator *iterator;
@property(nonatomic) NSUInteger depth, getter;
@property(nonatomic) BOOL entered, recognized;
@end
@implementation IDEnumerationFrame
@end
@interface IDEnumerationCursor : NSObject
@property(nonatomic, strong) id master;
@property(nonatomic, strong) NSMutableArray<IDEnumerationFrame *> *stack;
@property(nonatomic, strong) NSHashTable *seen;
@property(nonatomic, strong) NSMutableSet *identities;
@property(nonatomic, strong) NSMutableArray *requests;
@property(nonatomic) NSUInteger root, visits, nativeCount, slices;
@property(nonatomic) BOOL countKnown, supported, unread, done, reliable, primaryComplete, cancelled;
@property(nonatomic, copy) NSString *failure;
@end
@implementation IDEnumerationCursor
@end
static IDEnumerationCursor *IDEnumerationBegin(id master) {
    IDEnumerationCursor *cursor = [IDEnumerationCursor new]; cursor.master = master;
    cursor.stack = [NSMutableArray new]; cursor.requests = [NSMutableArray new];
    cursor.identities = [NSMutableSet new];
    cursor.seen = [NSHashTable hashTableWithOptions:NSPointerFunctionsObjectPointerPersonality];
    NSUInteger count = 0; cursor.countKnown = master && IDEnumerationNativeCount(master, &count); cursor.nativeCount = count;
    if (!master) { cursor.done = YES; cursor.failure = @"missing-master"; }
    return cursor;
}
static void IDEnumerationCancel(IDEnumerationCursor *cursor) {
    cursor.cancelled = YES; cursor.done = YES; cursor.reliable = NO; cursor.failure = @"cancelled";
    [cursor.stack removeAllObjects]; [cursor.requests removeAllObjects]; [cursor.identities removeAllObjects];
    [cursor.seen removeAllObjects]; cursor.master = nil;
}
static void IDEnumerationPush(IDEnumerationCursor *cursor, id value, NSUInteger depth) {
    if (!value) return;
    IDEnumerationFrame *frame = [IDEnumerationFrame new]; frame.value = value; frame.depth = depth;
    [cursor.stack addObject:frame];
}
/* Advance at most 64 operations / 2ms. Native getters themselves are not preemptible.
 * Collections are retained, not copied wholesale. Native mutation events cancel the
 * generation before continuation; enumerator exceptions also fail closed. */
static BOOL IDEnumerationAdvance(IDEnumerationCursor *cursor, NSMutableDictionary *diagnostic) {
    static const char * const roots[] = { "allNotificationRequests", "_notificationSectionListsForEnumeration",
        "sectionLists", "notificationSections", "incomingSectionList", "historySectionList", "missedSectionList" };
    static const char * const getters[] = { "allNotificationRequests", "notificationRequests", "filteredNotificationRequests",
        "hiddenNotificationRequests", "allNotificationGroups", "notificationGroups", "hiddenNotificationGroups" };
    double deadline = IDMonotonic() + 0.002; cursor.slices++;
    @try {
    for (NSUInteger step = 0; !cursor.done && step < 64 && (step == 0 || IDMonotonic() < deadline); step++) {
        if (!cursor.stack.count) {
            // A fully walked authoritative aggregate with an independently known
            // exact count proves coverage. Unsupported alternate wrappers cannot
            // invalidate that proof. An unknown count NEVER grants this shortcut.
            if (cursor.root == 1 && cursor.countKnown && cursor.nativeCount > 0
                && cursor.requests.count == cursor.nativeCount && !cursor.unread && cursor.supported) {
                cursor.primaryComplete = YES; cursor.done = YES; break;
            }
            if (cursor.root == sizeof(roots)/sizeof(roots[0])) { cursor.done = YES; break; }
            BOOL failed = NO;
            NSUInteger root = cursor.root++;
            id value = IDEnumerationTryRead(cursor.master, roots[root], &failed);
            if (value) {
                diagnostic[@"enumerationReadPath"] = [NSString stringWithUTF8String:roots[root]];
                diagnostic[@"collectionClass"] = NSStringFromClass(object_getClass(value));
                diagnostic[@"collectionCount"] = IDEnumerationCollection(value) ? @([value count]) : @0;
            }
            if (failed) cursor.unread = YES;
            IDEnumerationPush(cursor, value, 0); continue;
        }
        IDEnumerationFrame *frame = cursor.stack.lastObject;
        if (!frame.entered) {
            frame.entered = YES;
            if (++cursor.visits > 100000 || frame.depth > 32) {
                cursor.failure = @"capacity-fallback"; cursor.done = YES; break;
            }
            if ([cursor.seen containsObject:frame.value]) { [cursor.stack removeLastObject]; continue; }
            [cursor.seen addObject:frame.value];
            if (IDEnumerationCollection(frame.value)) {
                cursor.supported = YES;
                frame.iterator = [frame.value objectEnumerator];
                continue;
            }
            id app = IDEnumerationRead(frame.value, "sectionIdentifier");
            if ([app isKindOfClass:NSString.class] && [app length]) {
                id identifier = IDEnumerationRead(frame.value, "notificationIdentifier");
                id identity = [identifier isKindOfClass:NSString.class] && [identifier length]
                    ? @[app, identifier] : [NSValue valueWithPointer:(__bridge const void *)frame.value];
                if (![cursor.identities containsObject:identity]) {
                    if (cursor.requests.count >= 20000) { cursor.failure = @"capacity-fallback"; cursor.done = YES; break; }
                    [cursor.identities addObject:identity]; [cursor.requests addObject:frame.value];
                }
                [cursor.stack removeLastObject]; continue;
            }
        }
        if (frame.iterator) {
            id child = [frame.iterator nextObject];
            if (child) IDEnumerationPush(cursor, child, frame.depth + 1);
            else [cursor.stack removeLastObject];
        } else if (frame.getter < sizeof(getters)/sizeof(getters[0])) {
            BOOL failed = NO;
            id child = IDEnumerationTryRead(frame.value, getters[frame.getter++], &failed);
            if (failed) cursor.unread = YES;
            if (child) { frame.recognized = YES; IDEnumerationPush(cursor, child, frame.depth + 1); }
        } else {
            if (!frame.recognized) cursor.unread = YES;
            [cursor.stack removeLastObject];
        }
    }
    } @catch (__unused NSException *exception) { cursor.failure = @"collection-mutated"; cursor.done = YES; }
    if (cursor.done && !cursor.cancelled) {
        NSUInteger currentCount = 0;
        BOOL countStillKnown = IDEnumerationNativeCount(cursor.master, &currentCount);
        if (!cursor.failure && cursor.countKnown && (!countStillKnown || currentCount != cursor.nativeCount)) cursor.failure = @"model-changed";
        else if (!cursor.failure && cursor.countKnown && cursor.nativeCount > cursor.requests.count) cursor.failure = @"native-count-mismatch";
        cursor.reliable = cursor.supported && !cursor.unread && !cursor.failure;
    }
    diagnostic[@"nativeCountKnown"] = @(cursor.countKnown);
    diagnostic[@"nativeModelCount"] = @(cursor.nativeCount);
    diagnostic[@"enumerationSupported"] = @(cursor.supported && !cursor.unread);
    diagnostic[@"enumerationComplete"] = @(cursor.done && cursor.reliable);
    diagnostic[@"enumerationVisits"] = @(cursor.visits);
    diagnostic[@"enumerationSlices"] = @(cursor.slices);
    diagnostic[@"enumerationCaptured"] = @(cursor.requests.count);
    diagnostic[@"enumerationState"] = !cursor.done ? @"capturing" : (cursor.failure ?: (!cursor.supported ? @"unsupported"
        : cursor.unread ? @"unread-shape" : cursor.requests.count ? @"read" : @"supported-empty"));
    return cursor.done;
}
