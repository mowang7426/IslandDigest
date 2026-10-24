"""Compile actual production method bodies against recording collaborators.
Not a UIKit emulator: verifies sequencing/side effects, not real gesture latency.
"""
from pathlib import Path
import subprocess, tempfile
r=Path(__file__).resolve().parents[1]
t=(r/'Tweak.xm').read_text(); c=(r/'IDCollapseCoordinator.inc').read_text()
def method(text, start, end): return text.split(start,1)[1].split(end,1)[0]
exit_body=method(t,'- (void)endAppearance {','\n- (void)togglePinnedIdentifier:')
cancel_body=method(c,'- (void)cancelCollapse {','\n- (void)finishCollapse')
for forbidden in ('IDPreferenceValue(', 'IDWritePreference(', 'layoutIfNeeded', 'IDPref('):
    assert forbidden not in exit_body, forbidden
source=r'''
#import <Foundation/Foundation.h>
#include <assert.h>
#include <stdbool.h>
#include <math.h>
#include "IDCollapseLogic.h"
static BOOL IDRememberFilter;
static unsigned scrubs, clears, restores, presentations, geometry, statuses, stops;
static double IDMonotonic(void) { return 0; }
static void IDRecordPerformance(NSString *s, double d) { (void)s; (void)d; }
static void IDStatus(NSString *s, NSString *r) { (void)s; (void)r; statuses++; }
@interface IDDigestView : NSObject
- (void)updateCollapsePresentation;
@end
@implementation IDDigestView
- (void)updateCollapsePresentation { presentations++; }
@end
@interface IDPanelFixture : NSObject
@property BOOL isViewLoaded;
@property(strong) IDDigestView *view;
@end
@implementation IDPanelFixture
@end
@interface IDAnimatorFixture : NSObject
- (void)stopAnimation:(BOOL)stop;
@end
@implementation IDAnimatorFixture
- (void)stopAnimation:(BOOL)stop { assert(stop); stops++; }
@end
@interface IDExitFixture : NSObject
@property BOOL active, dismissed, presentationPending, nativeVisibilityPending, geometryCacheValid, openingPending, renderAfterCollapse, collapseGeometryAllowed;
@property NSUInteger presentationGeneration;
@property(strong) id observedNativeList;
@property(strong) NSString *selected, *desiredSelection;
@property(strong) IDPanelFixture *panel;
@property(strong) IDAnimatorFixture *collapseAnimator;
@property IDPanelState collapseState;
- (void)endAppearance;
- (void)cancelCollapse;
- (void)dropContainedContent;
- (void)cancelClear;
- (void)resetFilter;
- (void)updatePanelGeometry;
- (void)recordCollapseDiagnostic:(NSString *)s reason:(NSString *)r consumed:(BOOL)b;
@end
@implementation IDExitFixture
- (void)recordCollapseDiagnostic:(NSString *)s reason:(NSString *)r consumed:(BOOL)b { (void)s; (void)r; (void)b; }
- (void)dropContainedContent { assert(!self.active && self.dismissed); scrubs++; }
- (void)cancelClear { assert(!self.active); clears++; }
- (void)resetFilter { assert(!self.active && self.selected == nil); restores++; }
- (void)updatePanelGeometry { geometry++; }
'''
source+='- (void)endAppearance {'+exit_body+'\n- (void)cancelCollapse {'+cancel_body+'\n@end\n'
source+=r'''
int main(void) { @autoreleasepool {
    for (unsigned remember=0; remember<2; remember++) {
        IDRememberFilter=remember;
        IDExitFixture *s=[IDExitFixture new]; s.active=YES;
        s.panel=[IDPanelFixture new]; s.panel.isViewLoaded=YES; s.panel.view=[IDDigestView new];
        s.collapseAnimator=[IDAnimatorFixture new]; s.collapseState=IDPanelCollapsed;
        s.selected=@"app"; s.desiredSelection=@"app";
        s.presentationPending=YES; s.nativeVisibilityPending=YES; s.geometryCacheValid=YES;
        [s endAppearance];
        assert(s.dismissed && !s.active && s.presentationGeneration==1);
        assert(!s.presentationPending && !s.nativeVisibilityPending && !s.geometryCacheValid);
        assert(!s.collapseAnimator && s.collapseState==IDPanelExpanded);
        assert((s.desiredSelection != nil)==remember);
        assert(scrubs==remember+1 && clears==remember+1 && restores==remember+1 && statuses==remember+1);
        assert(presentations==0 && geometry==0); // no inactive UI expansion, including loaded panel
    }
    IDExitFixture *s=[IDExitFixture new]; s.active=YES; s.panel=[IDPanelFixture new];
    s.panel.isViewLoaded=YES; s.panel.view=[IDDigestView new]; [s cancelCollapse];
    assert(presentations==1 && geometry==1 && stops==2); // visible behavior unchanged
    puts("PASS extracted production exit/cancel: synchronous safety, no inactive presentation/layout, remember-filter states");
} }
'''
with tempfile.TemporaryDirectory() as d:
    p=Path(d)/'Exit.m'; p.write_text(source)
    subprocess.run(['clang','-fobjc-arc','-Wall','-Wextra','-Werror','-framework','Foundation','-I',str(r),str(p),'-o',d+'/test'],check=True)
    subprocess.run([d+'/test'],check=True)
