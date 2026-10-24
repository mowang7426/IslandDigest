# 0.3.19 — Settings UI only

Base: `c52081bf7bb51d67fee758176b04aeb896594cf2` (0.3.18).

## Visible changes
- Remove collapse summary, refresh-diagnostic buttons, runtime hook audit and long runtime status blocks entirely. Settings no longer reads the diagnostic status file.
- First section now contains enable/card-mode controls. Group order: privacy, presentation/native visibility, grouping/sort/pins, category layout, background/text, position/size, manual clearing, advanced/recovery, compatibility.
- Native PSGroupCell footers replace long technical prose. Shorter row titles and choice labels; no custom header, fixed heights, fonts, colors or margins. Native Preferences owns spacing, appearance and text sizing.
- Preview warning explicitly says per-app “never preview” may not be respected; lock/protected-data requirements remain visible. Always-hide risk includes the blank-screen failure and recovery switch. Clearing remains confirmed and irreversible.
- Keep all 24 preference controls, choice values, ranges, defaults, dependencies, pin management, layout reset and retry action. Keep original branded icon assets.

## Scope / checks
- Core, notification/capsule UI and internal diagnostics unchanged byte-for-byte against 0.3.18, except version string/startup-marker version. No 0.3.4 synchronous refresh code and no new logging.
- New settings contract freezes baseline controls, helper/default behavior and 41 core/icon files; checks group order, concise warnings, removal of status readers, and absence of fixed sizing or custom colors/fonts.
- Existing internal lifecycle/diagnostic tests retained; obsolete requirements to display diagnostics in Settings replaced. Strict package checker extends both schemes to 0.3.19 and rejects old diagnostic methods in the Preferences executable.
- Both RootHide and Rootless macOS CI compile portable C/Foundation tests and real Theos packages, followed by metadata/Mach-O/payload/icon checks.

## Manual verification still required
No installation, respring or real notification interaction was performed. Native Settings layout, dark/light appearance, largest accessibility text, private iOS APIs and actual automatic-collapse behavior are NOT device-verified. Test these on the target iOS/PreferenceLoader combination before claiming device acceptance.
