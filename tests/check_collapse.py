"""Production wiring contracts complement the executable shared C tests; not runtime proof."""
from pathlib import Path
r = Path(__file__).resolve().parents[1]
t = (r/'Tweak.xm').read_text()
u = (r/'IDDigestUI.inc').read_text()
c = (r/'IDCollapseCoordinator.inc').read_text()
n = (r/'IDContainedCoordinator.inc').read_text()
p = (r/'prefs/IDPrefs.m').read_text()
assert '#import "IDCollapseLogic.h"' in t and '#include "IDCollapseCoordinator.inc"' in t
for key in ('UIViewPropertyAnimator', 'UIAccessibilityIsReduceMotionEnabled()', 'performWithoutAnimation:', '[self cancelClear]', 'IDPanelToggle(', 'IDPanelComplete(', 'IDPanelTransitioning(', 'UIAccessibilityLayoutChangedNotification'):
    assert key in c, key
for forbidden in ('openCardRequest', 'accessibilityActivate', 'beginClear', 'dropCards', 'dropContainedContent', 'IDWritePreference', 'applyFilter', 'transform', 'scale'):
    assert forbidden not in c, forbidden
transition = t.split('if (IDPanelTransitioning(self.collapseState)) {', 1)[1].split('}', 1)[0]
assert 'self.renderAfterCollapse = YES;' in transition and 'return;' in transition
assert 'recordCollapseDiagnostic:@"render" reason:@"transition-in-progress" consumed:NO' in transition
assert 'IDPanelTransitioning(self.collapseState) && !self.collapseGeometryAllowed' in t
assert 'if (IDPanelCompact(self.collapseState)) rect = IDPanelCapsule(rect)' in t
for anchor in ('overlayHeight.constant=rect.height', 'overlayTop.constant=rect.y', 'overlayLeft.constant=rect.x', 'overlayWidth.constant=rect.width'):
    assert anchor in t
assert 'self.expandedContent = [[UIView alloc]' in u and '[self.expandedContent addSubview:view]' in u
assert 'if (IDPanelTransitioning(self.coordinator.collapseState)) return;' in u
assert 'self.capsuleButton.frame = self.bounds;' in u
for key in ('IslandDigestCollapse', 'IslandDigestCapsule', 'self.capsuleButton.accessibilityLabel', 'self.expandedContent.accessibilityElementsHidden', 'self.collapseButton.frame = CGRectMake(MAX(0, width - 96), 0, 44, 44)', 'self.clearButton.frame = CGRectMake(MAX(0, width - 48), 0, 44, 44)'):
    assert key in u, key
for key in ('panelCollapseState', 'retainedExpandedLayoutReady', 'capsuleReachable', 'capsuleHideReady'):
    assert f'@"{key}"' in n and f'@"{key}"' in p
assert 'IDContainedRepresentationReady(compact, capsuleReady, cardViewportVisible, cardsReady)' in n
assert 'digest.retainedLayoutReady && [digest expandedCardsLayoutReady]' in n
assert 'IDExperienceGridColumns' in u and 'GridColumns' in t and 'GridColumns' in p
assert 'NSLineBreakByTruncatingTail' in u and 'UILayoutPriorityDefaultLow' in u
assert 'IDGridLayout ? 8 : 14' in u and 'IDGridLayout ? 6 : 8' in u
assert '[self.cardTap requireGestureRecognizerToFail:self.cardScroll.panGestureRecognizer]' in u
assert 'cardTapLayer' not in t + u and 'hitTest:' not in u
for method in ('endAppearance', 'recover'):
    body = t.split(f'- (void){method} {{', 1)[1].split('\n}', 1)[0]
    assert '[self cancelCollapse]' in body
# Real compact geometry is shared with the executable C test, not a scale transform.
logic = (r/'IDCollapseLogic.h').read_text()
assert 'double width = fmin(80,' in logic and 'double height = fmin(44,' in logic
for key in ('UIButtonTypeCustom', 'self.capsuleButton.configuration = nil',
            'self.capsuleButton.backgroundColor = UIColor.clearColor',
            'self.capsuleButton.layer.backgroundColor = UIColor.clearColor.CGColor',
            'self.capsuleButton.layer.shadowOpacity = 0',
            'self.glass.hidden = compact ||', 'self.glass.effect = compact ? nil :',
            'NSUnderlineStyleAttributeName: @0', 'total > 999 ? @"999+"',
            '共 %lu 条通知', 'forState:UIControlStateHighlighted'):
    assert key in u, key
assert 'secondarySystemBackgroundColor' not in u
# Loader registration stays a standard PSLinkCell, never custom inset/image sizing.
import plistlib, struct
entry = plistlib.loads((r/'layout/Library/PreferenceLoader/Preferences/IslandDigest.plist').read_bytes())['entry']
assert entry == {'bundle':'IslandDigestPrefs', 'cell':'PSLinkCell', 'detail':'IDPrefsController',
                 'label':'灵屿通知', 'icon':'IslandDigest.png', 'isController':True}
for folder in ('layout/Library/PreferenceLoader/Preferences', 'prefs/Resources'):
    for suffix, size in (('',29), ('@2x',58), ('@3x',87)):
        image = (r/folder/f'IslandDigest{suffix}.png').read_bytes()
        assert image[:8] == b'\x89PNG\r\n\x1a\n'
        assert struct.unpack('>II',image[16:24]) == (size,size)
info = plistlib.loads((r/'prefs/Resources/Info.plist').read_bytes())
assert info['CFBundleIconFiles'] == ['IslandDigest']
assert all(key not in p for key in ('setIndentationLevel:', 'setSeparatorInset:', 'setLayoutMargins:'))
print('PASS 0.3.1 compact 80x44 hit bounds, clear/no-blur appearance, bounded title/full AX count, 29pt entry scales and retained collapse contracts')
