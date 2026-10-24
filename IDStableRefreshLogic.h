#ifndef ID_STABLE_REFRESH_LOGIC_H
#define ID_STABLE_REFRESH_LOGIC_H
#include <stdbool.h>
#include "IDCollapseLogic.h"
/* Only an ABI-checked explicit user-presentation event may start a cycle.
 * Owner appearances, notification mutations and screen wakes are not events. */
typedef struct { bool live; bool compact; } IDStablePresentation;
static inline bool IDStableUserPull(IDStablePresentation *cycle, bool began, bool duplicate, bool enabled) {
    if (!began || duplicate || !enabled) return false;
    // Each distinct user gesture is a new attempt, even after a cancelled pull.
    // isPresented may already be true when the input callback is delivered.
    cycle->live = true; cycle->compact = true;
    return true;
}
static inline void IDStableEndPresentation(IDStablePresentation *cycle) {
    cycle->live = false; cycle->compact = false;
}
static inline IDPanelState IDStablePresentationState(IDStablePresentation cycle, bool supported) {
    return supported && cycle.live && cycle.compact ? IDPanelCollapsed : IDPanelExpanded;
}
static inline bool IDStableRetainWhilePending(bool pending, bool mounted, bool privacyInvalidated) {
    return pending && mounted && !privacyInvalidated;
}
static inline bool IDStableReuseView(bool sameIdentity, bool sameContent, bool sameOptions, bool scrubbed) {
    return sameIdentity && sameContent && sameOptions && !scrubbed;
}
static inline double IDStableScrollOffset(double previous, bool hasAnchor, double anchorY, double viewportY, double maximum) {
    double target = hasAnchor ? anchorY - viewportY : previous;
    return fmin(fmax(0, target), fmax(0, maximum));
}
#endif
