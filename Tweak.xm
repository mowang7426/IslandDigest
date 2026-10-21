#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#import <objc/runtime.h>
#import <objc/message.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>
#import "IDShared.h"
#include <sys/stat.h>
static BOOL IDEnabled = YES, IDGrouping = YES, IDShowCounts = YES;
static NSMutableDictionary *IDDiagnostics;
static void IDStatus(NSString *stage, NSString *reason) {
    if (!IDDiagnostics) IDDiagnostics = [NSMutableDictionary new];
    IDDiagnostics[@"version"] = @"0.1.5";
    IDDiagnostics[@"system"] = UIDevice.currentDevice.systemVersion;
    IDDiagnostics[@"pid"] = @(getpid());
    IDDiagnostics[@"updated"] = NSDate.date;
    IDDiagnostics[@"stage"] = stage;
    IDDiagnostics[@"reason"] = reason ?: @"";
    [IDDiagnostics writeToFile:IDStatusPath atomically:YES];
}
static void IDLoadOptions(void) {
    IDReloadPreferences();
    IDEnabled = IDPref(@"Enabled", YES);
    IDGrouping = IDPref(@"Grouping", YES);
    IDShowCounts = IDPref(@"ShowCounts", YES);
}

@interface NCNotificationStructuredListViewController : UIViewController
@end

@interface NCNotificationListSupplementaryViewConfiguration : NSObject
- (instancetype)_initWithGroupingIdentifier:(NSString *)identifier groupName:(NSString *)name
    defaultAction:(id)action supplementaryActions:(NSArray *)actions
    auxiliaryOptionsSummaryText:(NSString *)summary auxiliaryOptionActions:(NSArray *)options
    tintColor:(UIColor *)tint textColor:(UIColor *)text materialRecipe:(NSInteger)recipe
    containsUnmanagedContent:(BOOL)unmanaged preventsUserDismissal:(BOOL)prevents;
@end

static void IDLog(const char *event) {
#ifdef ID_REGRESSION
    return;
#endif
    FILE *file = fopen("/var/mobile/Library/Logs/IslandDigest.log", "a");
    if (!file) {
        int failure = errno;
        file = fopen("/tmp/IslandDigest.log", "a");
        if (!file) {
            fprintf(stderr, "[IslandDigest] log unavailable primary_errno=%d fallback_errno=%d event=%s\n",
                failure, errno, event);
            return;
        }
        fprintf(file, "primary_log_errno=%d ", failure);
    }
    fprintf(file, "%lld pid=%d %s\n", (long long)time(NULL), getpid(), event);
    fclose(file);
}

static int IDAcquireStartupMarker(const char *path) {
    int descriptor = open(path, O_WRONLY | O_CREAT | O_EXCL, 0600);
    if (descriptor < 0) return errno == EEXIST ? 1 : 2;
    char record[64];
    int length = snprintf(record, sizeof(record), "pid=%d time=%lld\n", getpid(), (long long)time(NULL));
    ssize_t written = write(descriptor, record, (size_t)length);
    close(descriptor);
    if (written != length) {
        unlink(path);
        return 2;
    }
    return 0;
}

#ifdef ID_REGRESSION
static NSString *IDStartupMarkerOverride;
#endif

static const char *IDStartupMarkerPath(void) {
#ifdef ID_REGRESSION
    return IDStartupMarkerOverride.fileSystemRepresentation;
#else
    return "/var/mobile/Library/Preferences/IslandDigest-0.1.5.startup-pending";
#endif
}

static NSString *IDText(const char *text) {
    return [NSString stringWithUTF8String:text];
}

static BOOL IDMethod(Class target, const char *name, const char *result, unsigned int arguments) {
    Method method = class_getInstanceMethod(target, sel_registerName(name));
    if (!method || method_getNumberOfArguments(method) != arguments) return NO;
    char *type = method_copyReturnType(method);
    BOOL matches = type && strcmp(type, result) == 0;
    free(type);
    return matches;
}

static BOOL IDArgument(Class target, const char *name, unsigned int index, const char *expected) {
    Method method = class_getInstanceMethod(target, sel_registerName(name));
    if (!method || index >= method_getNumberOfArguments(method)) return NO;
    char *type = method_copyArgumentType(method, index);
    BOOL matches = type && strcmp(type, expected) == 0;
    free(type);
    return matches;
}

static id IDRead(id object, const char *name) {
    SEL selector = sel_registerName(name);
    if (!object || !IDMethod(object_getClass(object), name, "@", 2)) return nil;
    return ((id (*)(id, SEL))objc_msgSend)(object, selector);
}

static NSArray *IDItems(id object) {
    if ([object isKindOfClass:[NSDictionary class]]) return [object allValues];
    if ([object isKindOfClass:[NSArray class]]) return object;
    if ([object isKindOfClass:[NSSet class]]) return [object allObjects];
    if ([object isKindOfClass:[NSOrderedSet class]]) return [object array];
    return @[];
}

