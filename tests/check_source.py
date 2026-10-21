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
assert 'IDWritePreference' in prefs and 'IDNotifyPreferencesChanged' in prefs
assert 'com.local.islanddigest/changed' in tweak and 'com.local.islanddigest/changed' in shared
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
assert info['CFBundleVersion'] == '0.1.6'
assert 'preferenceloader' in (root/'control').read_text()
assert plistlib.loads((root/'IslandDigest.plist').read_bytes())['Filter']['Bundles'] == ['com.apple.springboard']
print('PASS 0.1.6 portable source/config contracts (not runtime validation)')

ui = (root / 'IDDigestUI.inc').read_text()
for key in ('PinnedApps', 'SortMode', 'LayoutMode', 'RememberFilter'):
    assert f'@"{key}"' in prefs or f'@"{key}"' in shared
assert 'IDExperienceOrder' in tweak and 'IDExperienceVisibleRows' in ui
assert 'desiredSelection' in tweak and 'openingPending' in tweak and 'dismissed' in tweak
assert '[state endAppearance]' in tweak and '[state beginAppearance]' in tweak
assert 'hold.cancelsTouchesInView = YES' in ui
assert 'cancelTrackingWithEvent:nil' in ui and 'accessibilityCustomActions' in ui
assert 'geometryUpdating' in tweak and 'CGSizeEqualToSize' in tweak
assert 'self.owner.additionalSafeAreaInsets = self.savedInsets' in tweak
assert 'self.renderedRevision != IDOptionsRevision' in tweak
assert 'previous.latest' in tweak and 'previous.hasCriticalAlert' in tweak
assert 'IDPinnedAppsController.class' in prefs
print('PASS experience source contracts (not runtime validation)')
