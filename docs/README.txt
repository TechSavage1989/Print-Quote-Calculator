Version 1.6.1 is published after passing hosted Windows CI. Read README.md and DEVELOPMENT_HANDOFF.md for updater behavior and release status.

3D Print Quote Calculator 1.6.1 - WebView2 Edition

RUN
Open PrintQuoteCalculator.exe on Windows 10 or Windows 11 x64. It is a single portable application executable. The computer must have Microsoft Edge WebView2 Runtime installed. If the runtime is missing, the app offers Microsoft's official download page. Choose Evergreen Runtime, install it, and reopen the app.

The full calculator page, styles, pricing logic, converters, PDF library and icon are embedded in the EXE. It does not load the hosted website, run a local server, or download app assets. Once WebView2 Runtime is installed, the calculator works offline. Runtime installation and updates are separate Microsoft operations.

This edition displays the actual web interface. It does not recreate the web interface with native form controls. Resizing updates the WebView2 bounds and lets CSS handle the layout. First launch is maximized; later launches restore window position, size and maximized state. The layout remains responsive on smaller windows, where scrolling may be necessary.

FEATURES
Resin, FDM and UV printing retain separate inputs and rates. The primary project fields, pricing, quote and custom-price check remain on the main screen. Rates, fees, logo and settings transfer are available through Settings. Converters has its own separate dialog, opened directly from the main header. The icon is flat and minimalist, with solid indigo, mint and gold colors. It has no gradients, shadows or surface detailing.

UV PRINTING
The initial printer profile is EufyMake E1. Add named profiles in Settings for other printers or pricing setups. Each profile keeps its own ink purchase and machine rates. Labor, shipping, sales channel fees and profit margin use the same pricing method as Resin and FDM. Machine hourly rate, power and electricity start at zero for UV and should be entered for your printer when known.

Ink presets use the prices supplied for this update: $29.99 per 100 ml; individual subscription purchases at $19.99, $21.89 or $23.99 per 100 ml; ink and cleaning kits at $209.99, $139.98, $153.29 or $167.99 per 600 ml of printable ink. Subscription intervals are 4, 8 and 12 weeks. These are editable purchase prices, not live retailer prices. Presets do not charge a recurring fee on each quote.

The kit's separate 380 ml cleaning cartridge is not printable ink. Use the non-ink bundle allocation to assign part of a kit purchase to cleaning or other supplies; zero conservatively assigns the whole purchase cost to printable ink. Cleaning usage can be included as job extras. Custom purchases support other bundles and suppliers.

For advance quotes, open Ink estimate and choose a completed print as a reference. The included examples use your supplied print-history totals: 0.23, 1.25, 4.73, 0.48, 4.64 and 33.53 ml. Adjust relative ink use for a new job, or enter your own measured reference volume. A factor of 1 uses the reference volume, 2 doubles it, and 0.5 halves it. Reference dimensions and layer settings were not supplied, so these are rough comparison examples, not calibrated area or coverage presets. Estimates are labeled on screen and in the PDF. Print time does not determine ink volume. A visual software indicator is not assumed to have a measurable ml scale.

For completed prints or known usage, select total ml per run, or use the per-color mode to enter C, M, Y, K, W, flexible W and gloss separately. Optional per-channel prices override the pooled purchase price per ml. Rounded channel values may not equal the software's total; use total-volume mode when that total is the more reliable figure. There is no automatic image-to-ink prediction.

Blank/material quantity is for the entire job. Enter its unit price directly or use Pack cost to divide the supplier purchase price by the number of usable pieces from a pack or sheet. Optional material/supplier notes appear on the exported estimate. Blanks are not multiplied again by the print-run batch multiplier. Ink, print time and per-run labor are multiplied by the number of runs; additional labor and extras apply once to the job.

Existing Resin/FDM settings remain compatible. Older settings files without UV receive UV defaults. The first migration copies common labor and fee settings from the last selected calculator. Settings export/import includes UV profiles and job inputs.

The converters include mm, cm, m, inches and feet, plus filament meters-to-grams and grams-to-meters. Density and diameter are editable. Calculated filament weight can be applied to the FDM quote.

