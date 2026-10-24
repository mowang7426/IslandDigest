# 0.3.18 — primary lifecycle presentation boundary

## Evidence and root cause

1. **Device evidence (high):** the supplied 0.3.17 diagnostic shows runtime slider `SBCoverSheetPrimarySlidingViewController`, manager prepare reached with `userGesture=false`, accepted/cycle stayed `0`, and raw/helper/indirect recognizer paths all stayed `0`. Therefore 0.3.17 deliberately rejected the path the device actually used.
2. **Header/ABI evidence (high for declaration, not device execution):** pinned iOS 17 runtime headers declare the primary subclass's own `_beginTransitionFromAppeared:` and `_endTransitionToAppeared:` methods. The base class declares `setPresented:forUserGesture:animated:withCompletion:` with three booleans and a completion block. Runtime installation accepts `B`/`c` for boolean and `@?`/`@` for the block after checking arity and return type.
3. **Generated Logos evidence (build-time):** internal Logos output registers both lifecycle hooks directly on `SBCoverSheetPrimarySlidingViewController`; it does not install them on `UIViewController`. The generated primary override calls the captured original exactly once.
4. **Inference (medium, requires device acceptance):** a primary transition from not-appeared to appearing, while `uiLockStateProvider.isUILocked` is known false, is the narrow real visibility edge for an unlocked pull. It is stronger than notification/owner renders and does not depend on the unreliable `forUserGesture` flag.

## Boundary and behavior

- New cycle: primary runtime identity + `_beginTransitionFromAppeared:NO` + no current occupancy + known unlocked + feature prerequisites.
- Duplicate prepare/show/begin callbacks during one occupancy do not re-collapse.
- Tapping the existing transparent bell/count capsule expands it; model/new-notification refresh retains that expanded state for the cycle.
- `_endTransitionToAppeared:NO` on the primary ends the cycle, covering cancelled pull and exit; the next pull can collapse again.
- Screen-off ends the cycle and clears occupancy. A presentation beginning while locked marks the boundary occupied but never collapses; an unlock/status change in place cannot manufacture a fresh edge.
- Non-primary subclass instances cannot begin/end this state.
- manager/slider `forUserGesture` callbacks and recognizer callbacks remain diagnostics only. Owner appearance, rendering, enumeration, and notification changes never begin a cycle.
- Existing `IDDigestView` capsule and its 44-point transparent bell/count presentation are reused unchanged. Notification actions, deletion, installation and respring are untouched.

## Verification scope

Executable state tests cover new cycle, duplicate show, same-cycle manual expansion, notification refresh state, locked wake, unlock in place, cancelled pull, exit/re-pull, non-primary dispatch and disabled/enabled in-place behavior. A Foundation runtime fixture covers primary subclass override dispatch, original-call count, end reset and BOOL/block ABI encodings. CI additionally regenerates Logos and builds/verifies both RootHide and rootless packages on macOS 14 with SDK 16.5.

This is not a claim of device success. The exact iOS 17.0 RootHide transition execution/order and visual behavior still require installing one matching package and testing on the user's device.
