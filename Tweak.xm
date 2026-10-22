#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#import <objc/runtime.h>
#import <objc/message.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>
#import "IDShared.h"
#import "IDExperienceLogic.h"
#import "IDClearLogic.h"
#import "IDContainedLogic.h"
#import "IDLayoutLogic.h"
#include <sys/stat.h>
static BOOL IDEnabled = YES, IDGrouping = YES, IDShowCounts = YES;
static BOOL IDPreviewWhenUnlocked = NO;
static BOOL IDGridLayout = NO, IDRememberFilter = NO, IDMiniControls = YES;
static BOOL IDContainedMode = YES, IDHideNative = NO;
static CGFloat IDPanelTopOffset = 220, IDPanelSideInset = 8, IDPanelMaxHeight = 360;
static NSInteger IDSortMode;
static NSUInteger IDOptionsRevision;
static NSMutableDictionary *IDDiagnostics;
static void IDStatus(NSString *stage, NSString *reason) {
    if (!IDDiagnostics) IDDiagnostics = [NSMutableDictionary new];
    IDDiagnostics[@"version"] = @"0.2.8";
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
    IDPreviewWhenUnlocked = IDPref(@"AllowPreviewWhenUnlocked", NO);
    IDGridLayout = [IDStringOption(@"LayoutMode", @[@"compact", @"grid"], @"compact") isEqualToString:@"grid"];
    IDMiniControls = [IDStringOption(@"CategoryStyle", @[@"mini", @"standard"], @"mini") isEqualToString:@"mini"];
    IDRememberFilter = IDPref(@"RememberFilter", NO);
    IDContainedMode = IDPref(@"ContainedMode", YES);
    IDHideNative = IDPref(@"HideNativeList", NO) && IDContainedMode;
    IDSortMode = [@[@"recent", @"count", @"name"] indexOfObject:IDStringOption(@"SortMode", @[@"recent", @"count", @"name"], @"recent")];
    IDPanelTopOffset = IDNumberOption(@"PanelTopOffset", 220, 0, 420);
    IDPanelSideInset = IDNumberOption(@"PanelSideInset", 8, 8, 64);
    IDPanelMaxHeight = IDNumberOption(@"PanelMaxHeight", 360, 240, 600);
    IDOptionsRevision++;
}

@interface NCNotificationStructuredListViewController : UIViewController
@end

@interface NCNotificationViewController : UIViewController
- (instancetype)_initWithNotificationRequest:(id)request revealingAdditionalContentOnPresentation:(BOOL)reveal;
- (void)_executeDefaultAction:(BOOL)animated;
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

static BOOL IDOpenFeedback(NSString *message);

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
        return "/var/mobile/Library/Preferences/IslandDigest-0.2.8.startup-pending";
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

/* Runtime encodings use `c` for the BOOL declared as `bool` in iOS 17 headers,
 * while some builds expose the equivalent Objective-C spelling `B`.  Accept only
 * this one scalar variation; every other slot remains position- and kind-checked. */
