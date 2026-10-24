/* All state is main-queue confined. A busy writer retains one latest dirty
 * request, not an unbounded queue of snapshots. No UIKit objects cross queues. */
static BOOL IDStatusWritePending, IDStatusWriteDirty;
static NSDictionary *IDBuildStatusSnapshot(void);
static void IDQueueStatusWrite(void) {
    IDStatusWriteDirty = YES;
    if (IDStatusWritePending) return;
    IDStatusWritePending = YES;
    dispatch_async(dispatch_get_main_queue(), ^{
        IDStatusWriteDirty = NO;
        NSDictionary *snapshot = IDBuildStatusSnapshot();
        static dispatch_queue_t writer;
        static dispatch_once_t once;
        dispatch_once(&once, ^{
            writer = dispatch_queue_create("com.local.islanddigest.status-writer", DISPATCH_QUEUE_SERIAL);
        });
        dispatch_async(writer, ^{
            @autoreleasepool {
                [snapshot writeToFile:IDStatusPath atomically:YES];
            }
            dispatch_async(dispatch_get_main_queue(), ^{
                IDStatusWritePending = NO;
                if (IDStatusWriteDirty) IDQueueStatusWrite();
            });
        });
    });
}