static void IDCollectRequests(id object, NSMutableSet *requests, NSUInteger depth) {
    if (!object || depth > 8) return;
    if (IDMethod(object_getClass(object), "sectionIdentifier", "@", 2)) {
        [requests addObject:object];
        return;
    }
    NSArray *items = [object isKindOfClass:NSDictionary.class] ? [object allValues] : IDItems(object);
    for (id item in items) IDCollectRequests(item, requests, depth + 1);
}

@interface IDGroup : NSObject
@property(nonatomic, copy) NSString *identifier;
@property(nonatomic, copy) NSString *name;
@property(nonatomic, strong) UIImage *icon;
@property(nonatomic, strong) NSDate *latest;
@property(nonatomic) NSUInteger count;
@property(nonatomic) BOOL hasCriticalAlert;
@end
@implementation IDGroup
@end

@class IDCoordinator;

@interface IDDigestView : UIView
@property(nonatomic, strong) UIVisualEffectView *glass;
@property(nonatomic, strong) UILabel *heading;
@property(nonatomic, strong) UILabel *detail;
@property(nonatomic, strong) UIScrollView *scroll;
@property(nonatomic, strong) UIStackView *stack;
@property(nonatomic, weak) IDCoordinator *coordinator;
- (void)showGroups:(NSArray<IDGroup *> *)groups selected:(NSString *)selected;
@end

@interface IDDigestController : UIViewController
@property(nonatomic, weak) IDCoordinator *coordinator;
@end

@interface IDCoordinator : NSObject
@property(nonatomic, weak) UIViewController *owner;
@property(nonatomic, strong) IDDigestController *panel;
@property(nonatomic, strong) id container;
@property(nonatomic, strong) id configuration;
@property(nonatomic, copy) NSArray<IDGroup *> *groups;
@property(nonatomic, copy) NSString *selected;
@property(nonatomic, copy) NSString *renderedSelection;
@property(nonatomic, strong) NSMutableSet<NSString *> *filtered;
@property(nonatomic) BOOL overlay;
@property(nonatomic) UIEdgeInsets savedInsets;
@property(nonatomic) BOOL inserted;
@property(nonatomic) BOOL updating;
@property(nonatomic) BOOL applyingFilter;
@property(nonatomic) BOOL filterReconcilePending;
@property(nonatomic) BOOL active;
@property(nonatomic) BOOL disabled;
@property(nonatomic) NSUInteger recoveryAttempts;
- (void)refresh;
- (void)selectIdentifier:(NSString *)identifier;
- (void)resetFilter;
- (void)disable;
- (void)recover;
@end

static char IDCoordinatorKey;
static NSHashTable *IDOwners;
static BOOL IDRefreshPending;
static BOOL IDFilteringReady;

static IDCoordinator *IDState(id owner) {
    return objc_getAssociatedObject(owner, &IDCoordinatorKey);
}

