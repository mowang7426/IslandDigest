#import <Foundation/Foundation.h>
#import <CoreFoundation/CoreFoundation.h>
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
