# Print Quote Calculator: read first

Read DEVELOPMENT_HANDOFF.md next. Preserve the WebView2 interface, calculator math, converters, PDF/logo support, saved data and window placement.

Repository: https://github.com/TechSavage1989/Print-Quote-Calculator (public).
The initial inspection found only a README at cdd2d4b455b94eb773514d4a311600db20982a49; no releases or CI runs. The user explicitly authorized public source publication, Windows CI and release publication by replying "publish" to the concrete proposal in this chat. Continue that authorized publication without asking again. Do not change repository access or create/embed credentials.

Current released baseline is v1.6.1, source 1e84f4dda2049942a7961b34d36b9bf778954c15, verified by Windows CI run 40159326458. The actual published 1.6.0-to-1.6.1 in-app upgrade passed qualification run 40160208389, including project/theme/window preservation, C:-to-D: staging, one backup and cleanup. Read the handoff for exact artifacts and remaining limits. Stable launch filename: PrintQuoteCalculator.exe. Older Library candidate ZIPs are superseded. Install manually when coming from 1.5.x; 1.6.0 can update to 1.6.1 in the app.

Use native HWND timers and WinHTTP workers for update scheduling/networking. Never add browser polling, auto-download, auto-install, or silent restart. Preserve the exact current executable path for later updates, including custom filenames; do not rewrite user shortcuts.

Updates must validate the bounded HTTPS manifest, repository asset path, SHA-256, PE architecture, OriginalFilename, product identity and numeric version before launch. Hashes provide integrity against the repository feed, not publisher signing. Never unpack update archives; the update payload is one explicitly named EXE.

Settings and runtime files belong in existing per-user app storage. Keep one rollback file, and clean only owned transaction files after health confirmation and helper exit. Do not delete user-created files or old version downloads.

Before claiming a release ready, verify hosted CI, exact tag/commit, assets, hashes and the real Windows upgrade/rollback path. Linux cross-compilation is not Windows execution. Record limitations honestly in DEVELOPMENT_HANDOFF.md and the final response.

No server setup, Codex CLI automation, unattended workers, payments or API account setup. No subagents unless the user asks.

## Repository layout (2026-10-26)

The root is reserved for README.md, AGENTS.md, DEVELOPMENT_HANDOFF.md, manifests, build entry points and the existing tool configuration. src/ holds native host and desktop bridge source; web/ holds calculator UI; assets/ holds icons/resources; scripts/ holds build/package tooling; tests/ holds regression suites; sdk/ and vendor/ hold dependencies. Supplementary guides belong in docs/. Current release notes are docs/releases/RELEASE_NOTES.md; before the next app release archive the previous notes as docs/releases/v<version>.md. Never add versioned notes or build outputs at the root. Keep published package filenames unchanged.

Run `node scripts/check-layout.cjs` before committing; CI rejects unexpected tracked or unignored root entries. Layout-only changes do not increment the app version, create tags/releases, or replace published assets. Do not use a release-trigger commit marker or a release branch for cleanup work.
