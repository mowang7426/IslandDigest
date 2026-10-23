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
assert '[candidate isKindOfClass:nativeClass]' in coordinator
assert 'object_getClass(candidate) == nativeClass' not in coordinator
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
adapter = (root/"IDEnumerationAdapter.inc").read_text()
assert "depth > 8" in adapter and "requests.count >= 4096" in adapter
assert "IDEnumerationSnapshot(master, IDDiagnostics, &reliable)" in main
assert "self.enumerationReliable = reliable;" in main

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
assert 'IDContainedRepresentationReady(compact, capsuleReady, cardViewportVisible, cardsReady)' in coordinator
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

# 0.3.6 targeted recovery: real post-layout callback, lease-aware reassert,
# replacement tracking and bounded exact-class hooks; not a global UIView hook.
assert 'IDContainedReassert(shouldHide' in coordinator
assert 'self.hiddenNativeList != candidate || leaseLost' in coordinator
assert 'nativeHideToken.coordinator = self' in coordinator
assert 'candidate.window == self.owner.viewIfLoaded.window' in coordinator
assert 'IDCurrentNativeOwner == self' in coordinator
assert 'nativeLastActiveDecision' in coordinator and 'nativeCandidateClass' in coordinator
assert 'nativeDecisionStage' in coordinator and 'pending-onscreen-layout' in coordinator
assert '%hook NCNotificationListView' in main and '%hook UIView' not in main
assert 'IDMethod(nativeListClass, "setHidden:", "v", 3)' in main
assert 'IDArgument(nativeListClass, "setHidden:", 2, "B")' in main
assert 'IDContainedVisibilityEnqueue([self workAllowed]' in main
assert 'generation != state.presentationGeneration' in main
callback = main.split('- (void)scheduleNativeListVisibility {', 1)[1].split('- (void)schedulePresentation {',1)[0]
for forbidden in ('updatePanelGeometry', '[state refresh]', 'layoutIfNeeded', 'scheduleNativeListVisibility];'):
    assert forbidden not in callback
assert '[self.coordinator scheduleNativeListVisibility]' in digest
assert 'self.nativeLayoutCards != self.coordinator.cards' in digest
assert 'self.nativeLayoutWindow != self.window' in digest
assert 'self.nativeLayoutSignatureValid = NO' in ui
# Never synchronize the prohibited 0.3.4 preparation implementation.
assert 'prepareFirstFrame' not in main + digest + coordinator
assert 'dispatch_sync' not in main + digest + coordinator
assert 'valid == self.builtCardCount && visible' in ui  # ANY visible card, not all cards.
print('PASS 0.3.6 post-layout recovery, native subclass/window/lease/reassert/owner and no-preparation contracts')

# 0.3.6 Settings must expose only the saved verdict's safe scalar fields.
prefs = (root / 'prefs/IDPrefs.m').read_text()
for key in ('nativeVisibilityHooks', 'nativeCandidateClass', 'nativeCandidateSameWindow',
            'nativeCurrentOwner', 'nativeDecisionStage', 'nativeLastActiveDecision'):
    assert key in prefs, key
assert '[lastActive isKindOfClass:NSDictionary.class]' in prefs
assert '[value isKindOfClass:NSString.class]' in prefs
assert '[value isKindOfClass:NSNumber.class]' in prefs
print('PASS 0.3.6 Settings current and last-active diagnostics scalar whitelist')
