# 0.3.17 presentation-path audit and bounded fallback

## What the 0.3.16 device screenshot establishes

The live process/version/settings and owner render are confirmed, while all three recognizer entry probes, accept/reject and cycle counters remain zero. The missing capsule follows from `no-live-cycle`; it is not evidence of unread preferences. These observations do **not** prove that iOS bypasses the hooked methods. Inheritance, subclass overrides and subsequent hook replacements must be separated from a genuinely unused input path.

## Source and generated-code evidence

Pinned system declarations:

- https://github.com/MTACS/iOS-17-Runtime-Headers/blob/5a141185d0adb16bf83e96edc565abab5ec5de53/PrivateFrameworks/SpringBoard.framework/SBCoverSheetSlidingViewController.h
- https://github.com/MTACS/iOS-17-Runtime-Headers/blob/5a141185d0adb16bf83e96edc565abab5ec5de53/PrivateFrameworks/SpringBoard.framework/SBCoverSheetPresentationManager.h
- https://github.com/MTACS/iOS-17-Runtime-Headers/blob/5a141185d0adb16bf83e96edc565abab5ec5de53/protocols/SBUILockStateProvider.h

These declare `setPresented:forUserGesture:animated:withCompletion:`, `coverSheetSlidingViewController:prepareForPresentationTransitionForUserGesture:`, `coverSheetSlidingViewControllerIfLoaded`, `sharedInstanceIfExists`, `uiLockStateProvider` and `isUILocked`. The explicit user-gesture flag plus primary-slider identity and confirmed UI-unlocked state is stronger evidence than owner appearance or authentication alone. A header proves existence/signature, **not** actual call order or that this device takes the path.

The actual baseline and modified `Tweak.xm` were processed using Logos internal generator commit `777925d1add4ee3485a2432a6f6d5e3ded59de7b`. Generated `_logos_register_hook` walks each class's own method list: an owned method uses `method_setImplementation`; an inherited implementation is saved as original and a target-only override is added via `class_addMethod`. Groups generate distinct function/original variables. No generated superclass-wide gesture hook was found. However `class_addMethod` success is not checked by that helper; setting a source installation flag to YES is not runtime proof. The added audit compares the current IMP to the actual generated hook function, never to a snapshot that might already be wrong.

## Bounded production change

Keep existing recognizer-Began input and probes. Add two ABI-checked explicit presentation callbacks before `%orig`, passing the system flag rather than inferring a gesture. Accept only when all of these are true:

1. Main-thread call, presentation direction, explicit `forUserGesture` true.
2. Callback slider is exactly the manager's `coverSheetSlidingViewControllerIfLoaded` (not a secure-app/secondary slider).
3. The manager's lock-state provider exposes an ABI-checked `isUILocked`, and it returns false. Missing/unknown is rejected.
4. Plugin/grouping/contained/auto-collapse are enabled and the screen-off reset hook is available.
5. No cycle is already live.

Use the existing immediate geometry-only fold, not render/enumeration/full rebuild. Cross-entry duplicates, including a raw Began after an accepted explicit transition, cannot re-collapse manual expansion. Dismissal/cancellation/screen-off retains existing reset behavior. A next genuine presentation after reset folds again. Unknown lock state, false user flag or unknown primary slider fails closed. There is no owner inactive-to-active input trigger, global UIView/UIGestureRecognizer hook, privacy weakening or preference mutation.

## Runtime audit fields

Seven narrow hooks record selector, target class, method-defining class before installation, `installedExpected`, `targetExpectedNow`, actual `runtimeClass`, current `runtimeDefinition`, and `runtimeExpectedNow`. Generated function symbols are compared directly; no addresses are exported. On actual owner appearance, collect at most 16 ancestor class names and read the already-existing manager/primary slider (`sharedInstanceIfExists`, not constructing `sharedInstance`). Sampling again detects a later replacement without rehooking or fighting another tweak.

Interpretation:

- `installedExpected=0`: installation was not verified; prior support/installation flags were insufficient.
- Installed true but target current false: the target's method changed later. This may still chain to IslandDigest; mismatch alone is not proof of bypass.
- Target current true but runtime current false, subclass defining class: a subclass has different dispatch. Again it may call super.
- Both true but counts zero: stronger evidence that this method was not entered; still not disassembly proof of another OS path.
- Runtime missing: live instance was not discoverable through the safe existing-instance getter; ancestor and transition samples remain useful, and this is not proof no controller exists.

`transitionEntries`, `transitionOffMain` and `transitionSample` record exact source name, event, presenting/user flag, primary identity, lock-known/UI-locked, slider runtime class and accepted/rejected reason. Existing immutable last accept/reject and consumer/clear samples remain intact. No notification text, identifiers, IMP values or memory addresses are added.

## Validation and limits

Portable source contracts and the production C cycle model pass locally. New cases cover lock wake, unknown/locked UI, non-primary slider, disabled mode, raw/manager/slider ordering, same-cycle manual expansion and next-cycle reset. macOS `RuntimeHookAudit.m` tests the production audit helper against real Objective-C inherited methods, subclass overrides and later method replacement; the existing macOS RootHide/rootless matrix now includes it. CI build success and installed-device behavior must be reported separately.

No device installation, respring or actual notification operation was performed. Remaining unknowns: device callback order/reachability, whether the lock-state provider is available at that precise stage on each OS, later hook chains, and whether a cycle-ending callback can be missed. The conservative live-cycle dedup deliberately waits for a real reset; it does not use owner disappearance as a substitute. This is an evidence-backed fallback plus targeted diagnostic delivery, not a claim of a confirmed device fix.

## Minimal manual verification

With auto-collapse and contained mode enabled, from an already unlocked home/app screen pull Notification Center down. Check the capsule and the transition/accept source. Expand it; a same-cycle new notification must not fold it. Dismiss and pull again; the capsule should return. Lock and allow a new notification to wake the screen; it must not be accepted as this unlocked user-presentation cycle. If unsuccessful, capture the fresh process/version summary plus runtime class/hook-audit rows, transition sample and accept/reject/consumer/clear sample; do not send notification content.
