#define ID_REGRESSION 1
#include "../.theos/regression/Core.mm"

@implementation NCNotificationListSupplementaryViewConfiguration
- (instancetype)_initWithGroupingIdentifier:(NSString *)identifier groupName:(NSString *)name
    defaultAction:(id)action supplementaryActions:(NSArray *)actions
    auxiliaryOptionsSummaryText:(NSString *)summary auxiliaryOptionActions:(NSArray *)options
    tintColor:(UIColor *)tint textColor:(UIColor *)text materialRecipe:(NSInteger)recipe
    containsUnmanagedContent:(BOOL)unmanaged preventsUserDismissal:(BOOL)prevents {
    return [super init];
}
@end

@interface IDTestRequest : NSObject
@property(nonatomic, copy) NSString *sectionIdentifier;
@property(nonatomic, strong) NSDate *timestamp;
@property(nonatomic) BOOL isCriticalAlert;
@end
@implementation IDTestRequest
@end

@interface IDTestSection : NSObject
@property(nonatomic, strong) NSArray *allNotificationRequests;
@property(nonatomic, strong) id filteredNotificationRequests;
@end
@implementation IDTestSection
@end

@interface IDTestMaster : NSObject
@property(nonatomic, strong) NSArray *sections;
@end
@implementation IDTestMaster
- (NSArray *)_notificationSectionListsForEnumeration {
    return self.sections;
}
@end

@interface IDTestContainer : NSObject
@property(nonatomic) NSUInteger inserts;
@property(nonatomic) NSUInteger removals;
@property(nonatomic) BOOL throwsOnInsert;
@property(nonatomic) NSUInteger removalFailures;
@end
@implementation IDTestContainer
- (void)insertSupplementaryViewController:(id)controller withConfiguration:(id)configuration {
    self.inserts += 1;
    if (self.throwsOnInsert) {
        @throw [NSException exceptionWithName:IDText("TestInsert") reason:nil userInfo:nil];
    }
}
- (void)removeSupplementaryViewController:(id)controller {
    self.removals += 1;
    if (self.removalFailures) {
        self.removalFailures -= 1;
        @throw [NSException exceptionWithName:IDText("TestRemoval") reason:nil userInfo:nil];
    }
}
@end

@interface IDTestOwner : UIViewController
@property(nonatomic, strong) IDTestMaster *masterList;
@property(nonatomic, strong) IDTestContainer *testContainer;
@property(nonatomic, strong) NSMutableSet *hidden;
@property(nonatomic, copy) NSString *throwOnIdentifier;
@property(nonatomic) NSUInteger filterCalls;
@property(nonatomic) NSUInteger restorationFailures;
@property(nonatomic) NSUInteger restorationReentryBudget;
@property(nonatomic) NSUInteger filterReentryBudget;
@end
@implementation IDTestOwner
- (void)loadView {
    self.view = [[UIView alloc] initWithFrame:CGRectMake(0, 0, 390, 600)];
}
- (id)_sectionSettingsForSectionIdentifier:(NSString *)identifier {
    return nil;
}
- (id)insertSupplementaryViewsContainerAtListPosition:(NSUInteger)position
    identifier:(NSString *)identifier withDescription:(NSString *)description {
    return self.testContainer;
}
- (void)toggleFilteringForSectionIdentifier:(NSString *)identifier shouldFilter:(BOOL)filter {
    self.filterCalls += 1;
    if (!filter && self.restorationReentryBudget) {
        self.restorationReentryBudget -= 1;
        [IDState(self) resetFilter];
    }
    if (filter && self.filterReentryBudget) {
        self.filterReentryBudget -= 1;
        [IDState(self) resetFilter];
    }
    if (!filter && self.restorationFailures) {
        self.restorationFailures -= 1;
        @throw [NSException exceptionWithName:IDText("TestRestore") reason:nil userInfo:nil];
    }
    if (filter) [self.hidden addObject:identifier];
    else [self.hidden removeObject:identifier];
    if (filter && [identifier isEqualToString:self.throwOnIdentifier]) {
        @throw [NSException exceptionWithName:IDText("TestFilter") reason:nil userInfo:nil];
    }
}
@end

static void IDCheck(BOOL condition, const char *message) {
    if (!condition) {
        @throw [NSException exceptionWithName:IDText("RegressionFailure")
            reason:IDText(message) userInfo:nil];
    }
    printf("PASS %s\n", message);
}

static IDTestRequest *IDRequest(const char *identifier, NSTimeInterval timestamp) {
    IDTestRequest *request = [IDTestRequest new];
    request.sectionIdentifier = IDText(identifier);
    request.timestamp = [NSDate dateWithTimeIntervalSince1970:timestamp];
    return request;
}

