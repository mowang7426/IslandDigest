#ifndef ID_CONTAINED_LOGIC_H
#define ID_CONTAINED_LOGIC_H
#include <stddef.h>
enum { IDContainedLimit = 160 };
static inline int IDContainedPrivacyGate(int onMain, int authKnown, int authenticated,
    int protectedKnown, int protectedAvailable, int policyKnown, int policyAllows) {
    return onMain && authKnown && authenticated && protectedKnown && protectedAvailable && policyKnown && policyAllows;
}
static inline int IDContainedShouldHide(int enabled, int grouping, int contained, int hideRequested,
    int active, int disabled, int supported, int panelVisible, size_t cardCount) {
    return enabled && grouping && contained && hideRequested && active && !disabled
        && supported && panelVisible && cardCount > 0;
}
static inline int IDContainedMustRestore(int candidateChanged, int shouldHide) {
    return candidateChanged || !shouldHide;
}
#endif
