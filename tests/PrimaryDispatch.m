#import <Foundation/Foundation.h>
#import <objc/runtime.h>
#import <objc/message.h>
#include <assert.h>
#include <math.h>
#include "../IDPrimaryCycleLogic.h"
static IDPrimaryBoundary boundary;
static IDStablePresentation cycle;
static unsigned baseCalls, primaryCalls, hookCalls;
static IMP original;
static void baseEnd(id self, SEL sel, BOOL appeared) { (void)self; (void)sel; (void)appeared; baseCalls++; }
static void primaryEnd(id self, SEL sel, BOOL appeared) { (void)self; (void)sel; (void)appeared; primaryCalls++; }
static void hookEnd(id self, SEL sel, BOOL appeared) {
    ((void (*)(id, SEL, BOOL))original)(self, sel, appeared);
    IDPrimaryEnd(&boundary, &cycle, appeared, true); hookCalls++;
}
static void setPresented(id self, SEL sel, BOOL p, BOOL u, BOOL a, id completion) {
    (void)self; (void)sel; (void)p; (void)u; (void)a; (void)completion;
}
int main(void) {
    @autoreleasepool {
        SEL end = sel_registerName("_endTransitionToAppeared:");
        Class base = objc_allocateClassPair(NSObject.class, "IDBoundaryBase", 0);
        assert(class_addMethod(base, end, (IMP)baseEnd, "v@:B")); objc_registerClassPair(base);
        Class primary = objc_allocateClassPair(base, "IDBoundaryPrimary", 0);
        assert(class_addMethod(primary, end, (IMP)primaryEnd, "v@:B")); objc_registerClassPair(primary);
        original = class_replaceMethod(primary, end, (IMP)hookEnd, "v@:B");
        id slider = [primary new];
        assert(IDPrimaryBegin(&boundary, &cycle, false, true, true, true));
        ((void (*)(id, SEL, BOOL))objc_msgSend)(slider, end, NO);
        assert(!cycle.live && !boundary.occupied && hookCalls == 1 && primaryCalls == 1 && baseCalls == 0);
        assert(IDPrimaryBegin(&boundary, &cycle, false, true, true, true));
        SEL set = sel_registerName("setPresented:forUserGesture:animated:withCompletion:");
        assert(class_addMethod(base, set, (IMP)setPresented, "v@:BBB@?"));
        Method m = class_getInstanceMethod(primary, set);
        assert(method_getNumberOfArguments(m) == 6);
        for (unsigned i = 2; i <= 4; i++) { char *t = method_copyArgumentType(m, i); assert(strcmp(t,"B") == 0); free(t); }
        char *block = method_copyArgumentType(m,5);
        assert(strcmp(block,"@") != 0 && strcmp(block,"@?") == 0); free(block);
        puts("PASS primary override dispatch preserves orig once and resets cycle; BOOL/block ABI fixture (not device ABI)");
    }
}
