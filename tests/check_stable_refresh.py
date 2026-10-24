"""Source wiring checks, not a claim of UIKit lifecycle/device coverage."""
from pathlib import Path
r = Path(__file__).resolve().parents[1]
t = (r/'Tweak.xm').read_text()
u = (r/'IDContainedUI.inc').read_text()
s = (r/'IDContainedSupport.inc').read_text()
p = (r/'prefs/IDPrefs.m').read_text()
begin = t.split('- (void)beginAppearance {')[1].split('- (void)endAppearance {')[0]
assert 'IDStableBeginPresentation' not in begin
assert 'IDStableAppearanceState' not in t
for name in ('_handlePresentGesture:', '_endTransitionToAppeared:', 'noteScreenDidTurnOff'):
    assert name in t
assert 'IDInstallPullHooks(); // Independent retry' in t
assert '#include "IDPrimaryLifecycle.inc"' in t
assert 'IDStableBeginPresentation(&IDPresentationCycle' not in t
assert 'IDStableEndPresentation(&IDPresentationCycle)' in t
assert 'if (!IDContainedMode || !self.overlay) self.collapseState = IDPanelExpanded' in t
assert 'IDPresentationCycle.compact = IDPanelCompact' in (r/'IDCollapseCoordinator.inc').read_text()
assert 'IDPref(@"CollapseOnPresentation", NO)' in t
assert p.count('[key isEqualToString:@"CollapseOnPresentation"]') == 2
assert '每次下拉先折叠' in p and 'supplementary' in p
assert 'presentationGestureSupported' in t and 'presentationGestureSupported' not in p
mutation = t.split('static void IDModelDidChange(void) {')[1].split('static void IDRedactContainedNow')[0]
assert 'if (state.active && !mustScrub) [state cancelSnapshot]' in mutation
assert 'IDStableRetainWhilePending(YES, self.inserted, self.renderedCards == nil)' in t
assert 'if (!self.snapshotGroups) return self.groups ?: @[]' in t
assert 'IDStableReuseView(' in u and 'valueWithNonretainedObject:card[@"request"]' in u
assert u.count('objc_setAssociatedObject(view, &IDStableCardModelKey, nil') == 2
assert 'IDStableScrollOffset' in u
assert 'IDContainedAuthenticated(owner)' in s
assert 'IDClearBool(node, "isAuthenticated", &authenticated)' in s
assert s.index('IDContainedAuthenticated(owner)') < s.index('id content = IDRead(request, "content")')
assert 'if (!IDPreviewWhenUnlocked || !IDContainedMode' in t
for test in ('StableRefresh.c', 'check_stable_refresh.py'):
    assert test in (r/'.github/workflows/build.yml').read_text()
print('PASS stable refresh production wiring, explicit user cycle ABI gates, supported mode, privacy scrubbing (not device validation)')