static IDCoordinator *IDEnsureState(UIViewController *owner) {
    IDCoordinator *state = IDState(owner);
    if (!state) {
        state = [IDCoordinator new];
        state.owner = owner;
        objc_setAssociatedObject(owner, &IDCoordinatorKey, state, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        [IDOwners addObject:owner];
        IDDiagnostics[@"ownerSeen"] = @YES;
        IDStatus(@"owner-observed", @"通知控制器钩子已触达；等待可见界面");
    }
    return state;
}

static void IDScheduleRefresh(void) {
    if (![NSThread isMainThread]) {
        dispatch_async(dispatch_get_main_queue(), ^{ IDScheduleRefresh(); });
        return;
    }
    if (IDRefreshPending) return;
    IDRefreshPending = YES;
    dispatch_async(dispatch_get_main_queue(), ^{
        IDRefreshPending = NO;
        for (id owner in IDOwners.allObjects) {
            IDCoordinator *state = IDState(owner);
            if (state.active) [state refresh];
        }
    });
}

@implementation IDDigestView

- (instancetype)initWithFrame:(CGRect)frame {
    self = [super initWithFrame:frame];
    if (!self) return nil;
    self.layer.cornerRadius = 24;
    self.layer.cornerCurve = kCACornerCurveContinuous;
    self.clipsToBounds = YES;
    self.glass = [[UIVisualEffectView alloc] initWithEffect:
        [UIBlurEffect effectWithStyle:UIBlurEffectStyleSystemUltraThinMaterial]];
    self.glass.userInteractionEnabled = NO;
    [self addSubview:self.glass];
    self.heading = [UILabel new];
    self.heading.text = IDText("按应用分组");
    self.heading.font = [UIFont systemFontOfSize:16 weight:UIFontWeightSemibold];
    self.heading.textColor = UIColor.labelColor;
    self.heading.adjustsFontForContentSizeCategory = YES;
    [self addSubview:self.heading];
    self.detail = [UILabel new];
    self.detail.font = [UIFont systemFontOfSize:11 weight:UIFontWeightMedium];
    self.detail.textColor = UIColor.secondaryLabelColor;
    self.detail.textAlignment = NSTextAlignmentRight;
    self.detail.adjustsFontSizeToFitWidth = YES;
    [self addSubview:self.detail];
    self.scroll = [UIScrollView new];
    self.scroll.showsHorizontalScrollIndicator = NO;
    self.scroll.alwaysBounceHorizontal = YES;
    [self addSubview:self.scroll];
    self.stack = [[UIStackView alloc] initWithFrame:CGRectZero];
    self.stack.axis = UILayoutConstraintAxisHorizontal;
    self.stack.spacing = 8;
    self.stack.alignment = UIStackViewAlignmentFill;
    [self.scroll addSubview:self.stack];
    self.accessibilityIdentifier = IDText("IslandDigest");
    return self;
}

- (CGSize)sizeThatFits:(CGSize)size {
    return CGSizeMake(size.width, 112);
}

- (CGSize)intrinsicContentSize {
    return CGSizeMake(UIViewNoIntrinsicMetric, 112);
}

- (void)layoutSubviews {
    [super layoutSubviews];
    CGFloat width = CGRectGetWidth(self.bounds);
    self.glass.frame = self.bounds;
    self.heading.frame = CGRectMake(16, 12, width * 0.45, 22);
    self.detail.frame = CGRectMake(width * 0.5, 15, width * 0.5 - 16, 18);
    self.scroll.frame = CGRectMake(0, 44, width, 56);
    CGSize fitting = [self.stack systemLayoutSizeFittingSize:
        CGSizeMake(UILayoutFittingCompressedSize.width, 52)
        withHorizontalFittingPriority:UILayoutPriorityFittingSizeLevel
        verticalFittingPriority:UILayoutPriorityRequired];
    self.stack.frame = CGRectMake(14, 0, fitting.width, 52);
    self.scroll.contentSize = CGSizeMake(fitting.width + 28, 56);
}

- (void)showGroups:(NSArray<IDGroup *> *)groups selected:(NSString *)selected {
    CGPoint offset = self.scroll.contentOffset;
    for (UIView *view in self.stack.arrangedSubviews.copy) {
        [self.stack removeArrangedSubview:view];
        [view removeFromSuperview];
    }
    NSUInteger total = 0;
    for (IDGroup *group in groups) total += group.count;
    self.detail.text = IDShowCounts ? [NSString stringWithFormat:IDText("%lu 个应用 · %lu 条"),
        (unsigned long)groups.count, (unsigned long)total] : IDText("保留原生通知操作");
    NSMutableArray *entries = [NSMutableArray arrayWithObject:NSNull.null];
    [entries addObjectsFromArray:groups];
    for (id entry in entries) {
        IDGroup *group = entry == NSNull.null ? nil : entry;
        BOOL active = group ? [selected isEqualToString:group.identifier] : !selected;
        UIButton *button = [UIButton buttonWithType:UIButtonTypeCustom];
        button.layer.cornerRadius = 17;
        button.layer.cornerCurve = kCACornerCurveContinuous;
        button.backgroundColor = active ? [UIColor.labelColor colorWithAlphaComponent:0.9]
            : [UIColor.secondaryLabelColor colorWithAlphaComponent:0.09];
        UIColor *foreground = active ? UIColor.systemBackgroundColor : UIColor.labelColor;
        UIStackView *content = [UIStackView new];
        content.axis = UILayoutConstraintAxisHorizontal;
        content.alignment = UIStackViewAlignmentCenter;
        content.spacing = 7;
        content.userInteractionEnabled = NO;
        content.translatesAutoresizingMaskIntoConstraints = NO;
        [button addSubview:content];
        UIImageView *icon = [[UIImageView alloc] initWithImage:
            group.icon ?: [UIImage systemImageNamed:IDText(group ? "app.fill" : "square.stack.3d.up.fill")]];
        icon.tintColor = foreground;
        icon.contentMode = UIViewContentModeScaleAspectFit;
        icon.layer.cornerRadius = 7;
        icon.clipsToBounds = YES;
        [content addArrangedSubview:icon];
        [icon.widthAnchor constraintEqualToConstant:26].active = YES;
        [icon.heightAnchor constraintEqualToConstant:26].active = YES;
        UILabel *name = [UILabel new];
        name.text = group ? group.name : IDText("全部");
        name.textColor = foreground;
        name.font = [UIFont systemFontOfSize:12 weight:UIFontWeightSemibold];
        name.lineBreakMode = NSLineBreakByTruncatingTail;
        [name.widthAnchor constraintLessThanOrEqualToConstant:80].active = YES;
        [content addArrangedSubview:name];
        UILabel *badge = [UILabel new];
        NSUInteger count = group ? group.count : total;
        badge.text = count > 99 ? IDText("99+") : [NSString stringWithFormat:IDText("%lu"), (unsigned long)count];
        badge.font = [UIFont monospacedDigitSystemFontOfSize:11 weight:UIFontWeightBold];
        badge.textColor = foreground;
        badge.alpha = 0.75;
        badge.hidden = !IDShowCounts;
        [content addArrangedSubview:badge];
        button.enabled = !group || (IDFilteringReady && IDGrouping);
        [NSLayoutConstraint activateConstraints:@[
            [content.leadingAnchor constraintEqualToAnchor:button.leadingAnchor constant:11],
            [content.trailingAnchor constraintEqualToAnchor:button.trailingAnchor constant:-11],
            [content.centerYAnchor constraintEqualToAnchor:button.centerYAnchor],
            [content.topAnchor constraintGreaterThanOrEqualToAnchor:button.topAnchor constant:8],
            [content.bottomAnchor constraintLessThanOrEqualToAnchor:button.bottomAnchor constant:-8]
        ]];
        button.accessibilityLabel = [NSString stringWithFormat:IDText("%@，%lu 条通知"),
            group ? group.name : IDText("全部"), (unsigned long)count];
        button.accessibilityTraits = UIAccessibilityTraitButton | (active ? UIAccessibilityTraitSelected : 0);
        __weak IDCoordinator *coordinator = self.coordinator;
        NSString *identifier = group.identifier;
        [button addAction:[UIAction actionWithHandler:^(__kindof UIAction *action) {
            [coordinator selectIdentifier:identifier];
        }] forControlEvents:UIControlEventTouchUpInside];
        [self.stack addArrangedSubview:button];
    }
    [self setNeedsLayout];
    [self layoutIfNeeded];
    CGFloat maxOffset = MAX(0, self.scroll.contentSize.width - self.scroll.bounds.size.width);
    self.scroll.contentOffset = CGPointMake(MIN(MAX(0, offset.x), maxOffset), 0);
}
@end

@implementation IDDigestController
- (void)loadView {
    IDDigestView *view = [[IDDigestView alloc] initWithFrame:CGRectMake(0, 0, 350, 112)];
    view.coordinator = self.coordinator;
    self.view = view;
    self.preferredContentSize = CGSizeMake(350, 112);
}
- (BOOL)_canShowWhileLocked {
    return YES;
}
@end

static BOOL IDSupplementaryReady(id owner) {
    Class target = object_getClass(owner);
    Class configuration = objc_getClass("NCNotificationListSupplementaryViewConfiguration");
    const char *init = "_initWithGroupingIdentifier:groupName:defaultAction:supplementaryActions:auxiliaryOptionsSummaryText:auxiliaryOptionActions:tintColor:textColor:materialRecipe:containsUnmanagedContent:preventsUserDismissal:";
    if (!IDMethod(target, "insertSupplementaryViewsContainerAtListPosition:identifier:withDescription:", "@", 5)
        || !IDArgument(target, "insertSupplementaryViewsContainerAtListPosition:identifier:withDescription:", 2, "Q")
        || !IDArgument(target, "insertSupplementaryViewsContainerAtListPosition:identifier:withDescription:", 3, "@")
        || !IDArgument(target, "insertSupplementaryViewsContainerAtListPosition:identifier:withDescription:", 4, "@")
        || !IDMethod(configuration, init, "@", 13)) return NO;
    for (unsigned int index = 2; index <= 9; index++) if (!IDArgument(configuration, init, index, "@")) return NO;
    return IDArgument(configuration, init, 10, "q") && IDArgument(configuration, init, 11, "B") && IDArgument(configuration, init, 12, "B");
}

@implementation IDCoordinator

- (instancetype)init {
    self = [super init];
    if (self) self.filtered = [NSMutableSet new];
    return self;
}

- (NSArray<IDGroup *> *)snapshot {
    id master = IDRead(self.owner, "masterList");
    if (!master) master = IDRead(self.owner, "notificationMasterList");
    id sectionObjects = IDRead(master, "_notificationSectionListsForEnumeration");
    if (!sectionObjects) sectionObjects = IDRead(master, "sectionLists");
    NSArray *sections = IDItems(sectionObjects);
    NSMutableSet *requests = [NSMutableSet new];
    IDCollectRequests(IDRead(master, "allNotificationRequests"), requests, 0);
    IDDiagnostics[@"masterSeen"] = @(master != nil);
    IDDiagnostics[@"sections"] = @(sections.count);
    IDDiagnostics[@"enumerationSupported"] = @(sectionObjects != nil || IDMethod(object_getClass(master), "allNotificationRequests", "@", 2));
    for (id section in sections) {
        IDCollectRequests(IDRead(section, "allNotificationRequests"), requests, 0);
        IDCollectRequests(IDRead(section, "filteredNotificationRequests"), requests, 0);
    }
    NSMutableDictionary<NSString *, IDGroup *> *groups = [NSMutableDictionary new];
    for (id request in requests) {
        NSString *identifier = IDRead(request, "sectionIdentifier");
        if (![identifier isKindOfClass:NSString.class] || identifier.length == 0) continue;
        IDGroup *group = groups[identifier];
        if (!group) {
            group = [IDGroup new];
            group.identifier = identifier;
            SEL settingsSelector = sel_registerName("_sectionSettingsForSectionIdentifier:");
            id settings = IDMethod(object_getClass(self.owner), "_sectionSettingsForSectionIdentifier:", "@", 3)
                && IDArgument(object_getClass(self.owner), "_sectionSettingsForSectionIdentifier:", 2, "@")
                ? ((id (*)(id, SEL, id))objc_msgSend)(self.owner, settingsSelector, identifier) : nil;
            NSString *name = IDRead(settings, "displayName");
            group.name = [name isKindOfClass:NSString.class] && name.length ? name : identifier;
            id icon = IDRead(settings, "settingsIcon");
            if (![icon isKindOfClass:UIImage.class]) icon = IDRead(IDRead(request, "content"), "icon");
            if ([icon isKindOfClass:UIImage.class]) group.icon = icon;
            group.latest = NSDate.distantPast;
            groups[identifier] = group;
        }
        group.count += 1;
        if (IDMethod(object_getClass(request), "isCriticalAlert", "B", 2)
            && ((BOOL (*)(id, SEL))objc_msgSend)(request, sel_registerName("isCriticalAlert"))) {
            group.hasCriticalAlert = YES;
        }
        NSDate *date = IDRead(request, "timestamp");
        if ([date isKindOfClass:NSDate.class] && [date compare:group.latest] == NSOrderedDescending) {
            group.latest = date;
        }
    }
    return [groups.allValues sortedArrayUsingComparator:^NSComparisonResult(IDGroup *first, IDGroup *second) {
        NSComparisonResult result = [second.latest compare:first.latest];
        return result == NSOrderedSame ? [first.identifier compare:second.identifier] : result;
    }];
}

- (BOOL)applyFilter {
    if (!IDFilteringReady) return YES;
    if (self.applyingFilter) {
        self.filterReconcilePending = YES;
        return YES;
    }
    self.applyingFilter = YES;
    BOOL succeeded = YES;
    @try {
        for (NSUInteger pass = 0; pass < 2; pass++) {
            self.filterReconcilePending = NO;
            NSMutableSet *excluded = [NSMutableSet new];
            for (IDGroup *group in self.groups) {
                if (self.selected && !group.hasCriticalAlert && ![group.identifier isEqualToString:self.selected]) {
                    [excluded addObject:group.identifier];
                }
            }
            NSMutableSet *changed = self.filtered.mutableCopy;
            [changed unionSet:excluded];
            for (NSString *identifier in changed) {
                BOOL wasFiltered = [self.filtered containsObject:identifier];
                BOOL shouldFilter = [excluded containsObject:identifier];
                if (wasFiltered == shouldFilter) continue;
                if (shouldFilter) [self.filtered addObject:identifier];
                @try {
                    ((void (*)(id, SEL, id, BOOL))objc_msgSend)(self.owner,
                        sel_registerName("toggleFilteringForSectionIdentifier:shouldFilter:"), identifier, shouldFilter);
                    if (!shouldFilter) [self.filtered removeObject:identifier];
                } @catch (NSException *exception) {
                    NSLog(IDText("[IslandDigest] filter exception: %@"), exception.name);
                    succeeded = NO;
                }
            }
            if (!succeeded || !self.filterReconcilePending) break;
        }
        if (self.filterReconcilePending) succeeded = NO;
    } @finally {
        self.applyingFilter = NO;
    }
    return succeeded;
}

- (void)render {
    if (!self.panel.isViewLoaded) return;
    [(IDDigestView *)self.panel.view showGroups:self.groups selected:self.selected];
    self.renderedSelection = self.selected;
}

- (void)resetFilter {
    self.selected = nil;
    if (self.disabled) {
        [self recover];
        return;
    }
    if (![self applyFilter]) {
        [self disable];
        return;
    }
    if (!self.disabled && self.active) [self render];
}

- (void)recover {
    if (self.recoveryAttempts >= 3) return;
    self.recoveryAttempts += 1;
    self.selected = nil;
    if (![self applyFilter]) IDLog("filter restoration pending");
    if (self.inserted && self.overlay) {
        [self.panel willMoveToParentViewController:nil];
        [self.panel.view removeFromSuperview];
        [self.panel removeFromParentViewController];
        self.owner.additionalSafeAreaInsets = self.savedInsets;
        self.inserted = NO;
        self.overlay = NO;
    }
    if (self.inserted) {
        @try {
            ((void (*)(id, SEL, id))objc_msgSend)(self.container,
                sel_registerName("removeSupplementaryViewController:"), self.panel);
            self.inserted = NO;
        } @catch (NSException *exception) {
            IDLog("panel removal failed");
        }
    }
}

- (void)disable {
    self.disabled = YES;
    self.selected = nil;
    if (self.panel.isViewLoaded) {
        self.panel.view.userInteractionEnabled = NO;
        self.panel.view.hidden = YES;
        self.panel.view.accessibilityElementsHidden = YES;
    }
    [self recover];
    IDStatus(@"interface-failed", @"面板或筛选操作失败；已停止新增修改，可在设置重试");
    IDLog("disabled after interface failure");
}

- (void)selectIdentifier:(NSString *)identifier {
    if (self.disabled || !self.active || self.updating) return;
    self.updating = YES;
    @try {
        self.selected = [self.selected isEqualToString:identifier] ? nil : identifier;
        if (![self applyFilter]) {
            [self disable];
            return;
        }
        [self render];
        UISelectionFeedbackGenerator *feedback = [UISelectionFeedbackGenerator new];
        [feedback selectionChanged];
        UIAccessibilityPostNotification(UIAccessibilityLayoutChangedNotification, nil);
    } @catch (NSException *exception) {
        [self disable];
    } @finally {
        self.updating = NO;
    }
}

- (void)refresh {
    if (!self.active || self.updating || !self.owner.isViewLoaded) return;
    if (!IDEnabled || !IDGrouping) {
        self.selected = nil;
        [self recover];
        IDStatus(@"disabled-by-preference", @"已按设置关闭分组；原生通知不变");
        return;
    }
    if (self.disabled) {
        [self recover];
        return;
    }
    self.updating = YES;
    @try {
        NSArray<IDGroup *> *groups = [self snapshot];
        BOOL unchanged = self.groups.count == groups.count;
        BOOL selectionExists = !self.selected;
        for (NSUInteger index = 0; index < groups.count; index++) {
            IDGroup *group = groups[index];
            if ([group.identifier isEqualToString:self.selected]) selectionExists = YES;
            if (unchanged) {
                IDGroup *previous = self.groups[index];
                unchanged = [previous.identifier isEqualToString:group.identifier]
                    && previous.count == group.count && [previous.name isEqualToString:group.name];
            }
        }
        self.groups = groups;
        self.recoveryAttempts = 0;
        IDDiagnostics[@"apps"] = @(groups.count);
        NSUInteger requestCount = 0;
        for (IDGroup *group in groups) requestCount += group.count;
        IDDiagnostics[@"requests"] = @(requestCount);
        IDDiagnostics[@"filterAvailable"] = @(IDFilteringReady);
        if (!selectionExists) self.selected = nil;
        if (![self applyFilter]) {
            [self disable];
            return;
        }
        if (groups.count == 0) {
            if (self.inserted) [self recover];
            BOOL supported = [IDDiagnostics[@"enumerationSupported"] boolValue];
            IDStatus(supported ? @"waiting-notifications" : @"enumeration-unavailable",
                supported ? @"控制器已出现；目前未读到通知" : @"控制器已出现，但当前系统的通知枚举接口不可用");
            return;
        }
        if (!self.container && !self.inserted && !IDSupplementaryReady(self.owner)) {
            // Public UIKit fallback: no notification cells, requests or delegates replaced.
            self.panel = [IDDigestController new];
            self.panel.coordinator = self;
            self.savedInsets = self.owner.additionalSafeAreaInsets;
            UIEdgeInsets insets = self.savedInsets;
            insets.top += 112;
            self.owner.additionalSafeAreaInsets = insets;
            [self.owner addChildViewController:self.panel];
            UIView *view = self.panel.view;
            view.translatesAutoresizingMaskIntoConstraints = NO;
            [self.owner.view addSubview:view];
            [NSLayoutConstraint activateConstraints:@[
                [view.topAnchor constraintEqualToAnchor:self.owner.view.safeAreaLayoutGuide.topAnchor constant:-112],
                [view.leadingAnchor constraintEqualToAnchor:self.owner.view.leadingAnchor constant:8],
                [view.trailingAnchor constraintEqualToAnchor:self.owner.view.trailingAnchor constant:-8],
                [view.heightAnchor constraintEqualToConstant:112]
            ]];
            [self.panel didMoveToParentViewController:self.owner];
            self.overlay = YES;
            self.inserted = YES;
        }
        if (self.overlay) {
            [self render];
            IDDiagnostics[@"presentation"] = @"UIKit-safe-area";
            IDStatus(@"panel-visible", IDFilteringReady ? @"应用分组面板已加入可见控制器" : @"分组面板可见；原生筛选接口不兼容，应用按钮停用");
            return;
        }
        if (!self.container) {
            Class configurationClass = objc_getClass("NCNotificationListSupplementaryViewConfiguration");
            self.configuration = [(NCNotificationListSupplementaryViewConfiguration *)[configurationClass alloc]
                _initWithGroupingIdentifier:IDText("com.local.islanddigest") groupName:IDText("IslandDigest")
                defaultAction:nil supplementaryActions:@[] auxiliaryOptionsSummaryText:nil
                auxiliaryOptionActions:@[] tintColor:nil textColor:nil materialRecipe:0
                containsUnmanagedContent:YES preventsUserDismissal:YES];
            if (!self.configuration) {
                [self disable];
                return;
            }
            self.panel = [IDDigestController new];
            self.panel.coordinator = self;
            IDLog("creating supplementary container");
            self.container = ((id (*)(id, SEL, NSUInteger, id, id))objc_msgSend)(self.owner,
                sel_registerName("insertSupplementaryViewsContainerAtListPosition:identifier:withDescription:"),
                0, IDText("com.local.islanddigest.panel"), IDText("IslandDigest"));
        }
        if (!self.configuration || !self.container
            || !IDMethod(object_getClass(self.container), "insertSupplementaryViewController:withConfiguration:", "v", 4)
            || !IDArgument(object_getClass(self.container), "insertSupplementaryViewController:withConfiguration:", 2, "@")
            || !IDArgument(object_getClass(self.container), "insertSupplementaryViewController:withConfiguration:", 3, "@")
            || !IDMethod(object_getClass(self.container), "removeSupplementaryViewController:", "v", 3)
            || !IDArgument(object_getClass(self.container), "removeSupplementaryViewController:", 2, "@")) {
            [self disable];
            return;
        }
        if (!self.inserted) {
            [(IDDigestView *)self.panel.view showGroups:groups selected:self.selected];
            self.inserted = YES;
            IDLog("inserting panel");
            ((void (*)(id, SEL, id, id))objc_msgSend)(self.container,
                sel_registerName("insertSupplementaryViewController:withConfiguration:"), self.panel, self.configuration);
            IDLog("panel inserted");
            IDDiagnostics[@"presentation"] = @"native-supplementary";
            IDStatus(@"panel-visible", IDFilteringReady ? @"原生通知分组面板已插入" : @"面板已插入；筛选接口不可用，应用按钮停用");
            self.renderedSelection = self.selected;
        } else if (!unchanged || !selectionExists
            || !((!self.selected && !self.renderedSelection) || [self.selected isEqualToString:self.renderedSelection])) {
            [self render];
        }
    } @catch (NSException *exception) {
        NSLog(IDText("[IslandDigest] interface exception: %@"), exception.name);
        [self disable];
    } @finally {
        self.updating = NO;
    }
}
@end

#if !defined(ID_REGRESSION) || defined(ID_INTEGRATION)
%group IDMain
%hook NCNotificationStructuredListViewController
- (void)viewDidLoad {
    %orig;
    IDEnsureState(self);
    IDLog("owner loaded; panel deferred");
}
- (void)viewDidAppear:(BOOL)animated {
    %orig;
    IDCoordinator *state = IDEnsureState(self);
    state.active = YES;
    IDLog("owner appeared");
    IDScheduleRefresh();
}
- (void)viewDidLayoutSubviews {
    %orig;
    IDCoordinator *state = IDEnsureState(self);
    if (self.view.window && !state.active) {
        state.active = YES;
        IDDiagnostics[@"visibleOwnerSeen"] = @YES;
        IDScheduleRefresh(); // Never insert UI synchronously from a layout callback.
    }
}
- (void)viewWillDisappear:(BOOL)animated {
    IDCoordinator *state = IDState(self);
    state.active = NO;
    %orig;
    __weak IDCoordinator *weakState = state;
    dispatch_async(dispatch_get_main_queue(), ^{
        IDCoordinator *current = weakState;
        if (!current || current.active) return;
        @try { [current resetFilter]; }
        @catch (NSException *exception) { [current disable]; }
    });
}
- (void)insertNotificationRequest:(id)request {
    %orig;
    IDScheduleRefresh();
}
- (void)modifyNotificationRequest:(id)request {
    %orig;
    IDScheduleRefresh();
}
- (void)removeNotificationRequest:(id)request {
    %orig;
    IDScheduleRefresh();
}
- (void)notificationListComponent:(id)component didRemoveNotificationRequest:(id)request {
    %orig;
    IDScheduleRefresh();
}
- (void)notificationListBaseComponentDidRemoveAll:(id)component {
    %orig;
    IDScheduleRefresh();
}
%end
%end

%group IDFilter
%hook NCNotificationStructuredListViewController
- (BOOL)notificationMasterList:(id)master shouldFilterNotificationRequest:(id)request {
    BOOL original = %orig;
    NSString *selected = IDState(self).selected;
    if (original || !selected) return original;
    if (IDMethod(object_getClass(request), "isCriticalAlert", "B", 2)
        && ((BOOL (*)(id, SEL))objc_msgSend)(request, sel_registerName("isCriticalAlert"))) return NO;
    NSString *identifier = IDRead(request, "sectionIdentifier");
    for (IDGroup *group in IDState(self).groups) {
        if (group.hasCriticalAlert && [group.identifier isEqualToString:identifier]) return NO;
    }
    return identifier && ![selected isEqualToString:identifier];
}
%end
%end

static BOOL IDHooksInstalled;
static NSUInteger IDInstallAttempts;
static void IDInstallHooks(void) {
    if (IDHooksInstalled) return;
    IDInstallAttempts++;
    IDDiagnostics[@"initAttempts"] = @(IDInstallAttempts);
    Class owner = objc_getClass("NCNotificationStructuredListViewController");
    if (!owner) {
        IDStatus(@"waiting-classes", @"通知框架尚未加载；有限重试后仍可从设置手动重试");
        return;
    }
    const char *marker = IDStartupMarkerPath();
    struct stat info;
    if (stat(marker, &info) == 0 && time(NULL) - info.st_mtime < 120) {
        IDStatus(@"startup-guard", @"检测到两分钟内未完成的启动；等待保护期或在设置手动重试");
        return;
    }
    unlink(marker); // Only this version's expired guard; older versions' markers are ignored.
    int markerResult = IDAcquireStartupMarker(marker);
    if (markerResult != 0) {
        IDStatus(@"guard-unavailable", @"启动保护文件不可写；为安全不安装钩子，可在设置重试");
        return;
    }
    IDFilteringReady = IDMethod(owner, "notificationMasterList:shouldFilterNotificationRequest:", "B", 4)
        && IDArgument(owner, "notificationMasterList:shouldFilterNotificationRequest:", 2, "@")
        && IDArgument(owner, "notificationMasterList:shouldFilterNotificationRequest:", 3, "@")
        && IDMethod(owner, "toggleFilteringForSectionIdentifier:shouldFilter:", "v", 4)
        && IDArgument(owner, "toggleFilteringForSectionIdentifier:shouldFilter:", 2, "@")
        && IDArgument(owner, "toggleFilteringForSectionIdentifier:shouldFilter:", 3, "B");
    IDOwners = [NSHashTable weakObjectsHashTable];
    if (IDFilteringReady) {
        %init(IDFilter);
    }
    %init(IDMain);
    IDHooksInstalled = YES;
    IDDiagnostics[@"hooksInstalled"] = @YES;
    IDDiagnostics[@"filterAvailable"] = @(IDFilteringReady);
    IDStatus(@"hooks-installed", @"钩子已注册，尚未确认通知控制器触达；请打开通知中心");
    // Hook registration is synchronous; no permanent 30-second startup marker.
    unlink(marker);
}

static void IDPreferenceChanged(CFNotificationCenterRef center, void *observer, CFStringRef name,
                                const void *object, CFDictionaryRef userInfo) {
    dispatch_async(dispatch_get_main_queue(), ^{
        IDLoadOptions();
        BOOL retry = CFEqual(name, CFSTR("com.local.islanddigest/retry"));
        if (retry && !IDHooksInstalled) {
            unlink(IDStartupMarkerPath()); // Explicit user recovery, not an automatic guard bypass.
            IDInstallHooks();
        }
        for (UIViewController *owner in IDOwners.allObjects) {
            IDCoordinator *state = IDState(owner);
            if (retry) {
                [state recover];
                if (state.inserted || state.filtered.count) continue;
                state.disabled = NO;
                state.recoveryAttempts = 0;
                state.panel = nil;
                state.container = nil;
                state.configuration = nil;
            }
            if (!IDEnabled || !IDGrouping) { state.recoveryAttempts = 0; [state resetFilter]; [state recover]; }
            else if (state.active && !state.disabled) [state render];
        }
        if (!IDEnabled || !IDGrouping) IDStatus(@"disabled-by-preference", @"已按设置关闭分组");
        IDScheduleRefresh();
    });
}
%ctor {
    dispatch_async(dispatch_get_main_queue(), ^{
        IDLoadOptions();
        IDStatus(@"constructor-reached", @"SpringBoard 已加载本插件；尚未安装控制器钩子");
        CFNotificationCenterAddObserver(CFNotificationCenterGetDarwinNotifyCenter(), NULL, IDPreferenceChanged,
            CFSTR("com.local.islanddigest/changed"), NULL, CFNotificationSuspensionBehaviorDeliverImmediately);
        CFNotificationCenterAddObserver(CFNotificationCenterGetDarwinNotifyCenter(), NULL, IDPreferenceChanged,
            CFSTR("com.local.islanddigest/retry"), NULL, CFNotificationSuspensionBehaviorDeliverImmediately);
        IDInstallHooks();
        for (NSNumber *delay in @[@1, @3, @8, @20, @125]) {
            dispatch_after(dispatch_time(DISPATCH_TIME_NOW, delay.longLongValue * NSEC_PER_SEC),
                           dispatch_get_main_queue(), ^{ IDInstallHooks(); });
        }
    });
}
#endif
