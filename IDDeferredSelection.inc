/* Main-queue mailbox: one in-flight immutable value and one latest value.
 * All DesiredApp writes (including clears) pass here, so an older write can
 * never finish after the latest write. No UIKit or disk I/O on the caller. */
static BOOL IDDesiredWritePending, IDDesiredWriteDirty;
static NSString *IDDesiredWriteValue;
static void IDQueueDesiredAppWrite(NSString *value) {
    IDDesiredWriteValue = [value copy];
    IDDesiredWriteDirty = YES;
    if (IDDesiredWritePending) return;
    IDDesiredWritePending = YES;
    dispatch_async(dispatch_get_main_queue(), ^{
        IDDesiredWriteDirty = NO;
        NSString *snapshot = IDDesiredWriteValue;
        static dispatch_queue_t writer;
        static dispatch_once_t once;
        dispatch_once(&once, ^{
            writer = dispatch_queue_create("com.local.islanddigest.selection-writer", DISPATCH_QUEUE_SERIAL);
        });
        dispatch_async(writer, ^{
            @autoreleasepool { IDWritePreference(@"DesiredApp", snapshot); }
            dispatch_async(dispatch_get_main_queue(), ^{
                IDDesiredWritePending = NO;
                if (IDDesiredWriteDirty) IDQueueDesiredAppWrite(IDDesiredWriteValue);
                else IDDesiredWriteValue = nil;
            });
        });
    });
}
