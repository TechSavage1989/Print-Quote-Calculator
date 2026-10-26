# Development handoff

## Read-first baseline and ownership

- App: Print Quote Calculator, Windows portable x64, WebView2 UI.
- Repository: https://github.com/TechSavage1989/Print-Quote-Calculator, confirmed public.
- Repository inspected at main cdd2d4b455b94eb773514d4a311600db20982a49: README only; no source, handoff, releases or CI history.
- Latest earlier delivered baseline: 1.5.1, sourced from this conversation's editable WebView2 tree. Windows execution was not confirmed.
- Current released baseline: v1.6.1, exact source/tag commit 1e84f4dda2049942a7961b34d36b9bf778954c15. Windows CI run 40159326458 passed and published the release. Actual published 1.6.0-to-1.6.1 UI upgrade qualification run 40160208389 passed. Public source/release publication remains authorized. The official Windows CI artifact supersedes the earlier local candidates.
- Intended single-repository update feed: https://github.com/TechSavage1989/Print-Quote-Calculator/releases/latest/download/update.json . It is published and was fetched successfully over HTTPS, serving the v1.6.1 manifest with the verified EXE digest. No authenticated updater or separate public binary repository is configured. If the owner chooses private source instead, resolve update authentication/feed architecture explicitly before publishing anything.

## Approved scope

Preserve Resin/FDM/UV quoting, ink subscription and bundle presets, per-channel pricing, reference-print ink estimates, material pack costs, profiles, labor/shipping/Etsy math, logo/PDF export, converters, themes, window placement and the web interface. Add reliable updates, a stable launch filename, tests, build/release assets and this handoff. No servers, Codex CLI automation, unattended workers, payments or API accounts. No permission to change repository visibility or create/embed credentials.

## Code map

- src/main.cpp: native host, HWND timers, window/focus handling, restricted WebView2 bridge, file export, startup, health checks.
- src/update_core.h: version 1.6.1, app ID, HTTPS feed, manifest/asset validation and timing/backoff policy.
- src/update_windows.h: WinHTTP worker, conditional requests, bounded downloads, SHA-256 and PE validation, helper install/rollback and cleanup.
- src/bridge.js / src/updater_ui.js: desktop-only update messages/UI and active-operation guards.
- web/: unchanged calculator design and features. scripts/build_assets.py creates embedded.html and standalone HTML. Desktop bridge/UI are injected only into embedded.html.
- assets/app.rc: icon, manifest and numeric/string executable identity at link time. No post-build executable patching.
- scripts/package_release.py / verify_artifacts.py: allowlisted portable/release output, update.json and SHA256SUMS.txt.

## Startup, polling and errors

The check worker starts before WebView2 initialization/slow diagnostics. A one-second HWND-bound native timer lets the policy initiate a check when five minutes have elapsed since the prior attempt. WM_ACTIVATE initiates a focus-return check if two minutes have elapsed. Startup/manual requests have a 60-second minimum interval. All triggers share one worker. No browser polling timer is used.

Requests use GitHub's HTTPS release download feed, not the rate-limited REST API. WinHTTP follows a bounded, manually validated redirect chain. Only github.com and release-assets.githubusercontent.com are allowed, with strict repository/asset paths. Server ETags and validated manifest data are cached. Failures use capped exponential backoff and bounded numeric or HTTP-date Retry-After. A failed/offline/404 check is never marked up to date. Checks never auto-download or auto-install.

## Install and storage

- Stable distributed filename: PrintQuoteCalculator.exe. The exact current path is the later update target, even with a custom name. Existing shortcut targets are preserved; no shortcut rewriting is attempted.
- Existing WebView2 settings, theme, logo and calculator data: %LOCALAPPDATA%\PrintQuoteCalculatorWebView2\UserData, origin https://printquote.local/ , localStorage key print-quote-v1.
- Window placement: %APPDATA%\PrintQuoteCalculatorWebView2\window.json.
- One-time native settings import: %APPDATA%\PrintQuoteCalculatorNative\settings.json.
- Original Microsoft loader cache: %LOCALAPPDATA%\PrintQuoteCalculatorWebView2\Loader-1.0.3537.50.
- Update cache, transaction files, helper, downloaded payload and one rollback backup: %LOCALAPPDATA%\PrintQuoteCalculatorWebView2\Updates . Previous executable: rollback\previous.exe.
- No updater log with user data or credentials is produced. State/error text is shown in the UI; check-cache.json stores attempts, success time, backoff, ETag and manifest.

