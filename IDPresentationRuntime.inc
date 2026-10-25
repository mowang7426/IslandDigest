// Main-thread, fixed-size, low-frequency runtime audit. No pointers leave the process.
typedef struct {
    Class target, beforeDefinition;
    SEL selector;
    IMP expected;
    BOOL captured, checked, matchedAtInstall;
} IDPresentationHookRecord;
static IDPresentationHookRecord IDPresentationHooks[7];
static void IDPreparePresentationAudit(unsigned index, Class target, const char *selector, IMP expected) {
    IDPresentationHookRecord *record = &IDPresentationHooks[index];
    if (record->captured || !target) return;
    record->target = target; record->selector = sel_registerName(selector); record->expected = expected;
    record->beforeDefinition = IDHookDefiningClass(target, record->selector);
    record->captured = YES;
}
static void IDFinishPresentationAudit(void) {
    for (unsigned i = 0; i < 7; i++) {
        IDPresentationHookRecord *r = &IDPresentationHooks[i];
        if (!r->captured || r->checked) continue;
        r->matchedAtInstall = IDHookMatches(r->target, r->selector, r->expected);
        r->checked = YES;
    }
}
static id IDPresentationManagerIfLoaded(void) {
    Class cls = objc_getClass("SBCoverSheetPresentationManager");
    // No sharedInstance construction: observing must not instantiate system UI.
    if (!IDMethod(object_getClass(cls), "sharedInstanceIfExists", "@", 2)) return nil;
    return ((id (*)(id, SEL))objc_msgSend)(cls, sel_registerName("sharedInstanceIfExists"));
}
static void IDSchedulePresentationAudit(id owner) {
    // Diagnostics only: do not traverse class hierarchies ahead of Apple's callback.
    static BOOL pending;
    static __weak id pendingOwner;
    if (!NSThread.isMainThread) return;
    if (owner) pendingOwner = owner;
    if (pending) return;
    pending = YES;
    dispatch_async(dispatch_get_main_queue(), ^{
        pending = NO;
        id current = pendingOwner; pendingOwner = nil;
        IDAuditPresentationRuntime(current);
    });
}
static void IDAuditPresentationRuntime(id owner) {
    if (!NSThread.isMainThread) return;
    IDCollapseDiagnosticInit();
    id manager = IDPresentationManagerIfLoaded();
    id slider = IDRead(manager, "coverSheetSlidingViewControllerIfLoaded");
    NSMutableArray *ancestors = [NSMutableArray new];
    id node = owner;
    for (unsigned depth = 0; node && depth < 16; depth++) {
        [ancestors addObject:IDHookClassName(object_getClass(node))];
        node = [node isKindOfClass:UIViewController.class] ? [(UIViewController *)node parentViewController] : nil;
    }
    if (owner) IDCollapseTrace[@"ownerAncestors"] = ancestors;
    IDCollapseTrace[@"runtimeManager"] = IDHookClassName(object_getClass(manager));
    IDCollapseTrace[@"runtimeSlider"] = IDHookClassName(object_getClass(slider));
    NSMutableArray *rows = [NSMutableArray new];
    for (unsigned i = 0; i < 7; i++) {
        IDPresentationHookRecord *r = &IDPresentationHooks[i];
        if (!r->captured) {
            [rows addObject:@{}]; // Keep seven stable positions; missing is unknown.
            continue;
        }
        id instance = [manager isKindOfClass:r->target] ? manager : [slider isKindOfClass:r->target] ? slider : nil;
        Class runtime = object_getClass(instance);
        [rows addObject:@{@"selector":NSStringFromSelector(r->selector),
            @"target":IDHookClassName(r->target), @"beforeDefinition":IDHookClassName(r->beforeDefinition),
            @"installedExpected":@(r->matchedAtInstall),
            @"targetExpectedNow":@(IDHookMatches(r->target, r->selector, r->expected)),
            @"runtimeClass":IDHookClassName(runtime),
            @"runtimeDefinition":IDHookClassName(IDHookDefiningClass(runtime, r->selector)),
            @"runtimeExpectedNow":runtime ? (id)@(IDHookMatches(runtime, r->selector, r->expected)) : (id)@"unknown"}];
    }
    IDCollapseTrace[@"hookAudit"] = rows;
}
