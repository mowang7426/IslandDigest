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
assert 'allNotificationRequests' in (root/'IDEnumerationAdapter.inc').read_text() and 'filteredNotificationRequests' in (root/'IDEnumerationAdapter.inc').read_text()
assert 'containsUnmanagedContent:YES preventsUserDismissal:YES' in tweak
assert 'while (' not in tweak, 'No unbounded polling loops'
assert '@[@1, @3, @8, @20, @125]' in tweak
assert 'IslandDigest-0.1.4.startup-pending' not in tweak
assert 'hooks-installed' in tweak and 'panel-visible' in tweak and 'owner-observed' in tweak
entry = plistlib.loads((root/'layout/Library/PreferenceLoader/Preferences/IslandDigest.plist').read_bytes())['entry']
info = plistlib.loads((root/'prefs/Resources/Info.plist').read_bytes())
assert entry['detail'] == info['NSPrincipalClass'] == 'IDPrefsController'
assert info['CFBundleVersion'] == '0.3.10'
ui_contained = (root / 'IDContainedUI.inc').read_text()
assert 'IDCardView *shell = [IDCardView new]' in ui_contained
assert 'shell.userInteractionEnabled = YES' in ui_contained
assert '打开诊断：点击已收到' in tweak
assert 'NCNotificationMasterList' in tweak and 'IDRead(self.owner, "masterList")' in tweak
assert 'IDActionABIDiagnostic' in tweak
assert 'method_getNumberOfArguments(method) != 8' in tweak
assert 'IDBoolEncoding' in tweak and "*unqualified == 'c' || *unqualified == 'B'" in tweak
assert 'IDObjectEncoding' in tweak
assert '原生接收器缺失' in tweak and '原生动作选择器缺失' in tweak
assert '原生组件类型不匹配' in tweak
assert 'UIApplication.sharedApplication.openURL' not in tweak and 'openURL:' not in tweak
assert 'requestsExecuteAction:forNotificationRequest:requestAuthentication:withParameters:completion:' in tweak
assert 'void (^completion)(void)' in tweak
assert 'NCNotificationViewController alloc' not in tweak
assert 'AllowPreviewWhenUnlocked' in tweak and 'AllowPreviewWhenUnlocked' in prefs
assert 'MoWang' in prefs
assert 'contentPreviewSetting' in (root / 'IDContainedSupport.inc').read_text()
assert 'preferenceloader' in (root/'control').read_text()
assert plistlib.loads((root/'IslandDigest.plist').read_bytes())['Filter']['Bundles'] == ['com.apple.springboard']
print('PASS 0.1.9 portable source/config contracts (not runtime validation)')

ui = (root / 'IDDigestUI.inc').read_text()
for key in ('PinnedApps', 'SortMode', 'LayoutMode', 'CategoryStyle', 'RememberFilter'):
    assert f'@"{key}"' in prefs or f'@"{key}"' in shared
assert 'IDExperienceOrder' in tweak and 'IDExperienceVisibleRows' in ui
assert 'desiredSelection' in tweak and 'openingPending' in tweak and 'dismissed' in tweak
assert '[state endAppearance]' in tweak and '[state beginAppearance]' in tweak
assert 'hold.cancelsTouchesInView = YES' in ui
assert 'cancelTrackingWithEvent:nil' in ui and 'accessibilityCustomActions' in ui
assert 'geometryUpdating' in tweak and 'CGSizeEqualToSize' in tweak
assert 'additionalSafeAreaInsets =' not in tweak, 'Own overlays must never change the system owner safe area'
assert 'window.safeAreaInsets' in tweak and 'CGRectIntersection(owner.bounds, safe)' in tweak
assert 'IDLayoutPanel' in tweak and 'UIKit-window-bounded' in tweak
assert 'self.renderedRevision != IDOptionsRevision' in tweak
assert 'previous.latest' in tweak and 'previous.hasCriticalAlert' in tweak
assert 'IDMiniControls = [IDStringOption(@"CategoryStyle"' in tweak
assert 'self.cardScroll.userInteractionEnabled = YES' in ui
# The only touch recognizer is attached to the actual scroll viewport.
assert 'cardTapLayer' not in ui + tweak and 'rebuildCardTapOverlay' not in ui + tweak
assert 'hitTest:' not in ui
for required in ('[self.cardScroll addGestureRecognizer:self.cardTap]',
    '[self.cardTap requireGestureRecognizerToFail:self.cardScroll.panGestureRecognizer]',
    'self.cardTap.cancelsTouchesInView = NO', 'self.cardScroll.scrollEnabled = YES',
    'self.cardScroll.canCancelContentTouches = YES', 'self.cardScroll.clipsToBounds = YES',
    'self.cardScroll.showsVerticalScrollIndicator = YES', 'IDScrollTapAllowed(',
    '[recognizer locationInView:self.cardScroll]',
    '[view convertRect:view.bounds toView:self.cardScroll]',
    '[(IDCardView *)view accessibilityActivate]'):
    assert required in ui, required
