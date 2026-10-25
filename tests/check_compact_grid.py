"""0.3.25 category-only source contract; not UIKit/device visual proof."""
from pathlib import Path
import subprocess
r = Path(__file__).resolve().parents[1]
u = (r/'IDDigestUI.inc').read_text()
t = (r/'Tweak.xm').read_text()
p = (r/'prefs/IDPrefs.m').read_text()
h = (r/'IDExperienceLogic.h').read_text()
base = 'e99086ae60bf657405b6ae97ff849b76e4e84ae5'
# Compare the actual reviewed .24 parent, not an old generic baseline exemption.
for name in subprocess.check_output(['git','ls-tree','-r','--name-only',base], cwd=r, text=True).splitlines():
    if name.endswith(('.h','.inc','.xm','.m','.mk')) and name not in (
        'IDDigestUI.inc','IDExperienceLogic.h','IDShared.h','Tweak.xm','prefs/IDPrefs.m'):
        assert (r/name).read_bytes() == subprocess.check_output(['git','show',base+':'+name],cwd=r), name
old = subprocess.check_output(['git','show',base+':Tweak.xm'],cwd=r,text=True)
for before, after in (
    ('IDGridLayout = NO','IDGridLayout = YES'),
    ('IDGridColumns = 3','IDGridColumns = 0'),
    ('@"LayoutMode", @[@"compact", @"grid"], @"compact"','@"LayoutMode", @[@"compact", @"grid"], @"grid"'),
    ('@"GridColumns", @[@"three", @"two", @"auto"], @"three"','@"GridColumns", @[@"three", @"two", @"auto"], @"auto"'),
    ('IslandDigest-0.3.24.startup-pending','IslandDigest-0.3.25.startup-pending')):
    assert before in old
    old = old.replace(before,after)
assert t == old, 'Non-category notification core drift from .24'
for s in ('IDMiniControls ? 44 : 52', 'IDMiniControls ? 9 : 17',
          'IDGridLayout ? 16 :', 'IDGridLayout ? 11 : 12', 'IDGridLayout ? 10 : 11',
          'visible * row + (visible - 1) * 4', '(index / columns) * (row + 4)',
          'IDExperienceRows(index, columns) * (row + 4) - 4',
          'NSLineBreakByTruncatingTail', 'UIAccessibilityTraitSelected',
          'button.accessibilityCustomActions = @[pinAction]', 'hold.cancelsTouchesInView = YES',
          '[coordinator selectIdentifier:identifier]', '[coordinator togglePinnedIdentifier:identifier]',
          'self.scroll.alwaysBounceVertical = IDGridLayout',
          'self.scroll.contentOffset = CGPointMake'):
    assert s in u, s
assert 'mini ? 6 : 0' in h and 'width >= 650 ? 5 : 4' in h
assert 'preference == 2 ? 2 : preference == 3 ? 3' in h
assert '66 + candidate * rowHeight + (candidate - 1) * 4' in h
for s in (t,p):
    assert '@"GridColumns", @[@"three", @"two", @"auto"], @"auto"' in s or 'IDStringOption(key, @[@"three", @"two", @"auto"], @"auto")' in s
assert 'IDSetPref' not in h
print('PASS 0.3.25 compact category dimensions, stored explicit columns, actions/AX/scroll; .24 non-category production byte identity')
