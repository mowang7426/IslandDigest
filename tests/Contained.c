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
    /* 0.3.6: exercise the actual production reassert/coalescing predicates. */
    for (unsigned mask = 0; mask < 16; ++mask) {
        assert(!!IDContainedReassert(mask&1, mask&2, mask&4, mask&8) == (mask == 7)); ++count;
        int ready = IDContainedRepresentationReady(mask&1, mask&2, mask&4, mask&8);
        assert(!!ready == ((mask&1) ? !!(mask&2) : !!((mask&4) && (mask&8)))); ++count;
    }
    for (unsigned mask = 0; mask < 8; ++mask) {
        assert(!!IDContainedVisibilityEnqueue(mask&1, mask&2, mask&4) == (mask == 1)); ++count;
    }
    /* First fitting pass has zero viewport -> native access retained. After the
     * deferred on-window layout, exactly the same snapshot is now eligible. */
    int ready = IDContainedRepresentationReady(0, 0, 0, 0);
    assert(!IDContainedShouldHide(1,1,1,1,1,0,ready,1,7)); ++count;
    ready = IDContainedRepresentationReady(0, 0, 1, 1);
    int hide = IDContainedShouldHide(1,1,1,1,1,0,ready,1,7);
    assert(hide && IDContainedReassert(hide,1,1,0)); ++count;
    /* Same owned view, system hidden=NO: reassert, not a no-op from saved pointer. */
    assert(!IDContainedMustRestore(0, hide)); ++count;
    assert(IDContainedReassert(hide,1,1,0)); ++count;
    assert(!IDContainedReassert(hide,1,1,1)); ++count;
    /* Replacement: restore the old lease before hiding new; stolen lease may not
     * be reasserted. Noncurrent owner is unsupported and releases immediately. */
    assert(IDContainedMustRestore(1, hide)); ++count;
    assert(!IDContainedReassert(hide,0,1,0)); ++count;
    assert(!IDContainedReassert(hide,1,0,0)); ++count;
    int oldOwnerHide = IDContainedShouldHide(1,1,1,1,1,0,0,1,7);
    assert(!oldOwnerHide && IDContainedMustRestore(0,oldOwnerHide)); ++count;
    /* A verified retained expanded layout can keep native hidden while collapsed,
     * not an empty or unreachable capsule. Invalid/empty/window-loss always restores. */
    assert(IDContainedRepresentationReady(1,1,0,0)); ++count;
    assert(!IDContainedRepresentationReady(1,0,1,1)); ++count;
    assert(!IDContainedShouldHide(1,1,1,1,1,0,1,0,7)); ++count;
    assert(!IDContainedShouldHide(1,1,1,1,1,0,1,1,0)); ++count;
    /* Pending callbacks coalesce and setters called by the current decision cannot
     * recursively queue another pass; inactive callbacks cannot enqueue. */
    assert(IDContainedVisibilityEnqueue(1,0,0)); ++count;
    assert(!IDContainedVisibilityEnqueue(1,1,0)); ++count;
    assert(!IDContainedVisibilityEnqueue(1,0,1)); ++count;
    assert(!IDContainedVisibilityEnqueue(0,0,0)); ++count;
    assert(IDContainedLimit == 160); ++count;
    printf("PASS contained production predicates: %u assertions (not device runtime verification)\n", count);
    return 0;
}
