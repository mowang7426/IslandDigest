#include "../IDSnapshotLogic.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
    size_t built = 0, captured = 180;
    unsigned slices = 0, sorts = 0;
    int done = 0, sorting = 0;
    while (!done || built < captured) {
        size_t remaining = done ? captured - built : 64;
        if (!done) { built += remaining; if (built >= captured) { built = captured; done = 1; } }
        else built = captured;
        IDSnapshotAction action = IDSnapshotNext(done, built, captured, sorting);
        if (!done) assert(action == IDSnapshotPending);
        else { assert(action == IDSnapshotSort); sorts++; sorting = 1; }
        slices++;
        assert(slices < 10);
    }
    assert(slices == 3 && sorts == 1);
    assert(IDSnapshotNext(1, captured, captured, sorting) == IDSnapshotIgnore);
    assert(IDSnapshotNext(0, 0, 0, 0) == IDSnapshotPending);
    assert(IDSnapshotNext(1, 0, 0, 0) == IDSnapshotSort);
    assert(IDSnapshotNext(1, 31, 64, 0) == IDSnapshotPending);
    puts("PASS production snapshot gate: 180 captured across 3 slices, no premature sort, complete-empty distinct from pending");
    return 0;
}
