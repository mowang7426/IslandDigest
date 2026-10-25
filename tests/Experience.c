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
    assert(IDExperienceColumns(260,1) == 2);
    assert(IDExperienceColumns(390,1) == 3);
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
    // Auto uses four columns only when each 44pt-high button remains >=82pt wide.
    assert(IDExperienceGridColumns(361,1,0,1) == 3);
    assert(IDExperienceGridColumns(362,1,0,1) == 4);
    assert(IDExperienceGridColumns(649,1,0,1) == 4);
    assert(IDExperienceGridColumns(650,1,0,1) == 5);
    // The 66pt header/hint plus actual 4pt row gap fit exactly at this boundary.
    assert(IDExperienceVisibleRows(3,450,44) == 2);
    assert(IDExperienceVisibleRows(3,460,44) == 3);
    for (int pref = 0; pref <= 3; pref++) {
        for (int width = 200; width <= 1000; width++) {
            size_t cols = IDExperienceGridColumns(width,1,pref,1);
            double cell = (width - 16 - (cols - 1) * 6) / cols;
            assert(cell >= 44);
            // 16pt icon, 10pt pin + 2pt gap, conservative 24pt count + 2pt gap,
            // name gap 2pt, tile inset 8pt and content padding 10pt.
            assert(cell - 18 - 16 - 12 - 26 - 2 >= 0);
        }
    }
    // Actual production tile constants: 32pt mini visual inside >=44pt hit area.
    for (int mini = 0; mini <= 1; mini++) {
        for (int row = 0; row <= 80; row++) {
            IDCategoryGeometry g = IDExperienceCategoryGeometry(row, mini);
            assert(g.hitHeight >= 44);
            assert(g.visualHeight + 2 * g.insetY == g.hitHeight);
            assert(g.visualHeight >= 32);
            for (int panel = 200; panel <= 1000; panel += 10) {
                for (int scale = 10; scale <= 24; scale++) {
                    size_t cols = IDExperienceGridColumns(panel, 1, 3, scale / 10.0);
                    double cell = (panel - 16 - (cols - 1) * 6) / cols;
                    double visual = cell - 2 * g.insetX;
                    double available = visual - 2 * g.contentPadding;
                    assert(cell >= 44 && available > 0);
                    // Centered content constrained to the tile: short/long names,
                    // counts off/99+, optional pin. Name alone yields space.
                    for (int name = 0; name <= 150; name += 5) {
                        for (int pin = 0; pin <= 1; pin++) {
                            for (int badge = 0; badge <= 24; badge += 24) {
                                double fixed = 16 + pin * 12 + (badge ? badge + 2 : 0) + 2;
                                double nameWidth = name < available - fixed ? name : available - fixed;
                                assert(nameWidth >= 0);
                                double group = fixed + nameWidth;
                                double left = (cell - group) / 2;
                                assert(left >= g.insetX + g.contentPadding - 0.0001);
                                assert(left + group <= cell - g.insetX - g.contentPadding + 0.0001);
                                double delta = left + group / 2 - cell / 2;
                                assert(delta > -0.0001 && delta < 0.0001);
                            }
                        }
                    }
                }
            }
        }
    }
    assert(IDExperienceCategoryGeometry(44, 1).visualHeight == 32);
    assert(IDExperienceCategoryGeometry(52, 0).visualHeight == 52);
    puts("PASS real shared C ordering/layout logic; no iOS runtime claims");
    return 0;
}
