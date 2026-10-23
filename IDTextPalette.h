#ifndef ID_TEXT_PALETTE_H
#define ID_TEXT_PALETTE_H
#include <stddef.h>
#include <string.h>
/* Production specification shared by Settings, renderer and portable tests.
 * RGBA rows and locations are consumed with count, never a fixed two-stop size. */
typedef struct {
    const char *identifier;
    const char *title;
    size_t count;
    double rgba[16];
} IDTextPaletteSpec;
#define ID_RGB(r,g,b) (r)/255.0, (g)/255.0, (b)/255.0, 1.0
static const IDTextPaletteSpec IDTextPalettes[] = {
    {"cyanPurple", "青色 → 淡紫（双色）", 2, {0.55,1,1,1, 0.87,0.72,1,1}},
    {"pinkOrange", "粉色 → 暖橙（双色）", 2, {1,0.68,0.85,1, 1,0.85,0.58,1}},
    {"whiteCyan", "白色 → 青色（双色）", 2, {1,1,1,1, 0.60,1,1,1}},
    {"auroraFour", "极光星河（四色）· 青 → 蓝 → 紫 → 粉", 4, {ID_RGB(96,238,245), ID_RGB(112,169,255), ID_RGB(185,145,255), ID_RGB(255,151,213)}},
    {"sunsetFour", "落日霓霞（四色）· 黄 → 橙 → 粉 → 紫", 4, {ID_RGB(255,225,126), ID_RGB(255,177,112), ID_RGB(255,151,195), ID_RGB(193,155,255)}},
    {"glacierFour", "冰川微光（四色）· 白 → 青 → 蓝 → 紫", 4, {ID_RGB(250,253,255), ID_RGB(142,242,248), ID_RGB(132,188,255), ID_RGB(195,172,255)}},
    {"nebulaFour", "星云焰彩（四色）· 蓝 → 紫 → 洋红 → 橙", 4, {ID_RGB(125,180,255), ID_RGB(186,150,255), ID_RGB(245,133,220), ID_RGB(255,184,123)}},
    {"mintFour", "薄荷幻境（四色）· 薄荷 → 青 → 蓝 → 紫", 4, {ID_RGB(166,246,207), ID_RGB(116,233,240), ID_RGB(137,187,255), ID_RGB(201,163,255)}},
    {"peachFour", "蜜桃奶霜（四色）· 粉 → 桃 → 淡黄 → 奶白", 4, {ID_RGB(255,167,201), ID_RGB(255,195,169), ID_RGB(255,231,167), ID_RGB(255,247,223)}},
    {"coralFour", "珊瑚流金（四色）· 珊瑚 → 橙 → 金 → 淡黄", 4, {ID_RGB(255,153,142), ID_RGB(255,184,119), ID_RGB(247,211,112), ID_RGB(255,239,173)}},
    {"roseFour", "薰衣玫瑰（四色）· 薰衣草 → 玫瑰 → 浅粉 → 白", 4, {ID_RGB(201,181,255), ID_RGB(241,158,196), ID_RGB(255,206,227), ID_RGB(255,250,253)}},
    {"skyFour", "晴空海沫（四色）· 天蓝 → 蓝绿 → 薄荷 → 白", 4, {ID_RGB(140,206,255), ID_RGB(110,220,213), ID_RGB(177,245,215), ID_RGB(247,255,252)}},
    {"emeraldFour", "翡翠碧海（四色）· 翡翠 → 蓝绿 → 青 → 蓝", 4, {ID_RGB(112,221,171), ID_RGB(94,210,199), ID_RGB(116,233,244), ID_RGB(134,182,255)}}
};
#undef ID_RGB
#define ID_TEXT_PALETTE_COUNT (sizeof(IDTextPalettes) / sizeof(IDTextPalettes[0]))
static inline const IDTextPaletteSpec *IDTextPaletteForIdentifier(const char *identifier) {
    if (identifier) for (size_t i = 0; i < ID_TEXT_PALETTE_COUNT; ++i)
        if (strcmp(identifier, IDTextPalettes[i].identifier) == 0) return &IDTextPalettes[i];
    return &IDTextPalettes[0];
}
static inline double IDTextPaletteLocation(const IDTextPaletteSpec *palette, size_t stop) {
    return (double)stop / (double)(palette->count - 1);
}
#ifdef __OBJC__
static inline NSArray<NSString *> *IDTextPaletteValues(void) {
    NSMutableArray<NSString *> *values = [NSMutableArray arrayWithCapacity:ID_TEXT_PALETTE_COUNT];
    for (size_t i = 0; i < ID_TEXT_PALETTE_COUNT; ++i)
        [values addObject:[NSString stringWithUTF8String:IDTextPalettes[i].identifier]];
    return values;
}
static inline NSArray<NSString *> *IDTextPaletteTitles(void) {
    NSMutableArray<NSString *> *titles = [NSMutableArray arrayWithCapacity:ID_TEXT_PALETTE_COUNT];
    for (size_t i = 0; i < ID_TEXT_PALETTE_COUNT; ++i)
        [titles addObject:[NSString stringWithUTF8String:IDTextPalettes[i].title]];
    return titles;
}
#endif
#endif