assert 'shouldRecognizeSimultaneously' not in ui
# Notification card shells still use ONLY the viewport tap recognizer. The two
# pagination buttons are distinct controls and must not hijack card activation.
assert 'shell addTarget:' not in ui_contained
assert ui_contained.count('forControlEvents:UIControlEventTouchUpInside') == 2
assert '[self.previousPageButton addTarget:state action:@selector(previousCardPage)' in ui_contained
assert '[self.nextPageButton addTarget:state action:@selector(nextCardPage)' in ui_contained
assert 'shell.openAction = ^{' in ui_contained
assert 'CGPoint previousOffset = self.cardScroll.contentOffset' in ui_contained
assert 'MIN(MAX(0, previousOffset.y), maximum)' in ui_contained
assert 'IDCardCoverageUsable(self.cardScroll.contentSize.height, CGRectGetMaxY(self.cardStack.frame))' in ui_contained
assert 'self.cardTap.enabled = NO; self.cardTap.enabled = YES' in ui_contained
for key in ('cardsContentHeight', 'cardsViewportHeight', 'cardsStackHeight', 'cardsScrollEnabled'):
    assert f'@"{key}"' in prefs and f'@"{key}"' in (root/'IDContainedCoordinator.inc').read_text()
assert 'return [weakShell accessibilityActivate]' in ui_contained
assert '- (BOOL)accessibilityActivate' in tweak and 'self.openAction();' in tweak
for key in ('PanelBackgroundOpacity', 'PanelBlur'):
    assert f'@"{key}"' in tweak and f'@"{key}"' in prefs
assert 'IDPanelBlur = IDPref(@"PanelBlur", NO)' in tweak
assert 'IDNumberOption(@"PanelBackgroundOpacity", 0, 0, 100) / 100.0' in tweak
assert 'self.glass.effect = compact ? nil : (IDPanelBlur ?' in ui
assert 'self.glass.hidden = compact || (!IDPanelBlur && IDPanelOpacity == 0)' in ui
assert 'colorWithAlphaComponent:0.20' in ui_contained
assert 'systemBlueColor' not in ui and 'systemGreenColor' not in ui
feedback = tweak.split('static BOOL IDOpenFeedback(NSString *message) {', 1)[1].split('\n}', 1)[0]
assert 'IDStatus(@"card-tap-feedback", message)' in feedback
assert 'cardNotice' not in feedback and 'IDCardFeedbackUntil' not in tweak
assert '@"stage", @"reason"' in prefs, 'Settings must retain existing feedback diagnostics'
print('PASS 0.2.9 native scroll/tap ownership, shared AX action, transparent defaults, Settings diagnostics')
assert 'content.userInteractionEnabled = NO' in (root / 'IDContainedUI.inc').read_text()
assert '打开诊断：点击已收到；正在检查原生动作' in (root / 'IDContainedUI.inc').read_text()
assert 'accessibilityCustomActions' in (root / 'IDContainedUI.inc').read_text()
assert 'UIApplication.sharedApplication.openURL' not in tweak and 'openURL:' not in tweak

clear = (root / 'IDClearCoordinator.inc').read_text()
support = (root / 'IDClearSupport.inc').read_text()
logic = (root / 'IDClearLogic.h').read_text()
assert '#include "IDClearCoordinator.inc"' in tweak
assert '#include "IDClearSupport.inc"' in tweak
assert '#import "IDClearLogic.h"' in tweak
assert 'IDClearEligible(' in clear and 'IDClearConfirm(' in clear and 'IDClearSnapshotMember(' in clear
assert 'notificationStructuredListViewController:requestsClearingNotificationRequests:' in support
for forbidden in ('requestsClearingNotificationRequestsInSections:', 'RequestsClearingAll:', 'clearAllNotification', 'sqlite', 'BBServer'):
    assert forbidden not in clear + support
