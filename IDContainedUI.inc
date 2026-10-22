- (void)dropCards {
    // A snapshot rebuild must not retarget a finger already down to a new request.
    self.cardTap.enabled = NO; self.cardTap.enabled = YES;
    self.builtCardCount = 0;
    self.retainedLayoutReady = NO;
    // Scrub synchronously, but retain empty layout shells until the next visible render.
    // Removing hundreds of active constraints during an interactive dismissal is avoidable.
    for (UIView *view in self.cardStack.arrangedSubviews) {
        IDContainedClearView(view);
        if ([view isKindOfClass:IDCardView.class]) ((IDCardView *)view).openAction = nil;
        view.accessibilityCustomActions = nil;
    }
    self.cardScroll.hidden = YES; // Hide one viewport, not 160 arranged views/UISV-hiding constraints.
    self.cardNotice.text = nil;
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
- (void)showCards {
    CGPoint previousOffset = self.cardScroll.contentOffset;
    [self dropCards];
    for (UIView *view in self.cardStack.arrangedSubviews.copy) {
        [self.cardStack removeArrangedSubview:view]; [view removeFromSuperview];
    }
    self.cardScroll.hidden = !IDContainedMode; self.cardNotice.hidden = !IDContainedMode;
    if (!IDContainedMode) return;
    IDCoordinator *state = self.coordinator;
    NSUInteger rendered = 0;
    for (IDGroup *group in state.groups) {
        if (state.selected && ![state.selected isEqualToString:group.identifier]) continue;
        NSMutableArray *cards = [NSMutableArray new];
        for (NSDictionary *card in state.cards) if ([card[@"app"] isEqualToString:group.identifier]) [cards addObject:card];
        if (!cards.count) continue;
        UILabel *heading = [self cardLabel:group.name style:UIFontTextStyleHeadline color:UIColor.labelColor];
        heading.accessibilityTraits = UIAccessibilityTraitHeader;
        [self.cardStack addArrangedSubview:heading];
        for (NSDictionary *card in cards) {
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
            [self.cardStack addArrangedSubview:shell]; rendered++;
        }
    }
    if (!rendered) {
        NSString *empty = state.enumerationReliable ? @"当前范围暂无通知" : @"系统通知枚举接口不可用；请使用原生列表";
        [self.cardStack addArrangedSubview:[self cardLabel:empty style:UIFontTextStyleBody color:UIColor.secondaryLabelColor]];
    }
    if (state.cards.count >= IDContainedLimit) [self.cardStack addArrangedSubview:[self cardLabel:@"仅展示当前范围最近 160 条；较早通知仍在原生列表。" style:UIFontTextStyleFootnote color:UIColor.secondaryLabelColor]];
    NSUInteger redacted = 0;
    for (NSDictionary *card in state.cards) if ([card[@"redacted"] boolValue]) redacted++;
    NSString *policyNote = redacted ? @"系统策略未确认或设备未满足条件，部分内容已隐藏" : @"仅在解锁且受保护数据可用时预览；无法核验每 App 的 Never 映射";
    self.cardNotice.text = [NSString stringWithFormat:@"%@\n%@", policyNote, state.nativeHideReason ?: @"原生列表保留"];
    self.builtCardCount = rendered;
    [self.cardStack setNeedsLayout];
    [self.cardStack layoutIfNeeded];
    [self invalidateIntrinsicContentSize];
    [self setNeedsLayout];
    [self layoutIfNeeded];
    CGFloat maximum = MAX(0, self.cardScroll.contentSize.height - self.cardScroll.bounds.size.height);
    self.cardScroll.contentOffset = CGPointMake(0, MIN(MAX(0, previousOffset.y), maximum));
}
- (BOOL)cardsReadyForNativeHide {
    if (IDPanelCompact(self.coordinator.collapseState)) return NO;
    return [self expandedCardsLayoutReady];
}
- (BOOL)expandedCardsLayoutReady {
    // Retained expanded coordinates are independently verified, never called visible capsule cards.
    CGFloat requiredHeight = CGRectGetMaxY(self.cardStack.frame) + 12;
    if (!self.cardScroll.scrollEnabled || !self.cardScroll.panGestureRecognizer.enabled
        || !isfinite(self.cardScroll.contentSize.height)
        || self.cardScroll.contentSize.height + 1 < requiredHeight) return NO;
    if (!self.window || self.hidden || self.cardScroll.hidden || !self.builtCardCount
        || self.builtCardCount != self.coordinator.cards.count
        || self.cardScroll.bounds.size.height < 44 || self.cardScroll.bounds.size.width < 44
        || self.cardStack.bounds.size.width < 44 || self.cardStack.bounds.size.height < 44) return NO;
    NSUInteger valid = 0; BOOL visible = NO;
    for (UIView *shell in self.cardStack.arrangedSubviews) {
        if (![shell.accessibilityIdentifier isEqualToString:@"IDContainedCard"]) continue;
        if (shell.hidden || shell.bounds.size.width < 44 || shell.bounds.size.height < 44
            || !shell.accessibilityLabel.length) return NO;
        BOOL readable = NO;
        for (UIView *content in shell.subviews) {
            if (![content isKindOfClass:UIStackView.class]) continue;
            for (UIView *line in ((UIStackView *)content).arrangedSubviews) {
                if ([line isKindOfClass:UILabel.class] && ((UILabel *)line).text.length
                    && !line.hidden && line.bounds.size.width > 0 && line.bounds.size.height > 0) readable = YES;
            }
        }
        if (!readable) return NO;
        valid++;
        CGRect frame = [shell convertRect:shell.bounds toView:self.cardScroll];
        CGRect intersection = CGRectIntersection(frame, self.cardScroll.bounds);
        if (!CGRectIsNull(intersection) && intersection.size.width >= 44 && intersection.size.height >= 22) visible = YES;
    }
    return valid == self.builtCardCount && visible;
}
