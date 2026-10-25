Print Quote Calculator 1.6.0 adds background update checks and explicit download/install actions while preserving the existing WebView2 calculator interface and saved settings.

- Checks at startup, every five minutes while active, and after returning to focus following two minutes without a check.
- Validates repository URLs, size, SHA-256, x64 PE identity and version.
- Stages on the destination volume, preserves the launch filename and keeps one rollback copy.
- Requires a visible calculator window and a live JavaScript render response before confirming installation.
- Preserves Resin, FDM, UV ink estimates, converters, logos, PDF export, themes and window placement.

First installation is manual: 1.5.x builds do not contain an updater. Extract the portable ZIP and run PrintQuoteCalculator.exe, or replace the old executable using its existing filename if a shortcut targets that name. WebView2 Runtime remains required. This build is not publisher signed; SHA-256 is an integrity check against this repository's feed.

Publication is gated on Windows CI, including native install/rollback fixtures, real timers/focus checks and actual WebView2 startup/render responsiveness. A real published-release upgrade cannot be tested until a second update-capable version exists. Real cross-volume replacement and controlled throttling/TLS/ETag transport scenarios remain separate qualification tasks.
