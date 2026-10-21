#include "../IDContainedLogic.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
    unsigned count = 0;
    /* Every missing proof must fail closed, including policy-never even when unlocked. */
    for (unsigned mask = 0; mask < 128; ++mask) {
        int allowed = IDContainedPrivacyGate(mask&1, mask&2, mask&4, mask&8, mask&16, mask&32, mask&64);
        assert(!!allowed == (mask == 127)); ++count;
    }
    assert(!IDContainedPrivacyGate(1, 1, 1, 1, 1, 1, 0)); ++count;
    /* Capture-hide is legal only with all positive requirements and no failure. */
    for (unsigned mask = 0; mask < 256; ++mask) {
        int hide = IDContainedShouldHide(mask&1, mask&2, mask&4, mask&8,
            mask&16, 0, mask&32, mask&64, (mask&128) ? 1 : 0);
        assert(!!hide == (mask == 255)); ++count;
    }
    assert(!IDContainedShouldHide(1,1,1,1,1,1,1,1,1)); ++count;
    /* Turning off, departing, empty snapshot, interface failure all imply restoration. */
    assert(IDContainedMustRestore(0, 0)); ++count;
    assert(IDContainedMustRestore(1, 0)); ++count;
    assert(IDContainedMustRestore(1, 1)); ++count;
    assert(!IDContainedMustRestore(0, 1)); ++count;
    assert(IDContainedLimit == 160); ++count;
    printf("PASS contained production predicates: %u assertions (not device runtime verification)\n", count);
    return 0;
}
