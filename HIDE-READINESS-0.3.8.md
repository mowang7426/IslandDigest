# 0.3.8 — native-hide readiness and 1000-point height ceiling

Based on 0.3.7 (`a916a4b`), stacked onto `feature/0.3.7-four-color` only. No main merge and no 0.3.4 preparation implementation/ancestry.

## Production change, not just logging

The reported 0.3.7 state had 18 cards, a 414×187 viewport, content height 1854, stack height 1838 and `pending-onscreen-layout`. That identifies a readiness rejection, not an enumeration or candidate-owner failure; those aggregate values cannot identify the exact failing label.

`IDContainedUI.inc` now validates the current selected model count against built cards and every actual `IDCardView` shell, finite viewport/stack bounds, scroll/pan availability and full stack-bottom-plus-inset content coverage. It then checks at least one visible card for genuinely laid-out, nonempty UILabel text. A bounded depth-4 traversal includes the nested app/date header and ordinary or attributed label text. Privacy placeholders count as valid text without granting preview or open permission. Offscreen shell geometry/model coverage is still required, but offscreen label descendants no longer have to finish deferred layout or intersect the viewport. An unrelated group heading does not satisfy visible-card readiness.

The old shallow direct-arranged-label check missed nested metadata; the old total-model count comparison also rejected a correctly selected subset. These are concrete source-level rejection paths removed by this version. Gradient labels are UILabel subclasses with ordinary `.text`; no unsupported claim that gradients erase their text. These changes are not proof that the reported devices now hide correctly.

After a snapshot render, one existing coalesced, generation-cancelled native visibility check is scheduled. No synchronous first-frame preparation, global UIView hook, notification enumeration in that callback, or new layout pass there. Existing 0.3.6 window/current-owner/lease/action-ABI checks and restoration/reassertion remain. No data, stale rendering, missing readable viewport text, invalid shell/coverage or unsupported ownership still retains the native list.

Readiness diagnostics contain only reason, index, class and bounds, whitelisted in Settings. No notification text, request object, application identifier or model content is persisted.

## Height

Settings and the runtime numeric sanitizer accept 240–1000 points; default/reset remains 360. Existing safe-window intersection, bottom-control reservation and internal UIScrollView remain. A 1000-point ceiling is not a promise of a 1000-point panel on a smaller screen. Long content remains internally scrollable.

## Verification boundary

Source guardrails, shared production C readiness/geometry predicates and the Foundation enumeration fixture run in both macOS-14 / iPhoneOS16.5 SDK jobs. The new readiness fixture includes 18 cards, 17 offscreen/privacy placeholders, a selected subset, and empty/hidden/invalid/coverage/count fail-safe cases. These are predicate fixtures, not a UIKit/device screenshot test. Package checking verifies exact version, scheme layout, dual slices, SHA-256 CodeDirectory page hashes, load commands, install names, payload whitelist and new traversal selectors/diagnostic keys.

0.3.3 nonblocking/coalesced performance design, 0.3.6 ownership recovery, 0.3.7 ten four-color palettes, 0.2.9 scroll/tap ownership and 0.1.7 privacy/clear-confirmation safeguards are preserved. No device installation, respring, real notification opening or deletion is performed. No zero-flash, latency or performance guarantee is claimed.
