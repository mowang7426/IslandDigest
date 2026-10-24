"""Settings-only 0.3.23 contract; not UIKit rendering or device verification."""
from pathlib import Path
import hashlib
import json
import re
r = Path(__file__).resolve().parents[1]
p = (r/'prefs/IDPrefs.m').read_text()
f = json.loads((r/'tests/settings_ui_baseline.json').read_text())
def signature(s):
    result = {}
    for line in s.splitlines():
        m = re.search(r'\[self (toggle|choice|slider):.*?key:@"([^"]+)"(.*)', line)
        if not m: continue
        kind, key, rest = m.groups()
        assert key not in result, key
        # Display labels may change; stored values, order, range and defaults may not.
        result[key] = [kind, rest.split(' titles:')[0] if kind == 'choice' else rest]
    return result
assert signature(p) == f['controls'], 'Preference key/type/values/range/default drift'
for start, digest in f['methods'].items():
    body = p.split(start, 1)[1].split('\n}', 1)[0]
    assert hashlib.sha256(body.encode()).hexdigest() == digest, start
for name, digest in f['unchanged'].items():
    # 0.3.23 intentionally changes these two production paths; executable
    # dismissal fixtures cover them instead of asserting old byte identity.
    if name in ('Tweak.xm', 'IDCollapseCoordinator.inc', 'IDContainedCoordinator.inc', 'IDContainedSupport.inc', 'IDContainedUI.inc'): continue
    # Footer-only 0.3.23 changes are independently pinned in check_card_footer.py.
    if name == 'IDDigestUI.inc': continue
    data = (r/name).read_bytes().replace(b'0.3.23', b'0.3.18') if name == 'IDShared.h' else (r/name).read_bytes()
    assert hashlib.sha256(data).hexdigest() == digest, name
for removed in ('collapseSummary', 'runtimeSummary', '- (NSString *)diagnostics', 'IDStatusPath', '刷新折叠诊断', '刷新运行状态', '运行时钩子审计'):
    assert removed not in p, removed
# Native table controls, semantic system colors and automatic native sizing only.
for forbidden in ('heightForRowAtIndexPath', 'heightForHeaderInSection', 'heightForFooterInSection', 'setRowHeight', 'UIColor', 'UIFont', 'CGRectMake', 'NSLog', 'NSTimer', 'dispatch_sync'):
    assert forbidden not in p, forbidden
for native in ('PSListController', 'PSGroupCell', 'PSSwitchCell', 'PSSliderCell', 'PSLinkListCell', 'footerText'):
    assert native in p, native
sections = re.findall(r'\[self group:@"([^"]+)" footer:@"([^"]*)"', p)
assert [x[0] for x in sections] == ['灵屿通知 0.3.23 · MoWang', '预览与隐私', '展开与原生列表', '原生列表可见性', '应用分组与排序', '分类布局', '背景与文字', '位置与大小', '手动清理', '高级与恢复', '关于与兼容性']
assert all(len(footer) <= 130 for _, footer in sections)
assert '可能无法遵循每个 App 的“永不显示预览”' in p
assert '始终隐藏为高风险' in p and '插件故障可能导致空白' in p
assert '关闭“隐藏原生列表”' in p
assert '始终确认且不可撤销' in p and '性能计时默认关闭' in p
assert p.index('启用灵屿通知') < p.index('预览与隐私') < p.index('解锁时显示预览')
assert 'IDPref(@"ContainedMode", YES) && IDPref(@"HideNativeList", NO)' in p
assert '@selector(resetPanelLayout)' in p and '@selector(retry)' in p
assert 'IDPinnedAppsController.class' in p and '@selector(resetPins)' in p
print('PASS 0.3.23 native Settings groups, concise risks, all 24 controls/defaults, unchanged core/icons, no status reads or fixed sizing; not device layout proof')
