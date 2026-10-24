/* Only the ABI-checked masterListView getter and NCNotificationListView hierarchy
 * are eligible (including Apple's subclasses). Never hide a window, owner root,
 * banner, guessed collection view or an ancestor of our independent panel. */
- (void)restoreNativeList {
    UIView *view = self.hiddenNativeList;
    if (view && self.nativeHideToken && objc_getAssociatedObject(view, &IDNativeHideOwnerKey) == self.nativeHideToken) {
        // Clear the lease before restoring: the precise native setHidden: hook must
        // not interpret our own restoration as a system visibility reset.
        objc_setAssociatedObject(view, &IDNativeHideOwnerKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        view.hidden = self.nativeWasHidden;
    }
    self.nativeHideToken = nil;
    self.hiddenNativeList = nil;
    IDDiagnostics[@"nativeHidden"] = @NO;
    IDDiagnostics[@"nativeDecisionStage"] = self.active ? @"active-restored" : @"inactive-restored";
}
- (void)updateNativeListVisibility {
    if (!NSThread.isMainThread || self.nativeVisibilityUpdating) return;
    if (![self workAllowed]) {
        [self restoreNativeList];
        IDDiagnostics[@"nativeDecisionStage"] = @"inactive-restored";
        return;
    }
    self.nativeVisibilityUpdating = YES;
    @try {
    id rawCandidate = IDRead(self.owner, "masterListView");
    UIView *candidate = [rawCandidate isKindOfClass:UIView.class] ? rawCandidate : nil;
    self.observedNativeList = candidate;
    Class nativeClass = objc_getClass("NCNotificationListView");
    BOOL candidateTypeSupported = nativeClass && [candidate isKindOfClass:nativeClass]
        && [candidate isKindOfClass:UIView.class] && candidate != self.owner.view;
    BOOL compatible = candidateTypeSupported
        && candidate.window && candidate.window == self.owner.viewIfLoaded.window
        && [candidate isDescendantOfView:self.owner.view];
    // A current window pointer alone is insufficient during a system transition.
    // Walk only this candidate's ancestors, bounded; never scan unrelated views.
    UIView *ancestor = compatible ? candidate.superview : nil;
    for (NSUInteger depth = 0; ancestor && depth < 32; depth++, ancestor = ancestor.superview) {
        if (ancestor.hidden || ancestor.alpha <= 0.01) compatible = NO;
    }
    if (ancestor || candidate.alpha <= 0.01) compatible = NO;
    // High-risk opt-in: hide the exact current owner's typed list before
    // enumeration, card rendering, or panel layout can complete.
    if (IDAlwaysHideNative && compatible && IDCurrentNativeOwner == self) {
        id token = objc_getAssociatedObject(candidate, &IDNativeHideOwnerKey);
        if (!token || token == self.nativeHideToken) {
            if (self.hiddenNativeList && self.hiddenNativeList != candidate) [self restoreNativeList];
            if (!self.hiddenNativeList) {
                self.nativeWasHidden = candidate.hidden; self.hiddenNativeList = candidate;
                self.nativeHideToken = [IDNativeHideLease new]; self.nativeHideToken.coordinator = self;
                objc_setAssociatedObject(candidate, &IDNativeHideOwnerKey, self.nativeHideToken, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            }
            candidate.hidden = YES;
            IDDiagnostics[@"alwaysHideActive"] = @YES;
            IDDiagnostics[@"nativeHidden"] = @YES;
            IDDiagnostics[@"nativeHideSupported"] = @YES;
            IDDiagnostics[@"nativeDecisionStage"] = @"always-hide-immediate";
            IDDiagnostics[@"nativeHideReason"] = @"始终隐藏模式：已立即隐藏精确原生列表；卡片/面板仍可为空白";
            return;
        }
    }
    BOOL separate = self.overlay && self.inserted && self.panel.isViewLoaded && self.panel.view.superview
        && ![self.panel.view isDescendantOfView:candidate] && ![candidate isDescendantOfView:self.panel.view];
    NSUInteger totalRequests = 0;
    for (IDGroup *group in self.groups) totalRequests += group.count;
    IDDigestView *coverageDigest = self.panel.isViewLoaded ? (IDDigestView *)self.panel.view : nil;
    BOOL snapshotComplete = IDContainedSnapshotCoverage(self.snapshotRecords.count, totalRequests,
        self.selectedRequestCount, self.cardPage, self.cards.count, [coverageDigest pageNavigationReady]);
    id currentToken = compatible ? objc_getAssociatedObject(candidate, &IDNativeHideOwnerKey) : nil;
    BOOL ownsOrUnclaimed = !currentToken || currentToken == self.nativeHideToken;
    IDDigestView *digest = self.panel.isViewLoaded ? (IDDigestView *)self.panel.view : nil;
    BOOL cardViewportVisible = digest.cardScroll.window && !digest.cardScroll.hidden
        && digest.cardScroll.bounds.size.height >= 44 && digest.cardScroll.bounds.size.width >= 44;
    BOOL cardsReady = [digest cardsReadyForNativeHide] && [self.cards isEqual:self.renderedCards];
    BOOL compact = IDPanelCompact(self.collapseState);
    BOOL capsuleReady = compact && IDPanelRetainedHide([digest capsuleReachable],
        digest.retainedLayoutReady && [digest expandedCardsLayoutReady], [self.cards isEqual:self.renderedCards]);
    BOOL representationReady = IDContainedRepresentationReady(compact, capsuleReady, cardViewportVisible, cardsReady);
    id actionComponent = IDRead(self.owner, "masterList");
    NSString *actionABIDiagnostic = IDActionABIDiagnostic(self.owner, actionComponent, NULL);
    BOOL forwardingSupported = !actionABIDiagnostic;
    BOOL currentOwner = IDCurrentNativeOwner == self;
    BOOL supported = compatible && separate && currentOwner && self.enumerationReliable && snapshotComplete && ownsOrUnclaimed && representationReady && forwardingSupported;
    BOOL shouldHide = IDContainedShouldHide(IDEnabled, IDGrouping, IDContainedMode, IDHideNative,
        self.active, self.disabled, supported, self.panel.view.window && !self.panel.view.hidden, self.cards.count);
    BOOL leaseLost = self.hiddenNativeList && (!self.nativeHideToken
        || objc_getAssociatedObject(self.hiddenNativeList, &IDNativeHideOwnerKey) != self.nativeHideToken);
    if (IDContainedMustRestore(self.hiddenNativeList != candidate || leaseLost, shouldHide)) [self restoreNativeList];
    if (shouldHide && !self.hiddenNativeList) {
        self.nativeWasHidden = candidate.hidden;
        self.hiddenNativeList = candidate;
        self.nativeHideToken = [IDNativeHideLease new];
        self.nativeHideToken.coordinator = self;
        objc_setAssociatedObject(candidate, &IDNativeHideOwnerKey, self.nativeHideToken, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    // A retained pointer is not proof that UIKit left hidden=YES. Reassert only
    // after the entire current decision succeeds and the lease still belongs to us.
    if (IDContainedReassert(shouldHide, self.hiddenNativeList == candidate,
        self.nativeHideToken && objc_getAssociatedObject(candidate, &IDNativeHideOwnerKey) == self.nativeHideToken,
        candidate.hidden)) candidate.hidden = YES;
    IDDiagnostics[@"containedEnabled"] = @(IDContainedMode);
    IDDiagnostics[@"hideNativeRequested"] = @(IDHideNative);
    IDDiagnostics[@"alwaysHideActive"] = @(IDAlwaysHideNative);
    IDDiagnostics[@"nativeHideSupported"] = @(supported);
    IDDiagnostics[@"nativeHidden"] = @(self.hiddenNativeList == candidate && candidate.hidden
        && self.nativeHideToken && objc_getAssociatedObject(candidate, &IDNativeHideOwnerKey) == self.nativeHideToken);
    IDDiagnostics[@"nativeCandidateClass"] = rawCandidate ? NSStringFromClass(object_getClass(rawCandidate)) : @"none";
    IDDiagnostics[@"nativeCandidateSameWindow"] = @(candidate.window && candidate.window == self.owner.viewIfLoaded.window);
    IDDiagnostics[@"nativeCurrentOwner"] = @(currentOwner);
    IDDiagnostics[@"nativeDecisionStage"] = shouldHide ? @"active-hidden"
        : !candidateTypeSupported ? @"unsupported-candidate-type" : !compatible ? @"pending-candidate-window"
        : !currentOwner ? @"noncurrent-owner"
        : !representationReady ? @"pending-onscreen-layout" : @"active-fallback";
    self.nativeHideReason = !IDContainedMode ? @"插件内展示已关闭；原生列表保留"
        : !IDHideNative ? @"原生列表保留（可在设置中隐藏）"
        : !currentOwner ? @"当前显示宿主已切换；本列表已释放"
        : !self.enumerationReliable ? [NSString stringWithFormat:@"通知完整快照尚未验证（%@）；原生列表保留，可访问全部通知", IDDiagnostics[@"enumerationState"] ?: @"capturing"]
        : !snapshotComplete ? @"完整快照或分页入口尚未就绪；原生列表保留"
        : !ownsOrUnclaimed ? @"原生列表被其他面板持有；保留原生列表"
        : !forwardingSupported ? [NSString stringWithFormat:@"原生列表保留（%@）", actionABIDiagnostic ?: @"原生动作 ABI 不支持"]
        : !compatible ? @"原生列表接口不匹配；保留原生列表"
        : !separate ? @"面板未独立挂载；保留原生列表"
        : compact && !capsuleReady ? @"折叠胶囊或保留布局未验证；原生列表保留"
        : compact && shouldHide ? @"面板已折叠；完整布局与动作能力已验证，原生列表继续隐藏；点按胶囊恢复"
        : !cardViewportVisible ? @"卡片可视空间不足；保留原生列表"
        : !cardsReady ? @"卡片尚未完成可见布局；保留原生列表"
        : shouldHide ? @"已隐藏原生列表；未删除任何通知"
        : @"面板不可见或无卡片；保留原生列表";
    IDDiagnostics[@"nativeHideReason"] = self.nativeHideReason;
    [self recordFirstFrameHandoff];
    IDDiagnostics[@"previewPolicyReason"] = IDPreviewWhenUnlocked
        ? @"contentPreviewSetting-unmapped; explicit-default-off-unlocked-opt-in; per-app Never cannot be verified"
        : @"user-disabled; fail-closed";
    IDDiagnostics[@"cardsRendered"] = @(IDContainedMode ? digest.builtCardCount : 0);
    NSUInteger redacted = 0;
    for (NSDictionary *card in self.cards) if ([card[@"redacted"] boolValue]) redacted++;
    IDDiagnostics[@"cardsRedacted"] = @(IDContainedMode ? redacted : 0);
    IDDiagnostics[@"cardsLayoutReady"] = @(cardsReady);
    NSDictionary *readiness = digest.cardReadinessDiagnostic;
    IDDiagnostics[@"cardsReadinessReason"] = ![self.cards isEqual:self.renderedCards] ? @"stale-rendered-model" : (readiness[@"reason"] ?: @"not-evaluated");
    IDDiagnostics[@"cardsReadinessIndex"] = readiness[@"index"] ?: @(-1);
    IDDiagnostics[@"cardsReadinessType"] = readiness[@"type"] ?: @"none";
    IDDiagnostics[@"cardsReadinessWidth"] = readiness[@"width"] ?: @0;
    IDDiagnostics[@"cardsReadinessHeight"] = readiness[@"height"] ?: @0;
    IDDiagnostics[@"panelCollapseState"] = @[@"expanded", @"collapsing", @"collapsed", @"expanding"][self.collapseState];
    IDDiagnostics[@"retainedExpandedLayoutReady"] = @(digest.retainedLayoutReady);
    IDDiagnostics[@"capsuleReachable"] = @([digest capsuleReachable]);
    IDDiagnostics[@"capsuleHideReady"] = @(capsuleReady);
    // Geometry only, never notification text; helps verify real device content guides.
    IDDiagnostics[@"cardsContentHeight"] = @(digest.cardScroll.contentSize.height);
    IDDiagnostics[@"cardsViewportHeight"] = @(digest.cardScroll.bounds.size.height);
    IDDiagnostics[@"cardsStackHeight"] = @(digest.cardStack.bounds.size.height);
    IDDiagnostics[@"cardsScrollEnabled"] = @(digest.cardScroll.scrollEnabled && digest.cardScroll.panGestureRecognizer.enabled);
    // Preserve the measured active verdict when teardown later clears current state.
    // Counts/geometry/classes only: no notification content, identifiers or pointers.
    if (currentOwner) IDDiagnostics[@"nativeLastActiveDecision"] = @{
        @"stage": IDDiagnostics[@"nativeDecisionStage"],
        @"candidateClass": IDDiagnostics[@"nativeCandidateClass"],
        @"sameWindow": IDDiagnostics[@"nativeCandidateSameWindow"],
        @"currentOwner": @(currentOwner), @"nativeHidden": IDDiagnostics[@"nativeHidden"],
        @"cardsRendered": @(digest.builtCardCount), @"cardsLayoutReady": @(cardsReady),
        @"viewportWidth": @(digest.cardScroll.bounds.size.width),
        @"viewportHeight": @(digest.cardScroll.bounds.size.height),
        @"contentHeight": @(digest.cardScroll.contentSize.height),
        @"stackHeight": @(digest.cardStack.bounds.size.height),
        @"reason": self.nativeHideReason ?: @"unknown"
    };
    if (IDContainedMode && digest.cardNotice) digest.cardNotice.text = [NSString stringWithFormat:@"%@\n%@", redacted ? @"系统策略未确认或设备条件不足，部分内容已隐藏" : @"解锁预览已满足当前门控；无法核验每 App 的 Never 映射", self.nativeHideReason];
    } @finally { self.nativeVisibilityUpdating = NO; }
}
- (void)dropContainedContent {
    [self cancelSnapshot]; // Cancel pending slices
    if (!UIApplication.sharedApplication.protectedDataAvailable) [self invalidateFirstFrameHandoff];
    // Release retained native references too.
    [self restoreNativeList]; // Restore native access before any potentially large privacy walk.
    self.cards = @[];
    self.pendingActionRequest = nil; self.pendingAction = nil;
    self.renderedCards = nil;
    if (self.panel.isViewLoaded) [(IDDigestView *)self.panel.view dropCards];
}
