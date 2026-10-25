# Print Quote Calculator

Portable Windows x64 desktop calculator for Resin, FDM and UV printing. The calculator and PDF library run offline inside Microsoft WebView2. WebView2 Runtime is required; the EXE embeds the application assets, not an entire browser runtime.

Read AGENTS.md and DEVELOPMENT_HANDOFF.md before continuing development.

## Current status

Version 1.6.0 is published at [the GitHub release](https://github.com/TechSavage1989/Print-Quote-Calculator/releases/tag/v1.6.0). [Windows CI](https://github.com/TechSavage1989/Print-Quote-Calculator/actions/runs/40157955595) passed native install/rollback, real timer/focus tests and actual WebView2 startup/render health checks. Public release downloads and hashes were verified. See DEVELOPMENT_HANDOFF.md for the exact source commit and remaining manual/release-to-release qualification.

## Run and upgrade

Extract the portable package and launch PrintQuoteCalculator.exe. Keep this launch filename for future releases. If an existing shortcut targets a custom or older filename, replace that file under its existing name instead; the updater preserves the exact running path and does not rewrite shortcuts.

The first update-capable installation is manual. Versions 1.5.x cannot update themselves. Future releases use this repository's public GitHub release feed with no embedded credentials. Checks do not download or restart the application automatically. Open Settings for installed/latest versions and a manual check. An available update also appears in a compact banner.

Settings, browser data, cached loader, update downloads and rollback files live in the Windows user profile. Moving or renaming the EXE on the same computer does not reset calculator settings.

## Build and tests

Python 3 and MinGW-w64 x64 C++17 with windres are required. SDK headers, Microsoft loader and third-party licenses are included.

```sh
bash build.sh
npm ci
npm test
g++ -std=c++17 -O2 -Ivendor tests/update_core.cpp -o update_core_test
./update_core_test
python3 scripts/package_release.py
python3 scripts/verify_artifacts.py
```

Windows users can run build.cmd. The GitHub workflow also builds and runs native install/rollback, real timer/focus tests and actual WebView2 startup checks. See the handoff for limitations, data paths, release procedure and manual qualification.

Standalone browser HTML and editable SVG/ICO/PNG icons remain part of the package. Automatic EXE updates are desktop-only; the browser edition has no installer bridge.

The release EXE is not publisher signed. SHA-256 verifies content against the trusted repository release manifest; it does not establish a signing identity.