Downloads must match bounded manifest size, SHA-256, expected x64 non-DLL PE, ProductName, OriginalFilename, numeric/string file/product version. Archives are not update payloads and are never unpacked by the updater. The EXE's OriginalFilename is PrintQuoteCalculator.exe even when a user renames its filesystem name.

The current EXE is copied to a per-user helper transaction. The helper holds the parent process, validates the target image path and old digest, and signals readiness before the parent closes. The candidate is copied to a transaction-specific sibling stage on the target volume and validated again. One backup is committed in user storage. ReplaceFileW performs replacement on the target filesystem. A named event and process-specific visible native window check confirm the new app after three seconds plus a live JavaScript render probe. Failure closes only the newly launched child and restores/relaunches the old executable. Pending exports, logo operations and settings imports block installation; UI input is locked during preparation/restart.

App-owned result transactions are cleaned only after a healthy app window and helper exit. Old download-only transactions are cleaned after healthy startup. Symbolic-link/junction install paths are rejected; move the portable app to a regular folder if this applies. An incomplete install job without a result is retained for investigation rather than deleting an uncertain helper transaction. A downloaded update is not resumed after closing the app; an abandoned download-only transaction is removed and can be downloaded again. No user files or old manually downloaded EXEs are automatically deleted.

## Version/release process

1. Read repository HEAD, handoff, releases and CI first. Prefer newer confirmed source over this local tree.
2. Keep version consistent in src/update_core.h, src/main.cpp window title, assets/app.rc numeric/string versions, package.json/lockfile and release notes. Never reuse a published version or overwrite a release asset.
3. First authorized source sync must include source, vendored licenses/SDK, tests, icons, scripts, AGENTS.md, handoff and workflow. Exclude generated EXEs, objects, QA PDFs, local profiles, node_modules and release staging from Git source. Release assets contain the build output.
4. Run tests on the exact source. Public publication is authorized in this conversation. Let the main-branch Windows workflow run and inspect its exact commit and all results before tagging.
5. If tests pass and release publication is authorized, use a main-branch commit marked [release]. After the checks pass, the workflow creates the versioned tag/release against that exact commit, only if the release does not exist. Direct version tags and explicit workflow-dispatch publication are also supported. It uses GitHub's scoped ephemeral workflow token, not an embedded app credential.
6. Verify tag commit, release assets and every hosted hash after publication. Do not call the candidate ready based only on a workflow file or local EXE.
7. First manual installation bootstraps the updater; 1.5.x has no updater. The actual 1.6.0-to-1.6.1 published-release upgrade is now qualified. Repeat qualification for each new release; fixture installs alone are not a substitute.

## Commands and validation

```sh
bash build.sh  # or build.cmd; CXX/WINDRES overrides supported on Linux
npm ci
npm test
g++ -std=c++17 -O2 -Ivendor tests/update_core.cpp -o update_core_test
./update_core_test
python3 scripts/package_release.py
python3 scripts/verify_artifacts.py
```

The Windows workflow builds fixture EXEs and tests/update_native.exe. The native suite includes a missing-HWND timer negative control, real timers/elapsed time, startup single flight, focus return after two minutes, five-minute polling, actual HTTPS checks, x64 PE identity, helper replacement, unhealthy-child rollback, custom launch filename, one backup, preserved settings sentinel and deferred cleanup. It takes about nine minutes. The separate Windows smoke script starts the actual WebView2 calculator and requires visible-window/render/JS health confirmation. Test fixtures and QA executables never enter release packages.

