# 0.3.15 — direct pull input (device validation required)

## Findings, not a guessed device root cause

Baseline: `19e042c95c19b9163d913c3684c98ad9299e8653` (0.3.14 / PR #29). The worktree was clean; changes are isolated on `feature/0.3.15-collapse-trigger`, not merged into main.

The old path had three independently sufficient ways to miss folding: (1) all five manager-method checks had to pass, including three own-method-only checks; one unavailable/inherited cleanup method disabled the gesture hook too; (2) the delegate callback required `isPresented == false`, although the declarations do not establish its callback ordering; (3) accepted input relied on `render`, which can return before updating presentation when work/snapshot/animation gates are closed. The old finite retries also stopped once general owner hooks were installed. These are source-proven vulnerabilities, not a proven explanation of the user's device without runtime evidence.

## Input and cycle contract

The sole start signal is `SBCoverSheetSlidingViewController -_handlePresentGesture:` with a real `UIGestureRecognizer` in `UIGestureRecognizerStateBegan`, on the main thread. This is the present-pan input handler, not `viewWillAppear`, owner active state, notification mutation, generic presentation or a screen-wake callback. A recognizer-associated phase bit rejects repeated Began delivery; a distinct next gesture starts a fresh cycle even if the preceding gesture cancelled or an optional end hook was unavailable. No `isPresented` gate remains.

The handler writes the process-level compact state before `%orig`, so owners created during original processing consume it in `beginAppearance`/`render`. Already mounted active overlays are updated directly: cancel pending clear confirmation/animation, set collapsed state, update presentation and constraints, lay out owner and panel, reevaluate native visibility. This does not request a snapshot or synchronously enumerate notifications. Manual capsule expansion updates the same cycle; subsequent model refreshes preserve expansion. Supplementary mode stays expanded. Disabling the setting clears the cycle and updates existing overlays; enabling takes effect on the next actual pull, not immediately in Settings.

Cancellation/failure, `_endTransitionToAppeared:NO`, and `noteScreenDidTurnOff` end cycles. End/screen hooks have independent ABI probes and are not prerequisites for installing the input hook; their absence is visible. Acceptance requires the screen-off hook to be installed, otherwise `screen-off-hook-unavailable` rejects folding to prevent a stale compact cycle leaking into a lock wake. The transition-end hook is optional. They can only end, never start a cycle. A normal ended recognizer is not immediately treated as dismissal because it can precede successful presentation. No indirect/programmatic fallback was added. If the tested system routes touch pulls through a different input handler, diagnostics will expose zero input events instead of silently guessing from owner activity.

## Declaration evidence and ABI scope

Pinned declaration source (retrieved during this change):
https://github.com/MTACS/iOS-17-Runtime-Headers/blob/5a141185d0adb16bf83e96edc565abab5ec5de53/PrivateFrameworks/SpringBoard.framework/SBCoverSheetSlidingViewController.h

Actual declarations:

```objc
- (void)_handlePresentGesture:(id)arg1;
- (void)_endTransitionToAppeared:(bool)arg1;
```

Manager screen-off declaration:
https://github.com/MTACS/iOS-17-Runtime-Headers/blob/5a141185d0adb16bf83e96edc565abab5ec5de53/PrivateFrameworks/SpringBoard.framework/SBCoverSheetPresentationManager.h

```objc
- (void)noteScreenDidTurnOff;
```

Each hook checks class, selector, void return, argument count and payload ABI. Inherited methods are accepted only for the requested class (Logos internal class override), not hooked on UIViewController. Missing classes are retried through the existing bounded schedule and explicit retry action even after owner hooks are installed. Initial startup guard still precedes hook installation. Header evidence is not disassembly or callback-order/device proof. The retrieved evidence is iOS 17, not an exact 16.2 header dump; 16.2 Dopamine RootHide and 17.0 Relaxin must each report their runtime support and actual event counts. No claim of either device having passed is made.

## Preferences and persistent diagnostics

The existing `com.local.islanddigest` CFPreferences current-user/any-host domain is retained on both sides. No guessed RootHide path migration is introduced. Settings writes and synchronizes that domain, then posts `com.local.islanddigest/changed`; SpringBoard synchronizes and reads the same key with default NO. The contained-mode switch remains a separate prerequisite. Settings now reports failed synchronization instead of presenting it as confirmed saved.

Existing status plist and Settings diagnostic whitelist now show `collapseInputHook`, `collapseEndHook`, `collapseScreenHook` (class-missing / selector-missing / return-or-arity-mismatch / argument-ABI-mismatch / supported), `presentationGestureSupported` (input installed), `collapseGestureCount`, `effectiveCollapse`, `currentPrefCollapse`, `effectiveContained`, `collapseCycle`, `collapseCycleLive`, `collapseCycleCompact`, `collapseAppliedOwners`, `collapseLastReason`, `collapseLastEnd`. PID/version/system/update time already distinguish stale status. Diagnostics contain no notification text or application identifiers. A zero applied-owner count on an accepted pull is valid before the panel is mounted; beginAppearance then consumes the cycle.

Minimum feedback if folding still fails: copy Settings diagnostics after one pull and again after cancel/re-pull; include exact OS/jailbreak and installed package scheme. `supported` + zero gesture count means the declared handler did not observe the user's pull; method-missing/ABI reasons identify installation rejection; current/effective option mismatch identifies preference delivery; accepted cycle + expanded panel identifies downstream application/lifecycle trouble. This avoids requesting notification contents.

## Validation and preserved behavior

`tests/StableRefresh.c` executes the production cycle functions: switch off, duplicate Began, next distinct pull with stale live cycle, manual expansion followed by 100 non-user events, cancelled pull then retry, screen-off then non-user wake, unsupported mode, plus existing reuse/privacy/scroll rules. `tests/check_pull_trigger.py` checks actual settings write/read/notification wiring, primary input gate, independent installation/retry, direct mounted-panel constraint application and absence of model/owner-start signals. These are production helper/source tests, not UIKit gesture simulation. All existing source and C suites remain in CI; macOS additionally runs Objective-C enumeration tests and actual Theos builds.

0.3.14 stable refresh, card reuse, privacy/authentication and clear paths, scroll preservation, and 0.3.13 always-hide remain intact. No 0.3.4 synchronous full-list enumeration or deleted package is restored. Package checker version coverage includes both schemes and requires new selectors/diagnostic strings, exact metadata/payload, arm64+arm64e Mach-O, signatures, install names and dependencies.

Real-device acceptance remains necessary: enable and reopen Settings; pull and manually expand; insert/modify notifications without recollapse; cancel then pull again; dismiss and pull again; lock and let a notification wake the screen without starting a cycle; check contained OFF; check privacy/clear/scroll/always-hide recovery. This work does not install packages, respring, or access/remove real notifications.
