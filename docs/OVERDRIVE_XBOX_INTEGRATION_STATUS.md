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

The current base branch has no PS4/PS5 PKG import/install pipeline in the Xbox UWP launcher. It should not treat the filename extension as proof of format. A correct implementation requires separately validated parsers for PS4-style CNT packages and PS5 FIH envelopes, bounds/overflow checks, trusted test fixtures, and clear separation between recognition, extraction, installation, and execution. Do not infer that parsing or extracting a package makes it runnable.

No package decryption, signature bypass, DRM circumvention, or unauthorized commercial-game distribution is part of this branch.

## Legal downloads

The panel currently links to discovery pages only. It does not download or install content. A later downloader should use explicit, authorized URLs and source/license metadata, verify integrity and available storage, and never advertise a downloaded file as runnable unless the emulator supports its actual format.

## Build and test status

The repository's `.github/workflows/xbox.yml` creates the Xbox Developer Mode MSIX and dependency packages on a Windows runner. A successful code commit is not a successful build. The workflow must finish successfully and the resulting package must be installed and tested on an Xbox before the integration can be called functional.