static const char *IDUnqualifiedType(const char *type) {
    if (!type) return NULL;
    for (; *type && strchr("rnNoORV", *type); type++) {}
    return type;
}
static BOOL IDObjectEncoding(const char *type) {
    const char *unqualified = IDUnqualifiedType(type);
    return unqualified && *unqualified == '@';
}
static BOOL IDBoolEncoding(const char *type) {
    const char *unqualified = IDUnqualifiedType(type);
    return unqualified && (*unqualified == 'c' || *unqualified == 'B');
}
static NSString *IDActionABIDiagnostic(id owner, id component, Method *outMethod) {
    Class knownComponent = objc_getClass("NCNotificationMasterList");
    if (!component) return @"原生接收器缺失";
    if (!knownComponent || ![component isKindOfClass:knownComponent]) return @"原生组件类型不匹配";
    SEL selector = sel_registerName("notificationListComponent:requestsExecuteAction:forNotificationRequest:requestAuthentication:withParameters:completion:");
    Method method = class_getInstanceMethod(object_getClass(owner), selector);
    if (outMethod) *outMethod = method;
    if (!method) return @"原生动作选择器缺失";
    if (method_getNumberOfArguments(method) != 8) return @"原生动作 ABI 参数数量不匹配";
    const char *runtimeEncoding = method_getTypeEncoding(method);
    if (!runtimeEncoding || !*runtimeEncoding) return @"原生动作 ABI 编码缺失";
    char *returnType = method_copyReturnType(method);
    BOOL returnOK = IDUnqualifiedType(returnType) && strcmp(IDUnqualifiedType(returnType), "v") == 0;
    free(returnType);
    if (!returnOK) return @"原生动作 ABI 返回值不匹配";
    for (unsigned int index = 2; index <= 7; index++) {
        char *argumentType = method_copyArgumentType(method, index);
        BOOL valid = index == 5 ? IDBoolEncoding(argumentType) : IDObjectEncoding(argumentType);
        free(argumentType);
        if (!valid) return [NSString stringWithFormat:@"原生动作 ABI 第 %u 参数类型不匹配", index - 1];
    }
    return nil;
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

static BOOL IDCollectRequests(id object, NSMutableSet *requests, NSUInteger depth) {
    if (!object) return YES;
    if (depth > 8 || requests.count >= 4096) return NO;
    if (IDMethod(object_getClass(object), "sectionIdentifier", "@", 2)) {
        [requests addObject:object];
        return YES;
    }
    BOOL collection = [object isKindOfClass:NSDictionary.class] || [object isKindOfClass:NSArray.class]
        || [object isKindOfClass:NSSet.class] || [object isKindOfClass:NSOrderedSet.class];
    if (!collection) return NO;
    NSArray *items = [object isKindOfClass:NSDictionary.class] ? [object allValues] : IDItems(object);
    for (id item in items) if (!IDCollectRequests(item, requests, depth + 1)) return NO;
    return YES;
}

#include "IDClearSupport.inc"
#include "IDContainedSupport.inc"

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
@property(nonatomic, strong) UILabel *hint;
@property(nonatomic, strong) UIButton *clearButton;
@property(nonatomic, strong) UIScrollView *scroll;
@property(nonatomic, strong) UIView *stack;
@property(nonatomic, strong) UIScrollView *cardScroll;
@property(nonatomic, strong) UIStackView *cardStack;
@property(nonatomic, strong) UILabel *cardNotice;
@property(nonatomic, strong) UIView *cardTapLayer;
@property(nonatomic, strong) NSMutableArray<UIButton *> *cardTapButtons;
@property(nonatomic) NSUInteger builtCardCount;
- (BOOL)cardsReadyForNativeHide;
- (void)showCards;
- (void)dropCards;
- (CGFloat)categoryHeightForWidth:(CGFloat)width;
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
@property(nonatomic, copy) NSArray<NSDictionary *> *cards;
@property(nonatomic, copy) NSArray<NSDictionary *> *renderedCards;
@property(nonatomic) BOOL enumerationReliable;
@property(nonatomic, weak) UIView *hiddenNativeList;
@property(nonatomic) BOOL nativeWasHidden;
@property(nonatomic, strong) NSObject *nativeHideToken;
@property(nonatomic, copy) NSString *nativeHideReason;
- (void)restoreNativeList;
- (void)updateNativeListVisibility;
- (void)dropContainedContent;
- (BOOL)openCardRequest:(id)request;
@property(nonatomic, copy) NSString *selected;
@property(nonatomic, copy) NSString *renderedSelection;
@property(nonatomic, copy) NSString *desiredSelection;
@property(nonatomic) BOOL openingPending;
@property(nonatomic) BOOL dismissed;
@property(nonatomic) NSUInteger renderedRevision;
@property(nonatomic, strong) NSLayoutConstraint *overlayHeight;
@property(nonatomic, strong) NSLayoutConstraint *overlayTop;
@property(nonatomic, strong) NSLayoutConstraint *overlayLeft;
@property(nonatomic, strong) NSLayoutConstraint *overlayWidth;
@property(nonatomic) BOOL geometryUpdating;
@property(nonatomic, strong) NSMutableSet<NSString *> *filtered;
@property(nonatomic) BOOL overlay;
@property(nonatomic) BOOL inserted;
@property(nonatomic) BOOL updating;
@property(nonatomic) BOOL applyingFilter;
@property(nonatomic) BOOL filterReconcilePending;
@property(nonatomic) BOOL active;
@property(nonatomic) BOOL disabled;
@property(nonatomic) NSUInteger recoveryAttempts;
@property(nonatomic) IDClearGate clearGate;
@property(nonatomic, copy) NSArray *clearSnapshot;
@property(nonatomic, copy) NSString *clearScope;
@property(nonatomic, strong) id clearDelegate;
@property(nonatomic, strong) UIAlertController *clearAlert;
@property(nonatomic, weak) UIViewController *clearPresenterOwner;
@property(nonatomic, weak) UIWindow *clearWindow;
@property(nonatomic, strong) id clearMaster;
@property(nonatomic, copy) NSString *clearMessage;
@property(nonatomic, strong) id pendingActionRequest;
@property(nonatomic, strong) id pendingAction;

@property(nonatomic) NSUInteger clearRevision;
- (void)beginClear;
- (void)cancelClear;
- (void)updateClearButton;
- (void)confirmClear:(unsigned long)generation;
- (void)observeClear:(NSArray *)sent generation:(unsigned long)generation attempt:(NSUInteger)attempt;
- (void)refresh;
- (void)updatePanelGeometry;
- (void)beginAppearance;
- (void)endAppearance;
- (void)togglePinnedIdentifier:(NSString *)identifier;
- (void)selectIdentifier:(NSString *)identifier;
- (void)resetFilter;
- (void)disable;
- (void)recover;
@end

static char IDCoordinatorKey;
static char IDNativeHideOwnerKey;
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

static void IDRedactContainedNow(void) {
    void (^redact)(void) = ^{
        for (id owner in IDOwners.allObjects) [IDState(owner) dropContainedContent];
    };
    if (NSThread.isMainThread) redact(); else dispatch_async(dispatch_get_main_queue(), redact);
}

#include "IDDigestUI.inc"

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

#include "IDClearCoordinator.inc"
#include "IDContainedCoordinator.inc"

- (void)dealloc {
    UIView *native = _hiddenNativeList;
    BOOL original = _nativeWasHidden;
    NSObject *token = _nativeHideToken;
    // A delayed dealloc must not restore a view already claimed by another owner.
    void (^restore)(void) = ^{
        if (native && token && objc_getAssociatedObject(native, &IDNativeHideOwnerKey) == token) {
            native.hidden = original;
            objc_setAssociatedObject(native, &IDNativeHideOwnerKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        }
    };
    if (NSThread.isMainThread) restore();
    else if (native) dispatch_async(dispatch_get_main_queue(), restore);
}

- (NSArray<IDGroup *> *)snapshot {
    id master = IDRead(self.owner, "masterList");
    if (!master) master = IDRead(self.owner, "notificationMasterList");
    id sectionObjects = IDRead(master, "_notificationSectionListsForEnumeration");
    if (!sectionObjects) sectionObjects = IDRead(master, "sectionLists");
    NSArray *sections = IDItems(sectionObjects);
    NSMutableSet *requests = [NSMutableSet new];
    id allRequests = IDRead(master, "allNotificationRequests");
    BOOL reliable = master && IDCollection(allRequests);
    BOOL sectionsReliable = master && IDCollection(sectionObjects);
    reliable = IDCollectRequests(allRequests, requests, 0) && reliable;
    IDDiagnostics[@"masterSeen"] = @(master != nil);
    IDDiagnostics[@"sections"] = @(sections.count);
    IDDiagnostics[@"enumerationSupported"] = @(sectionObjects != nil || IDMethod(object_getClass(master), "allNotificationRequests", "@", 2));
    for (id section in sections) {
        id sectionRequests = IDRead(section, "allNotificationRequests");
        id filteredRequests = IDRead(section, "filteredNotificationRequests");
        if (!IDCollection(sectionRequests) || (self.filtered.count && !IDCollection(filteredRequests))) sectionsReliable = NO;
        sectionsReliable = IDCollectRequests(sectionRequests, requests, 0) && sectionsReliable;
        sectionsReliable = IDCollectRequests(filteredRequests, requests, 0) && sectionsReliable;
    }
    self.enumerationReliable = reliable || sectionsReliable;
    self.cards = IDContainedMode ? IDContainedSnapshot(requests, self.selected, self.owner) : @[];
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
            if (![name isKindOfClass:NSString.class] || !name.length) {
                Class proxyClass = objc_getClass("LSApplicationProxy");
                const char *lookup = "applicationProxyForIdentifier:";
                if (IDMethod(object_getClass(proxyClass), lookup, "@", 3)
                    && IDArgument(object_getClass(proxyClass), lookup, 2, "@")) {
                    id proxy = ((id (*)(id, SEL, id))objc_msgSend)(proxyClass, sel_registerName(lookup), identifier);
                    name = IDRead(proxy, "localizedName");
                }
            }
            group.name = [name isKindOfClass:NSString.class] && name.length ? name : @"未命名应用";
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
    // Export only bounded local app names for Settings; never diagnostic or notification text.
    NSMutableDictionary *names = [NSMutableDictionary new];
    id storedNames = IDPreferenceValue(@"AppNames");
    NSSet *pins = [NSSet setWithArray:IDPinnedIdentifiers()];
    if ([storedNames isKindOfClass:NSDictionary.class]) for (NSString *identifier in pins)
        if ([storedNames[identifier] isKindOfClass:NSString.class]) names[identifier] = storedNames[identifier];
    NSMutableArray *observed = [NSMutableArray new];
    for (NSString *identifier in [groups.allKeys sortedArrayUsingSelector:@selector(compare:)]) {
        if (observed.count >= 128) break;
        names[identifier] = groups[identifier].name;
        [observed addObject:identifier];
    }
    if (![storedNames isEqual:names]) IDWritePreference(@"AppNames", names);
    if (![IDPreferenceValue(@"ObservedApps") isEqual:observed]) IDWritePreference(@"ObservedApps", observed);
    return [groups.allValues sortedArrayUsingComparator:^NSComparisonResult(IDGroup *first, IDGroup *second) {
        return (NSComparisonResult)IDExperienceOrder([pins containsObject:first.identifier], [pins containsObject:second.identifier],
            (int)IDSortMode, first.count, second.count, first.latest.timeIntervalSince1970, second.latest.timeIntervalSince1970,
            (int)[first.name localizedStandardCompare:second.name], (int)[first.identifier compare:second.identifier]);
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
                if (!IDContainedMode && self.selected && !group.hasCriticalAlert && ![group.identifier isEqualToString:self.selected]) {
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
    // Do not hide native notifications until newly built card views have laid out.
    [(IDDigestView *)self.panel.view showGroups:self.groups selected:self.selected];
    [(IDDigestView *)self.panel.view showCards];
    self.renderedCards = self.cards;
    self.renderedSelection = self.selected;
    self.renderedRevision = IDOptionsRevision;
    [self updatePanelGeometry];
    [self.owner.view layoutIfNeeded];
    [self.panel.view layoutIfNeeded];
    [self updateNativeListVisibility];
    [self updateClearButton];
}

- (void)updatePanelGeometry {
    if (self.geometryUpdating || !self.panel.isViewLoaded) return;
    self.geometryUpdating = YES;
    @try {
        UIView *owner = self.owner.view;
        UIWindow *window = owner.window;
        // Window safe bounds are independent of the notification owner's additional
        // insets. Intersect after conversion: owner may not fill the window.
        CGRect safe = window ? UIEdgeInsetsInsetRect(window.bounds, window.safeAreaInsets)
                             : UIEdgeInsetsInsetRect(owner.bounds, owner.safeAreaInsets);
        if (window) safe = [owner convertRect:safe fromView:window];
        safe = CGRectIntersection(owner.bounds, safe);
        if (CGRectIsNull(safe) || CGRectIsEmpty(safe)) safe = CGRectZero;
        IDPanelRect room = IDLayoutPanel(CGRectGetMinX(safe), CGRectGetMinY(safe),
            CGRectGetMaxX(safe), CGRectGetMaxY(safe), IDPanelTopOffset, IDPanelSideInset,
            IDPanelMaxHeight, IDPanelMaxHeight);
        CGFloat preferred = [(IDDigestView *)self.panel.view sizeThatFits:CGSizeMake(room.width, 0)].height;
        IDPanelRect rect = IDLayoutPanel(CGRectGetMinX(safe), CGRectGetMinY(safe),
            CGRectGetMaxX(safe), CGRectGetMaxY(safe), IDPanelTopOffset, IDPanelSideInset,
            preferred, IDPanelMaxHeight);
        CGSize size = CGSizeMake(rect.width, rect.height);
        if (!CGSizeEqualToSize(self.panel.preferredContentSize, size)) self.panel.preferredContentSize = size;
        if (self.overlay) {
            if (fabs(self.overlayHeight.constant-rect.height)>0.5) self.overlayHeight.constant=rect.height;
            if (fabs(self.overlayTop.constant-rect.y)>0.5) self.overlayTop.constant=rect.y;
            if (fabs(self.overlayLeft.constant-rect.x)>0.5) self.overlayLeft.constant=rect.x;
            if (fabs(self.overlayWidth.constant-rect.width)>0.5) self.overlayWidth.constant=rect.width;
        }
    } @finally { self.geometryUpdating = NO; }
}

- (void)beginAppearance {
    if (self.active) return;
    self.dismissed = NO;
    self.active = YES;
    self.openingPending = YES;
    id desired = IDRememberFilter ? IDPreferenceValue(@"DesiredApp") : nil;
    self.desiredSelection = [desired isKindOfClass:NSString.class] && [desired length] ? desired : nil;
}

- (void)endAppearance {
    [self dropContainedContent];
    [self cancelClear];
    self.active = NO;
    self.dismissed = YES;
    self.openingPending = NO;
    self.selected = nil; // actual native state must always be restored on disappearance.
    if (!IDRememberFilter) { self.desiredSelection = nil; IDWritePreference(@"DesiredApp", nil); }
    [self resetFilter];
}

- (void)togglePinnedIdentifier:(NSString *)identifier {
    if (!identifier.length || !self.active || self.disabled || self.updating) return;
    NSMutableArray *pins = IDPinnedIdentifiers().mutableCopy;
    BOOL pinned = [pins containsObject:identifier];
    if (pinned) [pins removeObject:identifier]; else if (pins.count < 128) [pins addObject:identifier]; else return;
    IDWritePreference(@"PinnedApps", pins);
    id savedNames = IDPreferenceValue(@"AppNames");
    NSMutableDictionary *names = [savedNames isKindOfClass:NSDictionary.class] ? [savedNames mutableCopy] : [NSMutableDictionary new];
    for (IDGroup *group in self.groups) if ([group.identifier isEqualToString:identifier]) names[identifier] = group.name;
    IDWritePreference(@"AppNames", names);
    IDOptionsRevision++;
    [self refresh];
    UIAccessibilityPostNotification(UIAccessibilityAnnouncementNotification, pinned ? @"已取消置顶" : @"已置顶应用");
    [[[UIImpactFeedbackGenerator alloc] initWithStyle:UIImpactFeedbackStyleLight] impactOccurred];
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
    [self dropContainedContent];
    [self cancelClear];
    if (self.recoveryAttempts >= 3) return;
    self.recoveryAttempts += 1;
    self.selected = nil;
    if (![self applyFilter]) IDLog("filter restoration pending");
    if (self.inserted && self.overlay) {
        [self.panel willMoveToParentViewController:nil];
        [self.panel.view removeFromSuperview];
        [self.panel removeFromParentViewController];
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
    [self cancelClear];
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
    [self cancelClear]; self.clearMessage = nil;
    self.updating = YES;
    @try {
        self.openingPending = NO; // A user tap wins over an already queued first-appearance refresh.
        self.selected = [self.selected isEqualToString:identifier] ? nil : identifier;
        if (IDContainedMode) self.groups = [self snapshot];
        self.desiredSelection = self.selected;
        IDWritePreference(@"DesiredApp", IDRememberFilter ? self.desiredSelection : nil);
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
    @try {
        if (_clearGate.pending && (self.clearRevision != IDOptionsRevision || self.clearDelegate != IDRead(self.owner, "delegate")
            || self.clearMaster != IDRead(self.owner, "masterList") || ![self clearAuthenticated])) [self cancelClear];
    } @catch (NSException *exception) { [self cancelClear]; }
    [self updateClearButton];
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
        BOOL unchanged = self.groups.count == groups.count && [self.cards isEqual:self.renderedCards];
        BOOL selectionExists = !self.selected;
        for (NSUInteger index = 0; index < groups.count; index++) {
            IDGroup *group = groups[index];
            if ([group.identifier isEqualToString:self.selected]) selectionExists = YES;
            if (unchanged) {
                IDGroup *previous = self.groups[index];
                unchanged = [previous.identifier isEqualToString:group.identifier]
                    && previous.count == group.count && [previous.name isEqualToString:group.name]
                    && [previous.latest isEqual:group.latest] && previous.icon == group.icon
                    && previous.hasCriticalAlert == group.hasCriticalAlert;
            }
        }
        self.groups = groups;
        self.recoveryAttempts = 0;
        IDDiagnostics[@"apps"] = @(groups.count);
        NSUInteger requestCount = 0;
        for (IDGroup *group in groups) requestCount += group.count;
        IDDiagnostics[@"requests"] = @(requestCount);
        IDDiagnostics[@"filterAvailable"] = @(IDFilteringReady);
        if (self.openingPending) {
            self.openingPending = NO;
            self.selected = IDRememberFilter ? self.desiredSelection : nil;
            if (IDContainedMode) self.groups = groups = [self snapshot];
            selectionExists = self.selected == nil;
            for (IDGroup *group in groups) if ([group.identifier isEqualToString:self.selected]) selectionExists = YES;
        }
        BOOL desiredExists = self.desiredSelection == nil;
        for (IDGroup *group in groups) if ([group.identifier isEqualToString:self.desiredSelection]) desiredExists = YES;
        if (!desiredExists) { self.desiredSelection = nil; IDWritePreference(@"DesiredApp", nil); }
        if (!selectionExists) {
            [self cancelClear]; self.selected = nil;
            if (IDContainedMode) self.groups = groups = [self snapshot];
        }
        if (![self applyFilter]) {
            [self disable];
            return;
        }
        if (groups.count == 0 && !IDContainedMode) {
            if (self.inserted) [self recover];
            BOOL supported = [IDDiagnostics[@"enumerationSupported"] boolValue];
            IDStatus(supported ? @"waiting-notifications" : @"enumeration-unavailable",
                supported ? @"控制器已出现；目前未读到通知" : @"控制器已出现，但当前系统的通知枚举接口不可用");
            return;
        }
        if (IDContainedMode && self.inserted && !self.overlay) {
            [self recover];
            if (self.inserted) return;
            self.container = nil; self.configuration = nil; self.panel = nil;
            // recover drops strings; refill only from the current native snapshot.
            self.groups = groups = [self snapshot];
        }
        if (!self.container && !self.inserted && (IDContainedMode || !IDSupplementaryReady(self.owner))) {
            // Public UIKit fallback: no notification cells, requests or delegates replaced.
            self.panel = [IDDigestController new];
            self.panel.coordinator = self;
            [self.owner addChildViewController:self.panel];
            UIView *view = self.panel.view;
            view.translatesAutoresizingMaskIntoConstraints = NO;
            [self.owner.view addSubview:view];
            [NSLayoutConstraint activateConstraints:@[
                (self.overlayTop = [view.topAnchor constraintEqualToAnchor:self.owner.view.topAnchor constant:0]),
                (self.overlayLeft = [view.leadingAnchor constraintEqualToAnchor:self.owner.view.leadingAnchor constant:8]),
                (self.overlayWidth = [view.widthAnchor constraintEqualToConstant:1]),
                (self.overlayHeight = [view.heightAnchor constraintEqualToConstant:112])
            ]];
            [self.panel didMoveToParentViewController:self.owner];
            self.overlay = YES;
            self.inserted = YES;
            // The detached-view cache is not proof that this new view has cards.
            [self render];
        }
        if (self.overlay) {
            IDDigestView *digest = (IDDigestView *)self.panel.view;
            if ((IDContainedMode && digest.builtCardCount != self.cards.count) || !unchanged || self.renderedRevision != IDOptionsRevision || !((!self.selected && !self.renderedSelection) || [self.selected isEqualToString:self.renderedSelection])) [self render];
            [self updatePanelGeometry];
            [self.owner.view layoutIfNeeded];
            [self updateNativeListVisibility];
            IDDiagnostics[@"presentation"] = @"UIKit-window-bounded";
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
            self.renderedRevision = IDOptionsRevision;
            [self updatePanelGeometry];
        } else if (self.renderedRevision != IDOptionsRevision || !unchanged || !selectionExists
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
static BOOL IDOpenFeedback(NSString *message) {
    // Keep action diagnostics in Settings, not on notification cards or the panel.
    IDStatus(@"card-tap-feedback", message);
    return YES;
}

- (BOOL)openCardRequest:(id)request {
    if (!NSThread.isMainThread || !request || !self.owner.view.window || self.owner.view.hidden
        || self.owner.view.window.hidden || !UIApplication.sharedApplication.protectedDataAvailable) {
        IDOpenFeedback(@"打开诊断：点击已收到；系统上下文不可用");
        return NO;
    }
    IDOpenFeedback(@"打开诊断：点击已收到；正在检查原生动作");
    SEL authSEL = sel_registerName("isDeviceAuthenticated");
    if (!IDMethod(object_getClass(self.owner), "isDeviceAuthenticated", "B", 2)
        || !((BOOL (*)(id, SEL))objc_msgSend)(self.owner, authSEL)) {
        IDOpenFeedback(@"打开诊断：设备认证不可用");
        return NO;
    }
    id action = IDRead(request, "defaultAction");
    if (!action) {
        IDOpenFeedback(@"打开诊断：该通知没有可用原生打开动作");
        return NO;
    }

    IDOpenFeedback(@"打开诊断：原生动作已找到");
    IDStatus(@"card-tap-native-action-found", @"defaultAction 与原始 request 均存在；未生成 URL");

    /*
     * iOS 17 declares this callback on NCNotificationStructuredListViewController,
     * but its first argument is the actual NCNotificationListComponent.  The
     * supplementary-view container is not that component and silently does nothing.
     * Resolve masterList, require its exact runtime class, and check the complete ABI
     * before objc_msgSend. No URL is manufactured and no notification text is logged.
     */
    const char *selectorName = "notificationListComponent:requestsExecuteAction:forNotificationRequest:requestAuthentication:withParameters:completion:";
    id component = IDRead(self.owner, "masterList");
    NSString *abiDiagnostic = IDActionABIDiagnostic(self.owner, component, NULL);
    if (abiDiagnostic) {
        IDOpenFeedback([NSString stringWithFormat:@"打开诊断：%@", abiDiagnostic]);
        IDStatus(@"card-tap-action-rejected", abiDiagnostic);
        return NO;
    }
    IDOpenFeedback(@"打开诊断：原生接收器已找到；动作 ABI 已核验");
    IDStatus(@"card-tap-native-receiver-verified", @"NCNotificationMasterList（含已知子类）与运行时回调 ABI 已核验");
    IDOpenFeedback(@"打开诊断：动作与原生组件已找到；正在派发");
    self.pendingActionRequest = request;
    self.pendingAction = action;
    __weak IDCoordinator *weakSelf = self;
    void (^completion)(void) = ^{
        dispatch_async(dispatch_get_main_queue(), ^{
            IDCoordinator *strongSelf = weakSelf;
            if (strongSelf.pendingActionRequest == request) {
                strongSelf.pendingActionRequest = nil;
                strongSelf.pendingAction = nil;
                IDOpenFeedback(@"打开诊断：原生动作回调已到达");
            }
        });
    };
    @try {
        ((void (*)(id, SEL, id, id, id, BOOL, id, id))objc_msgSend)(self.owner,
            sel_registerName(selectorName), component, action, request, YES, nil, completion);
    } @catch (NSException *exception) {
        self.pendingActionRequest = nil;
        self.pendingAction = nil;
        IDOpenFeedback(@"打开诊断：原生派发抛出异常");
        return NO;
    }
    // This means the verified callback was invoked, not that the app launched.
    IDOpenFeedback(@"打开诊断：派发已接受；等待原生回调");
    IDStatus(@"card-tap-dispatched", @"已向原生 notificationListComponent 派发；等待回调（不等同于 App 已启动）");
    return YES;
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
    [state beginAppearance];
    IDLog("owner appeared");
    IDScheduleRefresh();
}
- (void)viewDidLayoutSubviews {
    %orig;
    IDCoordinator *state = IDEnsureState(self);
    if (self.view.window && !state.active && !state.dismissed) {
        [state beginAppearance];
        IDDiagnostics[@"visibleOwnerSeen"] = @YES;
        IDScheduleRefresh(); // Never insert UI synchronously from a layout callback.
    }
    if (state.active) {
        [state updatePanelGeometry];
        [state updateNativeListVisibility];
    }
}
- (void)viewWillDisappear:(BOOL)animated {
    IDCoordinator *state = IDState(self);
    @try { [state endAppearance]; }
    @catch (NSException *exception) { [state disable]; }
    %orig;
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

%group IDPrivacy
%hook NCNotificationStructuredListViewController
- (void)setDeviceAuthenticated:(BOOL)authenticated {
    // Drop previous strings before native authentication state changes.
    if (!authenticated) IDRedactContainedNow();
    %orig;
    IDScheduleRefresh();
}
%end
%end

%group IDFilter
%hook NCNotificationStructuredListViewController
- (BOOL)notificationMasterList:(id)master shouldFilterNotificationRequest:(id)request {
    BOOL original = %orig;
    NSString *selected = IDContainedMode ? nil : IDState(self).selected;
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
    if (IDMethod(owner, "setDeviceAuthenticated:", "v", 3)
        && IDArgument(owner, "setDeviceAuthenticated:", 2, "B")) {
        %init(IDPrivacy);
    }
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
            [state cancelClear]; state.clearMessage = nil;
            if (!IDContainedMode || !IDEnabled || !IDGrouping || !state.active) [state dropContainedContent];
            if (!IDHideNative) [state restoreNativeList];
            if (retry) {
                state.recoveryAttempts = 0;
                [state recover];
                if (state.inserted || state.filtered.count) continue;
                state.disabled = NO;
                state.recoveryAttempts = 0;
                state.panel = nil;
                state.container = nil;
                state.configuration = nil;
            }
            if (!IDRememberFilter || !IDEnabled || !IDGrouping) {
                state.desiredSelection = nil; IDWritePreference(@"DesiredApp", nil);
            }
            if (!IDEnabled || !IDGrouping) { state.recoveryAttempts = 0; [state resetFilter]; [state recover]; }
            // Snapshot/sort first; a direct render here would briefly use the old order.
            else if (!state.active) { [state resetFilter]; }

        }
        if (!IDEnabled || !IDGrouping) IDStatus(@"disabled-by-preference", @"已按设置关闭分组");
        IDScheduleRefresh();
    });
}
%ctor {
    dispatch_async(dispatch_get_main_queue(), ^{
        IDLoadOptions();
        [NSNotificationCenter.defaultCenter addObserverForName:UIApplicationProtectedDataWillBecomeUnavailable
            object:nil queue:nil usingBlock:^(NSNotification *notification) { IDRedactContainedNow(); }];
        [NSNotificationCenter.defaultCenter addObserverForName:UIApplicationProtectedDataDidBecomeAvailable
            object:nil queue:nil usingBlock:^(NSNotification *notification) { IDScheduleRefresh(); }];
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
