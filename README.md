<p align="center">
  <img src="Print_Quote_Icon.png" alt="Print Quote Calculator icon" width="112">
</p>

<h1 align="center">Print Quote Calculator</h1>

<p align="center">Turn Resin, FDM and UV print estimates into clear, itemized quotes.</p>

<p align="center">
  <a href="#download">Download</a> · <a href="#getting-started">Get started</a> · <a href="#features">Features</a> · <a href="#updates-and-settings">Updates</a> · <a href="#development">Development</a>
</p>

---

## Features

| Estimate costs | Set your price | Share the quote |
| --- | --- | --- |
| Materials, labor, printer time, electricity, extras and shipping | Target margin, selling fees, break-even and custom-price comparison | PDF download or printing, with your own logo |

- **Three print modes:** Resin, FDM and UV, with saved inputs and rates for each.
- **UV tools:** ink subscription/bundle presets, per-channel costs, reference-print estimates and substrate pack costing.
- **Everyday helpers:** length/filament converters, light/dark themes and settings import/export.
- **Offline calculation:** calculator assets and the PDF library run locally.

## Download

**[Get Print Quote Calculator →](https://github.com/TechSavage1989/Print-Quote-Calculator/releases/latest)**

Choose from the release's **Assets**:

| Edition | File | Use |
| --- | --- | --- |
| Windows portable | `PrintQuoteCalculator-<version>-Windows-x64.zip` | Desktop app with in-app EXE updates |
| Standalone browser | `PrintQuoteCalculator.html` | Download and open in a browser; no installer bridge |

GitHub's **Source code** archives are for development. Release notes, checksums and editable SVG/ICO/PNG icons are on the release page.

## Requirements

- **Desktop:** Windows x64 and [Microsoft Edge WebView2 Runtime](https://developer.microsoft.com/en-us/microsoft-edge/webview2/). The app embeds its assets, not a whole browser runtime.
- A regular writable folder for the portable app.
- Internet access for initial downloads and update checks, not for calculating quotes.

The EXE is **not publisher signed**. Release checksums verify content against the trusted repository, not a signing identity.

## Getting started

1. **Extract the Windows ZIP** and run `PrintQuoteCalculator.exe`. Install WebView2 Runtime if prompted. For the browser edition, open the downloaded HTML instead.
2. **Choose Resin, FDM or UV Printing.**
3. **Set your real costs** in **Settings → Saved rates** and **Fee settings**.
4. **Enter project estimates and pricing.** Review the recommended quote and cost breakdown, or use **Try your own price**.
5. **Share the result:** choose **Download PDF** or **Open PDF for printing**. Add a PNG/JPG logo under **Settings → Export logo** if wanted.

> Prices are in USD. Customer totals include shipping and exclude sales tax. Target margin is not markup. Fee defaults are editable estimates, not a live fee feed—check your actual costs and selling-channel fees.

## How to use

- **Include all material:** supports, rafts and purge belong in the slicer estimate.
- **Separate labor from machine time:** print labor is hands-on time per run; additional labor applies once per job.
- **Repeat identical runs with the batch multiplier.** For different runs, add their estimates and use a multiplier of 1.
- **Treat UV references as comparisons**, not calibrated coverage presets. Use measured total/per-channel ink when available; print time does not predict ink use.
- **Keep your rates:** **Clear project estimates** leaves rates saved. Find the calculation explanation in **Settings → Quick guide**.

## Updates and settings

On desktop, open **Settings → About Print Quote Calculator → Check for updates**. Choose **Download** when an update is available, save your work, then **Install and restart**. Background checks are automatic; **downloads and restarts always require your action**.

- **Coming from 1.5.x?** Install the portable package manually to get the updater.
- **Back up or transfer:** use **Settings → Settings transfer → Export settings** for rates, project inputs and your logo. Browser data belongs to that browser; use export/import to move it.
- **Keep shortcuts working:** retain the EXE's folder and filename. The updater preserves custom names and does not rewrite shortcuts.
- Calculator data lives in `%LOCALAPPDATA%\PrintQuoteCalculatorWebView2\UserData`; window position lives in `%APPDATA%\PrintQuoteCalculatorWebView2\window.json`. Moving the EXE on the same computer does not reset those settings.

## Troubleshooting

- **App will not start?** Confirm WebView2 Runtime is installed and you extracted the portable package.
- **Update check failed?** Check connectivity and respect any retry delay. Offline/failed checks do not mean the app is up to date.
- **Update will not install?** Finish exports or settings/logo operations, and use a regular folder rather than a symbolic link or junction.
- **Closed the app after downloading?** Download the update again; downloads do not resume across restarts.

Update cache and one rollback backup are kept under `%LOCALAPPDATA%\PrintQuoteCalculatorWebView2\Updates`. Hosted tests do not cover every Windows PC, printer, display or native file dialog.

## Development

<details>
<summary><strong>Build instructions, tests and release evidence</strong></summary>

Read [AGENTS.md](AGENTS.md) and [DEVELOPMENT_HANDOFF.md](DEVELOPMENT_HANDOFF.md) first.

Build dependencies: Python 3 and MinGW-w64 x64 C++17 with `windres`; Node.js/npm for JavaScript tests. SDK headers, the Microsoft loader and third-party licenses are included.

```sh
bash build.sh
npm ci
npm test
g++ -std=c++17 -O2 -Ivendor tests/update_core.cpp -o update_core_test
./update_core_test
python3 scripts/package_release.py
python3 scripts/verify_artifacts.py
```

Windows users can run `build.cmd`. The Windows workflow also tests native installation/rollback, real timers/focus and actual WebView2 startup.

- [1.6.1 Windows build and tests](https://github.com/TechSavage1989/Print-Quote-Calculator/actions/runs/40159326458).
- [Published 1.6.0 → 1.6.1 UI upgrade qualification](https://github.com/TechSavage1989/Print-Quote-Calculator/actions/runs/40160208389): project/theme/window preservation, cross-volume staging and cleanup.
- [Development handoff](DEVELOPMENT_HANDOFF.md): exact source, hashes, release process and remaining transport/recovery qualification.
- [Local validation](LOCAL_VALIDATION.md): local checks and limits.

</details>
