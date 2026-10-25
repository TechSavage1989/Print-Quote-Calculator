# Local validation for 1.6.0 candidate

This is a cross-compiled local candidate, not a Windows-qualified or published release.

| Check | Result |
|---|---|
| GitHub baseline and visibility | Public repository, README only at cdd2d4b455b94eb773514d4a311600db20982a49; no releases, CI runs or existing handoff |
| Existing WebView2 calculator integration suite | Passed: quotes, migration, logo/PDF, cancel/error/print bridge, converters, custom zero, menus, theme, settings export and restart persistence |
| UV integration suite | Passed: reference estimates, scaling, validation, total/per-channel prices, bundle allocation, substrate quantities/costs, profile isolation, PDF, import/export and persistence |
| Desktop update UI integration suite | Passed: explicit actions, no auto-download/install, same-version display, progress, shortened notes, escaping, active-operation guard and failed-check text |
| Portable update policy/core suite | Passed: numeric versions, manifest identity, repository/asset allowlist, hash/size bounds, malformed notes, poll/focus/manual intervals, backoff and cooldown |
| Baseline preservation | All web files, placement.h, SVG/ICO/PNG and original Microsoft loader match the delivered 1.5.1 bytes |
| Windows application cross-compilation | Passed with MinGW-w64 x64 C++17; embedded page and original loader verified in the EXE |
| Native Windows test harness compilation | Passed; GUI health fixtures and native helper/timer/focus test harness build |
| Release packaging verification | Passed: executable bytes, PE x64 headers, manifest digest and size, all checksums and exact portable ZIP allowlist |
| Native Windows execution | Not performed |
| Hosted Windows CI | Not run; source publication requires approval |
| Published release/tag/assets | Not created; no hosted links or hashes verified |
| Real old-release download/install/relaunch | Not performed; 1.5.x has no updater. Requires a first manual updater-capable installation and a subsequent genuine published version |
| Real cross-volume replacement, offline/TLS/429/Retry-After/ETag transport scenarios | Not performed; qualification checklist remains in the handoff |

Candidate executable SHA-256: 36e75b9b7e96e364468e569bd96ed2e5dcb406e9a44de9c97f105f309c2b6db3

The SHA-256 describes these local candidate bytes. It does not establish publisher signing. CI rebuilds can produce different bytes; the release manifest must be generated from the exact CI artifact and verified after hosting.

The source package includes AGENTS.md, DEVELOPMENT_HANDOFF.md, build scripts, vendored dependencies/licenses, editable icons, calculator tests, updater tests and the Windows workflow. It excludes compiled test executables, objects, browser profiles, QA PDFs and update staging.
