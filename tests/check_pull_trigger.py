"""Production wiring + executable shared-state tests; not a UIKit/device test."""
from pathlib import Path
r = Path(__file__).resolve().parents[1]
t = (r/'Tweak.xm').read_text()
p = (r/'prefs/IDPrefs.m').read_text()
s = (r/'IDShared.h').read_text()
hook = t.split('%group IDPullInput')[1].split('%group IDPullTransitionEnd')[0]
assert 'IDStableUserPull' not in hook and 'IDApplyPullCollapse' not in hook
apply = t.split('static void IDApplyPullCollapse(void) {')[1].split('static void IDPullEnded')[0]
for call in ('[state cancelClear]', '[state.collapseAnimator stopAnimation:YES]', 'state.collapseState = IDStablePresentationState', 'updateCollapsePresentation', '[state updatePanelGeometry]', '[state.owner.view setNeedsLayout]'):
    assert call in apply, call
for forbidden in ('[state render]', 'snapshotGroups', 'dropContainedContent', 'IDEnumeration'):
    assert forbidden not in apply, forbidden
install = t.split('static void IDInstallPullHooks(void) {')[1].split('// Do not hook a superclass')[0]
for name in ('_handlePresentGesture:', '_endTransitionToAppeared:', 'noteScreenDidTurnOff'):
    assert name in install
assert 'IDOwnObjectEvent' not in install and 'isPresented' not in install
assert 'if (IDHooksInstalled) {\n        IDInstallPullHooks();' in t
assert t.index('%init(IDMain);') < t.rindex('IDInstallPullHooks();')
for reason in ('class-missing', 'selector-missing', 'return-or-arity-mismatch', 'argument-ABI-mismatch'):
    assert reason in t
for key in ('collapseInputHook','collapseEndHook','collapseScreenHook','effectiveCollapse','currentPrefCollapse','effectiveContained','collapseCycle','collapseCycleLive','collapseCycleCompact','collapseLastReason','collapseLastEnd'):
    assert f'@"{key}"' in t, key
assert 'return CFPreferencesSynchronize(IDPrefsDomain, kCFPreferencesCurrentUser, kCFPreferencesAnyHost)' in s
assert 'if (!IDWritePreference(key, value))' in p
assert '设置未能保存' in p and 'IDStatusPath' not in p
assert 'IDCollapseOnPresentation = IDPref(@"CollapseOnPresentation", NO)' in t
assert '[collapse setProperty:@(IDPref(@"ContainedMode", YES)) forKey:@"enabled"]' in p
begin = t.split('- (void)beginAppearance {')[1].split('- (void)endAppearance {')[0]
assert 'IDStableUserPull' not in begin
model = t.split('static void IDModelDidChange(void) {')[1].split('static void IDRedactContainedNow')[0]
assert 'IDStableUserPull' not in model and 'IDApplyPullCollapse' not in model
assert t.count('IDStableUserPull(&IDPresentationCycle') == 0
print('PASS 0.3.15 direct recognizer began, independent ABI probes/retries, immediate constraints, settings sync/read/diagnostics and no owner/wake/model trigger')
