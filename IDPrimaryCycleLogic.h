#ifndef ID_PRIMARY_CYCLE_LOGIC_H
#define ID_PRIMARY_CYCLE_LOGIC_H
#include "IDStableRefreshLogic.h"
/* Tracks the slider's real transition occupancy, independently of user choice.
 * A locked/disabled presentation also occupies the boundary: unlocking or enabling
 * in place must not manufacture another hidden -> appearing edge. */
typedef struct { bool occupied; } IDPrimaryBoundary;
static inline bool IDPrimaryBegin(IDPrimaryBoundary *boundary, IDStablePresentation *cycle,
                                   bool fromAppeared, bool primary, bool unlocked, bool enabled) {
    if (!primary) return false;
    bool fresh = !fromAppeared && !boundary->occupied;
    boundary->occupied = true;
    if (!fresh || !unlocked || !enabled) return false;
    cycle->live = true;
    cycle->compact = true;
    return true;
}
static inline void IDPrimaryEnd(IDPrimaryBoundary *boundary, IDStablePresentation *cycle,
                                 bool appeared, bool primary) {
    if (!primary) return;
    boundary->occupied = appeared;
    if (!appeared) IDStableEndPresentation(cycle);
}
#endif
