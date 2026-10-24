#ifndef ID_COLLAPSE_DIAGNOSTICS_H
#define ID_COLLAPSE_DIAGNOSTICS_H
#include <stdint.h>
/* Fixed-size, scalar-only telemetry. Caller serializes access. No UIKit dependency. */
typedef struct {
    uint64_t event, total[3], states[3][7], postStates[3][7], offMain[3];
    uint64_t lastEvent[3], postEvent[3];
    int pre[3], post[3], recognizer[3], mainThread[3];
    const char *classes[3];
} IDCollapseDiagnosticState;
static inline uint64_t IDCollapseDiagnosticEnter(IDCollapseDiagnosticState *s, unsigned source,
        int phase, int recognizer, int mainThread, const char *className) {
    uint64_t event = ++s->event;
    s->total[source]++; s->lastEvent[source] = event;
    s->pre[source] = phase; s->recognizer[source] = recognizer;
    s->mainThread[source] = mainThread; s->classes[source] = className;
    s->offMain[source] += !mainThread;
    s->states[source][phase >= 0 && phase <= 5 ? phase : 6]++;
    return event;
}
static inline void IDCollapseDiagnosticExit(IDCollapseDiagnosticState *s, unsigned source,
        uint64_t event, int phase) {
    s->postStates[source][phase >= 0 && phase <= 5 ? phase : 6]++;
    /* Nested callbacks must not replace the post state of the newest entry. */
    if (event == s->lastEvent[source]) { s->post[source] = phase; s->postEvent[source] = event; }
}
#endif
