#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "../IDPrimaryCycleLogic.h"
int main(void) {
    IDPrimaryBoundary b = {0}; IDStablePresentation c = {0};
    assert(!IDPrimaryBegin(&b, &c, false, false, true, true));
    assert(!b.occupied && !c.live);
    assert(IDPrimaryBegin(&b, &c, false, true, true, true));
    assert(c.live && c.compact);
    c.compact = false; // user taps existing capsule
    assert(!IDPrimaryBegin(&b, &c, false, true, true, true));
    IDPrimaryEnd(&b, &c, true, true);
    assert(c.live && !c.compact); // duplicate show / commit preserves expansion
    assert(IDStablePresentationState(c, true) == IDPanelExpanded); // notification refresh
    assert(!IDPrimaryBegin(&b, &c, true, true, true, true));
    IDPrimaryEnd(&b, &c, false, false); assert(c.live); // other slider cannot reset
    IDPrimaryEnd(&b, &c, false, true); assert(!c.live && !b.occupied);
    assert(IDPrimaryBegin(&b, &c, false, true, true, true)); // exit then pull
    IDPrimaryEnd(&b, &c, false, true); // cancellation ending hidden
    assert(IDPrimaryBegin(&b, &c, false, true, true, true)); // immediate retry
    IDStableEndPresentation(&c); b.occupied = false; // screen off
    assert(!IDPrimaryBegin(&b, &c, false, true, false, true)); // locked wake
    assert(!IDPrimaryBegin(&b, &c, false, true, true, true)); // unlock in place
    IDPrimaryEnd(&b, &c, true, true); assert(!c.live);
    IDPrimaryEnd(&b, &c, false, true);
    assert(!IDPrimaryBegin(&b, &c, false, true, true, false)); // disabled start
    assert(!IDPrimaryBegin(&b, &c, false, true, true, true)); // enabling in place
    IDPrimaryEnd(&b, &c, false, true);
    assert(IDPrimaryBegin(&b, &c, false, true, true, true));
    puts("PASS primary boundary: duplicate, manual expansion, refresh, wake, cancel, exit/reopen, identity, disabled");
}