assert clear.count('sel_registerName(IDClearSelector), self.owner, send.copy') == 1
assert 'self.clearSnapshot = tickets.copy' in clear
confirm = clear.split('- (void)confirmClear:')[1]
assert 'for (NSDictionary *ticket in self.clearSnapshot)' in confirm
assert 'request == original' in confirm and 'matchingKeys == 1' in confirm
assert 'self.clearMaster == IDRead' in confirm and 'self.clearDelegate == delegate' in confirm
assert 'self.clearRevision == IDOptionsRevision' in confirm and 'scopeMatches' in confirm
assert 'clearWindow' in confirm and 'presentation' in confirm
assert 'IDClearBool(request, "isCriticalAlert"' in clear
assert 'hideClearActionInList' in clear and 'lockScreenPersistence' in clear and '"clearAction"' in clear
assert 'isAuthenticated' in clear and 'CSCoverSheetViewController' in clear
assert 'attempt < 2' in clear and 'dispatch_after' in clear and 'while (' not in clear
assert 'UIAlertActionStyleDestructive' in clear and 'UIAlertActionStyleCancel' in clear
assert 'clearAvailable' in prefs and 'clearReason' in prefs
assert '始终确认' in prefs and 'EnableClearing' in prefs
assert 'IslandDigestClear' in ui and 'clearButton.enabled = NO' in ui
assert 'IDClearCancel(&_clearGate)' in clear and '[state cancelClear]' in tweak
assert '#include "../IDClearLogic.h"' in (root/'tests/Clear.c').read_text()
print('PASS clear source guardrails (not native behavior validation)')

# 0.3.10: actual UIKit constraints, paired with the shared geometry C tests.
assert 'IDExperienceCategoryGeometry([self rowHeight], IDMiniControls)' in ui
for contract in (
    'tile.userInteractionEnabled = NO; tile.isAccessibilityElement = NO',
    'button.backgroundColor = UIColor.clearColor',
    '[tile.heightAnchor constraintEqualToConstant:geometry.visualHeight]',
    '[content.centerXAnchor constraintEqualToAnchor:tile.centerXAnchor]',
    '[content.centerYAnchor constraintEqualToAnchor:tile.centerYAnchor]',
    '[content.leadingAnchor constraintGreaterThanOrEqualToAnchor:tile.leadingAnchor constant:geometry.contentPadding]',
    '[content.trailingAnchor constraintLessThanOrEqualToAnchor:tile.trailingAnchor constant:-geometry.contentPadding]',
    '[badge setContentCompressionResistancePriority:UILayoutPriorityRequired',
    '[name setContentHuggingPriority:UILayoutPriorityRequired',
    'NSLineBreakByTruncatingTail', 'return MAX(44, MIN(80,'):
    assert contract in ui, contract
assert 'content.leadingAnchor constraintEqualToAnchor:button.leadingAnchor' not in ui
assert 'content.trailingAnchor constraintEqualToAnchor:button.trailingAnchor' not in ui
assert 'if ([key isEqualToString:@"CategoryStyle"]) return IDStringOption' in prefs
assert info['CFBundleShortVersionString'] == '0.3.10'
assert 'Version: 0.3.10' in (root/'control').read_text()
assert "'0.3.10', '${{ matrix.scheme }}'" in (root/'.github/workflows/build.yml').read_text()
assert 'IslandDigest-0.3.10.startup-pending' in tweak
# The real coordinator, not just the adapter, must leave a pending cursor intact.
caller = tweak.split('- (void)continueSnapshot:(NSUInteger)generation {', 1)[1].split('- (void)finishSnapshot {', 1)[0]
pending = caller.split('if (action == IDSnapshotPending) {', 1)[1].split('if (action != IDSnapshotSort) return;', 1)[0]
assert '[weakSelf continueSnapshot:generation];' in pending and 'return;' in pending
assert 'self.snapshotSorting = YES;' in caller and 'state.enumerationCursor = nil;' in caller
assert caller.index('return;\n    }\n    if (action != IDSnapshotSort)') < caller.index('NSArray *records = [self.buildRecords copy]')
assert 'tests/Snapshot.c' in (root/'.github/workflows/build.yml').read_text()
print('PASS coordinator pending continuation returns before sorting/commit; production state gate is CI-tested')
print('PASS 0.3.10 inset visual tile, bounded intrinsic centered group, full count/pin space, 44pt hit area and versions')
