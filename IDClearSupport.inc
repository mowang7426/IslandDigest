// iOS 17 signatures: MTACS/iOS-17-Runtime-Headers (see CLEAR-API.md).
static const char *IDClearSelector = "notificationStructuredListViewController:requestsClearingNotificationRequests:";
static const char *IDInteractionSelector = "notificationStructuredListViewController:shouldAllowInteractionsForNotificationRequest:";
static BOOL IDClearPairMethod(id target, const char *name, const char *result) {
    return target && IDMethod(object_getClass(target), name, result, 4)
        && IDArgument(object_getClass(target), name, 2, "@") && IDArgument(object_getClass(target), name, 3, "@");
}
static BOOL IDClearBool(id target, const char *name, BOOL *value) {
    if (!target || !IDMethod(object_getClass(target), name, "B", 2)) return NO;
    *value = ((BOOL (*)(id, SEL))objc_msgSend)(target, sel_registerName(name)); return YES;
}
static NSArray *IDClearKey(id request) {
    id section = IDRead(request, "sectionIdentifier"), identifier = IDRead(request, "notificationIdentifier");
    id uuid = IDRead(request, "uuid"), date = IDRead(request, "timestamp");
    if (![section isKindOfClass:NSString.class] || ![section length] || ![identifier isKindOfClass:NSString.class]
        || ![identifier length] || ![uuid isKindOfClass:NSUUID.class] || ![date isKindOfClass:NSDate.class]) return nil;
    return @[[section copy], [identifier copy], [uuid copy], [date copy]];
}
static BOOL IDClearCollection(id value) {
    return [value isKindOfClass:NSArray.class] || [value isKindOfClass:NSSet.class] || [value isKindOfClass:NSOrderedSet.class];
}
