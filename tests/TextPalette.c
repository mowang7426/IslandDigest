#include "../IDTextPalette.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
int main(void) {
    const char *expected[] = {"cyanPurple", "pinkOrange", "whiteCyan", "auroraFour", "sunsetFour", "glacierFour", "nebulaFour", "mintFour", "peachFour", "coralFour", "roseFour", "skyFour", "emeraldFour"};
    assert(ID_TEXT_PALETTE_COUNT == 13);
    assert(IDTextPaletteForIdentifier(NULL) == &IDTextPalettes[0]);
    assert(IDTextPaletteForIdentifier("") == &IDTextPalettes[0]);
    assert(IDTextPaletteForIdentifier("unknownFuturePreset") == &IDTextPalettes[0]);
    assert(IDTextPaletteForIdentifier("AuroraFour") == &IDTextPalettes[0]);
    size_t four = 0;
    for (size_t i = 0; i < ID_TEXT_PALETTE_COUNT; ++i) {
        const IDTextPaletteSpec *p = &IDTextPalettes[i];
        assert(strcmp(p->identifier, expected[i]) == 0);
        assert(IDTextPaletteForIdentifier(expected[i]) == p);
        assert(p->count == (i < 3 ? 2 : 4));
        assert(strstr(p->title, i < 3 ? "双色" : "四色"));
        if (p->count == 4) ++four;
        for (size_t j = 0; j < i; ++j) {
            assert(strcmp(p->identifier, IDTextPalettes[j].identifier) != 0);
            assert(strcmp(p->title, IDTextPalettes[j].title) != 0);
        }
        assert(IDTextPaletteLocation(p, 0) == 0);
        assert(IDTextPaletteLocation(p, p->count-1) == 1);
        if (p->count == 4) {
            assert(IDTextPaletteLocation(p, 1) == 1.0/3.0);
            assert(IDTextPaletteLocation(p, 2) == 2.0/3.0);
        }
        for (size_t j = 0; j < p->count; ++j) {
            for (size_t c = 0; c < 4; ++c)
                assert(isfinite(p->rgba[j*4+c]) && p->rgba[j*4+c] >= 0 && p->rgba[j*4+c] <= 1);
            assert(p->rgba[j*4+3] == 1);
            if (j) {
                assert(IDTextPaletteLocation(p, j) > IDTextPaletteLocation(p, j-1));
                assert(memcmp(p->rgba+j*4, p->rgba+(j-1)*4, sizeof(double)*3) != 0);
            }
        }
    }
    assert(four == 10);
    const double legacy[3][8] = {{0.55,1,1,1,0.87,0.72,1,1}, {1,0.68,0.85,1,1,0.85,0.58,1}, {1,1,1,1,0.60,1,1,1}};
    for (size_t i = 0; i < 3; ++i) assert(memcmp(legacy[i], IDTextPalettes[i].rgba, sizeof(legacy[i])) == 0);
    puts("PASS production palettes: 3 legacy unchanged, 10 distinct four-color presets, RGBA bounds, stops, lookup/fallback");
    return 0;
}
