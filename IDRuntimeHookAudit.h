#import <Foundation/Foundation.h>
#import <objc/runtime.h>
// Exact method-list ownership, not class_getInstanceMethod's inherited result.
static inline Class IDHookDefiningClass(Class cls, SEL selector) {
    for (Class c = cls; c; c = class_getSuperclass(c)) {
        unsigned count = 0;
        Method *methods = class_copyMethodList(c, &count);
        BOOL found = NO;
        for (unsigned i = 0; i < count; i++)
            if (method_getName(methods[i]) == selector) { found = YES; break; }
        free(methods);
        if (found) return c;
    }
    return Nil;
}
static inline BOOL IDHookMatches(Class cls, SEL selector, IMP expected) {
    Method method = cls ? class_getInstanceMethod(cls, selector) : NULL;
    return method && expected && method_getImplementation(method) == expected;
}
static inline NSString *IDHookClassName(Class cls) {
    return cls ? NSStringFromClass(cls) : @"missing";
}
