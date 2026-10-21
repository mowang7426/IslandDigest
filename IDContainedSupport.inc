/* Contained notifications: no notification text is written to preferences or diagnostics. */
static BOOL IDCollection(id value) {
    return [value isKindOfClass:NSArray.class] || [value isKindOfClass:NSSet.class]
        || [value isKindOfClass:NSOrderedSet.class] || [value isKindOfClass:NSDictionary.class];
}
/* The iOS 17 headers expose contentPreviewSetting (unsigned long long), but do
 * not establish its enum mapping/effective per-request policy. Authentication
 * alone is NOT permission. Until a verified effective-policy adapter exists,
 * never fetch title, subtitle, message, nor hiddenPreviewsBodyPlaceholder.
 * A native content-view visibility flag is also not a privacy permission. */
static BOOL IDContainedPreviewAllowed(id request) {
    (void)request;
    return IDContainedPrivacyGate(NSThread.isMainThread, 0, 0, 0, 0, 0, 0);
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
static NSArray *IDContainedSnapshot(NSSet *requests, NSString *selected) {
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
        BOOL allowed = NSThread.isMainThread && IDContainedPreviewAllowed(request);
        id content = allowed ? IDRead(request, "content") : nil;
        [cards addObject:@{@"app":app, @"date":[date isKindOfClass:NSDate.class] ? date : NSDate.distantPast,
            @"identity":identity,
            @"title":allowed ? IDContainedText(IDRead(content, "title")) : @"",
            @"subtitle":allowed ? IDContainedText(IDRead(content, "subtitle")) : @"",
            @"body":allowed ? IDContainedText(IDRead(content, "message")) : @"通知内容已隐藏",
            @"redacted":@(!allowed)}];
    }
    return cards;
}
