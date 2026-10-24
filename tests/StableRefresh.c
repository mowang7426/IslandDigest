#include <assert.h>
#include <stdio.h>
#include "../IDStableRefreshLogic.h"
int main(void) {
    IDStablePresentation cycle = {false, false};
    assert(!IDStableBeginPresentation(&cycle, false, false, true)); // wake/mutation
    assert(!IDStableBeginPresentation(&cycle, true, true, true)); // already visible
    assert(!IDStableBeginPresentation(&cycle, true, false, false)); // preference off
    assert(IDStableBeginPresentation(&cycle, true, false, true));
    assert(IDStablePresentationState(cycle, true) == IDPanelCollapsed);
    assert(IDStablePresentationState(cycle, false) == IDPanelExpanded); // supplementary
    cycle.compact = false; // user taps capsule
    assert(!IDStableBeginPresentation(&cycle, true, false, true)); // duplicate callback
    assert(IDStablePresentationState(cycle, true) == IDPanelExpanded); // owner rebuilt
    IDStableEndPresentation(&cycle); // dismissal or screen-off
    assert(IDStablePresentationState(cycle, true) == IDPanelExpanded); // wake
    assert(IDStableBeginPresentation(&cycle, true, false, true)); // next actual pull
    assert(IDStablePresentationState(cycle, true) == IDPanelCollapsed);
    assert(IDStableRetainWhilePending(true, true, false));
    assert(!IDStableRetainWhilePending(true, true, true));
    assert(!IDStableRetainWhilePending(false, true, false));
    assert(!IDStableRetainWhilePending(true, false, false));
    assert(IDStableReuseView(true, true, true, false));
    assert(!IDStableReuseView(false, true, true, false));
    assert(!IDStableReuseView(true, false, true, false));
    assert(!IDStableReuseView(true, true, false, false));
    assert(!IDStableReuseView(true, true, true, true));
    assert(IDStableScrollOffset(120, false, 0, 0, 500) == 120);
    assert(IDStableScrollOffset(120, true, 200, 20, 500) == 180);
    assert(IDStableScrollOffset(120, true, 900, 20, 500) == 500);
    assert(IDStableScrollOffset(120, false, 0, 0, 0) == 0);
    puts("PASS production stable refresh/appearance/reuse/privacy/scroll gates (not UIKit lifecycle validation)");
    return 0;
}
