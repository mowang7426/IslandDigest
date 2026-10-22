#ifndef ID_SCROLL_LOGIC_H
#define ID_SCROLL_LOGIC_H
#include <math.h>
/* Shared production gate; UIKit's tap recognizer additionally enforces its
 * native movement tolerance and waits for this scroll view's pan to fail. */
static inline int IDScrollTapAllowed(int ended, int dragging, int decelerating,
    double startOffset, double offset, double contentHeight, double viewportHeight) {
    double maximum = fmax(0, contentHeight - viewportHeight);
    return ended && !dragging && !decelerating && isfinite(startOffset)
        && isfinite(offset) && isfinite(contentHeight) && isfinite(viewportHeight)
        && viewportHeight > 0 && contentHeight >= 0
        && offset >= 0 && offset <= maximum && fabs(offset-startOffset) <= 1;
}
#endif
