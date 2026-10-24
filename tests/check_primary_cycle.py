from pathlib import Path
r = Path(__file__).resolve().parents[1]
t = (r/'Tweak.xm').read_text()
l = (r/'IDPrimaryLifecycle.inc').read_text()
c = (r/'IDPrimaryCycleLogic.h').read_text()
assert '%hook SBCoverSheetPrimarySlidingViewController' in t
for selector in ('_beginTransitionFromAppeared:', '_endTransitionToAppeared:'):
    assert selector in t
assert 'IDBoolArgument(primary, "_beginTransitionFromAppeared:", 2)' in t
assert 'IDBoolArgument(primary, "_endTransitionToAppeared:", 2)' in t
assert '%init(IDPrimaryLifecycle)' in t
assert 'fromAppeared, primary, known && !locked, enabled' in l
assert 'slider == IDRead(manager, "coverSheetSlidingViewControllerIfLoaded")' in l
assert 'if (!appeared) IDPullEnded(@"primary-transition-ended-hidden")' in t
assert 'BOOL accepted = NO; // Intent metadata is diagnostic only' in t
assert 'IDStableUserPull(&IDPresentationCycle' not in t
assert 'IDApplyPullCollapse();' in l
assert 'bool fresh = !fromAppeared && !boundary->occupied' in c
assert 'boundary->occupied = true' in c
assert 'if (!fresh || !unlocked || !enabled) return false' in c
assert 'if (!appeared) IDStableEndPresentation(cycle)' in c
assert 'IDModelDidChange' in t and 'IDPrimaryTransitionBegin' not in t.split('static void IDModelDidChange')[1].split('static void IDRedactContainedNow')[0]
assert 'PRIMARY-CYCLE-0.3.18.md' in [p.name for p in r.iterdir()]
print('PASS primary hidden-to-appearing boundary, actual identity/unlocked gates, locked occupancy, primary end reset, observer-only legacy signals')
