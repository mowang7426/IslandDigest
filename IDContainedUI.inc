- (void)dropCards {
    self.builtCardCount = 0;
    // Clear accessible strings as well as removing labels; no stale preview in AX.
    for (UIView *view in self.cardStack.arrangedSubviews.copy) {
        IDContainedClearView(view);
        [self.cardStack removeArrangedSubview:view]; [view removeFromSuperview];
    }
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
    [self dropCards];
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
            UIView *shell = [UIView new]; shell.layer.cornerRadius = 16;
            shell.translatesAutoresizingMaskIntoConstraints = NO;
            shell.accessibilityIdentifier = @"IDContainedCard";
            [shell.heightAnchor constraintGreaterThanOrEqualToConstant:64].active = YES;
            shell.backgroundColor = [UIColor.secondaryLabelColor colorWithAlphaComponent:0.08];
            UIStackView *content = [UIStackView new]; content.axis = UILayoutConstraintAxisVertical; content.spacing = 5;
            content.translatesAutoresizingMaskIntoConstraints = NO; [shell addSubview:content];
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
                UILabel *line = [self cardLabel:text style:[key isEqualToString:@"title"] ? UIFontTextStyleHeadline : UIFontTextStyleBody
                    color:[card[@"redacted"] boolValue] ? UIColor.secondaryLabelColor : UIColor.labelColor];
                [content addArrangedSubview:line];
            }
            [NSLayoutConstraint activateConstraints:@[
                [content.leadingAnchor constraintEqualToAnchor:shell.leadingAnchor constant:12],
                [content.trailingAnchor constraintEqualToAnchor:shell.trailingAnchor constant:-12],
                [content.topAnchor constraintEqualToAnchor:shell.topAnchor constant:10],
                [content.bottomAnchor constraintEqualToAnchor:shell.bottomAnchor constant:-10]]];
            shell.isAccessibilityElement = YES;
            shell.accessibilityLabel = [NSString stringWithFormat:@"%@，%@。%@ %@ %@。仅预览，不支持点按打开或快捷回复。", group.name, relative, card[@"title"], card[@"subtitle"], card[@"body"]];
            shell.accessibilityTraits = UIAccessibilityTraitStaticText;
            [self.cardStack addArrangedSubview:shell]; rendered++;
        }
    }
    if (!rendered) {
        NSString *empty = state.enumerationReliable ? @"当前范围暂无通知" : @"系统通知枚举接口不可用；请使用原生列表";
        [self.cardStack addArrangedSubview:[self cardLabel:empty style:UIFontTextStyleBody color:UIColor.secondaryLabelColor]];
    }
    if (state.cards.count >= IDContainedLimit) [self.cardStack addArrangedSubview:[self cardLabel:@"仅展示当前范围最近 160 条；较早通知仍在原生列表。" style:UIFontTextStyleFootnote color:UIColor.secondaryLabelColor]];
    self.cardNotice.text = [NSString stringWithFormat:@"仅预览 · 未验证逐条预览策略，内容已隐藏\n%@", state.nativeHideReason ?: @"原生列表保留"];
    self.builtCardCount = rendered;
    [self.cardStack setNeedsLayout];
    [self.cardStack layoutIfNeeded];
    [self invalidateIntrinsicContentSize];
    [self setNeedsLayout];
    [self layoutIfNeeded];
    CGFloat maximum = MAX(0, self.cardScroll.contentSize.height - self.cardScroll.bounds.size.height);
    self.cardScroll.contentOffset = CGPointMake(0, MIN(MAX(0, self.cardScroll.contentOffset.y), maximum));
}
- (BOOL)cardsReadyForNativeHide {
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
