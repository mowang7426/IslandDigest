"""0.3.20 navigation/lazy-read contracts; Foundation fixtures run on macOS CI."""
from pathlib import Path
r = Path(__file__).resolve().parents[1]
home = (r/'prefs/IDPrefs.m').read_text()
page = (r/'prefs/IDDiagnosticsController.m').read_text()
header = (r/'prefs/IDDiagnosticsController.h').read_text()
snapshot = (r/'prefs/IDDiagnosticsSnapshot.h').read_text()
assert '#import "IDDiagnosticsController.h"' in home
link = '[PSSpecifier preferenceSpecifierNamed:@"诊断信息" target:self set:NULL get:NULL detail:IDDiagnosticsController.class cell:PSLinkCell edit:Nil]'
assert home.count(link) == 1
assert home.index(link) > home.index('关于与兼容性')
assert home.split(link, 1)[1].split('_specifiers = items;', 1)[0].strip() == '];'
assert '@interface IDDiagnosticsController : PSListController' in header
assert '@implementation IDDiagnosticsController' in page
assert 'IslandDigestPrefs_FILES := IDPrefs.m IDDiagnosticsController.m' in (r/'prefs/Makefile').read_text()
for forbidden in ('IDStatusPath', 'IDDiagnosticSnapshot(', 'IDCollapseSummary(', 'IDRuntimeSummary(', 'IDDiagnosticsSummary('):
    assert forbidden not in home
assert 'objectForInfoDictionaryKey:@"CFBundleVersion"' in page
import re
shared = (r/'IDShared.h').read_text()
assert re.search(r'IDStatusPath = (@"[^"]+")', page).group(1) == re.search(r'IDStatusPath = (@"[^"]+")', shared).group(1)
assert page.count('dictionaryWithContentsOfFile:IDStatusPath') == 1
refresh = page.split('- (void)idRefreshDiagnostics {', 1)[1].split('\n}', 1)[0]
assert 'dictionaryWithContentsOfFile:IDStatusPath' in refresh
assert '[self idRefreshDiagnostics];' in page.split('- (void)viewWillAppear:')[1]
assert '@selector(idRefreshDiagnostics)' in page
assert 'dictionaryWithContentsOfFile' not in page.split('- (NSArray *)specifiers {')[1].split('- (void)idRefreshDiagnostics')[0]
for forbidden in ('NSTimer', 'dispatch_after', 'NSLog', 'IDWritePreference(', 'IDNotifyPreferencesChanged(', 'CFNotificationCenterPostNotification', 'heightForFooterInSection', 'CGRectMake', 'UIColor', 'UIFont'):
    assert forbidden not in page
for section in ('IDCollapseSummary(snapshot)', 'IDRuntimeSummary(snapshot)', 'IDDiagnosticsSummary(snapshot)', 'IDDiagnosticAvailability(snapshot, version)'):
    assert section in page
for field in ('primaryBoundary', 'primaryEndAppeared', 'primaryLifecycleHook', 'hookAudit', 'transitionSample', 'runtimeManager', 'runtimeSlider', 'ownerAncestors'):
    assert '@"'+field+'"' in snapshot
for forbidden in ('@"body"', '@"notificationIdentifier"', '@"PinnedApps"', '@"AppNames"', '@"ObservedApps"', '%p', 'dictionaryWithContentsOfFile', 'NSLog'):
    assert forbidden not in snapshot
assert 'IDDiagnosticSnapshot' in snapshot and 'depth > 3' in snapshot
assert '诊断信息' in page and 'footerText' in page and 'PSGroupCell' in page
workflow = (r/'.github/workflows/build.yml').read_text()
assert 'tests/check_diagnostics_page.py' in workflow and 'tests/DiagnosticsSnapshot.m' in workflow
print('PASS 0.3.20 bottom native navigation, compiled class destination, one on-demand snapshot, full audit/primary/status fields, privacy and native sizing contracts; not device proof')
