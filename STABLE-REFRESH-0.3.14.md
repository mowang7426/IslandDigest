# IslandDigest 0.3.14 — stable refresh and guarded presentation

## Root cause and repair

`IDModelDidChange` previously called `dropContainedContent` for every owner. That invalidated the asynchronous reader AND scrubbed/remounted the displayed cards on every insert/modify/remove. Active owners now invalidate only the snapshot generation unless rendered plaintext needs privacy scrubbing. Inactive/privacy paths still scrub synchronously. Pending reads are not committed empty snapshots; an already mounted representation remains until commit. Existing generation/model/config gates still reject stale asynchronous work; no synchronous full-list enumeration was restored.

Cards are reused only for the identical request object, equal model content and equal options revision. Discarded cards lose text, accessibility content, actions and associated model. A retained visible card anchors scroll position; if removed/changed, offset falls back to bounded previous offset. Category buttons retain stable signatures. New/changed headings and pagination controls may still relayout; this is not a claim that UIKit does zero layout. While a snapshot is pending or stale, card opening fails closed; committed request membership is checked by pointer identity, not textual identifier. Original ABI-checked native actions remain the only dispatch path.

Privacy overrides UI continuity. Protected-data loss / deauthentication / preview opt-out clear cards, associated models and pending action references. Preview additionally requires an ABI-checked authenticated CoverSheet ancestor (protected data alone does not prove current unlock). Missing authentication evidence yields redacted cards. No notification text is persisted. System per-app `contentPreviewSetting` values remain unmapped; explicit unlocked preview is still default-off and cannot promise to preserve each app's Never policy.

## Presentation-cycle evidence and conservative boundary

The rejected draft used owner inactive→active. That is not a user-presentation signal: wake and owner reconstruction can also produce it. It has been removed.

Reference: MTACS/iOS-17-Runtime-Headers, commit `5a141185d0adb16bf83e96edc565abab5ec5de53`, `PrivateFrameworks/SpringBoard.framework/SBCoverSheetPresentationManager.h`:
https://github.com/MTACS/iOS-17-Runtime-Headers/blob/5a141185d0adb16bf83e96edc565abab5ec5de53/PrivateFrameworks/SpringBoard.framework/SBCoverSheetPresentationManager.h

Relevant actual declarations:
```objc
- (void)coverSheetSlidingViewControllerUserPresentGestureBegan:(id)controller;
- (void)coverSheetSlidingViewControllerCleanupDismissalTransition:(id)controller;
- (void)coverSheetSlidingViewControllerDidEndTransition:(id)controller;
- (void)noteScreenDidTurnOff;
- (bool)isPresented;
```

Installation requires the three object callbacks to be methods owned by that class, with void return / three Objective-C arguments / object payload. The other methods must have void/no payload and BOOL/no payload ABIs respectively. Missing/mismatched signals disable automatic collapsing, not the plugin. A user-present callback starts a process-level cycle only when `isPresented` is confirmed false and no cycle is live. Duplicate callbacks, notifications and owner lifecycles cannot start one. Manual capsule expansion updates the cycle, so a reconstructed owner in the same cycle remains expanded. Dismissal, screen-off or a transition ending unpresented clear the cycle. Only contained overlay mode consumes it; supplementary mode is forced expanded.

This is declaration evidence plus runtime ABI guards, NOT disassembly or a real-device callback-order proof. If a system reports already-presented by the time the user callback runs, that pull conservatively stays expanded. No guessed generic `viewWillAppear`, `setPresented`, notification-count or lock-state event fallback is used. A runtime header alone cannot prove every iOS minor version's semantics. Locked wakes do not start a new cycle. Unsupported systems may show no automatic folding at all. The diagnostic `presentationGestureSupported` reports ABI installation capability, not actual device success.

## Settings and safety

Settings → 灵屿通知 → 插件内显示通知卡片 → 每次下拉先折叠. New switch defaults OFF in both consumer and Preferences. It is disabled when contained mode is off. With a confirmed supported pull, a capsule appears first; tap it to expand. Notifications in the same cycle neither re-collapse nor auto-expand. Screen wake is not a new pull. The setting does not enable hiding or preview.

0.3.13 AlwaysHideNativeList and its exact-owner/list, bounded hide scope and restore paths remain intact. To recover a blank high-risk always-hide panel: turn off 始终隐藏原生通知列表 and 隐藏原生通知列表, then reopen Notification Center; retry initialization if necessary. No installation, respring or real notification removal is performed by this change.

## Validation boundary

CI runs all existing source checks and production C suites, plus `tests/StableRefresh.c` and `tests/check_stable_refresh.py`. New tests cover default off, non-user events, already-presented and duplicate rejection, manual expansion, owner-independent state, screen-off/dismissal and next pull, unsupported supplementary mode, reuse/content/config/privacy gates and scroll clamping. Source contracts verify runtime-hook wiring and privacy cleanup. Existing Objective-C Foundation enumeration tests run on macOS. These tests are not UIKit lifecycle, gesture integration, real scroll anchoring or device privacy tests.

Both schemes must compile separately on real macOS Actions with their own Theos repositories. `tests/check_package.py` validates exact package metadata, payload whitelist, SpringBoard-only filter, Preferences versions/icons, arm64/arm64e Mach-O slices, install names, dependencies, minimum OS, embedded signature page hashes, new preference keys and presentation selectors. Device acceptance still required: lock/new notification bursts; unlock and preview opt-out; cancelled pull; new pull after dismissal; owner reconstruction; capsule tap; rotation/dynamic type; deleted/replaced card during pending read; both native-list recovery switches.
