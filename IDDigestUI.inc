@implementation IDDigestView
- (instancetype)initWithFrame:(CGRect)frame {
    self = [super initWithFrame:frame];
    if (!self) return nil;
    self.layer.cornerRadius = 24; self.layer.cornerCurve = kCACornerCurveContinuous; self.clipsToBounds = YES;
    self.backgroundColor = UIColor.clearColor;
    self.glass = [[UIVisualEffectView alloc] initWithEffect:nil];
    self.glass.userInteractionEnabled = NO; [self addSubview:self.glass];
    self.heading = [UILabel new]; self.heading.text = @"按应用分组"; self.heading.textColor = UIColor.labelColor; [self addSubview:self.heading];
    self.detail = [UILabel new]; self.detail.textColor = UIColor.secondaryLabelColor; self.detail.textAlignment = NSTextAlignmentRight;
    self.detail.adjustsFontSizeToFitWidth = YES; [self addSubview:self.detail];
    self.hint = [UILabel new]; self.hint.text = @"长按应用置顶 / 取消 · 点按筛选"; self.hint.textColor = UIColor.secondaryLabelColor;
    self.hint.adjustsFontSizeToFitWidth = YES; [self addSubview:self.hint];
    self.scroll = [UIScrollView new]; self.scroll.contentInsetAdjustmentBehavior = UIScrollViewContentInsetAdjustmentNever;
    [self addSubview:self.scroll];
    self.stack = [UIView new]; [self.scroll addSubview:self.stack];
    self.cardScroll = [UIScrollView new]; self.cardScroll.alwaysBounceVertical = YES;
    self.cardScroll.userInteractionEnabled = YES; self.cardScroll.delaysContentTouches = YES;
    self.cardScroll.scrollEnabled = YES; self.cardScroll.canCancelContentTouches = YES;
    self.cardScroll.clipsToBounds = YES; self.cardScroll.showsVerticalScrollIndicator = YES;
    self.cardScroll.backgroundColor = UIColor.clearColor;
    self.cardTap = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(cardTapped:)];
    self.cardTap.cancelsTouchesInView = NO; self.cardTap.delegate = self;
    [self.cardTap requireGestureRecognizerToFail:self.cardScroll.panGestureRecognizer];
    [self.cardScroll addGestureRecognizer:self.cardTap];
    // This is already an explicitly bounded sub-region, not a system full-screen list.
    self.cardScroll.contentInsetAdjustmentBehavior = UIScrollViewContentInsetAdjustmentNever;
    self.cardScroll.contentInset = UIEdgeInsetsZero;
    self.cardScroll.accessibilityLabel = @"插件内通知元数据卡片"; [self addSubview:self.cardScroll];
    self.cardStack = [UIStackView new]; self.cardStack.axis = UILayoutConstraintAxisVertical;
    self.cardStack.spacing = 10; self.cardStack.translatesAutoresizingMaskIntoConstraints = NO;
    [self.cardScroll addSubview:self.cardStack];
    [NSLayoutConstraint activateConstraints:@[
        [self.cardStack.leadingAnchor constraintEqualToAnchor:self.cardScroll.contentLayoutGuide.leadingAnchor constant:14],
        [self.cardStack.trailingAnchor constraintEqualToAnchor:self.cardScroll.contentLayoutGuide.trailingAnchor constant:-14],
        [self.cardStack.topAnchor constraintEqualToAnchor:self.cardScroll.contentLayoutGuide.topAnchor constant:4],
        [self.cardStack.bottomAnchor constraintEqualToAnchor:self.cardScroll.contentLayoutGuide.bottomAnchor constant:-12],
        [self.cardStack.widthAnchor constraintEqualToAnchor:self.cardScroll.frameLayoutGuide.widthAnchor constant:-28]]];
    self.cardNotice = [UILabel new]; self.cardNotice.numberOfLines = 2;
    self.cardNotice.font = [UIFont systemFontOfSize:10]; self.cardNotice.textColor = UIColor.secondaryLabelColor;
    self.cardNotice.adjustsFontSizeToFitWidth = YES; [self addSubview:self.cardNotice];
    self.clearButton = [UIButton buttonWithType:UIButtonTypeSystem];
    [self.clearButton setImage:[UIImage systemImageNamed:@"trash"] forState:UIControlStateNormal];
    self.clearButton.tintColor = UIColor.systemRedColor; self.clearButton.enabled = NO;
    self.clearButton.accessibilityLabel = @"清理当前范围通知";
    self.clearButton.accessibilityIdentifier = @"IslandDigestClear";
    [self.clearButton addTarget:self action:@selector(clearTapped) forControlEvents:UIControlEventTouchUpInside];
    [self addSubview:self.clearButton];
    // Keep expanded views and scroll/model identity; only the outer hit bounds fold.
    self.expandedContent = [[UIView alloc] initWithFrame:self.bounds];
    for (UIView *view in self.subviews.copy) {
        if (view != self.glass) [self.expandedContent addSubview:view];
    }
    [self addSubview:self.expandedContent];
    self.collapseButton = [UIButton buttonWithType:UIButtonTypeSystem];
    [self.collapseButton setImage:[UIImage systemImageNamed:@"chevron.up"] forState:UIControlStateNormal];
    self.collapseButton.accessibilityLabel = @"折叠通知面板";
    self.collapseButton.accessibilityIdentifier = @"IslandDigestCollapse";
    [self.collapseButton addTarget:self action:@selector(collapseTapped) forControlEvents:UIControlEventTouchUpInside];
    [self.expandedContent addSubview:self.collapseButton];
    self.capsuleButton = [UIButton buttonWithType:UIButtonTypeSystem];
    [self.capsuleButton setImage:[UIImage systemImageNamed:@"bell.badge.fill"] forState:UIControlStateNormal];
    self.capsuleButton.titleLabel.font = [UIFont monospacedDigitSystemFontOfSize:17 weight:UIFontWeightSemibold];
    self.capsuleButton.titleLabel.adjustsFontSizeToFitWidth = YES;
    self.capsuleButton.backgroundColor = UIColor.secondarySystemBackgroundColor;
    self.capsuleButton.layer.cornerRadius = 24;
    self.capsuleButton.accessibilityIdentifier = @"IslandDigestCapsule";
    self.capsuleButton.accessibilityHint = @"点按恢复完整面板；不会打开或删除通知";
    [self.capsuleButton addTarget:self action:@selector(collapseTapped) forControlEvents:UIControlEventTouchUpInside];
    [self addSubview:self.capsuleButton];
    [self updateCollapsePresentation];
    self.accessibilityIdentifier = @"IslandDigest";
    return self;
}
- (void)collapseTapped { [self.coordinator toggleCollapsed]; }
- (BOOL)capsuleReachable {
    CGRect visible = self.window ? [self convertRect:self.window.bounds fromView:self.window] : CGRectZero;
    CGRect intersection = CGRectIntersection(self.bounds, visible);
    return IDPanelCapsuleReachable(self.window && !self.hidden && !CGRectIsNull(intersection),
        self.userInteractionEnabled && self.capsuleButton.enabled && !self.capsuleButton.hidden,
        intersection.size.width, intersection.size.height);
}
- (void)updateCollapsePresentation {
    BOOL compact = IDPanelCompact(self.coordinator.collapseState);
    BOOL moving = IDPanelTransitioning(self.coordinator.collapseState);
    self.expandedContent.alpha = compact ? 0 : 1;
    self.expandedContent.userInteractionEnabled = !compact && !moving;
    self.expandedContent.accessibilityElementsHidden = compact || moving;
    self.capsuleButton.alpha = compact ? 1 : 0;
    self.capsuleButton.hidden = !compact && !moving;
    self.capsuleButton.enabled = YES; // In-flight taps are ignored by the state machine.
    self.capsuleButton.accessibilityElementsHidden = !compact || moving;
    self.collapseButton.enabled = !moving;
}
- (void)clearTapped { [self.coordinator beginClear]; }
- (BOOL)gestureRecognizer:(UIGestureRecognizer *)recognizer shouldReceiveTouch:(UITouch *)touch {
    if (recognizer != self.cardTap) return YES;
    self.cardTapStartOffset = self.cardScroll.contentOffset.y;
    return IDScrollTapAllowed(YES, self.cardScroll.dragging, self.cardScroll.decelerating,
        self.cardTapStartOffset, self.cardTapStartOffset,
        self.cardScroll.contentSize.height, self.cardScroll.bounds.size.height);
}
- (void)cardTapped:(UITapGestureRecognizer *)recognizer {
    if (self.coordinator.collapseState != IDPanelExpanded) return;
    if (!IDScrollTapAllowed(recognizer.state == UIGestureRecognizerStateEnded,
        self.cardScroll.dragging, self.cardScroll.decelerating, self.cardTapStartOffset,
        self.cardScroll.contentOffset.y, self.cardScroll.contentSize.height,
        self.cardScroll.bounds.size.height)) return;
    CGPoint point = [recognizer locationInView:self.cardScroll];
    if (!CGRectContainsPoint(self.cardScroll.bounds, point)) return;
    // Convert at recognition time, not before scrolling: retain the original request
    // on each actual card, never a stale clipped overlay or an index into a new list.
    for (UIView *view in self.cardStack.arrangedSubviews) {
        if (![view isKindOfClass:IDCardView.class] || view.hidden) continue;
        if (CGRectContainsPoint([view convertRect:view.bounds toView:self.cardScroll], point)) {
            [(IDCardView *)view accessibilityActivate]; return;
        }
    }
}
- (CGFloat)rowHeight {
    CGFloat base = IDMiniControls ? 44 : 52;
    return MAX(44, MIN(80, [[UIFontMetrics metricsForTextStyle:UIFontTextStyleBody] scaledValueForValue:base]));
}
- (NSUInteger)categoryColumnsForWidth:(CGFloat)width {
    CGFloat scale = [[UIFontMetrics metricsForTextStyle:UIFontTextStyleCaption1] scaledValueForValue:12] / 12.0;
    return IDExperienceGridColumns(width, IDGridLayout, (int)IDGridColumns, scale);
}
- (CGFloat)categoryHeightForWidth:(CGFloat)width {
    CGFloat row = [self rowHeight];
    NSUInteger columns = [self categoryColumnsForWidth:width];
    NSUInteger rows = IDGridLayout ? IDExperienceRows(self.stack.subviews.count, columns) : 1;
    CGFloat ownerHeight = self.coordinator.owner.view.bounds.size.height;
    if (ownerHeight <= 0) ownerHeight = UIScreen.mainScreen.bounds.size.height;
    NSUInteger visible = IDGridLayout ? IDExperienceVisibleRows(rows, ownerHeight, row) : 1;
    return 66 + visible * row + (visible - 1) * 8;
}
- (CGSize)sizeThatFits:(CGSize)size {
    CGFloat category = [self categoryHeightForWidth:size.width];
    if (!IDContainedMode) return CGSizeMake(size.width, category);
    CGFloat contentWidth = MAX(1, size.width - 28);
    CGSize measured = [self.cardStack systemLayoutSizeFittingSize:CGSizeMake(contentWidth, UILayoutFittingCompressedSize.height)
        withHorizontalFittingPriority:UILayoutPriorityRequired verticalFittingPriority:UILayoutPriorityFittingSizeLevel];
    CGFloat content = MAX(44, measured.height + 16);
    return CGSizeMake(size.width, MIN(category + 38 + content, IDPanelMaxHeight));
}
- (CGSize)intrinsicContentSize { return CGSizeMake(UIViewNoIntrinsicMetric, [self sizeThatFits:self.bounds.size].height); }
- (void)layoutSubviews {
    [super layoutSubviews];
    self.capsuleButton.frame = self.bounds;
    if (IDPanelTransitioning(self.coordinator.collapseState)) return;
    CGSize contentSize = self.expandedSize.width > 0 ? self.expandedSize : self.bounds.size;
    self.expandedContent.frame = (CGRect){CGPointZero, contentSize};
    CGFloat width = contentSize.width, row = [self rowHeight];
    self.backgroundColor = UIColor.clearColor;
    self.glass.frame = self.bounds;
    self.glass.effect = IDPanelBlur ? [UIBlurEffect effectWithStyle:UIBlurEffectStyleSystemUltraThinMaterial] : nil;
    self.glass.hidden = !IDPanelBlur && IDPanelOpacity == 0;
    self.glass.backgroundColor = [UIColor.systemBackgroundColor colorWithAlphaComponent:IDPanelOpacity];
    self.heading.font = [[UIFontMetrics metricsForTextStyle:UIFontTextStyleHeadline] scaledFontForFont:[UIFont systemFontOfSize:16 weight:UIFontWeightSemibold] maximumPointSize:22];
    self.detail.font = [UIFont systemFontOfSize:11 weight:UIFontWeightMedium];
    self.hint.font = [UIFont systemFontOfSize:10 weight:UIFontWeightMedium];
    CGFloat contentWidth = MAX(0, width - 106);
    self.heading.frame = CGRectMake(14, 8, MAX(0, contentWidth * 0.52 - 14), 26);
    self.heading.adjustsFontSizeToFitWidth = YES;
    self.detail.frame = CGRectMake(contentWidth * 0.52, 12, MAX(0, contentWidth * 0.48), 20);
    self.collapseButton.frame = CGRectMake(MAX(0, width - 96), 0, 44, 44);
    self.clearButton.frame = CGRectMake(MAX(0, width - 48), 0, 44, 44);
    CGFloat category = [self categoryHeightForWidth:width];
    if (IDContainedMode) category = MIN(category, MAX(66, contentSize.height - 38 - 88));
    self.scroll.frame = CGRectMake(0, 44, width, MAX(0, category - 66));
    self.hint.frame = CGRectMake(14, category - 19, MAX(0, width - 28), 16);
    self.cardScroll.frame = CGRectMake(0, category, width, MAX(0, contentSize.height - category - 38));
    self.cardNotice.frame = CGRectMake(14, MAX(category, contentSize.height - 36), MAX(0, width - 28), 32);
    self.scroll.showsHorizontalScrollIndicator = !IDGridLayout;
    self.scroll.showsVerticalScrollIndicator = IDGridLayout;
    self.scroll.alwaysBounceHorizontal = !IDGridLayout;
    self.scroll.alwaysBounceVertical = IDGridLayout;
    NSUInteger columns = [self categoryColumnsForWidth:width], index = 0;
    CGFloat x = 14;
    CGFloat padding = IDGridLayout ? 8 : 14, gap = IDGridLayout ? 6 : 8;
    CGFloat cellWidth = MAX(1, (width - 2 * padding - (columns - 1) * gap) / columns);
    for (UIView *button in self.stack.subviews) {
        if (IDGridLayout) {
            button.frame = CGRectMake(padding + (index % columns) * (cellWidth + gap), (index / columns) * (row + 8), cellWidth, row);
        } else {
            CGSize fitting = [button systemLayoutSizeFittingSize:UILayoutFittingCompressedSize];
            CGFloat w = MAX(96, MIN(MAX(96, width - 28), fitting.width));
            button.frame = CGRectMake(x, 0, w, row); x += w + 8;
        }
        index++;
    }
    CGFloat height = IDGridLayout ? IDExperienceRows(index, columns) * (row + 8) - 8 : row;
    CGFloat scrollWidth = IDGridLayout ? width : MAX(width, x + 6);
    self.stack.frame = CGRectMake(0, 0, scrollWidth, height);
    self.scroll.contentSize = CGSizeMake(scrollWidth, height);
    // Content guides derive contentSize from the multiline card constraints.
    // Resolve after assigning the bounded viewport frame, including width changes.
    [self.cardScroll setNeedsLayout]; [self.cardScroll layoutIfNeeded];
    [self.cardStack layoutIfNeeded];
    self.retainedLayoutReady = [self expandedCardsLayoutReady];
    [self.coordinator updatePanelGeometry];
}
- (void)pinGesture:(UILongPressGestureRecognizer *)recognizer {
    if (recognizer.state != UIGestureRecognizerStateBegan) return;
    UIButton *button = (UIButton *)recognizer.view;
    [button cancelTrackingWithEvent:nil]; // recognizer cancels touches: no filter action on release.
    [self.coordinator togglePinnedIdentifier:button.accessibilityIdentifier];
}
- (void)showGroups:(NSArray<IDGroup *> *)groups selected:(NSString *)selected {
    CGPoint offset = self.scroll.contentOffset;
    for (UIView *view in self.stack.subviews.copy) [view removeFromSuperview];
    NSUInteger total = 0; for (IDGroup *group in groups) total += group.count;
    [self.capsuleButton setTitle:[NSString stringWithFormat:@"  %lu", (unsigned long)total] forState:UIControlStateNormal];
    self.capsuleButton.accessibilityLabel = [NSString stringWithFormat:@"通知面板已折叠，共 %lu 条通知，点按展开", (unsigned long)total];
    self.detail.text = IDShowCounts ? [NSString stringWithFormat:@"%lu 个应用 · %lu 条", (unsigned long)groups.count, (unsigned long)total] : @"保留原生通知操作";
    NSMutableArray *entries = [NSMutableArray arrayWithObject:NSNull.null];
    [entries addObjectsFromArray:groups.count > 128 ? [groups subarrayWithRange:NSMakeRange(0, 128)] : groups];
    self.hint.text = groups.count > 128 ? @"仅列前 128 应用 · 全部仍显示所有通知" : @"长按应用置顶 / 取消 · 点按筛选";
    NSSet *pins = [NSSet setWithArray:IDPinnedIdentifiers()];
    for (id entry in entries) {
        IDGroup *group = entry == NSNull.null ? nil : entry;
        BOOL active = group ? [selected isEqualToString:group.identifier] : !selected;
        BOOL pinned = group && [pins containsObject:group.identifier];
        UIButton *button = [UIButton buttonWithType:UIButtonTypeCustom];
        button.layer.cornerRadius = IDMiniControls ? 13 : 17; button.layer.cornerCurve = kCACornerCurveContinuous; button.clipsToBounds = YES;
        button.backgroundColor = active ? [UIColor.labelColor colorWithAlphaComponent:0.9] : [UIColor.secondaryLabelColor colorWithAlphaComponent:0.09];
        UIColor *foreground = active ? UIColor.systemBackgroundColor : UIColor.labelColor;
        UIStackView *content = [UIStackView new]; content.axis = UILayoutConstraintAxisHorizontal;
        content.alignment = UIStackViewAlignmentCenter; content.spacing = IDGridLayout ? 2 : (IDMiniControls ? 4 : 6); content.userInteractionEnabled = NO;
        content.translatesAutoresizingMaskIntoConstraints = NO; [button addSubview:content];
        UIImageView *icon = [[UIImageView alloc] initWithImage:group.icon ?: [UIImage systemImageNamed:group ? @"app.fill" : @"square.stack.3d.up.fill"]];
        icon.tintColor = foreground; icon.contentMode = UIViewContentModeScaleAspectFit; icon.layer.cornerRadius = 7; icon.clipsToBounds = YES;
        [content addArrangedSubview:icon]; [icon.widthAnchor constraintEqualToConstant:IDGridLayout ? 18 : (IDMiniControls ? 20 : 24)].active = YES; [icon.heightAnchor constraintEqualToConstant:IDGridLayout ? 18 : (IDMiniControls ? 20 : 24)].active = YES;
        UILabel *name = [UILabel new]; name.text = group ? group.name : @"全部"; name.textColor = foreground;
        name.font = [[UIFontMetrics metricsForTextStyle:UIFontTextStyleCaption1] scaledFontForFont:[UIFont systemFontOfSize:12 weight:UIFontWeightSemibold] maximumPointSize:22];
        name.adjustsFontForContentSizeCategory = YES;
        name.lineBreakMode = NSLineBreakByTruncatingTail;
        [name setContentCompressionResistancePriority:UILayoutPriorityDefaultLow forAxis:UILayoutConstraintAxisHorizontal];
        [name.widthAnchor constraintGreaterThanOrEqualToConstant:0].active = YES;
        [name.widthAnchor constraintLessThanOrEqualToConstant:IDMiniControls ? 100 : 120].active = YES; [content addArrangedSubview:name];
        if (pinned) {
            UIImageView *pin = [[UIImageView alloc] initWithImage:[UIImage systemImageNamed:@"pin.fill"]]; pin.tintColor = foreground;
            [pin.widthAnchor constraintEqualToConstant:10].active = YES; [pin.heightAnchor constraintEqualToConstant:12].active = YES; [content addArrangedSubview:pin];
        }
        NSUInteger count = group ? group.count : total;
        if (IDShowCounts) {
            UILabel *badge = [UILabel new]; badge.text = count > 99 ? @"99+" : [NSString stringWithFormat:@"%lu", (unsigned long)count];
            badge.font = [UIFont monospacedDigitSystemFontOfSize:11 weight:UIFontWeightBold]; badge.textColor = foreground; badge.alpha = 0.75;
            [content addArrangedSubview:badge];
        }
        [NSLayoutConstraint activateConstraints:@[[content.leadingAnchor constraintEqualToAnchor:button.leadingAnchor constant:IDGridLayout ? 5 : 10], [content.trailingAnchor constraintEqualToAnchor:button.trailingAnchor constant:IDGridLayout ? -5 : -10], [content.centerYAnchor constraintEqualToAnchor:button.centerYAnchor]]];
        // Leave buttons enabled for pin actions even if filtering is unavailable.
        button.accessibilityIdentifier = group.identifier;
        button.accessibilityLabel = [NSString stringWithFormat:@"%@%@，%lu 条通知", pinned ? @"已置顶，" : @"", group ? group.name : @"全部", (unsigned long)count];
        button.accessibilityHint = group ? @"点按筛选；长按置顶或取消置顶。也可在设置中管理。" : @"显示全部应用通知";
        button.accessibilityTraits = UIAccessibilityTraitButton | (active ? UIAccessibilityTraitSelected : 0);
        __weak IDCoordinator *coordinator = self.coordinator;
        NSString *identifier = group.identifier;
        [button addAction:[UIAction actionWithHandler:^(__kindof UIAction *action) {
            if (!identifier || ((IDContainedMode || IDFilteringReady) && IDGrouping)) [coordinator selectIdentifier:identifier];
        }] forControlEvents:UIControlEventTouchUpInside];
        if (group) {
            UILongPressGestureRecognizer *hold = [[UILongPressGestureRecognizer alloc] initWithTarget:self action:@selector(pinGesture:)];
            hold.minimumPressDuration = 0.55; hold.cancelsTouchesInView = YES; [button addGestureRecognizer:hold];
            UIAccessibilityCustomAction *pinAction = [[UIAccessibilityCustomAction alloc] initWithName:pinned ? @"取消置顶" : @"置顶应用" actionHandler:^BOOL(UIAccessibilityCustomAction *action) {
                [coordinator togglePinnedIdentifier:identifier]; return YES;
            }];
            button.accessibilityCustomActions = @[pinAction];
        }
        [self.stack addSubview:button];
    }
    [self.coordinator updateClearButton];
    [self invalidateIntrinsicContentSize]; [self setNeedsLayout]; [self layoutIfNeeded];
    self.scroll.contentOffset = CGPointMake(MIN(MAX(0, offset.x), MAX(0, self.scroll.contentSize.width - self.scroll.bounds.size.width)), MIN(MAX(0, offset.y), MAX(0, self.scroll.contentSize.height - self.scroll.bounds.size.height)));
}
#include "IDContainedUI.inc"
@end
