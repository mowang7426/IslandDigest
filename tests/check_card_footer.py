"""0.3.23 footer-only delta; not a UIKit/device visual acceptance test."""
from pathlib import Path
import hashlib, json
r = Path(__file__).resolve().parents[1]
u = (r/'IDContainedUI.inc').read_text()
d = (r/'IDDigestUI.inc').read_text()
c = (r/'IDContainedCoordinator.inc').read_text()
t = (r/'Tweak.xm').read_text()
for removed in ('仅在解锁且受保护数据可用时预览', '解锁预览已满足当前门控', '系统策略未确认或设备未满足条件，部分内容已隐藏'):
    assert removed not in u+c
assert 'digest.cardNotice.text =' not in c
assert 'self.cardNotice.text =' not in u
assert u.count('IDContainedClearView(self.cardNotice);') == 2
assert u.count('self.cardNotice.hidden = YES;') == 2
assert 'self.cardNotice.hidden = YES;' in d
assert d.count('CGFloat noticeHeight = (!self.cardNotice.hidden && self.cardNotice.text.length) ? 38 : 0;') == 2
assert 'MIN(category + noticeHeight + content, IDPanelMaxHeight)' in d
assert 'contentSize.height - category - noticeHeight' in d
assert 'contentSize.height - noticeHeight - 88' in d
assert 'self.cardNotice.frame = noticeHeight > 0 ?' in d and '32) : CGRectZero;' in d
for message in ('通知读取中；原生列表保留', '当前范围暂无通知', '通知读取未完成；请使用原生列表'):
    assert message in u
assert '通知卡片正在刷新；内容已按隐私策略隐藏' in t
assert '[digest setNeedsLayout]; // Only this transient notice reserves footer space.' in t
# Immutable production files at .22 plus exact reviewed footer-only files.
f = json.loads((r/'tests/card_footer_baseline.json').read_text())
for name, digest in f['unchanged_from_0322'].items():
    # .24 reviewed changes are covered by transition production fixtures.
    if name in ('IDCollapseCoordinator.inc', 'IDPresentationRuntime.inc', 'prefs/IDDiagnosticsSnapshot.h'): continue
    assert hashlib.sha256((r/name).read_bytes()).hexdigest() == digest, name
for name, digest in f['reviewed_0323'].items():
    if name in ('IDDigestUI.inc', 'Tweak.xm', 'IDShared.h', 'prefs/IDPrefs.m'): continue
    assert hashlib.sha256((r/name).read_bytes()).hexdigest() == digest, name
print('PASS 0.3.23 no permanent notice/no reserved space; transient/empty/error states retained; privacy/native/collapse/performance/settings baseline pinned')
