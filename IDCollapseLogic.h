#ifndef ID_COLLAPSE_LOGIC_H
#define ID_COLLAPSE_LOGIC_H
#include <stdbool.h>
#include "IDLayoutLogic.h"
typedef enum { IDPanelExpanded, IDPanelCollapsing, IDPanelCollapsed, IDPanelExpanding } IDPanelState;
static inline bool IDPanelTransitioning(IDPanelState state) {
    return state == IDPanelCollapsing || state == IDPanelExpanding;
}
static inline bool IDPanelCompact(IDPanelState state) {
    return state == IDPanelCollapsed || state == IDPanelCollapsing;
}
/* Ignore rapid taps until completion; no queued action can open a notification. */
static inline IDPanelState IDPanelToggle(IDPanelState state) {
    if (IDPanelTransitioning(state)) return state;
    return state == IDPanelExpanded ? IDPanelCollapsing : IDPanelExpanding;
}
static inline IDPanelState IDPanelComplete(IDPanelState state) {
    return IDPanelCompact(state) ? IDPanelCollapsed : IDPanelExpanded;
}
/* Same top/trailing anchor as expanded; actual constraints, not scaled hit bounds. */
static inline IDPanelRect IDPanelCapsule(IDPanelRect expanded) {
    double width = fmin(132, fmax(0, expanded.width));
    double height = fmin(48, fmax(0, expanded.height));
    return (IDPanelRect){expanded.x + expanded.width - width, expanded.y, width, height};
}
static inline bool IDPanelCapsuleReachable(bool mounted, bool enabled, double width, double height) {
    return mounted && enabled && isfinite(width) && isfinite(height) && width >= 44 && height >= 44;
}
static inline bool IDPanelRetainedHide(bool reachable, bool ready, bool sameSnapshot) {
    return reachable && ready && sameSnapshot;
}
#endif
