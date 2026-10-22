@implementation IDDigestView
- (instancetype)initWithFrame:(CGRect)frame {
    self = [super initWithFrame:frame];
    if (!self) return nil;
    self.layer.cornerRadius = 24; self.layer.cornerCurve = kCACornerCurveContinuous; self.clipsToBounds = YES;
    self.glass = [[UIVisualEffectView alloc] initWithEffect:[UIBlurEffect effectWithStyle:UIBlurEffectStyleSystemUltraThinMaterial]];
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
    self.cardScroll.userInteractionEnabled = YES; self.cardScroll.delaysContentTouches = NO;
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
    // A direct sibling interaction layer avoids UIScrollView/UIStackView hit testing.
    self.cardTapLayer = [UIView new]; self.cardTapLayer.userInteractionEnabled = YES;
    self.cardTapLayer.backgroundColor = UIColor.clearColor; self.cardTapLayer.accessibilityElementsHidden = YES;
    self.cardTapButtons = [NSMutableArray array]; [self addSubview:self.cardTapLayer];
    self.accessibilityIdentifier = @"IslandDigest";
    return self;
}
- (UIView *)hitTest:(CGPoint)point withEvent:(UIEvent *)event {
    // The explicit overlay owns only exact card frames; leave all other regions
    // to UIKit so category buttons and the clear control remain reachable.
    return [super hitTest:point withEvent:event];
}

