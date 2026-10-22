# Overdrive EXFAT compatibility status

## What the file is

`Overdrive.exfat` is a PlayStation 5 homebrew image intended to be detected and mounted by ShadowMount+ on a jailbroken PS5. It is not a Windows/UWP package, Xbox app package, or ordinary PS5 `.pkg` installer.

## Current KytyPS5 Xbox status

The launcher can now scan configured game folders (including two nested folder levels) for `.exfat` images and report their paths. This is discovery only. It does not mount the image, install an app, or execute the contents.

## Missing pieces before an app could launch

1. A reliable, read-only EXFAT image/filesystem reader that can operate under UWP storage restrictions.
2. A verified way to enumerate the image's root and locate PS5 app metadata and `eboot.bin`.
3. Compatibility for the PS5 app's executable format, imports, system calls, graphics/audio/input APIs, and runtime requirements.
4. A supported app/library loading path in the Xbox port, plus test fixtures and repeatable automated builds.

ShadowMount+ is a PS5-side payload; its mounting/registration behavior cannot be assumed to work in an Xbox UWP process. Simply adding the file to the game library or changing its extension will not make it executable.

## Safe implementation order

- Keep image discovery separate from game execution.
- Validate image structure with homebrew/test images that the developer is authorized to use.
- Implement and test read-only filesystem inspection before attempting any loader integration.
- Only add launch support when the underlying PS5 runtime and required APIs are actually implemented and tested.

No build or Xbox-console test is implied by this document.
