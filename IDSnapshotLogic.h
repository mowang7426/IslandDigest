#ifndef ID_SNAPSHOT_LOGIC_H
#define ID_SNAPSHOT_LOGIC_H
#include <stddef.h>
typedef enum { IDSnapshotPending, IDSnapshotSort, IDSnapshotIgnore } IDSnapshotAction;
/* A cursor is not a snapshot. Metadata completion and a single outstanding sort
 * are separate gates; nil/cancelled cursors are rejected by the caller. */
static inline IDSnapshotAction IDSnapshotNext(int enumerationDone, size_t built,
                                              size_t captured, int sorting) {
    if (sorting) return IDSnapshotIgnore;
    if (!enumerationDone || built < captured) return IDSnapshotPending;
    return IDSnapshotSort;
}
#endif
