# Overdrive and PS4/PS5 package integration status

## Confirmed facts

The upstream Overdrive project describes `Overdrive.exfat` as a plain exFAT volume containing a PS5 homebrew application. The app is built on `ps5-react`; its JavaScript bundle is embedded in `eboot.bin`. The image is intended for ShadowMount+ on a jailbroken PS5, and the upstream install instructions require the PS5-side payloads (ShadowMount+ and, in the current upstream instructions, kstuff). It is not an MSIX, APPX, Windows executable, or normal PS5 PKG installer.

References:
- https://github.com/MyLuxy/overdrive
- https://github.com/drakmor/ShadowMountPlus

## What this branch changes

- Adds a Homebrew / Overdrive entry point to the existing Xbox launcher.
- Provides links to public homebrew discovery pages.
- Scans the configured game folders and up to two nested folder levels for `.exfat` images.
- Reports the found paths without claiming the images were mounted, installed, or executed.

## Why Overdrive cannot be launched by this change

The scanner is not a filesystem mount implementation. UWP does not expose the PS5 kernel facilities used by ShadowMount+; the PS5 application also expects the PS5 runtime, APIs, executable loader, and system services. The current KytyPS5 Xbox port does not implement that complete runtime for this app. Renaming the image or adding it to the game library cannot bridge that gap.

The uploaded binary's raw bytes were not independently parsed in this tool session. The format assessment above is based on the upstream project's own build and install documentation, not on a claim that every byte of the supplied copy was validated.

## PKG support status

The launcher has a diagnostic scanner that reads only the first four bytes and file size, and labels the magic as possible CNT/PS4, FIH/PS5, or unknown. This is deliberately not a complete parser: it does not validate cryptographic signatures, internal metadata, offsets, bounds, licenses, or integrity. The Xbox UWP app still has no PKG extraction, installation, or execution pipeline. A complete implementation requires separately validated parsers, bounds/overflow checks, trusted legal test fixtures, and clear separation between recognition, extraction, installation, and execution. Do not infer that parsing or extracting a package makes it runnable.

No package decryption, signature bypass, DRM circumvention, or unauthorized commercial-game distribution is part of this branch.

## Legal downloads

The panel currently links to discovery pages only. It does not download or install content. A later downloader should use explicit, authorized URLs and source/license metadata, verify integrity and available storage, and never advertise a downloaded file as runnable unless the emulator supports its actual format.

## Build and test status

The repository's `.github/workflows/xbox.yml` creates the Xbox Developer Mode MSIX and dependency packages on a Windows runner. The branch `overdrive-integration-analysis-2026-10-22` contains the integration diagnostics and is the branch that must be built to validate these changes. A successful code commit is not a successful build. The workflow must finish successfully and the resulting package must be installed and tested on an Xbox before the integration can be called functional.

The workflow has a push trigger configured for this branch. This commit is intended to trigger a cloud build of the correct branch. Verify the actual run status and artifacts before treating the build as complete.


## Next implementation gate (do not skip)

1. **Storage/import UX:** add an Xbox-supported file picker or an explicit authorized downloader that writes to `LocalState/Games`. Use streaming I/O and a temporary filename; check free space, HTTP status, cancellation, and integrity before renaming. Never buffer a multi-gigabyte package in memory.
2. **PKG parser:** work from documented format references and legally created test fixtures. Parse the fixed header and table bounds with checked 64-bit arithmetic; reject truncated files, impossible offsets, integer overflow, and inconsistent lengths. Report metadata as *unverified* until the cryptographic and package-specific checks are implemented. Do not call a file installable based on its four-byte magic.
3. **Execution compatibility:** map each accepted package type to an implemented loader and runtime capability. If there is no loader, show a clear “not supported yet” state rather than a Play action.
4. **Overdrive feasibility:** treat the upstream PS5 app as a separate compatibility project. First inventory its executable format, imports, system calls, and runtime APIs; do not claim launch support until a minimal legal homebrew test app executes end-to-end on Xbox.
5. **Acceptance tests:** include empty/truncated/unknown/oversized PKGs, invalid offsets, interrupted imports, low storage, and a known legal homebrew fixture. Require a successful cloud build *and* a real Xbox Device Mode smoke test for release claims.

## Current user-facing claim boundary

The present branch is a launcher/build experiment with diagnostics, not a functioning PS4/PS5 PKG installer and not an Overdrive runtime. A successful MSIX build only proves compilation and packaging. The next code change should implement one bounded, testable capability at a time and preserve this distinction in the UI and release notes.
