/* Immutable metadata only. Call on a worker; request payloads remain opaque.
 * Rendering uses the same O(1) app lookup + bounded page slice in host tests. */
#include "IDContainedLogic.h"
static NSDictionary *IDBuildNotificationIndex(NSArray *records, NSArray **allRequests) {
    NSMutableArray *all = [NSMutableArray new];
    NSMutableDictionary *byApp = [NSMutableDictionary new];
    for (NSDictionary *record in records) {
        id request = record[@"request"]; NSString *app = record[@"app"];
        [all addObject:request];
        NSMutableArray *items = byApp[app];
        if (!items) { items = [NSMutableArray new]; byApp[app] = items; }
        [items addObject:request];
    }
    NSMutableDictionary *frozen = [NSMutableDictionary new];
    for (NSString *app in byApp) frozen[app] = [byApp[app] copy];
    *allRequests = [all copy]; return [frozen copy];
}
static NSArray *IDNotificationPageRequests(NSArray *all, NSDictionary *byApp,
    NSString *selected, NSUInteger *page, NSUInteger *selectedCount) {
    NSArray *matching = selected ? (byApp[selected] ?: @[]) : (all ?: @[]);
    *selectedCount = matching.count; *page = IDContainedPageClamp(*page, matching.count);
    NSUInteger offset = *page * IDContainedLimit;
    return [matching subarrayWithRange:NSMakeRange(offset, MIN((NSUInteger)IDContainedLimit, matching.count - offset))];
}
