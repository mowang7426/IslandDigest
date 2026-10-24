"""0.3.16 production wiring/privacy contracts; not a device gesture test."""
from pathlib import Path
r = Path(__file__).resolve().parents[1]
t = (r/'Tweak.xm').read_text()
d = (r/'IDCollapseDiagnostics.inc').read_text()
p = (r/'prefs/IDPrefs.m').read_text()
raw = t.split('%group IDPullInput')[1].split('%group IDPullHelperProbe')[0]
assert raw.index('IDCollapseEnter(0, gesture)') < raw.index('if (NSThread.isMainThread')
assert raw.index('%orig;') < raw.index('IDCollapseExit(0, gesture, idEvent)')
for group, index in [('IDPullHelperProbe', 1), ('IDPullIndirectProbe', 2)]:
    body = t.split('%group '+group)[1].split('%group ')[0]
    assert f'IDCollapseEnter({index}, gesture)' in body and f'IDCollapseExit({index}, gesture, idEvent)' in body
    assert body.count('%orig;') == 1
    for forbidden in ('IDStableUserPull', 'IDApplyPullCollapse', 'IDStableEndPresentation', 'IDCollapseDecision'):
        assert forbidden not in body
assert t.count('IDStableUserPull(&IDPresentationCycle') == 1
assert 'IDCollapseRaw = {0}' in d and 'pthread_mutex_lock' in d
assert 'phase != UIGestureRecognizerStateChanged' in d
enter = d.split('static uint64_t IDCollapseEnter')[1].split('static void IDCollapseFlushSoon')[0]
assert 'IDStatus(' not in enter and 'IDPref(' not in enter and 'dispatch_' not in enter
for forbidden in ('description]', 'NSLog', 'notificationRequest', 'bundleIdentifier', 'title', 'message'):
    assert forbidden not in d, forbidden
clear = d.split('static void IDCollapseWillClear')[1]
assert 'lastAccept' not in clear and 'lastReject' not in clear
assert t.index('IDCollapseWillClear(reason);') < t.index('IDStableEndPresentation(&IDPresentationCycle);')
assert 'IDCollapseWillClear(@"preferences-disabled")' in t
assert 'recordCollapseDiagnostic:@"directApply"' in t and 'recordCollapseDiagnostic:@"beginAppearance"' in t and 'recordCollapseDiagnostic:@"render"' in t
assert 'capsule.bounds.size.height' in t and 'capsule.window != nil' in t and 'capsule.alpha' in t
assert 'beforeExit' in t and 'lastConsumed' in t
assert p.index('折叠诊断摘要') < p.index('启用灵屿通知')
assert 'collapseSummary' in p and '@"collapseHelperHook"' in p and '@"collapseIndirectHook"' in p
assert '%hook UIGestureRecognizer' not in t and '%hook UIView\n' not in t
print('PASS 0.3.16 raw entry/pre-post probes, immutable decision history, pre-clear facts, consumption/view facts, privacy and summary wiring')
