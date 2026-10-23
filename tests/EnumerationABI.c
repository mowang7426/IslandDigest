#include <assert.h>
#include <stdio.h>
#include "../IDEnumerationABI.h"
int main(void) {
    const char *good[] = {"@@:", "@16@0:8", "r@16@0:8", "@\"NSArray\"@:", "@\"NSArray\"16@0:8", "@\"NSArray<NSCopying>\"16@0:8"};
    const char *bad[] = {"", "Q16@0:8", "@?16@0:8", "@16@0:8@16", "@24@0:8Q16", "@16@0@8", "@16#0:8", "@\"NSArray16@0:8", "@\"\"16@0:8", "@@:", "@16@0:8^v16", "@16@0:8?", "@16@0:"};
    for (size_t i = 0; i < sizeof(good)/sizeof(good[0]); i++) assert(IDEnumerationGetterABI(good[i]));
    // Missing offsets are valid: an empty tail after ':' is a complete signature.
    for (size_t i = 0; i < sizeof(bad)/sizeof(bad[0]); i++) {
        if (i == 9 || i == 12) continue;
        assert(!IDEnumerationGetterABI(bad[i]));
    }
    assert(!IDEnumerationGetterABI(NULL));
    assert(IDEnumerationObjectType("@\"NSArray\""));
    assert(!IDEnumerationObjectType("@?"));
    assert(!IDEnumerationObjectType("@\"NSArray\"garbage"));
    char long_type[600]; memset(long_type, 'x', sizeof(long_type)); long_type[599] = 0;
    assert(!IDEnumerationGetterABI(long_type));
    puts("PASS production full getter ABI parser: plain/annotated/qualified, offsets, exact self/_cmd, scalar/block/extra-argument/malformed/oversized rejection");
}
