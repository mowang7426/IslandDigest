"""Shared palette integration contracts, not UIKit/device performance proof."""
from pathlib import Path
r = Path(__file__).resolve().parents[1]
tweak = (r/'Tweak.xm').read_text()
prefs = (r/'prefs/IDPrefs.m').read_text()
style = (r/'IDTextStyle.inc').read_text()
header = (r/'IDTextPalette.h').read_text()
assert '#import "IDTextPalette.h"' in tweak
assert '#import "../IDTextPalette.h"' in prefs
assert 'IDStringOption(@"TextPalette", IDTextPaletteValues(), @"cyanPurple")' in tweak
assert 'IDStringOption(key, IDTextPaletteValues(), @"cyanPurple")' in prefs
assert 'values:IDTextPaletteValues() titles:IDTextPaletteTitles()' in prefs
assert '渐变配色（双色 / 四色）' in prefs
assert 'IDPref(@"GradientText", NO)' in tweak
assert 'IDTextPaletteForIdentifier(IDTextPalette.UTF8String)' in style
assert 'CGFloat components[16] = {0}, locations[4] = {0}' in style
assert 'i < palette->count * 4' in style and 'i < palette->count' in style
assert 'locations[i] = IDTextPaletteLocation(palette, i)' in style
assert 'CGGradientCreateWithColorComponents(space, components, locations, palette->count)' in style
assert '(double)stop / (double)(palette->count - 1)' in header
assert 'CGImageGetBytesPerRow(image.CGImage) * CGImageGetHeight(image.CGImage)' in style
assert 'palette->identifier' in style and 'pixelWidth' in style and 'pixelHeight' in style and '(double)scale' in style
assert 'if (gradient) CGGradientRelease(gradient)' in style
assert 'if (space) CGColorSpaceRelease(space)' in style
assert 'if (!context) { UIGraphicsEndImageContext(); return nil; }' in style
assert 'image = gradient ? UIGraphicsGetImageFromCurrentImageContext() : nil' in style
assert 'CGGradientCreateWithColors' not in style
assert 'dispatch_sync' not in tweak and 'CAGradientLayer' not in style
cards = (r/'IDContainedUI.inc').read_text()
assert cards.count('[IDNotificationLabel new]') == 1
assert 'styled.originalColor = [card[@"redacted"] boolValue] ? UIColor.secondaryLabelColor : UIColor.labelColor' in cards
assert '@implementation UILabel' not in tweak + style and '%hook UILabel' not in tweak
print('PASS 0.3.7 shared settings/renderer palette mapping, explicit count/stops, ownership, exact color-only cache cost, opt-in and scope')