Local validation performed in this environment is recorded in docs/LOCAL_VALIDATION.md. The earlier local candidate was cross-compiled. The released EXE was subsequently built and tested on hosted Windows Server 2025. Native install/rollback fixtures, real timer/focus checks, actual HTTPS error handling, and the actual WebView2 app startup/render/JS health check all passed. Actual released 1.6.0-to-1.6.1 download/install/restart through the app UI, project/theme/window preservation, C:-to-D: staging, one backup and helper cleanup subsequently passed. Controlled shared-IP throttling/TLS/ETag scenarios and manual Windows 10/11 UI/native file-dialog/printer checks remain unverified. Do not report these as passed.

## Windows acceptance checklist before claiming readiness

- Confirm hosted CI for the exact release commit, including actual WebView2 health; investigate failures rather than bypassing tests.
- Verify release tag, executable version/product/original name, manifest size/hash, ZIP allowlist and all hosted checksums.
- Starting from a prior update-capable release, observe startup banner, no automatic download, Download progress, hash/PE checks, explicit Install and restart, visible new calculator, helper exit and cleanup. Confirm launch filename and shortcut still work.
- Export calculator settings before and after the update and compare profiles, rates, quote inputs, theme/logo; exercise PDF save/open and converters.
- Test offline/DNS/TLS errors, HTTP 403/429 and both Retry-After formats, ETag/304, manual cooldown and focus/poll scheduling against a controlled HTTPS feed in a test-only build. Production allowlists must not be weakened.
- Test folder permissions/locks, a different download/target volume, unhealthy WebView2 startup, interrupted helper, rollback, renamed executable, preserved window geometry and one rollback backup.
- Keep native/hosted limitations explicit until each test has evidence.

## Publication validation history

The first hosted Windows run (40157804260, source a602ab249e8e3ab639f9285c50f367497b6e81eb) failed before compilation because scripts/build_assets.py relied on the platform default cp1252 text encoding. This is the negative control. The fix explicitly reads/writes UTF-8 and writes LF bundle output; the Windows build selects its native MinGW tools explicitly. Release assets are enumerated explicitly for PowerShell. The corrected Windows run 40157955595 passed every step, including release publication.

## Historical verified release v1.6.0

- Version/tag: v1.6.0. Exact release source: 65eca125de6fe45c0c0772c90f8e5de7c505a556.
- Release: https://github.com/TechSavage1989/Print-Quote-Calculator/releases/tag/v1.6.0
- Windows CI: https://github.com/TechSavage1989/Print-Quote-Calculator/actions/runs/40157955595 (success, Windows Server 2025).
- EXE: PrintQuoteCalculator.exe, 2,360,320 bytes; SHA-256 df817ea7db6466c0f80a52760b242008a1277d55f5db7fcc4f7f50d64e104a37.
- Portable ZIP SHA-256: 5aa23781df0c539c6d8b6a76fd4cdd87cd084b813b9db8572c57c4987f79af15.
- All eight public release assets were downloaded, checked against GitHub asset digests and SHA256SUMS.txt, and the EXE was checked against update.json. The portable ZIP contains only the matching EXE and READ_FIRST.txt. PE x64/GUI identity strings and absence of an EXE signing certificate were checked.
- The latest update feed returned HTTP 200 and the same manifest/digest, following only the production allowlisted GitHub and release-assets hosts. No tokens are required.
- No published asset or tag was overwritten. This handoff update is documentation-only after the verified release commit; it does not change the release binaries.

Use the GitHub release and tag as the baseline, not the older Library candidate ZIP. First install of this updater-capable version is manual; there is no release-to-release upgrade from 1.5.x because it lacked an updater. The subsequent actual published 1.6.0-to-1.6.1 upgrade passed; see the current release record below.

When incrementing the app version, also increment the CI payload fixture version in tests/update_fixture.rc and the expected test fixture versions in tests/update_native.cpp so the fixture remains newer than the installed/helper version. Never relax the production newer-version check to make a test pass.

## Current verified release v1.6.1

