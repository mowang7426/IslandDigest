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
assert 'if (IDPanelTransitioning(self.collapseState)) { self.renderAfterCollapse = YES; return; }' in t
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
assert 'compact ? capsuleReady : (cardViewportVisible && cardsReady)' in n
assert 'digest.retainedLayoutReady && [digest expandedCardsLayoutReady]' in n
assert 'IDExperienceGridColumns' in u and 'GridColumns' in t and 'GridColumns' in p
assert 'NSLineBreakByTruncatingTail' in u and 'UILayoutPriorityDefaultLow' in u
assert 'IDGridLayout ? 8 : 14' in u and 'IDGridLayout ? 6 : 8' in u
assert '[self.cardTap requireGestureRecognizerToFail:self.cardScroll.panGestureRecognizer]' in u
assert 'cardTapLayer' not in t + u and 'hitTest:' not in u
for method in ('endAppearance', 'recover'):
    body = t.split(f'- (void){method} {{', 1)[1].split('\n}', 1)[0]
    assert '[self cancelCollapse]' in body
print('PASS 0.3.0 collapse ownership, compact constraints, AX/ReduceMotion, native hide and adaptive three-column contracts')
