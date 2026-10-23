# 0.3.5 — direct 0.3.3 enumeration repair

Base: `17f3f43417bfd7a2993053bf443283d2d8d39acb` (`feature/0.3.3-performance-text`). No 0.3.4 merge/cherry-pick, synchronous prepareFirstFrame, four-color UI port, install/respring, or real notification removal.

## Evidence and limits
The supplied iOS 16.2 / Dopamine RootHide screenshots show native QQ notifications while 0.3.3 reports three sections and zero requests. That is not proof of an empty native model, and authentication/preview gating does not explain missing request metadata.

Confirmed source defects: 0.3.3 reports enumerationSupported merely because a section getter returned a non-nil object; its per-section traversal only checks allNotificationRequests and filteredNotificationRequests. A recognized outer collection with unrecognized section/group objects can therefore produce misleading zero data. It also refreshes all active owners in arbitrary NSHashTable order, letting stale controller diagnostics overwrite the live controller. IDRead accepts only literal @ return encoding, excluding qualified/annotated object returns. These are source-level facts; which specific shape occurs on this device remains an on-device inference, not a claimed runtime capture.

## Implementation
A Foundation-only adapter shared verbatim with executable host tests reads fixed getter paths: master allNotificationRequests, _notificationSectionListsForEnumeration, sectionLists, notificationSections, incomingSectionList, historySectionList and missedSectionList. It traverses NSArray/NSDictionary values/NSSet/NSOrderedSet and fixed section/group getters allNotificationRequests, notificationRequests, filteredNotificationRequests, hiddenNotificationRequests, allNotificationGroups, notificationGroups and hiddenNotificationGroups. Empty aggregate getters do not suppress other structural paths. No KVC, ivars, runtime-discovered arbitrary selectors or notification text/IDs in diagnostics. Arity and object-return ABI are checked before calls (object @ and annotated @"Class" accepted, blocks/scalars rejected); exceptions fail closed.

Collection support, complete read, supported-empty, missing-master, unread shape and bounded partial are distinct. Identity deduplication, depth 8, at most 4096 traversal visits/request cap and a 6ms cooperative enumeration budget prevent unbounded traversal. At most 160 request metadata records are grouped/rendered; partial snapshots cannot hide the native list. Cooperative timing cannot preempt an expensive native getter, so this is not a zero-latency guarantee.

Refresh remains event-driven and coalesced. The selected owner favors an actually visible native list and recent appearance, with bounded ancestor checks, never a UIWindow tree scan. Native master insertion/modification/removal events are hooked only if all three methods are owned by that exact class and have verified void/object ABI, not inherited global hooks. Existing layout/revision cache, asynchronous refresh, inactive teardown, native-hide readiness, preview/auth policy and exact clear-confirmation code remain in place.

Diagnostics add enumerationComplete, enumerationState, enumerationReadPath, collectionClass/count, enumerationVisits, ownerCount, activeOwnerCount, selectedOwnerState and masterEventHooks. These are fixed structural metadata, not notification bodies or identifiers.

## Header research and confidence
- iOS 16.4 exported NCNotificationListComponent protocol confirms void insertNotificationRequest:, modifyNotificationRequest:, removeNotificationRequest: with one NCNotificationRequest object argument: https://github.com/lechium/iPhone_OS_16.4_20E247/blob/master/System/Library/PrivateFrameworks/UserNotificationsUIKit/NCNotificationListComponent-Protocol.h
- MTACS runtime section/group getters: https://github.com/MTACS/iOS-17-Runtime-Headers/blob/main/PrivateFrameworks/UserNotificationsUIKit.framework/NCNotificationStructuredSectionList.h
- Earlier MTACS master model exposes incoming/history/missed section lists and notificationSections: https://github.com/MTACS/Apple-Runtime-Headers/blob/master/iOS/PrivateFrameworks/UserNotificationsUIKit.framework/NCNotificationMasterList.h

The available iOS16 class-dump repository contains component protocols but not the full native model class implementations; these sources do NOT prove the precise iOS16.2 object graph. Every candidate getter is runtime-gated; unsupported variants remain native-visible and explicitly unread rather than falsely empty. Host synthetic fixtures are candidate compatibility shapes, not an iOS16 device simulation.

## Validation
Dual macOS CI builds and strict local DEB validation are required before delivery. CI compiles/runs the actual production Foundation adapter on synthetic nested collections, empty aggregate plus group wrappers, duplicate routes, wrong ABI, annotated returns, cycles, unsupported-versus-empty and traversal/time limits. Existing compiled performance/collapse/scroll/contained/layout/experience/clear tests and source/privacy contracts remain enabled. Package checks validate both arm64 and arm64e slices, SHA-256 CodeDirectory pages, iOS15 minimum, no chained fixups, exact RootHide/rootless install names and payload whitelist, SpringBoard-only injection, versions and 29/58/87 preference icons.

No physical-device installation or measurement is performed. Successful CI/package checks prove build integrity and tested adapter behavior, not that this device's exact unread shape or dropdown latency has been observed fixed.
