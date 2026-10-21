/* Only the verified iOS 17 masterListView getter and exact NCNotificationListView
 * class are eligible. No collection-view guessing, reparenting, request deletion,
 * cell filtering or persisted view state. Own panel must be an independent sibling. */
- (void)restoreNativeList {
    UIView *view = self.hiddenNativeList;
    if (view && self.nativeHideToken && objc_getAssociatedObject(view, &IDNativeHideOwnerKey) == self.nativeHideToken) {
        view.hidden = self.nativeWasHidden;
        objc_setAssociatedObject(view, &IDNativeHideOwnerKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    self.nativeHideToken = nil;
    self.hiddenNativeList = nil;
    IDDiagnostics[@"nativeHidden"] = @NO;
}
- (void)updateNativeListVisibility {
    if (!NSThread.isMainThread) return;
    UIView *candidate = IDRead(self.owner, "masterListView");
    Class nativeClass = objc_getClass("NCNotificationListView");
    BOOL compatible = nativeClass && object_getClass(candidate) == nativeClass
        && [candidate isKindOfClass:UIView.class] && candidate != self.owner.view
        && [candidate isDescendantOfView:self.owner.view];
    BOOL separate = self.overlay && self.inserted && self.panel.isViewLoaded && self.panel.view.superview
        && ![self.panel.view isDescendantOfView:candidate] && ![candidate isDescendantOfView:self.panel.view];
    NSUInteger totalRequests = 0;
    for (IDGroup *group in self.groups) totalRequests += group.count;
    BOOL snapshotComplete = totalRequests < IDContainedLimit && self.cards.count < IDContainedLimit;
    id currentToken = compatible ? objc_getAssociatedObject(candidate, &IDNativeHideOwnerKey) : nil;
    BOOL ownsOrUnclaimed = !currentToken || currentToken == self.nativeHideToken;
    IDDigestView *digest = self.panel.isViewLoaded ? (IDDigestView *)self.panel.view : nil;
    BOOL cardViewportVisible = digest.cardScroll.window && !digest.cardScroll.hidden
        && digest.cardScroll.bounds.size.height >= 44 && digest.cardScroll.bounds.size.width >= 44;
    BOOL supported = compatible && separate && self.enumerationReliable && snapshotComplete && ownsOrUnclaimed && cardViewportVisible;
    BOOL shouldHide = IDContainedShouldHide(IDEnabled, IDGrouping, IDContainedMode, IDHideNative,
        self.active, self.disabled, supported, self.panel.view.window && !self.panel.view.hidden, self.cards.count);
    if (IDContainedMustRestore(self.hiddenNativeList != candidate, shouldHide)) [self restoreNativeList];
    if (shouldHide && !self.hiddenNativeList) {
        self.nativeWasHidden = candidate.hidden;
        self.hiddenNativeList = candidate;
        self.nativeHideToken = [NSObject new];
        objc_setAssociatedObject(candidate, &IDNativeHideOwnerKey, self.nativeHideToken, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        candidate.hidden = YES;
    }
    IDDiagnostics[@"containedEnabled"] = @(IDContainedMode);
    IDDiagnostics[@"hideNativeRequested"] = @(IDHideNative);
    IDDiagnostics[@"nativeHideSupported"] = @(supported);
    IDDiagnostics[@"nativeHidden"] = @(self.hiddenNativeList != nil);
    self.nativeHideReason = !IDContainedMode ? @"插件内展示已关闭；原生列表保留"
        : !IDHideNative ? @"原生列表保留（可在设置中隐藏）"
        : !self.enumerationReliable ? @"通知枚举未验证；原生列表保留"
        : !snapshotComplete ? @"卡片达到上限；保留原生列表以访问较早通知"
        : !ownsOrUnclaimed ? @"原生列表被其他面板持有；保留原生列表"
        : !compatible ? @"原生列表接口不匹配；保留原生列表"
        : !separate ? @"面板未独立挂载；保留原生列表"
        : !cardViewportVisible ? @"卡片可视空间不足；保留原生列表"
        : shouldHide ? @"已隐藏原生列表；未删除任何通知"
        : @"面板不可见或无卡片；保留原生列表";
    IDDiagnostics[@"nativeHideReason"] = self.nativeHideReason;
    IDDiagnostics[@"previewPolicyReason"] = @"effective-per-request-policy-unverified; fail-closed";
    IDDiagnostics[@"cardsRendered"] = @(IDContainedMode ? self.cards.count : 0);
    IDDiagnostics[@"cardsRedacted"] = @(IDContainedMode ? self.cards.count : 0);
}
- (void)dropContainedContent {
    self.cards = @[];
    self.renderedCards = nil;
    if (self.panel.isViewLoaded) [(IDDigestView *)self.panel.view dropCards];
    [self restoreNativeList];
}
