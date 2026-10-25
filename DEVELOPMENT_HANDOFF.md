# Development handoff

## Read-first baseline and ownership

- App: Print Quote Calculator, Windows portable x64, WebView2 UI.
- Repository: https://github.com/TechSavage1989/Print-Quote-Calculator, confirmed public.
- Repository inspected at main cdd2d4b455b94eb773514d4a311600db20982a49: README only; no source, handoff, releases or CI history.
- Latest earlier delivered baseline: 1.5.1, sourced from this conversation's editable WebView2 tree. Windows execution was not confirmed.
- Current continuation: 1.6.0. The user explicitly authorized public source, Windows CI and release publication by replying "publish" to the prepared proposal. Source publication is in progress; CI/release results must be recorded after verification. Do not confuse the earlier cross-compiled local candidate with the Windows CI artifact.
- Intended single-repository update feed: https://github.com/TechSavage1989/Print-Quote-Calculator/releases/latest/download/update.json . It does not exist until the first authorized release is published. No authenticated updater or separate public binary repository is configured. If the owner chooses private source instead, resolve update authentication/feed architecture explicitly before publishing anything.

## Approved scope

Preserve Resin/FDM/UV quoting, ink subscription and bundle presets, per-channel pricing, reference-print ink estimates, material pack costs, profiles, labor/shipping/Etsy math, logo/PDF export, converters, themes, window placement and the web interface. Add reliable updates, a stable launch filename, tests, build/release assets and this handoff. No servers, Codex CLI automation, unattended workers, payments or API accounts. No permission to change repository visibility or create/embed credentials.

## Code map

- main.cpp: native host, HWND timers, window/focus handling, restricted WebView2 bridge, file export, startup, health checks.
- update_core.h: version 1.6.0, app ID, HTTPS feed, manifest/asset validation and timing/backoff policy.
- update_windows.h: WinHTTP worker, conditional requests, bounded downloads, SHA-256 and PE validation, helper install/rollback and cleanup.
- bridge.js / updater_ui.js: desktop-only update messages/UI and active-operation guards.
- web/: unchanged calculator design and features. build_assets.py creates embedded.html and standalone HTML. Desktop bridge/UI are injected only into embedded.html.
- app.rc: icon, manifest and numeric/string executable identity at link time. No post-build executable patching.
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
2. Keep version consistent in update_core.h, main.cpp window title, app.rc numeric/string versions, package.json/lockfile and release notes. Never reuse a published version or overwrite a release asset.
3. First authorized source sync must include source, vendored licenses/SDK, tests, icons, scripts, AGENTS.md, handoff and workflow. Exclude generated EXEs, objects, QA PDFs, local profiles, node_modules and release staging from Git source. Release assets contain the build output.
4. Run tests on the exact source. Public publication is authorized in this conversation. Let the main-branch Windows workflow run and inspect its exact commit and all results before tagging.
5. If tests pass and release publication is authorized, use a main-branch commit marked [release]. After the checks pass, the workflow creates tag/release v1.6.0 against that exact commit, only if the release does not exist. Direct version tags and explicit workflow-dispatch publication are also supported. It uses GitHub's scoped ephemeral workflow token, not an embedded app credential.
6. Verify tag commit, release assets and every hosted hash after publication. Do not call the candidate ready based only on a workflow file or local EXE.
7. First manual installation bootstraps the updater; 1.5.x has no updater. Qualify the real 1.6.0-to-next-published-version path separately with the Windows checklist. Fixture-based installs are not a substitute for a published release upgrade.

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

Local validation performed in this environment is recorded in LOCAL_VALIDATION.md. All Windows executables are cross-compiled here. Native Windows, hosted CI, real cross-volume replacement, shared-IP network throttling, TLS/HTTP error branches, and a real previous-release-to-hosted-new-release upgrade remain unverified. Do not report these as passed.

## Windows acceptance checklist before claiming readiness

- Confirm hosted CI for the exact release commit, including actual WebView2 health; investigate failures rather than bypassing tests.
- Verify release tag, executable version/product/original name, manifest size/hash, ZIP allowlist and all hosted checksums.
- Starting from a prior update-capable release, observe startup banner, no automatic download, Download progress, hash/PE checks, explicit Install and restart, visible new calculator, helper exit and cleanup. Confirm launch filename and shortcut still work.
- Export calculator settings before and after the update and compare profiles, rates, quote inputs, theme/logo; exercise PDF save/open and converters.
- Test offline/DNS/TLS errors, HTTP 403/429 and both Retry-After formats, ETag/304, manual cooldown and focus/poll scheduling against a controlled HTTPS feed in a test-only build. Production allowlists must not be weakened.
- Test folder permissions/locks, a different download/target volume, unhealthy WebView2 startup, interrupted helper, rollback, renamed executable, preserved window geometry and one rollback backup.
- Keep native/hosted limitations explicit until each test has evidence.
