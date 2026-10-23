#include "../IDCardReadiness.h"
#include <assert.h>
#include <stdio.h>

/* Fixture inspired by the screenshot's 18 cards, 414x187 viewport and 1854
 * content height; 1842 is an assumed stack bottom, not a measured device frame.
 * Only one card needs readable viewport text. The other 17 may be offscreen/
 * redacted; no preview/action permission is inferred. Exact production helpers. */
static int fixture(size_t expected, size_t built, int visibleText, int invalidShell) {
    size_t valid = 0;
    int visible = 0;
    for (size_t i = 0; i < built; i++) {
        if (!IDCardGeometryUsable(386, invalidShell && i == built-1 ? 0 : 64)) return 0;
        valid++;
        double intersectionHeight = i == 0 ? 64 : 0;
        if (IDCardVisibleAreaUsable(386, intersectionHeight)) {
            /* Nested metadata/privacy placeholder is usable UILabel text.
             * Offscreen labels deliberately have no layout; never consulted. */
            visible |= IDCardSemanticUsable(visibleText, 0, 1, 130, 17);
        }
    }
    return IDCardsRepresentationUsable(expected, built, valid, visible);
}
int main(void) {
    assert(IDCardGeometryUsable(414,187));
    assert(IDCardCoverageUsable(1854,1842));
    assert(!IDCardCoverageUsable(1854,1844));
    assert(!IDCardCoverageUsable(NAN,1842));
    assert(!IDCardGeometryUsable(INFINITY,187));
    assert(!IDCardGeometryUsable(414,0));
    assert(!IDCardVisibleAreaUsable(43,64));
    assert(!IDCardVisibleAreaUsable(64,21));
    assert(fixture(18,18,1,0)); /* Includes 17 privacy placeholders. */
    assert(fixture(1,1,1,0));   /* Selected app's rendered range. */
    assert(!fixture(18,18,0,0));
    assert(!fixture(18,18,1,1));
    assert(!fixture(18,17,1,0));
    assert(!fixture(0,0,1,0));
    assert(!IDCardsRepresentationUsable(18,18,17,1));
    assert(!IDCardSemanticUsable(1,1,1,130,17));
    assert(!IDCardSemanticUsable(1,0,0,130,17));
    assert(!IDCardSemanticUsable(1,0,1,0,17));
    assert(!IDCardSemanticUsable(0,0,1,130,17));
    puts("PASS shared production readiness: 18-card viewport, 17 offscreen/redacted, nested metadata, selected range, invalid/empty/hidden/coverage fail-safe");
    return 0;
}
