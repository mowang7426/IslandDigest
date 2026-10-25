"""0.3.22 contracts; executable production cleanup/timing fixtures run on macOS."""
from pathlib import Path
r=Path(__file__).resolve().parents[1]
t=(r/'Tweak.xm').read_text();u=(r/'IDContainedUI.inc').read_text();c=(r/'IDContainedCoordinator.inc').read_text()
s=(r/'IDContainedSupport.inc').read_text();d=(r/'prefs/IDDiagnosticsSnapshot.h').read_text()
stages=('cancelSnapshot','restoreFilter','scrubCards','restoreNative','dropContent','ownerWillDisappearPre','ownerWillDisappearOrig','ownerWillDisappearPost','ownerWillDisappearTotal')
for stage in stages:
 assert '@"'+stage+'"' in t+u+c
 assert '@"perfCount_'+stage+'"' in d and '@"perfMaxMs_'+stage+'"' in d
assert 'IDPerformanceDiagnostics ? IDMonotonic() : 0' in t
assert 'MIN(1000000UL' in t
hook=t.split('- (void)viewWillDisappear:(BOOL)animated {',1)[1].split('- (void)insertNotificationRequest:',1)[0]
assert hook.count('%orig;')==1
assert hook.index('[state endAppearance]')<hook.index('%orig;')<hook.index('@"ownerWillDisappearPost"')
assert '@finally { IDRecordPerformance(@"ownerWillDisappearTotal"' in hook
assert 'removeTarget:nil action:NULL forControlEvents:UIControlEventAllEvents' in s
assert 'view.accessibilityCustomActions = nil' in s
show=u.split('- (void)showCards {',1)[1].split('- (',1)[0]
assert show.index('self.cardContentDirty = YES')<show.index('previousViews')
# Paging controls are rebuilt, not retained-card reused, then rebound after scrub.
for name,selector in [('previousPageButton','previousCardPage'),('nextPageButton','nextCardPage')]:
 assert show.index('self.'+name+' = [UIButton buttonWithType:')<show.index('[self.'+name+' addTarget:state action:@selector('+selector+')')
assert 'if (!weakShell.openAction) return;' in show
assert show.index('if (!weakShell.openAction) return;')<show.index('weakShell.accessibilityValue =')
assert 'IDSchedulePresentationAudit(self.owner)' in t.split('if (self.active)',1)[-1]
for method in ('dropCards','cancelSnapshot'):
 text=u if method=='dropCards' else t
 body=text.split('- (void)'+method+' {',1)[1].split('\n- (',1)[0]
 assert '@finally { IDRecordPerformance(' in body
workflow=(r/'.github/workflows/build.yml').read_text()
for test in ('run_cleanup_fixture.py','run_performance_fixture.py','check_transition_performance.py'):
 assert 'python3 tests/'+test in workflow
print('PASS 0.3.22 bounded default-off segmented diagnostics, synchronous scrub, pager rebuild/rebind and action privacy guard')
