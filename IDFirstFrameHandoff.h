#ifndef ID_FIRST_FRAME_HANDOFF_H
#define ID_FIRST_FRAME_HANDOFF_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* O(1) gate only: no native collection/card traversal is permitted here. */
typedef struct {
    int valid, protectedData, ownerMatch, windowMatch, settingsMatch, modelMatch;
    int shellReachable, shellVisible, candidateSupported, candidateUnclaimed;
    size_t cachedCards;
    uint64_t ageMs;
} IDFirstFrameHandoffInput;
static inline int IDFirstFrameHandoffSafe(const IDFirstFrameHandoffInput *v) {
    return v && v->valid && v->protectedData && v->ownerMatch && v->windowMatch
        && v->settingsMatch && v->modelMatch && v->shellReachable && v->shellVisible
        && v->candidateSupported && v->candidateUnclaimed && v->cachedCards > 0;
}
#ifdef __cplusplus
}
#endif
#endif
