"""Source contracts complement C production-predicate tests; not runtime evidence."""
from pathlib import Path
r=Path(__file__).resolve().parents[1]
s=(r/'Tweak.xm').read_text(); style=(r/'IDTextStyle.inc').read_text()
support=(r/'IDContainedSupport.inc').read_text(); prefs=(r/'prefs/IDPrefs.m').read_text()
prepare=s.split('- (void)prepareFirstFrame {',1)[1].split('- (void)endAppearance {',1)[0]
assert 'loaded ? owner.view : nil' in prepare
assert 'IDEnabled && IDGrouping && IDContainedMode && IDHideNative' in prepare
assert 'IDFirstFrameShouldPrepare(' in prepare
assert prepare.index('self.firstFramePrepared = YES') < prepare.index('[self refresh]')
for token in ('[CATransaction begin]', '[CATransaction setDisableActions:YES]', '[UIView performWithoutAnimation:', '@finally', '[CATransaction commit]'):
    assert token in prepare
for forbidden in ('snapshotView', 'drawViewHierarchy', 'renderInContext', 'dispatch_after', 'while ('):
    assert forbidden not in prepare
will=s.split('- (void)viewWillAppear:(BOOL)animated {',1)[1].split('- (void)viewDidAppear:',1)[0]
assert '[state prepareFirstFrame]' in will and will.index('%orig') < will.index('[state prepareFirstFrame]')
assert 'if (!state.firstFramePrepared) IDScheduleRefresh()' in s
assert 'IDMethod(owner, "viewIsAppearing:", "v", 3) && IDBoolEncoding(appearingArgument)' in s
assert 'if (earlyLifecycleSupported) { %init(IDFirstFrame); }' in s
assert support.index('IDContainedPrivacyGate(') < support.index('id content = IDRead(request, "content")')
assert 'owner.isViewLoaded' in support and 'isDeviceAuthenticated' in support
assert 'IDBoolEncoding(authType)' in support
for key in ('firstFramePhase','firstRenderMs','firstHideMs','firstFrameLifecycleSupported'):
    assert f'@"{key}"' in s+(r/'IDContainedCoordinator.inc').read_text() and f'@"{key}"' in prefs
assert 'IDPaletteStop(palette, index)' in style
assert '(__bridge CFArrayRef)colors, locations' in style
assert 'palette->identifier' in style
assert 'CGImageGetBytesPerRow(image.CGImage) * CGImageGetHeight(image.CGImage)' in style
for name in ('cyanPurple','pinkOrange','whiteCyan','cyanBluePurplePink','yellowOrangePinkPurple','whiteCyanBluePurple'):
    assert f'@"{name}"' in s and f'@"{name}"' in prefs
assert '青色 → 蓝色 → 紫色 → 粉色（四色）' in prefs
assert '黄色 → 橙色 → 粉色 → 紫色（四色）' in prefs
assert '白色 → 青色 → 蓝色 → 紫色（四色）' in prefs
print('PASS 0.3.4 early lifecycle/transaction/auth/color-cache contracts (not device proof)')
