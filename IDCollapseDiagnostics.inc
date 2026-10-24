#import "IDCollapseDiagnostics.h"
#include <pthread.h>
static IDCollapseDiagnosticState IDCollapseRaw = {0};
static pthread_mutex_t IDCollapseRawLock = PTHREAD_MUTEX_INITIALIZER;
static NSMutableDictionary *IDCollapseTrace;
static char IDCollapseAnonymousKey;
static NSUInteger IDCollapseNextAnonymous;
static void IDStatus(NSString *stage, NSString *reason);
static void IDCollapseDiagnosticInit(void) {
    if (IDCollapseTrace) return;
    IDCollapseTrace = [@{@"accepted":@0, @"rejected":@0, @"cycle":@0,
        @"directApply":@0, @"beginAppearance":@0, @"render":@0, @"consumed":@0,
        @"lastAccept":@"none", @"lastReject":@"none", @"clearReason":@"none"} mutableCopy];
}
static NSNumber *IDCollapseAnonymous(id object) {
    if (!object) return @0;
    NSNumber *number = objc_getAssociatedObject(object, &IDCollapseAnonymousKey);
    if (!number) {
        number = @(++IDCollapseNextAnonymous);
        objc_setAssociatedObject(object, &IDCollapseAnonymousKey, number, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    return number;
}
static void IDCollapseIncrement(NSString *key) {
    IDCollapseDiagnosticInit();
    IDCollapseTrace[key] = @([IDCollapseTrace[key] unsignedLongLongValue] + 1);
}
static uint64_t IDCollapseNextEvent(void) {
    pthread_mutex_lock(&IDCollapseRawLock);
    uint64_t event = ++IDCollapseRaw.event;
    pthread_mutex_unlock(&IDCollapseRawLock);
    return event;
}
static uint64_t IDCollapseEnter(unsigned source, id gesture) {
    BOOL main = NSThread.isMainThread;
    BOOL recognizer = [gesture isKindOfClass:UIGestureRecognizer.class];
    int phase = main && recognizer ? (int)[(UIGestureRecognizer *)gesture state] : -1;
    pthread_mutex_lock(&IDCollapseRawLock);
    uint64_t event = IDCollapseDiagnosticEnter(&IDCollapseRaw, source, phase, recognizer, main,
        gesture ? class_getName(object_getClass(gesture)) : "nil");
    pthread_mutex_unlock(&IDCollapseRawLock);
    return event;
}
static void IDCollapseFlushSoon(void) {
    // Main-thread callers only; one trailing flush, never one task per Changed.
    static BOOL pending;
    if (pending) return;
    pending = YES;
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, NSEC_PER_SEC), dispatch_get_main_queue(), ^{
        pending = NO; IDStatus(@"collapse-diagnostic", @"coalesced-input-or-consumer");
    });
}
static void IDCollapseExit(unsigned source, id gesture, uint64_t event) {
    BOOL main = NSThread.isMainThread;
    int phase = main && [gesture isKindOfClass:UIGestureRecognizer.class] ? (int)[(UIGestureRecognizer *)gesture state] : -1;
    pthread_mutex_lock(&IDCollapseRawLock);
    IDCollapseDiagnosticExit(&IDCollapseRaw, source, event, phase);
    pthread_mutex_unlock(&IDCollapseRawLock);
    if (main && phase != UIGestureRecognizerStateChanged) IDCollapseFlushSoon();
}
static NSDictionary *IDCollapseSnapshot(void) {
    IDCollapseDiagnosticInit();
    pthread_mutex_lock(&IDCollapseRawLock);
    IDCollapseDiagnosticState raw = IDCollapseRaw;
    pthread_mutex_unlock(&IDCollapseRawLock);
    NSMutableDictionary *snapshot = IDCollapseTrace.mutableCopy;
    snapshot[@"event"] = @(raw.event);
    NSArray *sources = @[@"raw", @"helper", @"indirect"];
    for (unsigned i = 0; i < 3; i++) {
        NSMutableArray *pre = [NSMutableArray new], *post = [NSMutableArray new];
        for (unsigned j = 0; j < 7; j++) { [pre addObject:@(raw.states[i][j])]; [post addObject:@(raw.postStates[i][j])]; }
        snapshot[sources[i]] = @{@"total":@(raw.total[i]), @"pre":@(raw.pre[i]), @"post":@(raw.post[i]),
            @"event":@(raw.lastEvent[i]), @"postEvent":@(raw.postEvent[i]), @"states":pre, @"postStates":post,
            @"main":@(raw.mainThread[i]), @"offMain":@(raw.offMain[i]), @"recognizer":@(raw.recognizer[i]),
            @"class":raw.classes[i] ? @(raw.classes[i]) : @"not-seen"};
    }
    return snapshot;
}
static void IDCollapseDecision(BOOL accepted, NSString *reason, uint64_t event, id source) {
    IDCollapseIncrement(accepted ? @"accepted" : @"rejected");
    IDCollapseTrace[accepted ? @"lastAccept" : @"lastReject"] = reason;
    IDCollapseTrace[accepted ? @"acceptEvent" : @"rejectEvent"] = @(event);
    if (accepted) {
        IDCollapseIncrement(@"cycle");
        IDCollapseTrace[@"source"] = IDCollapseAnonymous(source);
    }
}
static void IDCollapseWillClear(NSString *reason) {
    IDCollapseDiagnosticInit();
    IDCollapseTrace[@"clearEvent"] = @(IDCollapseNextEvent());
    IDCollapseTrace[@"clearReason"] = reason;
    IDCollapseTrace[@"clearLive"] = @(IDPresentationCycle.live);
    IDCollapseTrace[@"clearCompact"] = @(IDPresentationCycle.compact);
    IDCollapseTrace[@"clearCycle"] = IDCollapseTrace[@"cycle"];
    IDCollapseTrace[@"clearSource"] = IDCollapseTrace[@"source"] ?: @0;
}
