/* Read-only native-model adapter. Shared verbatim by SpringBoard and host tests.
 * No KVC, ivars, descriptions, request text, dynamically discovered selectors or mutation.
 * Calls are restricted to zero-argument object getters after complete return/arity checks.
 */
static BOOL IDEnumerationObjectType(const char *type) {
    if (!type) return NO;
    while (*type && strchr("rnNoORV", *type)) type++;
    return type[0] == '@' && (type[1] == '\0' || type[1] == '"');
}
static id IDEnumerationTryRead(id object, const char *name, BOOL *failed) {
    if (!object) return nil;
    Method method = class_getInstanceMethod(object_getClass(object), sel_registerName(name));
    if (!method || method_getNumberOfArguments(method) != 2) return nil;
    char *type = method_copyReturnType(method);
    BOOL valid = IDEnumerationObjectType(type);
    free(type);
    if (!valid) return nil;
    @try { return ((id (*)(id, SEL))objc_msgSend)(object, sel_registerName(name)); }
    @catch (__unused NSException *exception) { if (failed) *failed = YES; return nil; }
}
static id IDEnumerationRead(id object, const char *name) {
    return IDEnumerationTryRead(object, name, NULL);
}
static BOOL IDEnumerationCollection(id value) {
    return [value isKindOfClass:NSArray.class] || [value isKindOfClass:NSDictionary.class]
        || [value isKindOfClass:NSSet.class] || [value isKindOfClass:NSOrderedSet.class];
}
// NCNotificationListComponent declares unsigned-long-long notificationCount.
// Only the exact 64-bit integer ABI is invoked, not BOOL or an object guess.
static BOOL IDEnumerationNativeCount(id object, NSUInteger *count) {
    Method method = class_getInstanceMethod(object_getClass(object), sel_registerName("notificationCount"));
    if (!method || method_getNumberOfArguments(method) != 2) return NO;
    char *type = method_copyReturnType(method);
    BOOL valid = type && strcmp(type, "Q") == 0;
    free(type);
    if (!valid) return NO;
    @try { *count = (NSUInteger)((unsigned long long (*)(id, SEL))objc_msgSend)(object, sel_registerName("notificationCount")); return YES; }
    @catch (__unused NSException *exception) { return NO; }
}
typedef struct {
    NSUInteger visits;
    BOOL supported, complete, exhausted, unreadShape;
    double deadline;
} IDEnumerationBudget;
static BOOL IDEnumerationVisit(IDEnumerationBudget *budget) {
    if (++budget->visits > 4096 || IDMonotonic() > budget->deadline) {
        budget->complete = NO; budget->exhausted = YES; return NO;
    }
    return YES;
}
static void IDEnumerationWalk(id value, NSMutableSet *requests, NSHashTable *seen,
                              NSUInteger depth, IDEnumerationBudget *budget) {
    if (!value || budget->exhausted) return;
    if (depth > 8) { budget->complete = NO; budget->exhausted = YES; return; }
    if (!IDEnumerationVisit(budget)) return;
    if ([seen containsObject:value]) return;
    [seen addObject:value];
    if (IDEnumerationCollection(value)) {
        budget->supported = YES;
        // Fast enumeration avoids allValues/allObjects copies of unbounded collections.
        for (id item in value) {
            id child = [value isKindOfClass:NSDictionary.class] ? [value objectForKey:item] : item;
            IDEnumerationWalk(child, requests, seen, depth + 1, budget);
            if (budget->exhausted) break;
        }
        return;
    }
    // Request identity only. Privacy policy is applied later, never used to decide enumeration.
    id section = IDEnumerationRead(value, "sectionIdentifier");
    if ([section isKindOfClass:NSString.class]) {
        if (requests.count >= 4096) { budget->complete = NO; budget->exhausted = YES; }
        else [requests addObject:value];
        return;
    }
    // Fixed structural getter allowlist; no arbitrary runtime selector invocation.
    // iOS releases expose section/group aggregates differently. Empty aggregates do
    // not short-circuit sibling structural paths (incoming/history and group wrappers).
    static const char * const getters[] = {
        "allNotificationRequests", "notificationRequests", "filteredNotificationRequests",
        "hiddenNotificationRequests", "allNotificationGroups", "notificationGroups",
        "hiddenNotificationGroups"
    };
    BOOL recognized = NO;
    for (NSUInteger i = 0; i < sizeof(getters)/sizeof(getters[0]); i++) {
        if (budget->exhausted) break;
        BOOL failed = NO;
        id child = IDEnumerationTryRead(value, getters[i], &failed);
        if (failed) { budget->complete = NO; budget->unreadShape = YES; }
        if (!child) continue;
        recognized = YES;
        IDEnumerationWalk(child, requests, seen, depth + 1, budget);
    }
    if (!recognized) { budget->complete = NO; budget->unreadShape = YES; }
}
static NSSet *IDEnumerationSnapshot(id master, NSMutableDictionary *diagnostic, BOOL *reliable) {
    NSMutableSet *requests = [NSMutableSet new];
    NSHashTable *seen = [NSHashTable hashTableWithOptions:NSPointerFunctionsObjectPointerPersonality];
    IDEnumerationBudget budget = {0, NO, master != nil, NO, NO, IDMonotonic() + 0.006};
    NSUInteger sectionCount = 0;
    NSMutableArray *paths = [NSMutableArray new];
    NSString *collectionClass = @"none";
    NSUInteger collectionCount = 0;
    static const char * const getters[] = {
        "allNotificationRequests", "_notificationSectionListsForEnumeration", "sectionLists",
        "notificationSections", "incomingSectionList", "historySectionList", "missedSectionList"
    };
    for (NSUInteger i = 0; i < sizeof(getters)/sizeof(getters[0]); i++) {
        if (budget.exhausted) break;
        BOOL failed = NO;
        id value = IDEnumerationTryRead(master, getters[i], &failed);
        if (failed) { budget.complete = NO; budget.unreadShape = YES; }
        if (!value) continue;
        [paths addObject:[NSString stringWithUTF8String:getters[i]]];
        if (IDEnumerationCollection(value)) {
            collectionClass = NSStringFromClass(object_getClass(value));
            collectionCount = [value count];
            if (i > 0) sectionCount = MAX(sectionCount, collectionCount);
        } else if (i >= 4) sectionCount++;
        IDEnumerationWalk(value, requests, seen, 0, &budget);
    }
    NSUInteger nativeCount = 0;
    BOOL countKnown = master && IDEnumerationNativeCount(master, &nativeCount);
    BOOL countMismatch = countKnown && nativeCount > requests.count;
    if (countMismatch) budget.complete = NO;
    *reliable = budget.supported && budget.complete && !budget.exhausted;
    diagnostic[@"nativeCountKnown"] = @(countKnown);
    diagnostic[@"nativeModelCount"] = @(nativeCount);
    diagnostic[@"masterSeen"] = @(master != nil);
    diagnostic[@"sections"] = @(sectionCount);
    diagnostic[@"enumerationSupported"] = @(budget.supported && !budget.unreadShape);
    diagnostic[@"enumerationComplete"] = @(*reliable);
    diagnostic[@"enumerationState"] = !master ? @"missing-master" : budget.exhausted ? @"bounded-partial"
        : !budget.supported ? @"unsupported" : countMismatch ? @"native-count-mismatch" : !budget.complete ? @"unread-shape"
        : requests.count ? @"read" : @"supported-empty";
    diagnostic[@"enumerationReadPath"] = [paths componentsJoinedByString:@"+"];
    diagnostic[@"collectionClass"] = collectionClass;
    diagnostic[@"collectionCount"] = @(collectionCount);
    diagnostic[@"enumerationVisits"] = @(MIN(budget.visits, 4097UL));
    return requests;
}
