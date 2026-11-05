"""0.3.26 bounded selection changes; no exemptions for unrelated production drift."""
from pathlib import Path
import subprocess
r=Path(__file__).resolve().parents[1]
base='134718acfcfdc82037737f47d821b573c9668b00'
def old(name): return subprocess.check_output(['git','show',base+':'+name],cwd=r,text=True)
t=(r/'Tweak.xm').read_text()
u=(r/'IDContainedUI.inc').read_text()
a=old('Tweak.xm').replace('0.3.25','0.3.26')
b=t.replace('#include "IDDeferredSelection.inc"\n','')
for start,end in [('static void IDLoadOptions(void) {','\n@interface NCNotificationStructuredListViewController'),('- (void)selectIdentifier:(NSString *)identifier {','\n- (void)refresh {'),('static void IDPreferenceChanged(', '\n%ctor {')]:
 a=a.replace(a.split(start,1)[1].split(end,1)[0],'')
 b=b.replace(b.split(start,1)[1].split(end,1)[0],'')
a=a.replace('IDWritePreference(@"DesiredApp", nil);','IDQueueDesiredAppWrite(nil);')
assert a==b, 'Unrelated Tweak production drift (snapshot/filter/render/actions/hooks must remain intact)'
expected=old('IDContainedUI.inc').replace('    [self.cardStack layoutIfNeeded];\n','    // The parent pass below lays out cardScroll and cardStack at their final width.\n    // Do not first solve the same stack at the previous container geometry.\n')
assert expected==u, 'Only redundant preliminary child layout may change'
for name in subprocess.check_output(['git','ls-tree','-r','--name-only',base],cwd=r,text=True).splitlines():
 if name.endswith(('.h','.inc','.xm','.m','.mk')) and name not in ('Tweak.xm','IDContainedUI.inc') and not name.startswith('tests/'):
  assert (r/name).read_text()==old(name).replace('0.3.25','0.3.26'),name
assert 'IDWritePreference(@"DesiredApp"' not in t
assert 'IDDesiredWritePending ? IDDesiredWriteValue : IDPreferenceValue(@"DesiredApp")' in t
select=t.split('- (void)selectIdentifier:(NSString *)identifier {',1)[1].split('\n- (void)refresh',1)[0]
for token in ('self.cardPage = 0;', '[self.selected isEqualToString:identifier] ? nil : identifier', 'self.groups = [self snapshot]', 'IDQueueDesiredAppWrite(IDDesiredAppCache)', '[self applyFilter]', '[self render]', '[self disable]', 'self.updating = NO'):
 assert token in select,token
assert 'IDWritePreference' not in select and 'dispatch_' not in select
callback=t.split('static void IDPreferenceChanged(',1)[1].split('\n%ctor',1)[0]
assert callback.index('IDQueueDesiredAppWrite(nil)') < callback.index('for (UIViewController *owner')
assert '[self layoutIfNeeded]' in u and '[self.cardStack layoutIfNeeded]' in (r/'IDDigestUI.inc').read_text()
assert 'IDNotificationPageRequests(self.snapshotRequests, self.snapshotByApp' in t
print('PASS selection persistence wiring, no-owner clear, pending-cache reload, unchanged snapshot/index/privacy/filter/render/actions, .25 grid, parent layout/readiness retained')
