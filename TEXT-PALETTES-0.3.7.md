# 0.3.7 — static four-color notification text

Based only on 0.3.6 commit `7a83ddfe111a85e32c78fd8ecda6a10684ef3f68`.
No 0.3.4 code or artifacts are used. Native hiding/recovery, enumeration,
scroll/tap/AX actions, privacy gates and presentation scheduling are unchanged.

Settings → 灵屿通知 → 通知文字颜色 → 静态渐变文字 → 渐变配色（双色 / 四色）.
Gradient remains OFF by default; existing saved choices and all other preferences
are preserved. Unknown identifiers fall back to the previous cyanPurple default.

The shared production specification `IDTextPalette.h` supplies both renderer and
Settings (not two independently maintained lists). The three legacy two-color
identifiers and exact RGBA values are unchanged. Ten added presets:

| Identifier | Chinese name | Four colors in left-to-right order |
|---|---|---|
| auroraFour | 极光星河 | 青 → 蓝 → 紫 → 粉 |
| sunsetFour | 落日霓霞 | 黄 → 橙 → 粉 → 紫 |
| glacierFour | 冰川微光 | 白 → 青 → 蓝 → 紫 |
| nebulaFour | 星云焰彩 | 蓝 → 紫 → 洋红 → 橙 |
| mintFour | 薄荷幻境 | 薄荷 → 青 → 蓝 → 紫 |
| peachFour | 蜜桃奶霜 | 粉 → 桃 → 淡黄 → 奶白 |
| coralFour | 珊瑚流金 | 珊瑚 → 橙 → 金 → 淡黄 |
| roseFour | 薰衣玫瑰 | 薰衣草 → 玫瑰 → 浅粉 → 白 |
| skyFour | 晴空海沫 | 天蓝 → 蓝绿 → 薄荷 → 白 |
| emeraldFour | 翡翠碧海 | 翡翠 → 蓝绿 → 青 → 蓝 |

The generator copies exactly count×4 RGBA components and count explicit locations:
2 stops at 0/1, 4 stops at 0/⅓/⅔/1. CoreGraphics create/release ownership is paired,
including allocation-failure paths. No gradient layer, timers or animation.
Only plugin notification title/subtitle/body labels use these static color strips;
UIKit keeps drawing text/multiline/Dynamic Type/emoji. No global UILabel hooks,
classification/capsule style changes or cached glyph/text images.

NSCache retains the existing 48-item and 1 MiB limits. Keys include the canonical
palette identifier, pixel width/height and full-precision scale; cost uses actual
CGImage bytesPerRow×height. Plain system/white/black text and optional shadow remain.

Validation: portable production C palette tests cover all 13 identifiers, ten
four-stop counts, exact old values, bounds, uniqueness, explicit stop locations
and unknown/null fallback. Source integration tests check Settings mapping,
renderer counts, ownership, cache cost, scope and opt-in. Existing enumeration ABI,
Foundation enumeration, contained/native recovery, layout, performance/coalescing,
collapse, scroll, experience/state and clear tests remain in macOS CI. Both package
schemes use the strict versioned checker, payload whitelist, metadata, install-name,
arm64/arm64e and per-page SHA-256 signature verification.

These checks are not user-device runtime tests. No install, respring, notification
opening/clearing or device operation is performed. Real first-frame timing,
wallpaper contrast and UIKit device behavior need device validation; no guaranteed
legibility or frame-time claims are made.
