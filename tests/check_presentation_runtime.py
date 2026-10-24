"""0.3.23 production wiring. Runtime assertions execute separately on macOS."""
from pathlib import Path
r = Path(__file__).resolve().parents[1]
t = (r/'Tweak.xm').read_text()
a = (r/'IDPresentationRuntime.inc').read_text()
e = t.split('static void IDExplicitPresentation(')[1].split('%group IDPullExplicitManager')[0]
assert 'BOOL accepted = NO;' in e
assert 'slider && slider == primary' in e
assert '"coverSheetSlidingViewControllerIfLoaded"' in e
assert 'IDPullScreenHookInstalled' in e
assert 'if (accepted)' in e
assert 'IDApplyPullCollapse();' in e
assert 'isPresented' not in e and 'IDContainedAuthenticated' not in e
for forbidden in ('render]', 'snapshotGroups', 'IDEnumeration'):
    assert forbidden not in e
for group in ('IDPullExplicitManager', 'IDPullExplicitSlider'):
    b = t.split('%group '+group)[1].split('%end\n%end')[0]
    assert b.count('%orig;') == 1
    assert b.index('IDExplicitPresentation(') < b.index('%orig;')
assert 'IDStableUserPull(&IDPresentationCycle' not in t
for key in ('beforeDefinition', 'installedExpected', 'targetExpectedNow', 'runtimeClass', 'runtimeDefinition', 'runtimeExpectedNow', 'ownerAncestors'):
    assert '@"'+key+'"' in a
assert 'sharedInstanceIfExists' in a and 'sel_registerName("sharedInstance")' not in a
assert 'IDPreparePresentationAudit(6' in t
assert 'IDFinishPresentationAudit();' in t
assert 'IDAuditPresentationRuntime(self.owner);' in t
assert '%hook UIGestureRecognizer' not in t and '%hook UIView\n' not in t
assert '%p' not in a and 'NSLog' not in a
assert 'ID_VERSION @"0.3.23"' in (r/'IDShared.h').read_text()
p = (r/'prefs/IDPrefs.m').read_text()
for key in ('runtimeManager', 'runtimeSlider', 'ownerAncestors', 'hookAudit', 'transitionSample', 'installedExpected', 'targetExpectedNow', 'runtimeExpectedNow'):
    assert '@"'+key+'"' in a or '@"'+key+'"' in t or '@"'+key+'"' in (r/'IDCollapseDiagnostics.inc').read_text()
assert 'runtimeSummary' not in p and 'IDAuditFlag' not in p
assert '[rows addObject:@{}]' in a
print('PASS explicit-user presentation, primary identity, cross-entry dedup, exact hook/runtime audit contracts')
