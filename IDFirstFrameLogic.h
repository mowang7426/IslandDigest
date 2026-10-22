#ifndef ID_FIRST_FRAME_LOGIC_H
#define ID_FIRST_FRAME_LOGIC_H
/* Preparation is one lifecycle operation, never permission to hide native UI. */
static inline int IDFirstFrameShouldPrepare(int requested, int mainThread, int loaded,
    int window, int hidden, int disabled, int updating, int prepared) {
    return requested && mainThread && loaded && window && !hidden && !disabled && !updating && !prepared;
}
static inline int IDFirstFrameRepresentationReady(int compact, int capsule, int viewport, int cards) {
    return compact ? capsule : (viewport && cards);
}
#endif
