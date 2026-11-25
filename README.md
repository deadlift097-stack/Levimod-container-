# Mod Container for LeviLauncher Android

A native C++ `.levipack` for **Minecraft Bedrock on Android** (via
[LeviLauncher](https://levimc.org/LeviLauncher/)) that acts as:

- a **mod container**: several native features ("sub-mods") bundled in one `.levipack`,
- a **mod menu**: each sub-mod is a Mod Menu module with a toggle and its own settings,
- a **settings editor**: typed `config.json` + schema, editable from LeviLauncher,
- an optional on-screen **MODS button** that opens the Mod Menu.

It follows the official tutorial, [Your first Android native mod](https://levimc.org/tutorials/develop/first-android-mod/),
and is built on the official template and the
[Preloader SDK](https://github.com/LiteLDev/preloader-android).

> **Not a Fabric loader.** A `.levipack` is native ARM64 code for Minecraft: Bedrock.
> Fabric mods are Java bytecode for Minecraft: Java Edition. Nothing here can load a
> `.jar`. See [docs/LIMITATIONS.md](docs/LIMITATIONS.md).

## Status

| Check | Result |
|---|---|
| Config generator builds and runs natively; `config.json` / `config.schema.json` verified | done |
| All mod sources compile cleanly against the real SDK headers (syntax check, stubbed `jni.h` / `android/log.h`) | done |
| Android NDK build (`package.ps1`) | **not run yet**: run it or let GitHub Actions do it |
| Behaviour on a device (menu entries, MODS button, crosshair overlay) | **untested** |

## Quick start

Requirements are the same as the tutorial: Android SDK + NDK `28.2.13676358`, CMake 3.22+,
Ninja, PowerShell 7, Git, and a PC compiler (the build runs a small config generator on
the PC).

```powershell
git clone https://github.com/<you>/levipack-mod-container
cd levipack-mod-container

$env:ANDROID_HOME = "C:/Users/<you>/AppData/Local/Android/Sdk"
./scripts/package.ps1 -Abi arm64-v8a
```

Result: `build-arm64-v8a/mod_container-0.1.0-arm64-v8a.levipack`.

Or push to GitHub: the **Build** workflow produces the `.levipack` as an artifact, and a
tag `v*` attaches it to a release.

### Install

1. Copy the `.levipack` to the phone's **Download** folder.
2. LeviLauncher → **Launch** → **Manage** on the Mods card → **Add Mod** → pick the file.
3. Start the game. Open the Mod Menu (floating button or **MODS**) and toggle the sub-mods.
4. Optional log check: `adb logcat | grep "Mod Container"` should show
   `[hello] Hello from Mod Container!`.

Before building, set `MOD_AUTHOR` in `CMakeLists.txt` and make sure `MOD_MINECRAFT_VERSIONS`
matches the Minecraft version on your phone (default `["1.26.*"]`). Keep `MOD_ID` stable
after your first release.

## Layout

```
CMakeLists.txt            mod id/name/version, sources, packaging target
manifest.json.in          becomes manifest.json in the .levipack
scripts/package.ps1       config generation + NDK build + .levipack (from the template)
src/main.cpp              PL_REGISTER_MOD
src/mod/                  ContainerMod (lifecycle) + typed Config
src/container/            ISubMod interface, SubModRegistry, MODS button / floating button
src/submods/              HelloSubMod, CrosshairSubMod (examples)
docs/                     architecture, adding a sub-mod, limitations, guide corrections
```

## Documentation

- [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md): how the pieces fit and what is assumed
- [docs/ADDING_A_SUBMOD.md](docs/ADDING_A_SUBMOD.md): add your own feature in five steps
- [docs/LIMITATIONS.md](docs/LIMITATIONS.md): what a `.levipack` can and cannot do
- [docs/GUIDE_CORRECTIONS.md](docs/GUIDE_CORRECTIONS.md): where the earlier written guide
  disagreed with the real SDK, and what this project does instead

## Going further

Features that change game behaviour need hooks or patches
([Hook API](https://levimc.org/LeviLamina/developer_guides/android/api/hook/),
[Signature API](https://levimc.org/LeviLamina/developer_guides/android/api/signature/)).
Those are version-specific; the BedrockTools skill (`skills/references/` in that repo)
documents the workflow. Install hooks from `start()`, never from a constructor.

## Credits

Template: `levilauncher-android-mod-template`. SDK: `LiteLDev/preloader-android`.
Tutorial: levimc.org. No license file is included yet; add the one you want before
publishing.
