#import <Foundation/Foundation.h>
#import <CoreFoundation/CoreFoundation.h>
#include <math.h>
static CFStringRef const IDPrefsDomain = CFSTR("com.local.islanddigest");
static NSString *const IDStatusPath = @"/var/mobile/Library/Preferences/com.local.islanddigest.status.plist";
static BOOL IDPref(NSString *key, BOOL fallback) {
    CFPropertyListRef value = CFPreferencesCopyValue((__bridge CFStringRef)key, IDPrefsDomain,
        kCFPreferencesCurrentUser, kCFPreferencesAnyHost);
    if (!value) return fallback;
    BOOL result = [(__bridge id)value respondsToSelector:@selector(boolValue)] ? [(__bridge id)value boolValue] : fallback;
    CFRelease(value);
    return result;
}
static void IDReloadPreferences(void) {
    CFPreferencesSynchronize(IDPrefsDomain, kCFPreferencesCurrentUser, kCFPreferencesAnyHost);
}

// Experience preferences are local only. Never copy these identifiers into diagnostics.
static id IDPreferenceValue(NSString *key) {
    CFPropertyListRef value = CFPreferencesCopyValue((__bridge CFStringRef)key, IDPrefsDomain,
        kCFPreferencesCurrentUser, kCFPreferencesAnyHost);
    return CFBridgingRelease(value);
}
static void IDWritePreference(NSString *key, id value) {
    CFPreferencesSetValue((__bridge CFStringRef)key, (__bridge CFPropertyListRef)value,
        IDPrefsDomain, kCFPreferencesCurrentUser, kCFPreferencesAnyHost);
    CFPreferencesSynchronize(IDPrefsDomain, kCFPreferencesCurrentUser, kCFPreferencesAnyHost);
}
static NSArray<NSString *> *IDPinnedIdentifiers(void) {
    id value = IDPreferenceValue(@"PinnedApps");
    NSMutableArray *result = [NSMutableArray new];
    if ([value isKindOfClass:NSArray.class]) for (id item in value) {
        if ([item isKindOfClass:NSString.class] && [item length] && ![result containsObject:item]) [result addObject:item];
        if (result.count >= 128) break;
    }
    return result;
}
static NSString *IDStringOption(NSString *key, NSArray<NSString *> *allowed, NSString *fallback) {
    id value = IDPreferenceValue(key);
    return [value isKindOfClass:NSString.class] && [allowed containsObject:value] ? value : fallback;
}
static double IDNumberOption(NSString *key, double fallback, double low, double high) {
    id value = IDPreferenceValue(key);
    double number = [value isKindOfClass:NSNumber.class] ? [value doubleValue] : fallback;
    return isfinite(number) ? fmin(high, fmax(low, number)) : fallback;
}
static inline void IDNotifyPreferencesChanged(void) {
    CFNotificationCenterPostNotification(CFNotificationCenterGetDarwinNotifyCenter(),
        CFSTR("com.local.islanddigest/changed"), NULL, NULL, YES);
}
