# Print Quote Calculator: read first

Read DEVELOPMENT_HANDOFF.md next. Preserve the WebView2 interface, calculator math, converters, PDF/logo support, saved data and window placement.

Repository: https://github.com/TechSavage1989/Print-Quote-Calculator (public).
At inspection it contains only a README at cdd2d4b455b94eb773514d4a311600db20982a49; no releases or CI runs. The user explicitly authorized public source publication, Windows CI and release publication by replying "publish" to the concrete proposal in this chat. Continue that authorized publication without asking again. Do not change repository access or create/embed credentials.

Latest delivered baseline is 1.5.1 from this conversation, not yet confirmed on Windows. The update-capable continuation is 1.6.0, with a stable PrintQuoteCalculator.exe filename. No released 1.5.x executable contains an updater; first install of 1.6.0 is manual.

Use native HWND timers and WinHTTP workers for update scheduling/networking. Never add browser polling, auto-download, auto-install, or silent restart. Preserve the exact current executable path for later updates, including custom filenames; do not rewrite user shortcuts.

Updates must validate the bounded HTTPS manifest, repository asset path, SHA-256, PE architecture, OriginalFilename, product identity and numeric version before launch. Hashes provide integrity against the repository feed, not publisher signing. Never unpack update archives; the update payload is one explicitly named EXE.

Settings and runtime files belong in existing per-user app storage. Keep one rollback file, and clean only owned transaction files after health confirmation and helper exit. Do not delete user-created files or old version downloads.

Before claiming a release ready, verify hosted CI, exact tag/commit, assets, hashes and the real Windows upgrade/rollback path. Linux cross-compilation is not Windows execution. Record limitations honestly in DEVELOPMENT_HANDOFF.md and the final response.

No server setup, Codex CLI automation, unattended workers, payments or API account setup. No subagents unless the user asks.
