/* Executes the production telemetry module on macOS; recognizer is a mock, not UIKit. */
#import <Foundation/Foundation.h>
#import <objc/runtime.h>
#import "../IDStableRefreshLogic.h"
static IDStablePresentation IDPresentationCycle;
enum { UIGestureRecognizerStateChanged = 2 };
@interface UIGestureRecognizer : NSObject
@property(nonatomic) NSInteger state;
@end
@implementation UIGestureRecognizer
@end
#include "../IDCollapseDiagnostics.inc"
static unsigned IDTestFlushes;
static void IDStatus(NSString *stage, NSString *reason) {
    (void)stage; (void)reason; IDTestFlushes++;
}
int main(void) { @autoreleasepool {
    NSDictionary *initial = IDCollapseSnapshot();
    NSCAssert([initial[@"raw"][@"total"] unsignedIntegerValue] == 0, @"explicit zero");
    UIGestureRecognizer *gesture = [UIGestureRecognizer new]; gesture.state = 2;
    for (unsigned i = 0; i < 100000; i++) {
        uint64_t event = IDCollapseEnter(0, gesture); IDCollapseExit(0, gesture, event);
    }
    NSCAssert(IDTestFlushes == 0, @"Changed cannot flush");
    NSCAssert([IDCollapseSnapshot()[@"raw"][@"total"] unsignedIntegerValue] == 100000, @"raw count");
    uint64_t event = IDCollapseEnter(1, gesture); IDCollapseExit(1, gesture, event);
    NSCAssert(!IDPresentationCycle.live, @"helper observation cannot create cycle");
    IDCollapseDecision(YES, @"accepted-test", event, gesture);
    IDPresentationCycle.live = true; IDPresentationCycle.compact = true;
    IDCollapseDecision(NO, @"duplicate-test", event, gesture);
    IDCollapseWillClear(@"transition-ended-hidden");
    IDStableEndPresentation(&IDPresentationCycle);
    NSDictionary *last = IDCollapseSnapshot();
    NSCAssert([last[@"lastAccept"] isEqual:@"accepted-test"] && [last[@"lastReject"] isEqual:@"duplicate-test"], @"clear preserves decisions");
    NSCAssert([last[@"clearLive"] boolValue] && [last[@"clearCompact"] boolValue] && [last[@"clearCycle"] intValue] == 1, @"pre-clear state");
    NSCAssert([last[@"source"] isEqual:last[@"clearSource"]], @"anonymous source persists");
    NSCAssert([last[@"clearEvent"] unsignedLongLongValue] > event, @"ordered clear");
    NSData *data = [NSPropertyListSerialization dataWithPropertyList:last format:NSPropertyListBinaryFormat_v1_0 options:0 error:nil];
    NSCAssert(data != nil, @"safe plist snapshot");
    puts("PASS production diagnostics module: no Changed writes, probes observation-only, accept/reject survive clear, scalar plist");
} return 0; }
