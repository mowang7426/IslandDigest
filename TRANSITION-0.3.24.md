# 0.3.24 transition critical-path audit

Base: c2bead7f06015ca1f547b1f11fd804ebbfe43734 (0.3.23). No device trace was supplied. Source-level blocking work is proven; attribution of the reported 1–2 seconds is NOT proven.

## Evidence and changes

* `IDPrimaryTransitionBegin` calls `IDApplyPullCollapse` before `%orig`. Previously this cancelled collapse (updating expanded geometry), then updated compact geometry and synchronously forced owner and panel layout. Now it stops only the existing panel animator, applies final state/constraints once, marks layout dirty and queues native visibility readiness. No global hook or animation disable was added. Gesture/cycle hooks are retained.
* Compact geometry previously fitted the invisible expanded tree. It now skips fitting and supplies a 44pt cold-cache fallback before the shared 80x44 capsule clamp. `layoutSubviews` still updates the capsule touch frame but skips invisible expanded layout.
* Enumeration slicing did not slice the final `render`/`showCards`. A 160-card page could still be synchronously constructed while only a capsule was visible. Compact render now updates count/accessibility only, synchronously scrubs obsolete card content/actions, defers cards/groups, and schedules the unchanged native readiness gate. Expansion completion renders the newest current models, page, selection and preferences, not a captured stale snapshot. Expanded rendering remains synchronous: this patch does not claim to have time-sliced all UIKit construction or eliminated all animation competition.
* `beginAppearance` used synchronous CFPreferencesCopyValue for remembered selection (not itself a Synchronize call). The loaded preference snapshot now supplies it; user selection and invalidation update that snapshot. Option reload/notifications are retained.
* `finishSnapshot` called `IDWritePreference` twice, each synchronizing storage. The two cache values are now published together and one synchronization is queued on a serial background writer. Existing app-name/identifier preference semantics remain; no notification body is exported.
* `IDLog` performed stat/fopen/write/close inline. A serial writer now performs file work; callers copy the message before queueing.
* Explicit presentation and appearance runtime audits used to traverse class hierarchies synchronously before system callbacks. Requests now coalesce onto a later main-queue turn (runtime/UIKit observations stay on main). Hook installation and ABI/identity checks are unchanged.
* `viewWillAppear` native hide/readiness before and after `%orig` is deliberately retained: deleting it risks native flash or privacy regressions. Added default-off pre/orig/post/total and primary-begin pre/orig timings, exported through the existing safe bottom diagnostic page. Existing exit timings remain.
* Owner cleanup/restoration, authentication, action validation, selected pagination, native lease restore and synchronous privacy clearing are retained. .21 async status writer and .22 dirty/empty/idempotent cleanup are not new fixes in this release. .23 footer removal remains covered.

## Tests and limits

`tests/run_transition_fixture.py` extracts and executes the actual production render method on macOS with recording Foundation collaborators. It covers cold 160-card compact rendering, 100 warm/repeated/new-model updates, latest page/selection/options after expansion, same-cycle manual expansion, animation deferral, inactivity/unloaded panel and supplementary fallback. It also checks production critical-path wiring. This is NOT UIKit runtime or gesture smoothness verification.

Both CI schemes run it together with existing actual cleanup/dismissal/performance method fixtures, Foundation enumeration/runtime/dispatch/diagnostic tests, source/privacy/action/native-restoration contracts and shared C state/geometry/pagination/readiness tests. Strict deb checks verify layout, SpringBoard-only filter, arm64+arm64e, dependencies, minimum OS, preference bundle, diagnostic destination and every signed code page.

## Device reproduction (not performed by CI)

Test separately on iOS 17 Relaxin RootHide and iOS 16.2 Dopamine rootless; packages are not interchangeable. No installation, respring or live-notification operation was performed by this work.

1. Settings → 灵屿通知 → enable the existing performance diagnostics switch (default OFF); use the bottom 诊断信息 → 刷新诊断信息 page. It exports stage counts/max milliseconds, never body text, object addresses or app identifiers in these new timing fields.
2. In each environment record cold first pull, warm repeated pull/upward dismissal and canceled gesture, with 0/small/160+ existing notifications; compare performance OFF then ON. Report OS/jailbreak, package version, contained/collapse/native-hide modes and the diagnostic stage counts/maxima.
3. Exercise per-cycle capsule, manual expansion then same-cycle incoming notification, app selection, next/previous page, deactivate/reactivate, privacy lock/unlock, normal action/authentication and native restoration on disable. Do not infer device success from host fixtures.
4. Compare ownerWillAppearPre/Orig/Post/Total, primaryBeginPre/Orig and existing ownerWillDisappearPre/Orig/Post/Total. Maxima are not correlated traces and must not be summed as a single event. If Orig dominates, obtain a device Instruments/signpost trace before assigning cause.
5. Turn diagnostics OFF after collection. No absolute FPS or latency acceptance claim is made without on-device measurements.
