#ifndef ID_PERFORMANCE_LOGIC_H
#define ID_PERFORMANCE_LOGIC_H
/* Shared production/test predicate: never queue hidden/teardown work. */
static inline int IDPerformanceWorkAllowed(int active, int disabled, int loaded, int window, int hidden) {
    return active && !disabled && loaded && window && !hidden;
}
static inline int IDPerformanceEnqueue(int allowed, int pending) { return allowed && !pending; }
#endif
