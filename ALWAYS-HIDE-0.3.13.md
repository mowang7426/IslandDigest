# Always-hide native list — 0.3.13

This branch is stacked on `feature/0.3.12-first-frame-handoff` at `7e7508bf522f88afd2450f94846933a95989c7e3`; it does not target `main`.

## Behavior

When **插件内显示通知卡片**, **隐藏原生通知列表**, and **始终隐藏原生通知列表（高风险）** are enabled, the SpringBoard hook claims only the current `NCNotificationStructuredListViewController`'s typed `masterListView` when it is a visible same-window descendant. It sets that exact `NCNotificationListView` hidden synchronously in `viewWillAppear:` and again after `viewWillAppear:`. The `setHidden:` and `layoutSubviews` hooks reassert the lease synchronously if UIKit attempts to reveal it.

The always-hide path deliberately does not wait for enumeration, card rendering, layout readiness, a panel, or a full view traversal. It performs only the typed getter, identity/window/descendant checks, a bounded ancestor check, and one active owner lease. It never hides a window, banner, owner root, or unrelated view. The existing conservative path remains unchanged when the high-risk switch is off.

A blank notification center is an accepted failure mode. Diagnostics expose `alwaysHideActive`, `nativeDecisionStage=always-hide-immediate`, `nativeHidden`, and a recovery reason. To recover, turn off **始终隐藏原生通知列表（高风险）**, then **隐藏原生通知列表**, or disable **插件内显示通知卡片**; reopen Notification Center after the preference notification is delivered. Lock/protected-data redaction still clears plugin card content and does not expose notification text.

## Verification boundary

All source-contract checks pass locally. This environment has no Theos/iPhoneOS SDK (`THEOS` and `CA_THEOS_SDKROOT` are unset), so dual RootHide/rootless arm64/arm64e `.deb` builds require the macOS-14 GitHub Actions workflow. The workflow is pinned to version 0.3.13 and checks both schemes.
