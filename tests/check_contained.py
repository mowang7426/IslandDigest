from pathlib import Path
root = Path(__file__).resolve().parents[1]
main = (root/'Tweak.xm').read_text()
support = (root/'IDContainedSupport.inc').read_text()
coordinator = (root/'IDContainedCoordinator.inc').read_text()
ui = (root/'IDContainedUI.inc').read_text()
assert 'IDContainedMode = YES, IDHideNative = NO' in main
assert 'IDPref(@"ContainedMode", YES)' in main
assert 'IDPref(@"HideNativeList", NO) && IDContainedMode' in main
assert 'IDPreviewWhenUnlocked = NO' in main
assert 'IDPref(@"AllowPreviewWhenUnlocked", NO)' in main
assert 'application.protectedDataAvailable' in support
assert 'IDRead(request, "options")' in support
assert 'contentPreviewSetting remains intentionally uninterpreted' in support
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
assert 'shell.accessibilityTraits = UIAccessibilityTraitButton' in ui
assert 'openCardRequest' in ui and 'defaultAction' in support + main
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

# 0.1.9 rendering/layout regressions: check ordering and real-view fail-safe,
# not merely the model count or a nonzero empty viewport.
digest = (root/'IDDigestUI.inc').read_text()
assert digest.count('contentInsetAdjustmentBehavior = UIScrollViewContentInsetAdjustmentNever') == 2
assert 'contentLayoutGuide' in digest and 'frameLayoutGuide.widthAnchor constant:-28' in digest
assert 'withHorizontalFittingPriority:UILayoutPriorityRequired' in digest
assert 'MIN(category + 38 + content, IDPanelMaxHeight)' in digest
assert 'shell.translatesAutoresizingMaskIntoConstraints = NO' in ui
assert 'constraintGreaterThanOrEqualToConstant:64' in ui
assert 'self.builtCardCount = rendered' in ui and 'self.builtCardCount = 0' in ui
assert '- (BOOL)cardsReadyForNativeHide' in ui
assert 'if (!readable) return NO' in ui
assert 'valid == self.builtCardCount && visible' in ui
assert 'CGRectIntersection(frame, self.cardScroll.bounds)' in ui
assert '[digest cardsReadyForNativeHide] && [self.cards isEqual:self.renderedCards]' in coordinator
assert '&& ownsOrUnclaimed && representationReady && forwardingSupported' in coordinator
assert 'IDFirstFrameRepresentationReady(compact, capsuleReady, cardViewportVisible, cardsReady)' in coordinator
assert 'digest.retainedLayoutReady && [digest expandedCardsLayoutReady]' in coordinator
assert 'IDPanelRetainedHide([digest capsuleReachable]' in coordinator
assert 'digest.builtCardCount' in coordinator and 'cardsLayoutReady' in coordinator
render = main.split('- (void)render {',1)[1].split('\n- (void)updatePanelGeometry',1)[0]
assert render.index('showCards]') < render.index('[self updateNativeListVisibility]')
assert render.index('layoutIfNeeded]') < render.index('[self updateNativeListVisibility]')
mount = main.split('self.overlay = YES;',1)[1].split('if (self.overlay)',1)[0]
assert 'self.inserted = YES;' in mount and '[self render];' in mount
assert 'additionalSafeAreaInsets =' not in main
for key in ('PanelTopOffset', 'PanelSideInset', 'PanelMaxHeight'):
    assert f'@"{key}"' in main and f'@"{key}"' in prefs
assert 'resetPanelLayout' in prefs and 'layoutChangePending' in prefs
assert '不是已验证的时钟避让' in prefs
print('PASS 0.1.9 render-after-mount, no automatic content inset, actual card/label readiness, bounded layout controls')
reset = prefs.split('- (void)resetPanelLayout {',1)[1].split('\n}',1)[0]
assert 'self.layoutChangeGeneration++;' in reset and 'self.layoutChangePending = NO;' in reset
assert 'if (generation != self.layoutChangeGeneration) return;' in prefs
