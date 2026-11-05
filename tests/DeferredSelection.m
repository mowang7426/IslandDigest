#import <Foundation/Foundation.h>
#import <dispatch/dispatch.h>
#include <assert.h>
#include <stdlib.h>
static void *mainKey = &mainKey;
static NSUInteger writes;
static dispatch_semaphore_t started, releaseWriter;
static NSString *disk;
static BOOL IDWritePreference(NSString *key, id value) {
    assert(!dispatch_get_specific(mainKey));
    assert([key isEqual:@"DesiredApp"]);
    writes++;
    if (writes == 1) {
        assert([value isEqual:@"QQ"]);
        dispatch_semaphore_signal(started);
        assert(dispatch_semaphore_wait(releaseWriter, dispatch_time(DISPATCH_TIME_NOW, 3*NSEC_PER_SEC)) == 0);
        assert([value isEqual:@"QQ"]);
    } else if (writes == 2) assert(value == nil);
    else { assert(writes == 3); assert([value isEqual:@"Minis"]); }
    disk = value;
    return YES;
}
#include "IDDeferredSelection.inc"
static void IDAwaitDrain(NSUInteger phase) {
    if (IDDesiredWritePending) {
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, NSEC_PER_MSEC), dispatch_get_main_queue(), ^{ IDAwaitDrain(phase); });
        return;
    }
    assert(!IDDesiredWriteDirty && !IDDesiredWriteValue);
    if (phase == 1) {
        assert(writes == 2 && disk == nil);
        for (unsigned i = 0; i < 1000; i++) IDQueueDesiredAppWrite(@"QQ");
        IDQueueDesiredAppWrite(@"Minis");
        assert([IDDesiredWriteValue isEqual:@"Minis"]);
        IDAwaitDrain(2);
    } else {
        assert(writes == 3 && [disk isEqual:@"Minis"]);
        puts("PASS production selection writer: immutable input, off-main I/O, 2000+ requests coalesced, in-flight clear wins, latest value wins, bounded pending state");
        exit(0);
    }
}
int main(void) {
    started = dispatch_semaphore_create(0); releaseWriter = dispatch_semaphore_create(0);
    dispatch_queue_set_specific(dispatch_get_main_queue(), mainKey, mainKey, NULL);
    NSMutableString *input = [@"QQ" mutableCopy];
    IDQueueDesiredAppWrite(input); [input setString:@"mutated"];
    assert(writes == 0 && [IDDesiredWriteValue isEqual:@"QQ"]);
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_DEFAULT, 0), ^{
        assert(dispatch_semaphore_wait(started, dispatch_time(DISPATCH_TIME_NOW, 3*NSEC_PER_SEC)) == 0);
        dispatch_async(dispatch_get_main_queue(), ^{
            for (unsigned i = 0; i < 1000; i++) IDQueueDesiredAppWrite(@"Minis");
            IDQueueDesiredAppWrite(nil); // disable RememberFilter while old I/O is blocked
            assert(IDDesiredWritePending && IDDesiredWriteDirty && IDDesiredWriteValue == nil);
            dispatch_semaphore_signal(releaseWriter);
            IDAwaitDrain(1);
        });
    });
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 8*NSEC_PER_SEC), dispatch_get_global_queue(QOS_CLASS_DEFAULT,0), ^{ abort(); });
    dispatch_main();
}
