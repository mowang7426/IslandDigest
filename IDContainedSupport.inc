/* Contained notifications: no notification text is written to preferences or diagnostics. */
static BOOL IDCollection(id value) {
    return [value isKindOfClass:NSArray.class] || [value isKindOfClass:NSSet.class]
        || [value isKindOfClass:NSOrderedSet.class] || [value isKindOfClass:NSDictionary.class];
}
/*
 * Preview policy is deliberately fail-closed for the system enum: iOS 17's
 * contentPreviewSetting is an opaque unsigned value in the available headers,
 * and no authoritative per-request mapping is exposed here. We therefore do
 * not guess enum values or use a native visibility flag as permission.
 *
 * The explicit, default-off user preference is an opt-in authorization for
 * this plugin only. Because the system enum is unmapped, this opt-in may not
 * preserve an individual app's Never setting; Settings warns about that limit.
 * Content is read only on the main thread while SpringBoard's protected data is available and the owner
 * controller is visible (the authenticated/unlocked state). No body is read
 * while locked or when protected data is unavailable.
 */
static BOOL IDContainedPreviewAllowed(id request, UIViewController *owner) {
    if (!IDPreviewWhenUnlocked || !NSThread.isMainThread) return NO;
    UIApplication *application = UIApplication.sharedApplication;
    if (!application.protectedDataAvailable || application.applicationState == UIApplicationStateBackground) return NO;
    if (!owner || !owner.view.window || owner.view.hidden || owner.view.window.hidden) return NO;
    id content = IDRead(request, "content");
    id options = IDRead(request, "options");
    if (!content || !options) return NO;
    /* contentPreviewSetting remains intentionally uninterpreted. */
    return YES;
}
static void IDContainedClearView(UIView *view) {
    view.accessibilityLabel = nil; view.accessibilityValue = nil; view.accessibilityHint = nil;
    if ([view isKindOfClass:UILabel.class]) ((UILabel *)view).text = nil;
    for (UIView *child in view.subviews) IDContainedClearView(child);
}
static NSString *IDContainedText(id value) {
    if (![value isKindOfClass:NSString.class]) return @"";
    return [value substringToIndex:MIN((NSUInteger)2048, [value length])];
}
static NSArray *IDContainedSnapshot(NSSet *requests, NSString *selected, UIViewController *owner) {
    NSMutableArray *matching = [NSMutableArray new];
    for (id request in requests) {
        NSString *app = IDRead(request, "sectionIdentifier");
        if (![app isKindOfClass:NSString.class] || !app.length || (selected && ![app isEqualToString:selected])) continue;
        [matching addObject:request];
    }
    [matching sortUsingComparator:^NSComparisonResult(id a, id b) {
        if (a == b) return NSOrderedSame;
        id ad = IDRead(a, "timestamp"), bd = IDRead(b, "timestamp");
        NSDate *first = [ad isKindOfClass:NSDate.class] ? ad : NSDate.distantPast;
        NSDate *second = [bd isKindOfClass:NSDate.class] ? bd : NSDate.distantPast;
        NSComparisonResult order = [second compare:first];
        if (order != NSOrderedSame) return order;
        return (uintptr_t)(__bridge void *)a < (uintptr_t)(__bridge void *)b ? NSOrderedAscending : NSOrderedDescending;
    }];
    NSMutableArray *cards = [NSMutableArray new];
    NSMutableSet *identities = [NSMutableSet new];
    for (id request in matching) {
        if (cards.count >= IDContainedLimit) break;
        NSString *app = IDRead(request, "sectionIdentifier");
        id identifier = IDRead(request, "notificationIdentifier");
        id identity = [identifier isKindOfClass:NSString.class] && [identifier length]
            ? @[app, identifier] : [NSValue valueWithPointer:(__bridge const void *)request];
        if ([identities containsObject:identity]) continue;
        [identities addObject:identity];
        id date = IDRead(request, "timestamp");
        BOOL allowed = NSThread.isMainThread && IDContainedPreviewAllowed(request, owner);
        id content = allowed ? IDRead(request, "content") : nil;
        [cards addObject:@{@"app":app, @"date":[date isKindOfClass:NSDate.class] ? date : NSDate.distantPast,
            @"identity":identity,
            @"title":allowed ? IDContainedText(IDRead(content, "title")) : @"",
            @"subtitle":allowed ? IDContainedText(IDRead(content, "subtitle")) : @"",
            @"body":allowed ? IDContainedText(IDRead(content, "message")) : @"通知内容已隐藏",
            @"request":request,
            @"redacted":@(!allowed)}];
    }
    return cards;
}
