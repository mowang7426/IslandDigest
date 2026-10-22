#ifndef ID_PALETTE_LOGIC_H
#define ID_PALETTE_LOGIC_H
#include <stddef.h>
#include <string.h>
typedef struct { double r, g, b; } IDPaletteColor;
typedef struct { const char *identifier; size_t count; IDPaletteColor colors[4]; } IDPalette;
/* Immutable COLOR ONLY. Identifier is also the cache namespace. */
static const IDPalette IDPalettes[] = {
    {"cyanPurple", 2, {{.55,1,1},{.87,.72,1}}},
    {"pinkOrange", 2, {{1,.68,.85},{1,.85,.58}}},
    {"whiteCyan", 2, {{1,1,1},{.60,1,1}}},
    {"cyanBluePurplePink", 4, {{.30,1,1},{.30,.58,1},{.72,.44,1},{1,.48,.80}}},
    {"yellowOrangePinkPurple", 4, {{1,.92,.38},{1,.60,.25},{1,.42,.70},{.72,.48,1}}},
    {"whiteCyanBluePurple", 4, {{1,1,1},{.40,1,1},{.34,.60,1},{.78,.55,1}}}
};
static inline const IDPalette *IDPaletteForIdentifier(const char *identifier) {
    for (size_t i=0; identifier && i<sizeof(IDPalettes)/sizeof(IDPalettes[0]); i++)
        if (strcmp(identifier, IDPalettes[i].identifier)==0) return &IDPalettes[i];
    return &IDPalettes[0];
}
static inline double IDPaletteStop(const IDPalette *palette, size_t index) {
    return palette && palette->count > 1 && index < palette->count
        ? (double)index / (palette->count - 1) : 0;
}
#endif
