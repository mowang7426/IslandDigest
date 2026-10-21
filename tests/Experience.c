#include "../IDExperienceLogic.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
    // Every mode: pin priority is stronger than age/count/name.
    for (int mode = 0; mode <= 2; mode++) {
        assert(IDExperienceOrder(1,0,mode,1,999,1,999,1,1) == -1);
        assert(IDExperienceOrder(0,1,mode,999,1,999,1,-1,-1) == 1);
        assert(IDExperienceOrder(0,0,mode,5,5,100,100,0,-1) == -1);
        assert(IDExperienceOrder(1,1,mode,5,5,100,100,0,1) == 1);
        assert(IDExperienceOrder(0,0,mode,5,5,100,100,0,0) == 0);
    }
    assert(IDExperienceOrder(0,0,0,1,100,200,100,1,1) == -1);
    assert(IDExperienceOrder(0,0,1,100,1,1,200,1,1) == -1);
    assert(IDExperienceOrder(0,0,2,1,100,1,200,-1,1) == -1);
    assert(IDExperienceOrder(0,0,2,1,100,1,200,1,-1) == 1);
    // Same count can change most-recent ordering without a new count.
    assert(IDExperienceOrder(0,0,0,2,2,300,200,0,1) == -1);
    assert(IDExperienceOrder(0,0,0,2,2,100,200,0,1) == 1);
    // Bounds include small screens, landscape, tablet and capped large text.
    assert(IDExperienceColumns(260,1) == 1);
    assert(IDExperienceColumns(390,1) == 2);
    assert(IDExperienceColumns(768,1) == 3);
    assert(IDExperienceColumns(768,0) == 1);
    assert(IDExperienceRows(0,2) == 1);
    assert(IDExperienceRows(7,2) == 4);
    assert(IDExperienceVisibleRows(100,844,52) == 3);
    assert(IDExperienceVisibleRows(100,320,80) == 1);
    for (size_t entries = 0; entries < 512; entries++) {
        for (int width = 200; width <= 1000; width += 20) {
            size_t columns = IDExperienceColumns(width,1);
            size_t rows = IDExperienceRows(entries,columns);
            for (int height = 200; height <= 1200; height += 50) {
                size_t visible = IDExperienceVisibleRows(rows,height,80);
                assert(visible >= 1 && visible <= 3 && visible <= rows);
            }
        }
    }
    puts("PASS real shared C ordering/layout logic; no iOS runtime claims");
    return 0;
}
