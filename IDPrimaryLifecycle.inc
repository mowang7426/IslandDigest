/* Header-backed primary subclass lifecycle. Do not hook UIViewController appearance. */
#include "IDPrimaryCycleLogic.h"
static BOOL IDPrimaryBeginInstalled, IDPrimaryEndInstalled;
static BOOL IDIsPrimarySlider(id slider, id manager) {
    return slider && slider == IDRead(manager, "coverSheetSlidingViewControllerIfLoaded");
}
static void IDPrimaryTransitionBegin(id slider, BOOL fromAppeared) {
    if (!NSThread.isMainThread) return;
    id manager = IDPresentationManagerIfLoaded();
    BOOL primary = IDIsPrimarySlider(slider, manager);
    BOOL locked = YES;
    BOOL known = IDClearBool(IDRead(manager, "uiLockStateProvider"), "isUILocked", &locked);
    BOOL enabled = IDEnabled && IDGrouping && IDContainedMode && IDCollapseOnPresentation
        && IDPrimaryEndInstalled && IDPullScreenHookInstalled;
    BOOL accepted = IDPrimaryBegin(&IDPrimaryBoundaryState, &IDPresentationCycle,
        fromAppeared, primary, known && !locked, enabled);
    NSString *reason = accepted ? @"accepted-primary-hidden-to-appearing" : !primary ? @"primary-identity-unconfirmed"
        : !known ? @"primary-lock-unconfirmed" : locked ? @"primary-ui-locked"
        : fromAppeared ? @"primary-from-visible" : !enabled ? @"primary-disabled-or-reset-unavailable" : @"primary-same-occupancy";
    uint64_t event = IDCollapseNextEvent();
    IDCollapseTrace[@"primaryBoundary"] = @{@"fromAppeared":@(fromAppeared), @"primary":@(primary),
        @"lockKnown":@(known), @"uiLocked":@(locked), @"occupied":@(IDPrimaryBoundaryState.occupied), @"reason":reason};
    IDCollapseDecision(accepted, reason, event, slider);
    IDDiagnostics[@"collapseLastReason"] = reason;
    if (accepted) {
        IDDiagnostics[@"collapseCycle"] = @([IDDiagnostics[@"collapseCycle"] unsignedIntegerValue] + 1);
        IDApplyPullCollapse();
    }
    IDCollapseFlushSoon();
}