Logo upload accepts PNG and JPEG and is retained for PDF exports. Download PDF uses a native Save As dialog. Open PDF for printing saves the PDF and opens it in the default PDF viewer, where printing is available. A PDF viewer must be installed for that last step. The exported document remains an internal pricing estimate that includes costs and profit.

Settings export also uses a native Save As dialog. Settings import and logo upload use the web view's normal file picker. Light and dark themes use the original web app CSS, including the quote area, and update the Windows title bar theme.

DATA AND MIGRATION
On first launch, this edition imports the last native edition's rates, project inputs, theme, logo and window placement when available. Existing WebView2 data takes precedence on subsequent launches. Browser and older Electron data are separate; transfer those using Settings > Settings transfer > Export settings and then Import settings in this edition.

The WebView2 browser data, including saved calculator inputs and logo, lives under:
%LOCALAPPDATA%\PrintQuoteCalculatorWebView2\UserData

Window placement is saved under:
%APPDATA%\PrintQuoteCalculatorWebView2\window.json

The original signed Microsoft WebView2Loader.dll is embedded and cached once under:
%LOCALAPPDATA%\PrintQuoteCalculatorWebView2\Loader-1.0.3537.50

The loader is about 160 KB. A matching existing copy is reused; the browser runtime is not unpacked on every launch. Moving the EXE on the same computer preserves saved settings. Moving to a different computer requires a settings export/import to transfer those settings. Startup timing has not been measured on Windows.

STANDALONE HTML
3D_Print_Quote_Calculator_Offline.html contains the browser edition with all assets embedded. It does not require WebView2 Runtime. Open it in a current browser. Browser downloads and PDF handling apply in that edition.

EDITABLE ICON
Print_Quote_Icon.svg is the editable vector master. PNG and multi-size ICO exports are included. assets/app.rc embeds the icon during compilation. No finished executable is patched, and no icon file is required beside the executable.

BUILD
Requirements: Python 3 and a MinGW-w64 x64 C++17 compiler with windres. All source headers and the Microsoft loader are included, so the build needs no dependency downloads.

On Windows, run build.cmd with g++ and windres on PATH.
On Linux, use build.sh with a MinGW-w64 compiler; CXX and WINDRES can point to the installed tools.

scripts/build_assets.py builds the embedded document and standalone HTML from web/. src/bridge.js supplies the restricted WebView2 message channel. src/main.cpp provides the host, offline page response, native Save As operations, runtime detection, settings migration, title-bar theme, single-instance handling and window placement. The app serves only its embedded page and blocks external navigation and network requests from the page.

TESTS
For the JavaScript integration checks, install jsdom@26.1.0 in a temporary test environment, then run tests/test_webview.cjs. These checks execute the actual embedded page with a simulated WebView2 native channel. They cover migration, rates, quotes, PDF generation with a logo, PDF cancellation and errors, the print request, converters, a zero custom price, settings export, menus, theme changes and localStorage persistence across a reload.

Run tests/test_uv.cjs with the same test environment for UV total/per-channel ink costing, subscription and bundle rates, bundle allocation validation, pack unit costs, whole-job blank quantities, printer profile isolation, PDF generation, UV settings transfer and legacy imports.

tests/check_placement.cpp is a portable test of placement persistence and monitor recovery. Build with a standard C++17 compiler and -Ivendor -I..

The released executable was built in Windows CI. Native install/rollback, real timers/focus, and actual WebView2 startup/render/JS health checks passed. Manual Windows 10/11 file dialogs, printers, display configurations, controlled throttling/TLS/ETag scenarios remain unverified. The actual published 1.6.0-to-1.6.1 in-app upgrade, C:-to-D: staging, project/theme/window preservation and cleanup passed Windows qualification. See DEVELOPMENT_HANDOFF.md for the exact release commit and CI evidence.

NOTICES
The original Microsoft WebView2 SDK header and signed x64 loader are from Microsoft.Web.WebView2 1.0.3537.50. License and notices are retained in sdk/. uuid_mingw.h and EventToken.h supply MinGW compatibility declarations without modifying the official header or loader. nlohmann/json and pdf-lib license notices are retained under vendor/ and web/vendor/. MinGW/GCC runtime support libraries use the GCC Runtime Library Exception. Application code and icon source are included for editing.
