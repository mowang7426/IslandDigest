/* Session-only presentation; no notification action or preference mutation. */
- (void)toggleCollapsed {
    if (!self.active || self.disabled || !self.inserted || IDPanelTransitioning(self.collapseState)) return;
    // Supplementary host owns its hit bounds; compact mode requires independent overlay.
    if (!self.overlay) {
        UIAccessibilityPostNotification(UIAccessibilityAnnouncementNotification, @"当前系统面板不支持安全折叠");
        return;
    }
    [self cancelClear];
    IDDigestView *digest = (IDDigestView *)self.panel.view;
    [self.owner.view layoutIfNeeded];
    [digest layoutIfNeeded];
    digest.retainedLayoutReady = [digest expandedCardsLayoutReady];
    self.collapseState = IDPanelToggle(self.collapseState);
    BOOL reduceMotion = UIAccessibilityIsReduceMotionEnabled();
    [digest updateCollapsePresentation];
    // Start alpha from the old representation; geometry changes once inside animator.
    digest.expandedContent.alpha = IDPanelCompact(self.collapseState) ? 1 : 0;
    digest.capsuleButton.hidden = NO;
    digest.capsuleButton.alpha = IDPanelCompact(self.collapseState) ? 0 : 1;
    self.collapseAnimator = [[UIViewPropertyAnimator alloc] initWithDuration:reduceMotion ? 0.16 : 0.32
        curve:UIViewAnimationCurveEaseInOut animations:^{
            self.collapseGeometryAllowed = YES;
            [self updatePanelGeometry];
            self.collapseGeometryAllowed = NO;
            // Reduce Motion uses an instantaneous bounded resize plus a cross-fade.
            if (!reduceMotion) [self.owner.view layoutIfNeeded];
            [digest updateCollapsePresentation];
        }];
    if (reduceMotion) {
        self.collapseGeometryAllowed = YES;
        [UIView performWithoutAnimation:^{ [self updatePanelGeometry]; [self.owner.view layoutIfNeeded]; }];
        self.collapseGeometryAllowed = NO;
    }
    __weak IDCoordinator *weakSelf = self;
    [self.collapseAnimator addCompletion:^(UIViewAnimatingPosition position) { [weakSelf finishCollapse]; }];
    [self.collapseAnimator startAnimation];
}
- (void)cancelCollapse {
    [self.collapseAnimator stopAnimation:YES];
    self.collapseAnimator = nil;
    self.collapseState = IDPanelExpanded;
    self.renderAfterCollapse = NO;
    self.collapseGeometryAllowed = NO;
    if (self.panel.isViewLoaded) {
        [(IDDigestView *)self.panel.view updateCollapsePresentation];
        [self updatePanelGeometry];
    }
}
- (void)finishCollapse {
    self.collapseState = IDPanelComplete(self.collapseState);
    self.collapseAnimator = nil;
    IDDigestView *digest = (IDDigestView *)self.panel.view;
    [digest updateCollapsePresentation];
    [self updatePanelGeometry];
    [self.owner.view layoutIfNeeded];
    [digest setNeedsLayout]; [digest layoutIfNeeded];
    if (self.renderAfterCollapse && self.active) {
        self.renderAfterCollapse = NO;
        [self render];
    } else [self updateNativeListVisibility];
    if (self.active && digest.window) UIAccessibilityPostNotification(UIAccessibilityLayoutChangedNotification,
        IDPanelCompact(self.collapseState) ? digest.capsuleButton : digest.collapseButton);
}
