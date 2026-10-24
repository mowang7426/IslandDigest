#ifndef ID_LAYOUT_LOGIC_H
#define ID_LAYOUT_LOGIC_H
#include <math.h>
typedef struct { double x, y, width, height; } IDPanelRect;
static inline double IDLayoutClamp(double value, double low, double high) {
    if (!isfinite(value)) value = low;
    return fmin(fmax(value, low), fmax(low, high));
}
/* Coordinates already converted from window safe bounds into the owner view.
 * Reserve bottom controls, but never claim that safeArea describes the clock. */
static inline IDPanelRect IDLayoutPanel(double left, double top, double right, double bottom,
                                        double offset, double inset, double preferred, double cap) {
    double availableWidth = fmax(0, right-left);
    double margin = IDLayoutClamp(inset, 8, fmin(64, availableWidth/4));
    double availableHeight = fmax(0, bottom-top-110);
    double minimum = fmin(180, availableHeight);
    double y = IDLayoutClamp(offset, 0, fmax(0, availableHeight-minimum));
    double height = fmin(IDLayoutClamp(preferred, 0, cap), fmax(0, availableHeight-y));
    return (IDPanelRect){left+margin, top+y, fmax(0,availableWidth-2*margin), height};
}
#endif
