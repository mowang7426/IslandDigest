"""Source contracts supplement compiled shared predicates; not on-device proof."""
from pathlib import Path
r=Path(__file__).resolve().parents[1]
s=(r/'Tweak.xm').read_text(); ui=(r/'IDDigestUI.inc').read_text()
cards=(r/'IDContainedUI.inc').read_text(); style=(r/'IDTextStyle.inc').read_text()
layout=s.split('- (void)viewDidLayoutSubviews {',1)[1].split('- (void)viewWillDisappear:',1)[0]
for forbidden in ('updateNativeListVisibility', '[state refresh]', 'layoutIfNeeded', '[state snapshot]'):
    assert forbidden not in layout, forbidden
assert '[self.coordinator updatePanelGeometry]' not in ui
end=s.split('- (void)endAppearance {',1)[1].split('- (void)togglePinnedIdentifier:',1)[0]
assert end.index('self.active = NO') < end.index('[self cancelCollapse]')
assert '[self dropContainedContent]' in end and '[self resetFilter]' in end
assert 'self.presentationGeneration++' in end
assert 'generation != state.presentationGeneration' in s
assert 'IDPerformanceEnqueue([self workAllowed], self.presentationPending)' in s
assert 'self.geometryRevision != IDOptionsRevision' in s
assert 'CGRectEqualToRect(safe, self.geometrySafe)' in s
assert 'UIContentSizeCategoryDidChangeNotification' in s
assert 'CLOCK_MONOTONIC' in s and 'lastSlow >= 5' in s
clear=(r/'IDClearCoordinator.inc').read_text()
assert '[self workAllowed] ? [self clearUnavailableReason]' in clear
assert 'IDPref(@"GradientText", NO)' in s and 'IDPref(@"TextShadow", NO)' in s
for key in ('GradientText','TextPalette','SolidText','TextShadow','PerformanceDiagnostics'):
    assert f'@"{key}"' in s and f'@"{key}"' in (r/'prefs/IDPrefs.m').read_text()
assert 'cache.countLimit = 48' in style and 'cache.totalCostLimit = 1024 * 1024' in style
assert 'CGSizeMake(width, 1)' in style and 'colorWithPatternImage:strip' in style
for forbidden in ('self.text', 'drawTextInRect', 'CAGradientLayer', 'CGImageMask', 'writeToFile', 'NSLog', 'animation'):
    # textColor is permitted: no text/glyph is read into the cache generator.
    generator=style.split('static UIImage *IDGradientStrip',1)[1].split('@implementation',1)[0]
    assert forbidden not in generator, forbidden
assert 'self.styleRevision == IDOptionsRevision + 1' in style
assert 'styled.adjustsFontForContentSizeCategory = YES' in cards
scrub=cards.split('- (void)dropCards {',1)[1].split('- (UILabel *)cardLabel:',1)[0]
assert 'IDContainedClearView(view)' in scrub and '.openAction = nil' in scrub
assert 'view.accessibilityCustomActions = nil' in scrub
assert 'removeFromSuperview' not in scrub
assert 'dispatch_sync' not in s
print('PASS 0.3.3 bounded layout, dismissal privacy scrub/action release, color-only cache and opt-in diagnostics contracts')
