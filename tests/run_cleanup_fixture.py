"""Run actual production scrub/cancel bodies with recording Foundation collaborators.
macOS Foundation required; not a UIKit/device latency benchmark.
"""
from pathlib import Path
import subprocess, tempfile
r = Path(__file__).resolve().parents[1]
t = (r/'Tweak.xm').read_text()
u = (r/'IDContainedUI.inc').read_text()
s = (r/'IDContainedSupport.inc').read_text()
def body(text, start):
    pos = text.index(start) + len(start)
    depth = 1
    end = pos
    while depth:
        if text[end] == '{': depth += 1
        if text[end] == '}': depth -= 1
        end += 1
    return text[pos:end-1]
# Guard every non-render notice writer and the reuse/render entry point.
assert 'self.cardContentDirty = YES;' in body(u, '- (void)showCards {')
for text in (t, (r/'IDContainedCoordinator.inc').read_text()):
    for prefix in text.split('digest.cardNotice.text =')[:-1]:
        assert 'digest.cardContentDirty = YES;' in prefix[-200:]
source = r'''
#import <Foundation/Foundation.h>
#include <assert.h>
static unsigned walks, mutations, cancels;
static double IDPerformanceStart(void) { return 0; }
static void IDRecordPerformance(NSString *s, double d) { (void)s; (void)d; }
@interface UIView : NSObject
@property(copy) NSString *accessibilityLabel, *accessibilityValue, *accessibilityHint;
@property(nonatomic, strong) NSArray *accessibilityCustomActions, *subviews;
@property BOOL hidden;
@end
@implementation UIView
- (NSArray *)subviews { walks++; return _subviews; }
@end
@interface UILabel : UIView
@property(copy) NSString *text;
@end
@implementation UILabel
@end
static const NSUInteger UIControlEventAllEvents = ~0UL;
@interface UIControl : UIView
@property BOOL hasTarget;
- (void)removeTarget:(id)t action:(SEL)a forControlEvents:(NSUInteger)e;
@end
@implementation UIControl
- (void)removeTarget:(id)t action:(SEL)a forControlEvents:(NSUInteger)e {
    (void)t; (void)a; assert(e == UIControlEventAllEvents); self.hasTarget=NO;
}
@end
@interface IDCardView : UIView
@property(copy) void (^openAction)(void);
@end
@implementation IDCardView
@end
@interface Stack : UIView
@property(strong) NSArray *arrangedSubviews;
@end
@implementation Stack
@end
@interface Tap : NSObject
@property(nonatomic) BOOL enabled;
@end
@implementation Tap
- (void)setEnabled:(BOOL)v { mutations++; _enabled=v; }
@end
static char IDStableCardModelKey;
#define OBJC_ASSOCIATION_RETAIN_NONATOMIC 1
static void objc_setAssociatedObject(id o, const void *k, id v, int p) {
    (void)o; (void)k; (void)v; (void)p; mutations++;
}
static NSMutableDictionary *IDDiagnostics;
static void IDEnumerationCancel(id cursor) { if (cursor) cancels++; }
@interface Fixture : NSObject
@property BOOL cardContentDirty, retainedLayoutReady, nativeLayoutSignatureValid;
@property NSUInteger builtCardCount;
@property(strong) Tap *cardTap;
@property(strong) Stack *cardStack;
@property(strong) UIView *cardScroll;
@property(strong) UILabel *cardNotice;
@property NSUInteger snapshotGeneration, selectedRequestCount;
@property BOOL snapshotSorting, enumerationReliable;
@property(strong) id enumerationCursor, buildGroups, buildRecords, snapshotGroups, snapshotRecords, snapshotRequests, snapshotByApp, snapshotMaster;
- (void)dropCards;
- (void)cancelSnapshot;
@end
'''
source += 'static void IDContainedClearView(UIView *view) {' + body(s, 'static void IDContainedClearView(UIView *view) {') + '}\n'
source += '@implementation Fixture\n- (void)dropCards {' + body(u, '- (void)dropCards {') + '}\n'
source += '- (void)cancelSnapshot {' + body(t, '- (void)cancelSnapshot {') + '}\n@end\n'
source += r'''
int main(void) { @autoreleasepool {
    IDDiagnostics=[NSMutableDictionary new];
    Fixture *f=[Fixture new]; f.cardTap=[Tap new]; f.cardStack=[Stack new];
    f.cardNotice=[UILabel new]; f.cardScroll=[UIView new];
    IDCardView *card=[IDCardView new]; UILabel *label=[UILabel new];
    UIControl *button=[UIControl new]; button.hasTarget=YES;
    label.text=@"private body"; label.accessibilityLabel=@"private AX";
    card.accessibilityValue=@"private value"; card.accessibilityHint=@"private hint";
    card.accessibilityCustomActions=@[@"action"]; card.openAction=^{};
    card.subviews=@[label]; f.cardStack.arrangedSubviews=@[card, button];
    f.cardNotice.text=@"notice"; f.cardContentDirty=YES; f.builtCardCount=1;
    [f dropCards];
    assert(!f.cardContentDirty && !label.text && !label.accessibilityLabel);
    assert(!card.accessibilityValue && !card.accessibilityHint && !card.accessibilityCustomActions);
    assert(!card.openAction && !button.hasTarget && !f.cardNotice.text);
    assert(f.cardNotice.hidden && !f.cardNotice.accessibilityLabel);
    assert(f.cardScroll.hidden && !f.builtCardCount);
    unsigned oldWalks=walks, oldMutations=mutations;
    for (unsigned i=0; i<1000; i++) [f dropCards];
    assert(walks==oldWalks && mutations==oldMutations);
    // A new write/retained-shell reuse must re-arm synchronous scrubbing.
    f.cardContentDirty=YES; label.text=@"new body"; card.openAction=^{};
    [f dropCards]; assert(walks>oldWalks && !label.text && !card.openAction);
    f.enumerationCursor=[NSObject new]; f.snapshotRecords=@[@"request"];
    f.snapshotSorting=YES; f.enumerationReliable=YES; f.selectedRequestCount=2;
    [f cancelSnapshot]; assert(cancels==1 && f.snapshotGeneration==1);
    assert(!f.enumerationCursor && !f.snapshotRecords && !f.snapshotSorting && !f.enumerationReliable);
    for (unsigned i=0; i<1000; i++) [f cancelSnapshot];
    assert(cancels==1 && f.snapshotGeneration==1001 && !f.selectedRequestCount);
    f.snapshotMaster=[NSObject new]; [f cancelSnapshot]; assert(!f.snapshotMaster);
    puts("PASS production scrub/cancel: immediate text/AX/action invalidation, no repeat walk, dirty rearm, generation cancellation");
} }
'''
with tempfile.TemporaryDirectory() as d:
    p=Path(d)/'Cleanup.m'; p.write_text(source)
    subprocess.run(['clang','-fobjc-arc','-Wall','-Wextra','-Werror','-framework','Foundation',str(p),'-o',d+'/test'],check=True)
    subprocess.run([d+'/test'],check=True)
