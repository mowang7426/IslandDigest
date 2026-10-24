#import "../IDRuntimeHookAudit.h"
#include <assert.h>
static void IDOriginal(id self, SEL cmd) { (void)self; (void)cmd; }
static void IDHook(id self, SEL cmd) { (void)self; (void)cmd; }
static void IDLaterWrapper(id self, SEL cmd) { (void)self; (void)cmd; }
int main(void) {
    @autoreleasepool {
        SEL selector = sel_registerName("id_auditEvent");
        Class parent = objc_allocateClassPair(NSObject.class, "IDRuntimeTestParent", 0);
        assert(class_addMethod(parent, selector, (IMP)IDOriginal, "v@:"));
        objc_registerClassPair(parent);
        Class target = objc_allocateClassPair(parent, "IDRuntimeTestTarget", 0);
        objc_registerClassPair(target);
        Class child = objc_allocateClassPair(target, "IDRuntimeTestChild", 0);
        objc_registerClassPair(child);
        assert(IDHookDefiningClass(target, selector) == parent);
        assert(!IDHookMatches(target, selector, (IMP)IDHook));
        // Internal Logos's inherited branch installs an override only on target.
        assert(class_addMethod(target, selector, (IMP)IDHook, "v@:"));
        assert(IDHookDefiningClass(target, selector) == target);
        assert(IDHookMatches(target, selector, (IMP)IDHook));
        assert(IDHookMatches(child, selector, (IMP)IDHook));
        assert(IDHookMatches(parent, selector, (IMP)IDOriginal));
        // A subclass override is distinct from installation failure.
        assert(class_addMethod(child, selector, (IMP)IDLaterWrapper, "v@:"));
        assert(IDHookDefiningClass(child, selector) == child);
        assert(!IDHookMatches(child, selector, (IMP)IDHook));
        assert(IDHookMatches(target, selector, (IMP)IDHook));
        // Later replacement is detected, without claiming it cannot chain to us.
        method_setImplementation(class_getInstanceMethod(target, selector), (IMP)IDLaterWrapper);
        assert(!IDHookMatches(target, selector, (IMP)IDHook));
        assert(IDHookDefiningClass(Nil, selector) == Nil);
        assert(!IDHookMatches(Nil, selector, (IMP)IDHook));
        assert(!IDHookMatches(target, sel_registerName("id_missing"), (IMP)IDHook));
        assert([IDHookClassName(Nil) isEqualToString:@"missing"]);
        puts("PASS production runtime audit: inherited/override/later replacement/missing (not device dispatch)");
    }
    return 0;
}
