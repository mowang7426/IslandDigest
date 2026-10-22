#include "../IDCollapseLogic.h"
#include "../IDExperienceLogic.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
    IDPanelState s = IDPanelExpanded;
    for (int i = 0; i < 1000; i++) {
        s = IDPanelToggle(s); assert(s == IDPanelCollapsing);
        assert(IDPanelToggle(s) == s); s = IDPanelComplete(s); assert(s == IDPanelCollapsed);
        s = IDPanelToggle(s); assert(s == IDPanelExpanding);
        assert(IDPanelToggle(s) == s); s = IDPanelComplete(s); assert(s == IDPanelExpanded);
    }
    for (int width = 0; width <= 1200; width++) {
        for (int height = 0; height <= 700; height += 7) {
            IDPanelRect expanded = {17, 220, width, height};
            IDPanelRect c = IDPanelCapsule(expanded);
            assert(c.width <= 80 && c.height <= 44 && c.width >= 0 && c.height >= 0);
            assert(c.y == expanded.y && c.x + c.width == expanded.x + expanded.width);
            assert(c.width <= expanded.width && c.height <= expanded.height);
            assert(IDPanelCapsuleReachable(1,1,c.width,c.height) == (width >= 44 && height >= 44));
        }
    }
    IDPanelRect normal = IDPanelCapsule((IDPanelRect){8,220,374,360});
    assert(normal.x == 302 && normal.y == 220 && normal.width == 80 && normal.height == 44);
    assert(!IDPanelCapsuleReachable(1,1,43,44));
    assert(!IDPanelCapsuleReachable(1,1,80,43));
    assert(!IDPanelCapsuleReachable(0,1,80,44));
    assert(!IDPanelCapsuleReachable(1,0,80,44));
    assert(!IDPanelCapsuleReachable(1,1,NAN,44));
    assert(!IDPanelCapsuleReachable(1,1,80,INFINITY));
    for (int bits = 0; bits < 8; bits++) assert(IDPanelRetainedHide(bits&1,bits&2,bits&4) == (bits == 7));
    // Normal phones: THREE COLUMNS, not three visible rows. Narrow / AX font fallback.
    assert(IDExperienceGridColumns(359,1,3,1) == 3);
    assert(IDExperienceGridColumns(374,1,3,1) == 3);
    assert(IDExperienceGridColumns(304,1,3,1) == 2);
    assert(IDExperienceGridColumns(359,1,3,1.6) == 2);
    assert(IDExperienceGridColumns(359,1,3,2.3) == 1);
    assert(IDExperienceGridColumns(390,1,2,1) == 2);
    assert(IDExperienceGridColumns(390,1,0,1) == 3);
    assert(IDExperienceGridColumns(390,0,3,1) == 1);
    assert(IDExperienceRows(7,3) == 3);
    for (int w = 200; w <= 1000; w++) for (int scale = 10; scale <= 30; scale++) {
        size_t columns = IDExperienceGridColumns(w,1,3,scale / 10.0);
        assert(columns >= 1 && columns <= 3);
        double cell = (w-16-(columns-1)*6)/(double)columns;
        assert(cell >= 44);
    }
    puts("PASS production collapse state/geometry/reachability and three-column/AX sizing");
}
