#include <assert.h>
#include <stdio.h>
#include "../IDEnumerationABI.h"
int main(void) {
    const char *good[] = {"@@:", "@16@0:8", "r@16@0:8", "@\"NSArray\"@:", "@\"NSArray\"16@0:8", "@\"NSArray<NSCopying>\"16@0:8", "@16@0:"};
    const char *bad[] = {"", "Q16@0:8", "@?16@0:8", "@16@0:8@16", "@24@0:8Q16", "@16@0@8", "@16#0:8", "@\"NSArray16@0:8", "@\"\"16@0:8", "@16@0:8^v16", "@16@0:8?"};
    for (size_t i = 0; i < sizeof(good)/sizeof(good[0]); i++) assert(IDEnumerationGetterABI(good[i]));
    for (size_t i = 0; i < sizeof(bad)/sizeof(bad[0]); i++) assert(!IDEnumerationGetterABI(bad[i]));
    assert(!IDEnumerationGetterABI(NULL));
    assert(IDEnumerationObjectType("@\"NSArray\""));
    assert(!IDEnumerationObjectType("@?"));
    assert(!IDEnumerationObjectType("@\"NSArray\"garbage"));
    char long_type[600]; memset(long_type, 'x', sizeof(long_type)); long_type[599] = 0;
    assert(!IDEnumerationGetterABI(long_type));
    puts("PASS production full getter ABI parser: plain/annotated/qualified, offsets, exact self/_cmd, scalar/block/extra-argument/malformed/oversized rejection");
}