- Small app change: Settings now contains About Print Quote Calculator, the installed version and supported print types. Calculator web assets and pricing logic are unchanged.
- Source/tag: 1e84f4dda2049942a7961b34d36b9bf778954c15.
- Release: https://github.com/TechSavage1989/Print-Quote-Calculator/releases/tag/v1.6.1
- Build/Windows CI: https://github.com/TechSavage1989/Print-Quote-Calculator/actions/runs/40159326458 (success).
- Real published updater qualification: https://github.com/TechSavage1989/Print-Quote-Calculator/actions/runs/40160208389 (success, Windows Server 2025).
- Qualification tooling commit: 95525e1c825a7331429ea3bd2d552e1dea972895. Two earlier harness attempts failed on accessibility pattern activation; the app binaries were not changed or republished. The successful harness waits for the actual native health/cleanup and uses keyboard activation for popup buttons.
- EXE: 2,360,320 bytes, SHA-256 0c1a6055344d7dd68994f2f0a79272a3f7c8cbafefd423ca56ccb313c428771e.
- Portable ZIP SHA-256: 9370d99617873c1410c6eaaa3fbf95a49c384074d84d6d506725d1421a14786a.
- All eight public assets were downloaded and matched GitHub asset digests; the seven listed files matched SHA256SUMS.txt. The EXE matched update.json and the portable ZIP. Tag and release target match the exact successful build commit.
- Actual old released 1.6.0 was downloaded and checked against its pinned digest. Windows UI automation entered a project name, changed theme, clicked the old app's Download and Install and restart buttons, waited for a different visible app process and native health, checked the new hash/version and About UI, and confirmed project/theme/window preservation, a stable filename, one exact rollback backup, helper exit and transaction cleanup.
- Target application drive was D: and per-user updater staging/backup drive was C:, exercising the cross-drive installation path.
- The latest public feed serves 1.6.1. Existing 1.6.0 users can test using Settings, Check for updates, Download, Install and restart. No manual EXE replacement is needed for that test. Older 1.5.x installations still need a manual bootstrap.
- Remaining qualification: controlled throttling/Retry-After/TLS/ETag transport scenarios, manual Windows 10/11 native file-dialog/printer/display combinations, additional filesystems and interrupted-install recovery. Hashes are not publisher signing; release EXEs remain unsigned.

For the next release, increment native fixture versions above the app version. Also update the real-upgrade harness's pinned previous-release URL/hash (currently 1.6.0) and descriptive workflow label to the intended previous release. Do not weaken validation or overwrite old releases. The post-publication qualification workflow can be rerun after test-only fixes with a [qualify] commit; the main build ignores those test-only paths. Current documentation-only changes do not alter the released app bytes.


## README refresh (2026-10-25)

Documentation-only update: reorganized the root README around the existing app icon, release downloads, numbered first-run steps, features, settings and update guidance. Claims were checked against current source, workflows and release metadata. Application source, versions, tags, release assets and repository access are unchanged. This documentation pass did not rerun physical printing or manual Windows PC tests; existing qualification limits above still apply.

## Repository layout (2026-10-26)

The root is reserved for README.md, AGENTS.md, DEVELOPMENT_HANDOFF.md, manifests, build entry points and the existing tool configuration. src/ holds native host and desktop bridge source; web/ holds calculator UI; assets/ holds icons/resources; scripts/ holds build/package tooling; tests/ holds regression suites; sdk/ and vendor/ hold dependencies. Supplementary guides belong in docs/. Current release notes are docs/releases/RELEASE_NOTES.md; before the next app release archive the previous notes as docs/releases/v<version>.md. Never add versioned notes or build outputs at the root. Keep published package filenames unchanged.

Run `node scripts/check-layout.cjs` before committing; CI rejects unexpected tracked or unignored root entries. Layout-only changes do not increment the app version, create tags/releases, or replace published assets. Do not use a release-trigger commit marker or a release branch for cleanup work.

This layout-only revision preserves app version 1.6.1, all published assets and the native update identity. Local layout negative controls, calculator/UV/updater JavaScript checks and portable C++ policy/placement tests pass. The Windows workflow validates native build/install/rollback before promotion. Source archives without .git are supported by the layout guard. No new app release is authorized by this cleanup.
