from pathlib import Path
root = Path(__file__).resolve().parents[1]
main = (root/'Tweak.xm').read_text()
support = (root/'IDContainedSupport.inc').read_text()
coordinator = (root/'IDContainedCoordinator.inc').read_text()
ui = (root/'IDContainedUI.inc').read_text()
assert 'IDContainedMode = YES, IDHideNative = NO' in main
assert 'IDPref(@"ContainedMode", YES)' in main
assert 'IDPref(@"HideNativeList", NO) && IDContainedMode' in main
assert 'IDContainedPrivacyGate(NSThread.isMainThread, 0, 0, 0, 0, 0, 0)' in support
assert 'id content = allowed ? IDRead(request, "content") : nil' in support
for field in ('title', 'subtitle', 'message'):
    assert f'allowed ? IDContainedText(IDRead(content, "{field}"))' in support
assert 'cards.count >= IDContainedLimit' in support
assert 'notificationIdentifier' in support and 'containsObject:identity' in support
assert 'IDRead(self.owner, "masterListView")' in coordinator
assert 'object_getClass(candidate) == nativeClass' in coordinator
assert 'isDescendantOfView:candidate' in coordinator
assert 'view.hidden = self.nativeWasHidden' in coordinator
assert 'self.hiddenNativeList = nil' in coordinator
assert 'IDContainedMustRestore' in coordinator and 'IDContainedShouldHide' in coordinator
for forbidden in ('removeNotificationRequest', 'clearNotification', 'toggleFiltering', 'UICollectionView', 'removeFromSuperview', 'writeToFile', 'IDWritePreference'):
    assert forbidden not in coordinator
assert '[self dropContainedContent];' in main.split('- (void)endAppearance {',1)[1].split('\n}',1)[0]
assert '[self dropContainedContent];' in main.split('- (void)recover {',1)[1].split('\n}',1)[0]
assert 'dispatch_sync(dispatch_get_main_queue()' not in main
assert 'dispatch_async(dispatch_get_main_queue(), redact)' in main
assert 'UIApplicationProtectedDataWillBecomeUnavailable' in main
assert '- (void)setDeviceAuthenticated:(BOOL)authenticated' in main
assert '[self.cards isEqual:self.renderedCards]' in main
assert 'if (!IDContainedMode && self.selected' in main
assert 'NSString *selected = IDContainedMode ? nil : IDState(self).selected' in main
assert '仅预览，不支持点按打开或快捷回复' in ui
for forbidden in ('writeToFile', 'IDWritePreference', 'NSUserDefaults', 'NSLog', 'NSURLSession', 'openURL'):
    assert forbidden not in support + ui
print('PASS contained privacy/hide/source guardrails (not native runtime verification)')

prefs = (root/"prefs/IDPrefs.m").read_text()
for key in ("ContainedMode", "HideNativeList", "containedEnabled", "hideNativeRequested", "nativeHideSupported", "nativeHidden", "nativeHideReason", "previewPolicyReason", "cardsRendered", "cardsRedacted"):
    assert f'@"{key}"' in prefs
assert "snapshotComplete" in coordinator
assert "IDNativeHideOwnerKey" in coordinator
assert "cardViewportVisible" in coordinator and "bounds.size.height >= 44" in coordinator
assert "depth > 8 || requests.count >= 4096" in main
assert "reliable = IDCollectRequests(allRequests, requests, 0) && reliable" in main
