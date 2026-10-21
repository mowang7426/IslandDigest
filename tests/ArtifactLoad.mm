#import <UIKit/UIKit.h>
#import <objc/runtime.h>
#include <dlfcn.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

extern "C" int IDRunRegression(void) {
    @autoreleasepool {
        printf("STEP artifact test main thread=%d\n", pthread_main_np());
        fflush(stdout);
        if (!pthread_main_np()) return 1;
        void *framework = dlopen("/System/Library/PrivateFrameworks/UserNotificationsUIKit.framework/UserNotificationsUIKit", RTLD_NOW);
        if (!framework) {
            fprintf(stderr, "FAIL framework: %s\n", dlerror());
            return 1;
        }
        Class ownerClass = objc_getClass("NCNotificationStructuredListViewController");
        Method loadMethod = class_getInstanceMethod(ownerClass, @selector(viewDidLoad));
        IMP original = method_getImplementation(loadMethod);
        const char *artifact = getenv("ID_ARTIFACT_PATH");
        if (!artifact) return 1;
        printf("STEP dlopen packaged dylib\n");
        fflush(stdout);
        void *image = dlopen(artifact, RTLD_NOW);
        if (!image) {
            fprintf(stderr, "FAIL packaged dylib: %s\n", dlerror());
            return 1;
        }
        printf("PASS packaged dylib loaded\n");
        fflush(stdout);
        NSDate *deadline = [NSDate dateWithTimeIntervalSinceNow:2];
        while ([deadline timeIntervalSinceNow] > 0) {
            CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.05, false);
        }
        if (method_getImplementation(loadMethod) == original) {
            fprintf(stderr, "FAIL packaged constructor did not install hooks\n");
            return 1;
        }
        printf("PASS packaged constructor installed hooks\n");
        UIViewController *owner = [[ownerClass alloc] init];
        [owner loadViewIfNeeded];
        [owner viewDidAppear:NO];
        deadline = [NSDate dateWithTimeIntervalSinceNow:1];
        while ([deadline timeIntervalSinceNow] > 0) {
            CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.05, false);
        }
        [owner viewWillDisappear:NO];
        printf("PASS packaged empty notification controller lifecycle\n");
        fflush(stdout);
        deadline = [NSDate dateWithTimeIntervalSinceNow:31];
        while ([deadline timeIntervalSinceNow] > 0) {
            CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.05, false);
        }
        const char *marker = getenv("ID_ARTIFACT_MARKER");
        if (!marker) return 1;
        if ([[NSFileManager defaultManager] fileExistsAtPath:[NSString stringWithUTF8String:marker]]) {
            fprintf(stderr, "FAIL startup checkpoint left its marker\n");
            return 1;
        }
        printf("PASS packaged thirty-second startup checkpoint\n");
        return 0;
    }
}
