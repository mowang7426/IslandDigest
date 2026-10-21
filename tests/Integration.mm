#define ID_REGRESSION 1
#define ID_INTEGRATION 1
#include "../.theos/regression/Core.mm"
#include <dlfcn.h>

extern "C" int IDRunRegression(void) {
    @autoreleasepool {
        @try {
            void *framework = dlopen("/System/Library/PrivateFrameworks/UserNotificationsUIKit.framework/UserNotificationsUIKit", RTLD_NOW);
            if (!framework) {
                fprintf(stderr, "FAIL framework load: %s\n", dlerror());
                return 1;
            }
            Class configurationClass = objc_getClass("NCNotificationListSupplementaryViewConfiguration");
            id configuration = [(NCNotificationListSupplementaryViewConfiguration *)[configurationClass alloc]
                _initWithGroupingIdentifier:IDText("com.local.islanddigest.test") groupName:IDText("IslandDigest Test")
                defaultAction:nil supplementaryActions:@[] auxiliaryOptionsSummaryText:nil
                auxiliaryOptionActions:@[] tintColor:nil textColor:nil materialRecipe:0
                containsUnmanagedContent:YES preventsUserDismissal:YES];
            if (!configuration) return 1;
            printf("PASS real supplementary configuration initialization\n");
            Class sectionClass = objc_getClass("NCNotificationListSupplementaryViewsSection");
            id section = [[sectionClass alloc] init];
            IDDigestController *panel = [IDDigestController new];
            [panel loadViewIfNeeded];
            printf("STEP insert production panel into real supplementary section\n");
            fflush(stdout);
            ((void (*)(id, SEL, id, id))objc_msgSend)(section,
                sel_registerName("insertSupplementaryViewController:withConfiguration:"), panel, configuration);
            printf("PASS real supplementary section insertion\n");
            ((void (*)(id, SEL, id))objc_msgSend)(section,
                sel_registerName("removeSupplementaryViewController:"), panel);
            printf("PASS real supplementary section removal\n");
            IDStartupMarkerOverride = [NSTemporaryDirectory() stringByAppendingPathComponent:
                [NSString stringWithFormat:IDText("IslandDigest-integration-%@"), NSUUID.UUID.UUIDString]];
            if (IDAcquireStartupMarker(IDStartupMarkerPath()) != 0) return 1;
            Class notificationClass = objc_getClass("NCNotificationStructuredListViewController");
            Method loadMethod = class_getInstanceMethod(notificationClass, @selector(viewDidLoad));
            IMP originalLoad = method_getImplementation(loadMethod);
            IDInstallHooks();
            if (IDHooksInstalled || method_getImplementation(loadMethod) != originalLoad) return 1;
            printf("PASS unfinished startup skips all production hooks\n");
            unlink(IDStartupMarkerPath());
            IDInstallHooks();
            if (!IDHooksInstalled) {
                fprintf(stderr, "FAIL actual hook ABI validation\n");
                return 1;
            }
            printf("PASS production hook installation in isolated process\n");
            Class ownerClass = objc_getClass("NCNotificationStructuredListViewController");
            printf("STEP initialize real notification owner\n");
            fflush(stdout);
            UIViewController *owner = [[ownerClass alloc] init];
            [owner loadViewIfNeeded];
            printf("PASS real notification owner view load\n");
            [owner viewDidAppear:NO];
            IDCoordinator *state = IDState(owner);
            [state refresh];
            printf("PASS real notification owner appeared and refreshed; groups=%lu\n",
                (unsigned long)state.groups.count);
            Class requestClass = objc_getClass("NCNotificationRequest");
            id incoming = IDRead(IDRead(owner, "masterList"), "incomingSectionList");
            id destination = IDRead(incoming, "sectionListDestination");
            SEL factory = sel_registerName("notificationRequestWithSectionId:notificationId:requestDestination:");
            id first = ((id (*)(id, SEL, id, id, id))objc_msgSend)(requestClass, factory,
                IDText("com.local.islanddigest.test.a"), IDText("ID-INTEGRATION-A"), destination);
            id second = ((id (*)(id, SEL, id, id, id))objc_msgSend)(requestClass, factory,
                IDText("com.local.islanddigest.test.b"), IDText("ID-INTEGRATION-B"), destination);
            printf("STEP insert synthetic requests into isolated real notification owner\n");
            fflush(stdout);
            ((void (*)(id, SEL, id))objc_msgSend)(owner, sel_registerName("insertNotificationRequest:"), first);
            ((void (*)(id, SEL, id))objc_msgSend)(owner, sel_registerName("insertNotificationRequest:"), second);
            [state refresh];
            printf("STEP real notifications inserted; groups=%lu disabled=%d\n",
                (unsigned long)state.groups.count, state.disabled);
            fflush(stdout);
            if (state.groups.count != 2 || !state.inserted || state.disabled) {
                fprintf(stderr, "FAIL real populated list did not create the digest\n");
                return 1;
            }
            printf("PASS real populated list digest insertion\n");
            [state selectIdentifier:IDText("com.local.islanddigest.test.a")];
            if (state.disabled || state.filtered.count != 1) return 1;
            printf("PASS real native notification filtering\n");
            [state refresh];
            if (state.groups.count != 2 || state.filtered.count != 1) {
                fprintf(stderr, "FAIL filtered real requests disappeared from snapshot; groups=%lu filters=%lu\n",
                    (unsigned long)state.groups.count, (unsigned long)state.filtered.count);
                return 1;
            }
            printf("PASS real filtered snapshot remains stable\n");
            for (NSUInteger iteration = 0; iteration < 30; iteration++) [state refresh];
            if (state.filtered.count != 1 || state.groups.count != 2) return 1;
            printf("PASS thirty real-list refreshes preserve selected filter\n");
            [owner viewWillDisappear:NO];
            if (state.filtered.count != 0) return 1;
            printf("PASS real notification owner disappeared\n");
            ((void (*)(id, SEL, id))objc_msgSend)(owner, sel_registerName("removeNotificationRequest:"), first);
            ((void (*)(id, SEL, id))objc_msgSend)(owner, sel_registerName("removeNotificationRequest:"), second);
            [owner viewDidAppear:NO];
            [state refresh];
            if (state.groups.count != 0 || state.inserted) return 1;
            printf("PASS real notification removal clears digest\n");
            unlink(IDStartupMarkerPath());
            return 0;
        } @catch (NSException *exception) {
            fprintf(stderr, "FAIL real supplementary integration %s: %s\n",
                exception.name.UTF8String, exception.reason.UTF8String);
            return 1;
        }
    }
}
