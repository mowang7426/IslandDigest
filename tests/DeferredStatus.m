#import <Foundation/Foundation.h>
#import <dispatch/dispatch.h>
#include <assert.h>
#include <stdlib.h>
static NSString *const IDStatusPath = @"unused-fixture-path";
static void *mainKey = &mainKey;
static NSUInteger value, builds, writes;
static dispatch_semaphore_t started, releaseWriter;
@interface IDRecordingSnapshot : NSObject
@property NSUInteger value;
- (BOOL)writeToFile:(NSString *)path atomically:(BOOL)atomic;
@end
#include "../IDDeferredStatus.inc"
@implementation IDRecordingSnapshot
- (BOOL)writeToFile:(NSString *)path atomically:(BOOL)atomic {
    assert(!dispatch_get_specific(mainKey)); assert(atomic); assert([path isEqual:IDStatusPath]);
    writes++;
    if (writes == 1) {
        assert(self.value == 1);
        dispatch_semaphore_signal(started);
        assert(dispatch_semaphore_wait(releaseWriter, dispatch_time(DISPATCH_TIME_NOW, 5*NSEC_PER_SEC)) == 0);
        assert(self.value == 1); // immutable snapshot did not observe later mutations
    } else {
        assert(writes == 2 && self.value == 1000);
        dispatch_async(dispatch_get_main_queue(), ^{
            assert(builds == 2);
            // Completion follows this callback on main, so check on one more turn.
            dispatch_async(dispatch_get_main_queue(), ^{
                assert(!IDStatusWritePending && !IDStatusWriteDirty);
                puts("PASS production deferred writer: coalescing, blocked-I/O backpressure, latest snapshot, queue ownership");
                exit(0);
            });
        });
    }
    return YES;
}
@end
static NSDictionary *IDBuildStatusSnapshot(void) {
    assert(dispatch_get_specific(mainKey)); builds++;
    IDRecordingSnapshot *snapshot = [IDRecordingSnapshot new]; snapshot.value = value;
    return (NSDictionary *)snapshot;
}
int main(void) {
    started = dispatch_semaphore_create(0); releaseWriter = dispatch_semaphore_create(0);
    dispatch_queue_set_specific(dispatch_get_main_queue(), mainKey, mainKey, NULL);
    value = 1;
    for (unsigned i=0; i<1000; i++) IDQueueStatusWrite();
    assert(builds == 0 && writes == 0); // caller never snapshots or writes
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_DEFAULT, 0), ^{
        assert(dispatch_semaphore_wait(started, dispatch_time(DISPATCH_TIME_NOW, 5*NSEC_PER_SEC)) == 0);
        dispatch_async(dispatch_get_main_queue(), ^{
            for (value=2; value<1000; value++) IDQueueStatusWrite();
            IDQueueStatusWrite(); assert(builds == 1 && IDStatusWriteDirty);
            dispatch_semaphore_signal(releaseWriter);
        });
    });
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 10*NSEC_PER_SEC), dispatch_get_global_queue(QOS_CLASS_DEFAULT,0), ^{ abort(); });
    dispatch_main();
}
