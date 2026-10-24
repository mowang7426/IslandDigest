- (void)dropCards {
    double perfStarted = IDPerformanceStart();
    @try {
    // Clean retained shells contain no text, AX payload, model or actions.
    // Repeated inactive model changes must not recurse through them again.
    if (!self.cardContentDirty) return;
    // A snapshot rebuild must not retarget a finger already down to a new request.
    self.cardTap.enabled = NO; self.cardTap.enabled = YES;
    self.builtCardCount = 0;
    self.retainedLayoutReady = NO;
    self.nativeLayoutSignatureValid = NO;
    // Scrub synchronously, but retain empty layout shells until the next visible render.
    // Removing hundreds of active constraints during an interactive dismissal is avoidable.
    for (UIView *view in self.cardStack.arrangedSubviews) {
        IDContainedClearView(view);
        objc_setAssociatedObject(view, &IDStableCardModelKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        if ([view isKindOfClass:IDCardView.class]) ((IDCardView *)view).openAction = nil;
        view.accessibilityCustomActions = nil;
    }
    self.cardScroll.hidden = YES; // Hide one viewport, not 160 arranged views/UISV-hiding constraints.
    IDContainedClearView(self.cardNotice);
    self.cardNotice.hidden = YES;
    self.cardContentDirty = NO; // Never clear before the full privacy walk finishes.

    } @finally { IDRecordPerformance(@"scrubCards", perfStarted); }
}
- (UILabel *)cardLabel:(NSString *)text style:(UIFontTextStyle)style color:(UIColor *)color {
    UILabel *label = [UILabel new]; label.text = text; label.numberOfLines = 0;
    label.font = [UIFont preferredFontForTextStyle:style]; label.adjustsFontForContentSizeCategory = YES;
    label.translatesAutoresizingMaskIntoConstraints = NO;
    label.textColor = color; return label;
}
- (NSString *)relativeCardDate:(NSDate *)date {
    if ([date isEqual:NSDate.distantPast]) return @"时间未知";
    NSTimeInterval seconds = MAX(0, -date.timeIntervalSinceNow);
    if (seconds < 60) return @"刚刚";
    if (seconds < 3600) return [NSString stringWithFormat:@"%lu 分钟前", (unsigned long)(seconds / 60)];
    if (seconds < 86400) return [NSString stringWithFormat:@"%lu 小时前", (unsigned long)(seconds / 3600)];
    return [NSString stringWithFormat:@"%lu 天前", (unsigned long)(seconds / 86400)];
}
- (BOOL)pageNavigationReady {
    IDCoordinator *state = self.coordinator;
    if (state.selectedRequestCount <= IDContainedLimit) return YES;
    NSUInteger pages = IDContainedPageCount(state.selectedRequestCount);
    for (UIButton *button in @[self.previousPageButton ?: [UIButton new], self.nextPageButton ?: [UIButton new]]) {
        if (!button.window || button.window != self.window || button.hidden || button.alpha <= 0.01
            || !button.userInteractionEnabled || button.bounds.size.width < 44 || button.bounds.size.height < 44) return NO;
    }
    return self.previousPageButton.enabled == (state.cardPage > 0)
        && self.nextPageButton.enabled == (state.cardPage + 1 < pages)
        && [self.previousPageButton actionsForTarget:state forControlEvent:UIControlEventTouchUpInside].count > 0
        && [self.nextPageButton actionsForTarget:state forControlEvent:UIControlEventTouchUpInside].count > 0;
}
- (void)showCards {
    self.cardContentDirty = YES; // Includes retained-card reuse, empty states and paging controls.
    self.previousPageButton = nil; self.nextPageButton = nil;
    CGPoint previousOffset = self.cardScroll.contentOffset;
    NSArray<UIView *> *previousViews = self.cardStack.arrangedSubviews.copy;
    UIView *scrollAnchor = nil;
    CGFloat anchorViewportY = 0;
    for (UIView *view in previousViews) {
        if (![view isKindOfClass:IDCardView.class]) continue;
        CGRect frame = [view convertRect:view.bounds toView:self.cardScroll];
        if (CGRectIntersectsRect(frame, self.cardScroll.bounds)) {
            scrollAnchor = view; anchorViewportY = CGRectGetMinY(frame) - previousOffset.y; break;
        }
    }
    NSMutableArray<UIView *> *desiredViews = [NSMutableArray new];
    NSMutableDictionary<NSValue *, IDCardView *> *previousCards = [NSMutableDictionary new];
    for (UIView *view in previousViews) {
        NSDictionary *model = objc_getAssociatedObject(view, &IDStableCardModelKey);
        if ([view isKindOfClass:IDCardView.class] && model[@"request"])
            previousCards[[NSValue valueWithNonretainedObject:model[@"request"]]] = (IDCardView *)view;
    }
    self.cardTap.enabled = NO; self.cardTap.enabled = YES;
    self.retainedLayoutReady = NO;
    self.nativeLayoutSignatureValid = NO;
    self.cardScroll.hidden = !IDContainedMode;
    IDContainedClearView(self.cardNotice);
    self.cardNotice.hidden = YES;
    if (!IDContainedMode) { [self dropCards]; return; }
    IDCoordinator *state = self.coordinator;
    NSUInteger rendered = 0;
    for (IDGroup *group in state.groups) {
        if (state.selected && ![state.selected isEqualToString:group.identifier]) continue;
        NSMutableArray *cards = [NSMutableArray new];
        for (NSDictionary *card in state.cards) if ([card[@"app"] isEqualToString:group.identifier]) [cards addObject:card];
        if (!cards.count) continue;
        UILabel *heading = [self cardLabel:group.name style:UIFontTextStyleHeadline color:UIColor.labelColor];
        heading.accessibilityTraits = UIAccessibilityTraitHeader;
        [desiredViews addObject:heading];
        for (NSDictionary *card in cards) {
            IDCardView *retained = card[@"request"] ? previousCards[[NSValue valueWithNonretainedObject:card[@"request"]]] : nil;
            NSDictionary *oldModel = retained ? objc_getAssociatedObject(retained, &IDStableCardModelKey) : nil;
            NSNumber *revision = retained ? objc_getAssociatedObject(retained, &IDStableCardRevisionKey) : nil;
            if (IDStableReuseView(retained != nil, [oldModel isEqual:card], revision.unsignedIntegerValue == IDOptionsRevision, oldModel == nil)) {
                [desiredViews addObject:retained]; rendered++; continue;
            }
            IDCardView *shell = [IDCardView new]; shell.layer.cornerRadius = 16;
            shell.userInteractionEnabled = YES;
            shell.translatesAutoresizingMaskIntoConstraints = NO;
            shell.accessibilityIdentifier = @"IDContainedCard";
            [shell.heightAnchor constraintGreaterThanOrEqualToConstant:64].active = YES;
            shell.backgroundColor = [UIColor.systemBackgroundColor colorWithAlphaComponent:0.20];
            UIStackView *content = [UIStackView new]; content.axis = UILayoutConstraintAxisVertical; content.spacing = 5;
            content.translatesAutoresizingMaskIntoConstraints = NO; content.userInteractionEnabled = NO; [shell addSubview:content];
            UIStackView *header = [UIStackView new]; header.spacing = 8; header.alignment = UIStackViewAlignmentCenter;
            UIImageView *icon = [[UIImageView alloc] initWithImage:group.icon ?: [UIImage systemImageNamed:@"app.fill"]];
            icon.contentMode = UIViewContentModeScaleAspectFit; icon.tintColor = UIColor.secondaryLabelColor;
            icon.layer.cornerRadius = 6; icon.clipsToBounds = YES;
            [icon.widthAnchor constraintEqualToConstant:22].active = YES; [icon.heightAnchor constraintEqualToConstant:22].active = YES;
            [header addArrangedSubview:icon];
            UILabel *app = [self cardLabel:group.name style:UIFontTextStyleCaption1 color:UIColor.secondaryLabelColor];
            [header addArrangedSubview:app];
            NSString *relative = [self relativeCardDate:card[@"date"]];
            UILabel *date = [self cardLabel:relative style:UIFontTextStyleCaption2 color:UIColor.secondaryLabelColor];
            date.textAlignment = NSTextAlignmentRight; [header addArrangedSubview:date]; [content addArrangedSubview:header];
            for (NSString *key in @[@"title", @"subtitle", @"body"]) {
                NSString *text = card[key]; if (!text.length) continue;
                IDNotificationLabel *styled = [IDNotificationLabel new];
                styled.text = text; styled.numberOfLines = 0;
                styled.font = [UIFont preferredFontForTextStyle:[key isEqualToString:@"title"] ? UIFontTextStyleHeadline : UIFontTextStyleBody];
                styled.adjustsFontForContentSizeCategory = YES; styled.translatesAutoresizingMaskIntoConstraints = NO;
                styled.originalColor = [card[@"redacted"] boolValue] ? UIColor.secondaryLabelColor : UIColor.labelColor;
                UILabel *line = styled;
                [content addArrangedSubview:line];
            }
            [NSLayoutConstraint activateConstraints:@[
                [content.leadingAnchor constraintEqualToAnchor:shell.leadingAnchor constant:12],
                [content.trailingAnchor constraintEqualToAnchor:shell.trailingAnchor constant:-12],
                [content.topAnchor constraintEqualToAnchor:shell.topAnchor constant:10],
                [content.bottomAnchor constraintEqualToAnchor:shell.bottomAnchor constant:-10]]];
            shell.isAccessibilityElement = YES;
            BOOL canOpen = ![card[@"redacted"] boolValue] && card[@"request"];
            shell.accessibilityLabel = [NSString stringWithFormat:@"%@，%@。%@ %@ %@。", group.name, relative, card[@"title"], card[@"subtitle"], card[@"body"]];
            shell.accessibilityTraits = UIAccessibilityTraitButton;
            shell.accessibilityHint = canOpen ? @"点按打开原生通知动作；若应用提供目标将进入对应会话。" : @"通知内容受保护，当前不可打开。";
            __weak IDCoordinator *weakCoordinator = self.coordinator;
            __weak IDCardView *weakShell = shell;
            NSDictionary *cardModel = card;
            shell.openAction = ^{
                // First operation is visible feedback; native action/auth is secondary.
                IDOpenFeedback(@"打开诊断：点击已收到；正在检查原生动作");
                BOOL opened = canOpen && [weakCoordinator openCardRequest:cardModel[@"request"]];
                // Native opening can synchronously dismiss/scrub this shell. Do not
                // repopulate AX after that privacy boundary (or on a replaced shell).
                if (!weakShell.openAction) return;
                if (weakCoordinator.panel.isViewLoaded)
                    ((IDDigestView *)weakCoordinator.panel.view).cardContentDirty = YES;
                if (!opened) {
                    weakShell.accessibilityValue = @"打开通知不可用";
                    UIAccessibilityPostNotification(UIAccessibilityAnnouncementNotification, @"打开通知不可用");
                } else {
                    weakShell.accessibilityValue = @"已请求打开通知";
                }
            };
            UIAccessibilityCustomAction *openAction = [[UIAccessibilityCustomAction alloc] initWithName:@"打开通知" actionHandler:^BOOL(UIAccessibilityCustomAction *action) {
                return [weakShell accessibilityActivate];
            }];
            shell.accessibilityCustomActions = @[openAction];
            objc_setAssociatedObject(shell, &IDStableCardModelKey, card, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            objc_setAssociatedObject(shell, &IDStableCardRevisionKey, @(IDOptionsRevision), OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            [desiredViews addObject:shell]; rendered++;
        }
    }
    if (!rendered) {
        NSString *empty = state.enumerationCursor ? @"通知读取中；原生列表保留" :
            state.enumerationReliable ? @"当前范围暂无通知" : @"通知读取未完成；请使用原生列表";
        [desiredViews addObject:[self cardLabel:empty style:UIFontTextStyleBody color:UIColor.secondaryLabelColor]];
    }
    if (state.selectedRequestCount > IDContainedLimit) {
        NSUInteger pages = IDContainedPageCount(state.selectedRequestCount);
        UILabel *page = [self cardLabel:[NSString stringWithFormat:@"第 %lu / %lu 页 · 当前范围共 %lu 条；较早通知可翻页访问",
            (unsigned long)(state.cardPage + 1), (unsigned long)pages, (unsigned long)state.selectedRequestCount]
            style:UIFontTextStyleFootnote color:UIColor.secondaryLabelColor];
        [desiredViews addObject:page];
        self.previousPageButton = [UIButton buttonWithType:UIButtonTypeSystem];
        self.nextPageButton = [UIButton buttonWithType:UIButtonTypeSystem];
        [self.previousPageButton setTitle:@"上一页（较新）" forState:UIControlStateNormal];
        [self.nextPageButton setTitle:@"下一页（较早）" forState:UIControlStateNormal];
        self.previousPageButton.enabled = state.cardPage > 0;
        self.nextPageButton.enabled = state.cardPage + 1 < pages;
        [self.previousPageButton addTarget:state action:@selector(previousCardPage) forControlEvents:UIControlEventTouchUpInside];
        [self.nextPageButton addTarget:state action:@selector(nextCardPage) forControlEvents:UIControlEventTouchUpInside];
        for (UIButton *button in @[self.previousPageButton, self.nextPageButton]) {
            button.translatesAutoresizingMaskIntoConstraints = NO;
            [button.heightAnchor constraintGreaterThanOrEqualToConstant:44].active = YES;
            [desiredViews addObject:button];
        }
    }
    // Remove only deleted/changed shells; unchanged cards stay mounted and keep
    // their action identity. Scrub discarded content before releasing it.
    NSSet *desiredSet = [NSSet setWithArray:desiredViews];
    for (UIView *view in previousViews) if (![desiredSet containsObject:view]) {
        IDContainedClearView(view);
        objc_setAssociatedObject(view, &IDStableCardModelKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        if ([view isKindOfClass:IDCardView.class]) ((IDCardView *)view).openAction = nil;
        view.accessibilityCustomActions = nil;
        [self.cardStack removeArrangedSubview:view]; [view removeFromSuperview];
    }
    [desiredViews enumerateObjectsUsingBlock:^(UIView *view, NSUInteger index, BOOL *stop) {
        NSArray *current = self.cardStack.arrangedSubviews;
        if (index < current.count && current[index] == view) return;
        if ([current containsObject:view]) [self.cardStack removeArrangedSubview:view];
        [self.cardStack insertArrangedSubview:view atIndex:index];
    }];
    self.builtCardCount = rendered;
    [self.cardStack setNeedsLayout];
    [self.cardStack layoutIfNeeded];
    [self invalidateIntrinsicContentSize];
    [self setNeedsLayout];
    [self layoutIfNeeded];
    CGFloat maximum = MAX(0, self.cardScroll.contentSize.height - self.cardScroll.bounds.size.height);
    BOOL hasAnchor = scrollAnchor && [desiredSet containsObject:scrollAnchor];
    CGFloat anchorY = hasAnchor ? CGRectGetMinY([scrollAnchor convertRect:scrollAnchor.bounds toView:self.cardScroll]) : 0;
    self.cardScroll.contentOffset = CGPointMake(0, IDStableScrollOffset(previousOffset.y, hasAnchor, anchorY, anchorViewportY, maximum));
    // One coalesced next-run-loop verdict after descendant Auto Layout. The existing
    // presentation generation cancels stale callbacks on teardown; never enumerate.
    [self.coordinator scheduleNativeListVisibility];
}
- (BOOL)cardsReadyForNativeHide {
    if (IDPanelCompact(self.coordinator.collapseState)) return NO;
    return [self expandedCardsLayoutReady];
}
- (BOOL)cardReadinessFailure:(NSString *)reason index:(NSInteger)index view:(UIView *)view {
    self.cardReadinessDiagnostic = @{ @"reason": reason, @"index": @(index),
        @"type": view ? NSStringFromClass(view.class) : @"none",
        @"width": @(view.bounds.size.width), @"height": @(view.bounds.size.height) };
    return NO;
}
- (BOOL)semanticLabelsInView:(UIView *)view depth:(NSUInteger)depth visible:(BOOL *)visible {
    if (!view || depth > 4 || view.hidden || view.alpha <= 0.01) return NO;
    BOOL readable = NO;
    if ([view isKindOfClass:UILabel.class]) {
        UILabel *label = (UILabel *)view;
        readable = IDCardSemanticUsable(label.text.length > 0 || label.attributedText.length > 0, view.hidden,
            view.alpha, view.bounds.size.width, view.bounds.size.height);
        if (readable) {
            CGRect frame = [view convertRect:view.bounds toView:self.cardScroll];
            CGRect intersection = CGRectIntersection(frame, self.cardScroll.bounds);
            if (!CGRectIsNull(intersection) && intersection.size.width > 0 && intersection.size.height > 0) *visible = YES;
        }
    }
    for (UIView *child in view.subviews) {
        // Metadata is semantic too: app/date live inside a nested header stack.
        // Do not short circuit: we must find a visible label, not just any label.
        if ([self semanticLabelsInView:child depth:depth + 1 visible:visible]) readable = YES;
    }
    return readable;
}
- (BOOL)expandedCardsLayoutReady {
    // Count exactly the selected rendered range, not all native requests/history.
    NSUInteger expected = 0;
    for (NSDictionary *card in self.coordinator.cards) {
        if (!self.coordinator.selected || [card[@"app"] isEqual:self.coordinator.selected]) expected++;
    }
    if (!self.window || self.window.hidden || self.hidden || self.alpha <= 0.01 || self.cardScroll.hidden
        || (!IDPanelCompact(self.coordinator.collapseState)
            && (self.expandedContent.hidden || self.expandedContent.alpha <= 0.01))
        || self.cardScroll.window != self.window)
        return [self cardReadinessFailure:@"viewport-uninstalled" index:-1 view:self.cardScroll];
    if (!expected || self.builtCardCount != expected)
        return [self cardReadinessFailure:@"rendered-count-mismatch" index:-1 view:self.cardStack];
    if (!IDCardGeometryUsable(self.cardScroll.bounds.size.width, self.cardScroll.bounds.size.height)
        || !IDCardGeometryUsable(self.cardStack.bounds.size.width, self.cardStack.bounds.size.height))
        return [self cardReadinessFailure:@"viewport-or-stack-geometry" index:-1 view:self.cardScroll];
    if (!self.cardScroll.scrollEnabled || !self.cardScroll.panGestureRecognizer.enabled)
        return [self cardReadinessFailure:@"scroll-disabled" index:-1 view:self.cardScroll];
    if (!IDCardCoverageUsable(self.cardScroll.contentSize.height, CGRectGetMaxY(self.cardStack.frame)))
        return [self cardReadinessFailure:@"content-coverage" index:-1 view:self.cardStack];
    NSUInteger valid = 0; BOOL visible = NO;
    for (UIView *shell in self.cardStack.arrangedSubviews) {
        if (![shell isKindOfClass:IDCardView.class]) continue;
        if (shell.hidden || shell.alpha <= 0.01
            || !IDCardGeometryUsable(shell.bounds.size.width, shell.bounds.size.height)
            || !shell.accessibilityLabel.length)
            return [self cardReadinessFailure:@"card-shell-geometry" index:valid view:shell];
        valid++;
        CGRect frame = [shell convertRect:shell.bounds toView:self.cardScroll];
        CGRect intersection = CGRectIntersection(frame, self.cardScroll.bounds);
        // Offscreen descendants may not have completed UIKit's deferred layout.
        // Validate every shell/model, but require readable viewport text in only
        // one actually visible card, including nested app/date or privacy text.
        if (!CGRectIsNull(intersection)
            && IDCardVisibleAreaUsable(intersection.size.width, intersection.size.height)) {
            BOOL labelVisible = NO;
            [self semanticLabelsInView:shell depth:0 visible:&labelVisible];
            if (labelVisible) visible = YES;
        }
    }
    if (!IDCardsRepresentationUsable(expected, self.builtCardCount, valid, visible))
        return [self cardReadinessFailure:valid != self.builtCardCount ? @"card-shell-count" : @"no-visible-semantic-card" index:-1 view:self.cardStack];
    self.cardReadinessDiagnostic = @{ @"reason": @"ready", @"index": @(-1), @"type": @"none", @"width": @0, @"height": @0 };
    return YES;
}
