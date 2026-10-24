#ifndef ID_CARD_READINESS_H
#define ID_CARD_READINESS_H
#include <math.h>
#include <stddef.h>
/* Shared by the real UIKit traversal and portable regression fixtures. Bounds,
 * not screen intersections, validate offscreen shells; only ONE semantic label
 * must intersect the viewport. Preview permission/action availability is separate. */
static inline int IDCardGeometryUsable(double width, double height) {
    return isfinite(width) && isfinite(height) && width >= 44 && height >= 44;
}
static inline int IDCardVisibleAreaUsable(double width, double height) {
    return isfinite(width) && isfinite(height) && width >= 44 && height >= 22;
}
static inline int IDCardSemanticUsable(int hasText, int hidden, double alpha,
                                       double width, double height) {
    return hasText && !hidden && alpha > 0.01 && isfinite(width) && isfinite(height)
        && width > 0 && height > 0;
}
static inline int IDCardCoverageUsable(double contentHeight, double stackBottom) {
    return isfinite(contentHeight) && isfinite(stackBottom) && contentHeight + 1 >= stackBottom + 12;
}
static inline int IDCardsRepresentationUsable(size_t expected, size_t built, size_t valid,
                                             int visibleSemantic) {
    return expected > 0 && expected == built && built == valid && visibleSemantic;
}
#endif
