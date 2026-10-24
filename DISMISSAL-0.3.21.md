# 0.3.21 dismissal-path audit (before device validation)

Baseline: 856937de7295a3eb79d1c20bc52338166d08ef69, feature/0.3.20-diagnostics-page.
The starting worktree was clean. No installation, respring or real notification actions.

## Evidence levels

* Confirmed source path: primary begin executes before `%orig`; accepted hidden-to-visible begins synchronously apply capsule state and layout. This is presentation-only, not the upward dismissal (`fromAppeared=YES` rejects). Primary end executes `%orig` first, then primary-only cycle reset and `IDPullEnded`.
* Confirmed source path: `IDPullEnded` -> `IDStatus` previously read three CFPreferences values and atomically serialized/wrote the status plist on main. Owner `viewWillDisappear:` calls `endAppearance` before `%orig`; that path also read/wrote DesiredApp preferences (CFPreferencesSynchronize) and called the same synchronous status writer.
* Confirmed source path: inactive `cancelCollapse` previously called `updateCollapsePresentation` (alpha/interaction/AX/blur properties). `updatePanelGeometry` already early-returned for inactive owners; do NOT claim that this method definitely fitted geometry on every exit. The fix avoids the unnecessary presentation-property work entirely on inactive cancellation.
* Confirmed source comparison: c52081b (0.3.18) through 856937d (0.3.20) changed only the startup-marker version in Tweak.xm, not lifecycle behavior. 0.3.20's diagnostics controller lives in Settings, is independent/read-only/on-demand; no evidence attributes the dismissal delay to that page.
* Confirmed gesture scope: the only require-to-fail edge is card tap -> its own card-scroll pan. No system dismissal recognizer dependency is added. Card scroll competes within its viewport; source alone cannot prove real-device arbitration latency. No gesture edits.
* Confirmed animation scope: capsule tap animation is 0.32s (Reduce Motion 0.16s). No custom one-second dismissal animation or wait-for-completion was found. The existing one-second trailing diagnostic flush is not a close timer; it previously could cause unrelated main-thread I/O. No timer-based dismissal patch and no animation-duration changes.
* Candidate only: synchronous preference/diagnostic I/O, unnecessary UI mutation and concurrent UIKit transition work could contribute to latency. There is NO device trace proving any explains the reported approximately one second, no measured before/after frame-time claim.

## Minimal changes

Status entry now updates only in-memory stage/reason and queues one coalesced snapshot. Bounded diagnostic snapshot construction runs on a later main-queue turn, using loaded option values, never fresh preference reads; fields explicitly identify loaded-options-not-disk semantics. Deep immutable property-list ownership precedes serial background serialization/atomic disk writes. One in-flight write plus one dirty bit provides backpressure and latest-event follow-up; existing repeated-stage throttling remains. Background writer never accesses UIKit or mutable diagnostics.

Exit no longer reads/writes DesiredApp. With RememberFilter off it is not consumed; existing selection and preference-change paths clear persistent state. Inactive cancellation stops the animator and resets scalar state without updating presentation/geometry. Next visible rendering reconciles UI. No full synchronous snapshot refresh was introduced.

Privacy scrub, stale snapshot generation invalidation, text/AX/action clearing, clear-confirmation invalidation, native-list lease restoration and actual native filter restoration remain synchronous. They are safety work, not arbitrarily deferred. Active collapse, primary identity/lock gates, screen-off cleanup, same-cycle expansion, re-pull collapse, incremental refresh, always-hide acquisition and native action ABI guards remain intact. All 24 preference definitions and the independent bottom diagnostics page remain intact.

## Validation and remaining risks

Tests exercise the production asynchronous persistence module under a blocked writer (not just strings), coalescing/backpressure/thread ownership/latest-state delivery, and extracted production dismissal/cancel methods with recording collaborators. Existing primary dispatch and cycle runtime tests cover non-primary, locked, cancel/re-pull and same-cycle expansion. Source contracts additionally prohibit preference I/O in endAppearance and direct status writes.

macOS dual-scheme CI must pass all fixtures, actual Theos builds, and strict package checks. Linux fixtures do not replace Foundation/UIKit or device execution. Fixture collaborators are not UIKit; they cannot prove system gesture timing, native filter cost, compositor load, physical-device callback ordering or zero stalls. An exceptional disable/recover path may still tear down views; safety cleanup and native restoration are intentionally not removed. Real-device Instruments/signpost trace is still needed to attribute the reported one-second delay and assess remaining privacy-walk/native work.
