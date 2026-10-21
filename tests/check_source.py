"""Portable source/config contracts; not an iOS runtime test."""
from pathlib import Path
import plistlib
root = Path(__file__).resolve().parents[1]
tweak = (root / 'Tweak.xm').read_text()
shared = (root / 'IDShared.h').read_text()
prefs = (root / 'prefs/IDPrefs.m').read_text()
for key in ('Enabled', 'Grouping', 'ShowCounts'):
    assert f'@"{key}"' in tweak and f'@"{key}"' in prefs
assert 'kCFPreferencesCurrentUser, kCFPreferencesAnyHost' in shared
assert 'kCFPreferencesCurrentUser, kCFPreferencesAnyHost' in prefs
assert 'com.local.islanddigest/changed' in tweak and 'com.local.islanddigest/changed' in prefs
assert 'com.local.islanddigest/retry' in tweak and 'com.local.islanddigest/retry' in prefs
assert 'viewDidLayoutSubviews' in tweak and 'IDEnsureState(self)' in tweak
assert 'allNotificationRequests' in tweak and 'filteredNotificationRequests' in tweak
assert 'containsUnmanagedContent:YES preventsUserDismissal:YES' in tweak
assert 'while (' not in tweak, 'No unbounded polling loops'
assert '@[@1, @3, @8, @20, @125]' in tweak
assert 'IslandDigest-0.1.4.startup-pending' not in tweak
assert 'hooks-installed' in tweak and 'panel-visible' in tweak and 'owner-observed' in tweak
entry = plistlib.loads((root/'layout/Library/PreferenceLoader/Preferences/IslandDigest.plist').read_bytes())['entry']
info = plistlib.loads((root/'prefs/Resources/Info.plist').read_bytes())
assert entry['detail'] == info['NSPrincipalClass'] == 'IDPrefsController'
assert info['CFBundleVersion'] == '0.1.5'
assert 'preferenceloader' in (root/'control').read_text()
assert plistlib.loads((root/'IslandDigest.plist').read_bytes())['Filter']['Bundles'] == ['com.apple.springboard']
print('PASS 0.1.5 portable source/config contracts (not runtime validation)')