static IDCoordinator *IDFixture(IDTestOwner **output, NSArray *requests) {
    IDTestSection *section = [IDTestSection new];
    section.allNotificationRequests = requests;
    section.filteredNotificationRequests = requests;
    IDTestOwner *owner = [IDTestOwner new];
    owner.hidden = [NSMutableSet new];
    owner.testContainer = [IDTestContainer new];
    owner.masterList = [IDTestMaster new];
    owner.masterList.sections = @[section];
    [owner loadViewIfNeeded];
    IDCoordinator *state = [IDCoordinator new];
    state.owner = owner;
    objc_setAssociatedObject(owner, &IDCoordinatorKey, state, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    *output = owner;
    return state;
}

extern "C" int IDRunRegression(void) {
    @autoreleasepool {
        @try {
            NSString *marker = [NSTemporaryDirectory() stringByAppendingPathComponent:
                [NSString stringWithFormat:IDText("IslandDigest-test-%@"), NSUUID.UUID.UUIDString]];
            IDCheck(IDAcquireStartupMarker(marker.fileSystemRepresentation) == 0,
                "fresh startup acquires its persistent marker");
            IDCheck(IDAcquireStartupMarker(marker.fileSystemRepresentation) == 1,
                "unfinished startup is detected before hook installation");
            unlink(marker.fileSystemRepresentation);
            IDFilteringReady = YES;
            IDTestOwner *owner = nil;
            NSArray *requests = @[IDRequest("app.a", 1), IDRequest("app.b", 2), IDRequest("app.b", 3)];
            IDCoordinator *state = IDFixture(&owner, requests);
            [state refresh];
            IDCheck(owner.testContainer.inserts == 0, "startup does not insert a panel before appearance");
            state.active = YES;
            [state refresh];
            IDCheck(owner.testContainer.inserts == 1 && state.inserted, "visible populated list inserts once");
            IDCheck(state.groups.count == 2 && state.groups[0].count == 2
                && [state.groups[0].identifier isEqualToString:IDText("app.b")],
                "snapshot deduplicates requests and sorts latest first");
            IDTestSection *section = owner.masterList.sections[0];
            section.allNotificationRequests = @[requests[0]];
            section.filteredNotificationRequests = @{IDText("app.b"): @[requests[1], requests[2]]};
            IDCheck([state snapshot].count == 2,
                "dictionary-backed filtered requests remain in the notification snapshot");
            section.allNotificationRequests = requests;
            section.filteredNotificationRequests = @[];
            for (NSUInteger iteration = 0; iteration < 30; iteration++) [state refresh];
            IDCheck(owner.testContainer.inserts == 1, "repeated refreshes do not reinsert the panel");
            [state selectIdentifier:IDText("app.a")];
            IDCheck([owner.hidden isEqualToSet:[NSSet setWithObject:IDText("app.b")]],
                "selection filters only the other application");
            [state selectIdentifier:IDText("app.a")];
            IDCheck(owner.hidden.count == 0 && !state.selected, "second tap restores all notifications");
            [state selectIdentifier:IDText("app.b")];
            state.active = NO;
            [state resetFilter];
            IDCheck(owner.hidden.count == 0 && !state.selected, "disappearance restores temporary filters");
            state = IDFixture(&owner, requests);
            state.active = YES;
            [state refresh];
            [state selectIdentifier:IDText("app.a")];
            NSUInteger previousCalls = owner.filterCalls;
            owner.restorationReentryBudget = 8;
            state.active = NO;
            [state resetFilter];
            IDCheck(owner.filterCalls == previousCalls + 1 && owner.hidden.count == 0
                && state.filtered.count == 0 && !state.disabled,
                "synchronous restoration callbacks cannot recursively restore the same filter");
            state = IDFixture(&owner, requests);
            state.active = YES;
            [state refresh];
            owner.filterReentryBudget = 1;
            [state selectIdentifier:IDText("app.a")];
            IDCheck(owner.hidden.count == 0 && state.filtered.count == 0
                && !state.selected && !state.disabled && owner.filterCalls == 2,
                "reset during filter application reconciles after the system call returns");
            state = IDFixture(&owner, @[]);
            state.active = YES;
            [state refresh];
            IDCheck(owner.testContainer.inserts == 0, "empty notification list does not insert a panel");
            state = IDFixture(&owner, requests);
            state.active = YES;
            owner.testContainer.throwsOnInsert = YES;
            [state refresh];
            for (NSUInteger iteration = 0; iteration < 30; iteration++) [state refresh];
            IDCheck(state.disabled && owner.testContainer.inserts == 1
                && owner.testContainer.removals == 1, "insertion exception disables panel without retry loop");
            state = IDFixture(&owner, requests);
            state.active = YES;
            [state refresh];
            owner.throwOnIdentifier = IDText("app.b");
            [state selectIdentifier:IDText("app.a")];
            IDCheck(state.disabled && owner.hidden.count == 0 && state.filtered.count == 0,
                "partially applied filter exception restores notifications and disables panel");
            state = IDFixture(&owner, requests);
            state.active = YES;
            [state refresh];
            [state selectIdentifier:IDText("app.a")];
            owner.restorationFailures = 1;
            owner.testContainer.removalFailures = 1;
            [state disable];
            IDCheck(state.filtered.count == 1 && state.inserted,
                "failed cleanup preserves outstanding restoration and removal records");
            [state refresh];
            IDCheck(state.filtered.count == 0 && !state.inserted && owner.hidden.count == 0,
                "next external event retries failed cleanup without reinserting panel");
            state = IDFixture(&owner, requests);
            state.active = YES;
            [state refresh];
            owner.testContainer.removalFailures = 100;
            [state disable];
            for (NSUInteger iteration = 0; iteration < 30; iteration++) [state refresh];
            IDCheck(owner.testContainer.removals == 3 && state.disabled && state.inserted,
                "persistent cleanup failures stop after three attempts");
            state = IDFixture(&owner, requests);
            ((IDTestRequest *)requests[1]).isCriticalAlert = YES;
            state.active = YES;
            [state refresh];
            [state selectIdentifier:IDText("app.a")];
            IDCheck(owner.hidden.count == 0, "applications containing critical alerts stay visible");
            IDDigestView *view = [[IDDigestView alloc] initWithFrame:CGRectMake(0, 0, 350, 112)];
            [view showGroups:state.groups selected:nil];
            [view layoutIfNeeded];
            IDCheck(view.stack.arrangedSubviews.count == 3 && view.scroll.contentSize.width > 0,
                "real UIKit digest creates and lays out all application buttons");
            return 0;
        } @catch (NSException *exception) {
            fprintf(stderr, "FAIL %s: %s\n", exception.name.UTF8String, exception.reason.UTF8String);
            return 1;
        }
    }
}
