"""Execute the production render body with recording Foundation collaborators.
No UIKit emulation claim: verifies dispatch/deferral and latest-model consumption.
"""
from pathlib import Path
import subprocess, tempfile
r=Path(__file__).resolve().parents[1]
t=(r/'Tweak.xm').read_text()
def body(s, marker):
 p=s.index(marker)+len(marker); e=p; depth=1
 while depth:
  if s[e]=='{': depth+=1
  if s[e]=='}': depth-=1
  e+=1
 return s[p:e-1]
render=body(t,'- (void)render {')
apply=body(t,'static void IDApplyPullCollapse(void) {')
assert 'layoutIfNeeded' not in apply and '[state cancelCollapse]' not in apply
assert 'setNeedsLayout' in apply and 'scheduleNativeListVisibility' in apply
begin=body(t,'- (void)beginAppearance {')
assert 'IDPreferenceValue' not in begin and 'IDDesiredAppCache' in begin
assert 'IDSchedulePresentationAudit(self.owner)' in begin
finish=body(t,'- (void)finishSnapshot {')
assert 'IDWritePreference' not in finish and 'IDExportObservedApps(names, observed)' in finish
assert 'dispatch_async(queue' in body(t,'static void IDLog(const char *event) {')
assert 'dispatch_async(queue' in body(t,'static void IDExportObservedApps(NSDictionary *names, NSArray *observed) {')
ui=(r/'IDDigestUI.inc').read_text()
layout=body(ui,'- (void)layoutSubviews {')
assert layout.index('self.capsuleButton.frame')<layout.index('IDPanelCompact')<layout.index('self.expandedContent.frame')
geo=body(t,'- (void)updatePanelGeometry {')
assert 'if (!compact &&' in geo and 'compact ? MAX(44, self.geometryPreferred)' in geo
c=(r/'IDCollapseCoordinator.inc').read_text()
assert 'self.cardsDeferred && !IDPanelCompact(self.collapseState)' in c
source=r'''
#import <Foundation/Foundation.h>
#include <assert.h>
#include "IDStableRefreshLogic.h"
static BOOL IDContainedMode=YES, IDCollapseOnPresentation=YES, IDHideNative=NO, IDAlwaysHideNative=NO;
static NSUInteger IDOptionsRevision=1;
static IDStablePresentation IDPresentationCycle;
static double IDMonotonic(void) { return 1; }
static void IDRecordPerformance(NSString *s,double d) { (void)s;(void)d; }
static unsigned builds, groupsBuilt, capsules, scrubs, layouts, queued, hides, geometry;
@class Fixture;
@interface IDDigestView : NSObject
@property BOOL retainedLayoutReady;
@property(strong) NSArray *lastCards;
@property(weak) Fixture *coordinator;
- (void)updateCollapsePresentation;
- (void)showGroups:(NSArray *)groups selected:(NSString *)selected;
- (void)showCapsuleGroups:(NSArray *)groups;
- (void)showCards;
- (void)dropCards;
- (void)setNeedsLayout;
- (void)layoutIfNeeded;
- (BOOL)expandedCardsLayoutReady;
@end
@interface Host : NSObject
@property BOOL isViewLoaded;
@property(strong) IDDigestView *view;
@end
@implementation Host
@end
@interface Fixture : NSObject
@property BOOL allowed, overlay, geometryCacheValid, cardsDeferred, renderAfterCollapse;
@property IDPanelState collapseState;
@property NSUInteger renderedRevision;
@property(strong) Host *panel,*owner;
@property(copy) NSArray *groups,*cards,*renderedCards;
@property(copy) NSString *selected,*renderedSelection;
- (BOOL)workAllowed;
- (void)recordCollapseDiagnostic:(NSString *)s reason:(NSString *)r consumed:(BOOL)c;
- (void)updatePanelGeometry;
- (void)updateNativeListVisibility;
- (void)scheduleNativeListVisibility;
- (void)updateClearButton;
- (void)render;
@end
@implementation IDDigestView
- (void)updateCollapsePresentation {}
- (void)showGroups:(NSArray *)g selected:(NSString *)s {(void)g;(void)s;groupsBuilt++;}
- (void)showCapsuleGroups:(NSArray *)g {(void)g;capsules++;}
- (void)showCards { builds++; self.lastCards=self.coordinator.cards; }
- (void)dropCards { scrubs++; self.lastCards=nil; }
- (void)setNeedsLayout {}
- (void)layoutIfNeeded { layouts++; }
- (BOOL)expandedCardsLayoutReady { return YES; }
@end
@implementation Fixture
- (BOOL)workAllowed { return self.allowed; }
- (void)recordCollapseDiagnostic:(NSString *)s reason:(NSString *)r consumed:(BOOL)c {(void)s;(void)r;(void)c;}
- (void)updatePanelGeometry { geometry++; }
- (void)updateNativeListVisibility { hides++; }
- (void)scheduleNativeListVisibility { queued++; }
- (void)updateClearButton {}
'''
source+='- (void)render {'+render+'}\n@end\n'
source+=r'''
int main(void) { @autoreleasepool {
 Fixture *f=[Fixture new]; f.allowed=YES; f.overlay=YES;
 f.panel=[Host new]; f.owner=[Host new]; f.panel.isViewLoaded=YES;
 f.panel.view=[IDDigestView new]; f.panel.view.coordinator=f;
 f.owner.view=[IDDigestView new]; f.groups=@[@1];
 NSMutableArray *cards=[NSMutableArray new]; for(unsigned i=0;i<160;i++) [cards addObject:@(i)];
 f.cards=cards; IDPresentationCycle=(IDStablePresentation){true,true};
 [f render]; assert(f.cardsDeferred && !builds && !groupsBuilt && !layouts && queued==1 && !hides && capsules==1);
 // Warm repeated refresh/new notification consumes no hidden card construction.
 for(unsigned i=0;i<100;i++) { f.cards=@[@(i)]; [f render]; }
 assert(!builds && !layouts && capsules==101 && scrubs==101);
 // Latest page/selection and options are consumed after expansion; no stale snapshot.
 f.selected=@"fixture-app"; f.cards=@[@"latest-page"]; IDOptionsRevision++;
 IDPresentationCycle.compact=false; [f render];
 assert(builds==1 && groupsBuilt==1 && !f.cardsDeferred && f.renderedRevision==2);
 assert([f.panel.view.lastCards isEqual:f.cards] && [f.renderedSelection isEqual:f.selected]);
 // Same-cycle refresh must preserve manual expansion.
 [f render]; assert(builds==2 && f.collapseState==IDPanelExpanded);
 f.collapseState=IDPanelExpanding; [f render]; assert(builds==2 && f.renderAfterCollapse);
 // Inactive, unloaded, cold and supplementary paths.
 f.allowed=NO; [f render]; assert(builds==2);
 f.allowed=YES; f.panel.isViewLoaded=NO; [f render]; assert(builds==2);
 f.panel.isViewLoaded=YES; f.collapseState=IDPanelExpanded; f.overlay=NO;
 IDPresentationCycle.compact=true; [f render]; assert(builds==3 && !f.cardsDeferred);
 // Normal native hiding needs real retained cards; never weaken its proof.
 f.overlay=YES; IDHideNative=YES; [f render]; assert(builds==4 && !f.cardsDeferred);
 IDAlwaysHideNative=YES; [f render]; assert(builds==4 && f.cardsDeferred);
 puts("PASS production render: cold/warm 160-card deferral, repeats, latest page/selection, manual expansion, inactive/unloaded, supplementary");
} }
'''
with tempfile.TemporaryDirectory() as d:
 p=Path(d)/'Transition.m';p.write_text(source)
 subprocess.run(['clang','-fobjc-arc','-Wall','-Wextra','-Werror','-framework','Foundation','-I',str(r),str(p),'-o',d+'/test'],check=True)
 subprocess.run([d+'/test'],check=True)
