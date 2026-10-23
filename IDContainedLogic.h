#ifndef ID_CONTAINED_LOGIC_H
#define ID_CONTAINED_LOGIC_H
#include <stddef.h>
enum { IDContainedLimit = 160 };
/* UI page capacity is not a source coverage limit. Every captured request has
 * exactly one reachable page, including the boundary 160/161 cases. */
static inline size_t IDContainedPageCount(size_t count) {
    return count / IDContainedLimit + (count % IDContainedLimit != 0);
}
static inline size_t IDContainedPageClamp(size_t page, size_t count) {
    size_t pages = IDContainedPageCount(count);
    return pages ? (page < pages ? page : pages - 1) : 0;
}
static inline size_t IDContainedPageMove(size_t page, size_t count, int direction) {
    page = IDContainedPageClamp(page, count);
    if (direction < 0) return page ? page - 1 : 0;
    size_t pages = IDContainedPageCount(count);
    return pages && page < pages - 1 ? page + 1 : page;
}
static inline int IDContainedSnapshotCoverage(size_t captured, size_t grouped, size_t selected,
    size_t page, size_t rendered, int navigationReady) {
    if (!captured || captured != grouped || selected > captured || !selected) return 0;
    if (page != IDContainedPageClamp(page, selected)) return 0;
    size_t remaining = selected - page * IDContainedLimit;
    size_t expected = remaining < IDContainedLimit ? remaining : IDContainedLimit;
    return rendered == expected && (selected <= IDContainedLimit || navigationReady);
}
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
static inline int IDContainedRepresentationReady(int compact, int capsuleReady, int viewportVisible, int cardsReady) {
    return compact ? capsuleReady : (viewportVisible && cardsReady);
}
static inline int IDContainedReassert(int shouldHide, int sameCandidate, int leaseOwned, int currentlyHidden) {
    return shouldHide && sameCandidate && leaseOwned && !currentlyHidden;
}
static inline int IDContainedVisibilityEnqueue(int workAllowed, int pending, int updating) {
    return workAllowed && !pending && !updating;
}
#endif