- (void)clearTapped { [self.coordinator beginClear]; }
- (void)rebuildCardTapOverlay {
    if (!self.cardTapLayer) return;
    for (UIButton *button in self.cardTapButtons) [button removeFromSuperview];
    [self.cardTapButtons removeAllObjects];
    self.cardTapLayer.frame = self.cardScroll.frame;
    self.cardTapLayer.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    [self bringSubviewToFront:self.cardTapLayer];
    self.cardTapLayer.hidden = self.cardScroll.hidden;
    if (self.cardScroll.hidden) return;
    for (UIView *shell in self.cardStack.arrangedSubviews) {
        if (![shell.accessibilityIdentifier isEqualToString:@"IDContainedCard"] || shell.hidden) continue;
        CGRect frame = [shell convertRect:shell.bounds toView:self.cardTapLayer];
        frame = CGRectIntersection(frame, self.cardTapLayer.bounds);
        if (CGRectIsNull(frame) || frame.size.width < 44 || frame.size.height < 22) continue;
        UIButton *button = [UIButton buttonWithType:UIButtonTypeCustom];
        button.frame = frame; button.backgroundColor = [UIColor.systemBlueColor colorWithAlphaComponent:0.16];
        button.layer.cornerRadius = 12; button.layer.borderWidth = 1.5;
        button.layer.borderColor = [UIColor.systemBlueColor colorWithAlphaComponent:0.55].CGColor;
        [button setTitle:@"打开通知" forState:UIControlStateNormal];
        [button setTitleColor:UIColor.systemBlueColor forState:UIControlStateNormal];
        button.titleLabel.font = [UIFont systemFontOfSize:12 weight:UIFontWeightSemibold];
        button.accessibilityIdentifier = @"IslandDigestCardTapProbe";
        button.accessibilityLabel = shell.accessibilityLabel;
        button.accessibilityTraits = UIAccessibilityTraitButton;
        __weak IDCoordinator *coordinator = self.coordinator;
        __weak UIButton *weakButton = button;
        NSDictionary *cardModel = objc_getAssociatedObject(shell, @selector(rebuildCardTapOverlay));
        [button addAction:[UIAction actionWithHandler:^(__kindof UIAction *action) {
            UIButton *pressed = weakButton;
            [pressed setTitle:@"✓ 已收到点击" forState:UIControlStateNormal];
            [pressed setTitleColor:UIColor.systemGreenColor forState:UIControlStateNormal];
            pressed.backgroundColor = [UIColor.systemGreenColor colorWithAlphaComponent:0.28];
            pressed.layer.borderColor = [UIColor.systemGreenColor colorWithAlphaComponent:0.8].CGColor;
            IDOpenFeedback(@"打开诊断：点击已收到；正在检查原生动作");
            BOOL opened = cardModel[@"request"] && [coordinator openCardRequest:cardModel[@"request"]];
            pressed.accessibilityValue = opened ? @"已请求打开通知" : @"打开通知不可用";
            if (!opened) UIAccessibilityPostNotification(UIAccessibilityAnnouncementNotification, @"打开通知不可用");
        }] forControlEvents:UIControlEventTouchUpInside];
        [self.cardTapLayer addSubview:button]; [self.cardTapButtons addObject:button];
    }
}
- (void)scrollViewDidScroll:(UIScrollView *)scrollView {
    if (scrollView == self.cardScroll) [self rebuildCardTapOverlay];
}
- (CGFloat)rowHeight {
    CGFloat base = IDMiniControls ? 44 : 52;
    return MAX(44, MIN(80, [[UIFontMetrics metricsForTextStyle:UIFontTextStyleBody] scaledValueForValue:base]));
}
- (CGFloat)categoryHeightForWidth:(CGFloat)width {
    CGFloat row = [self rowHeight];
    NSUInteger columns = IDExperienceColumns(width, IDGridLayout);
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
    CGFloat width = self.bounds.size.width, row = [self rowHeight];
    self.glass.frame = self.bounds;
    self.heading.font = [[UIFontMetrics metricsForTextStyle:UIFontTextStyleHeadline] scaledFontForFont:[UIFont systemFontOfSize:16 weight:UIFontWeightSemibold] maximumPointSize:22];
    self.detail.font = [UIFont systemFontOfSize:11 weight:UIFontWeightMedium];
    self.hint.font = [UIFont systemFontOfSize:10 weight:UIFontWeightMedium];
    CGFloat contentWidth = MAX(0, width - 58);
    self.heading.frame = CGRectMake(14, 8, MAX(0, contentWidth * 0.52 - 14), 26);
    self.heading.adjustsFontSizeToFitWidth = YES;
    self.detail.frame = CGRectMake(contentWidth * 0.52, 12, MAX(0, contentWidth * 0.48), 20);
    self.clearButton.frame = CGRectMake(MAX(0, width - 48), 0, 44, 44);
    CGFloat category = [self categoryHeightForWidth:width];
    if (IDContainedMode) category = MIN(category, MAX(66, self.bounds.size.height - 38 - 88));
    self.scroll.frame = CGRectMake(0, 44, width, MAX(0, category - 66));
    self.hint.frame = CGRectMake(14, category - 19, MAX(0, width - 28), 16);
    self.cardScroll.frame = CGRectMake(0, category, width, MAX(0, self.bounds.size.height - category - 38));
    self.cardNotice.frame = CGRectMake(14, MAX(category, self.bounds.size.height - 36), MAX(0, width - 28), 32);
    self.scroll.showsHorizontalScrollIndicator = !IDGridLayout;
    self.scroll.showsVerticalScrollIndicator = IDGridLayout;
    self.scroll.alwaysBounceHorizontal = !IDGridLayout;
    self.scroll.alwaysBounceVertical = IDGridLayout;
    NSUInteger columns = IDExperienceColumns(width, IDGridLayout), index = 0;
    CGFloat x = 14;
    CGFloat cellWidth = MAX(1, (width - 28 - (columns - 1) * 8) / columns);
    for (UIView *button in self.stack.subviews) {
        if (IDGridLayout) {
            button.frame = CGRectMake(14 + (index % columns) * (cellWidth + 8), (index / columns) * (row + 8), cellWidth, row);
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
    // Rebuild after explicit frames exist; these sibling buttons are the only
    // card hit targets and are never hidden under native list content.
    [self rebuildCardTapOverlay];
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
        content.alignment = UIStackViewAlignmentCenter; content.spacing = IDMiniControls ? 4 : 6; content.userInteractionEnabled = NO;
        content.translatesAutoresizingMaskIntoConstraints = NO; [button addSubview:content];
        UIImageView *icon = [[UIImageView alloc] initWithImage:group.icon ?: [UIImage systemImageNamed:group ? @"app.fill" : @"square.stack.3d.up.fill"]];
        icon.tintColor = foreground; icon.contentMode = UIViewContentModeScaleAspectFit; icon.layer.cornerRadius = 7; icon.clipsToBounds = YES;
        [content addArrangedSubview:icon]; [icon.widthAnchor constraintEqualToConstant:IDMiniControls ? 20 : 24].active = YES; [icon.heightAnchor constraintEqualToConstant:IDMiniControls ? 20 : 24].active = YES;
        UILabel *name = [UILabel new]; name.text = group ? group.name : @"全部"; name.textColor = foreground;
        name.font = [[UIFontMetrics metricsForTextStyle:UIFontTextStyleCaption1] scaledFontForFont:[UIFont systemFontOfSize:12 weight:UIFontWeightSemibold] maximumPointSize:22];
        name.adjustsFontForContentSizeCategory = YES;
        name.lineBreakMode = NSLineBreakByTruncatingTail;
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
        [NSLayoutConstraint activateConstraints:@[[content.leadingAnchor constraintEqualToAnchor:button.leadingAnchor constant:10], [content.trailingAnchor constraintEqualToAnchor:button.trailingAnchor constant:-10], [content.centerYAnchor constraintEqualToAnchor:button.centerYAnchor]]];
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
