#ifndef ID_CLEAR_LOGIC_H
#define ID_CLEAR_LOGIC_H
#include <stdbool.h>
#include <stddef.h>
typedef struct { unsigned long generation; bool pending; bool dispatched; } IDClearGate;
static inline bool IDClearEligible(bool scope, bool identity, bool criticalKnown, bool critical,
    bool optionsKnown, bool hiddenClear, unsigned long long persistence, bool action, bool interaction) {
    return scope && identity && criticalKnown && !critical && optionsKnown && !hiddenClear && persistence == 0 && action && interaction;
}
static inline bool IDClearBegin(IDClearGate *g) {
    if (g->pending || g->dispatched) return false;
    g->pending = true; return true;
}
static inline void IDClearCancel(IDClearGate *g) { g->generation++; g->pending = false; }
static inline bool IDClearConfirm(IDClearGate *g, unsigned long generation, bool context) {
    if (!context || !g->pending || g->dispatched || generation != g->generation) return false;
    g->pending = false; g->dispatched = true; return true;
}
static inline bool IDClearSnapshotMember(bool sameObject, bool sameKey, bool present, bool eligible) {
    return sameObject && sameKey && present && eligible;
}
#endif
