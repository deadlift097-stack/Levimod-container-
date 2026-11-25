# LeviLauncher Mod Container

A native C++ **Mod Container & Mod Menu** `.levipack` for Minecraft: Bedrock Edition on Android ([LeviLauncher](https://github.com/LiteLDev/LeviLaunchroid)), built on the official [levilauncher-android-mod-template](https://github.com/LiteLDev/levilauncher-android-mod-template) and [LeviMC's Android native mod tutorial](https://levimc.org/tutorials/develop/first-android-mod/).

## What this project does (and cannot do)

- **Does:**
  - Registers a central **Mod Container** module and per-sub-mod toggles in the LeviLauncher Mod Menu (`pl::modmenu::ModuleBuilder`).
  - Optionally registers a floating **MODS** HUD button (`pl::modmenu::ButtonBuilder`) that opens/closes the Mod Menu.
  - Persists typed configuration (`config/config.json` and `config/config.schema.json`) through `pl::config::ConfigFile<ModConfig>`.
  - Provides a clean `ISubMod` C++ interface and a crash-safe `RTLD_NOLOAD` poll-then-settle `GameWatcher` for attaching verified native Bedrock hooks.
- **Cannot do:**
  - Cannot load, execute, or convert Java Edition **Fabric `.jar`** mods. Java Fabric bytecode targets the JVM and Java Edition's engine; `.levipack` mods are native ARM64 C++ libraries loaded into Bedrock Edition. To port a Fabric mod, reimplement its features in C++ as an `ISubMod`.

## Prerequisites

- CMake 3.22+ and Ninja
- Host C++ compiler (for `levi_config_generator`): Visual Studio C++ on Windows, or GCC/Clang on Linux/macOS
- Android SDK (API 28) + NDK `28.2.13676358`
- PowerShell 7+ (or Windows PowerShell on Windows; `scripts/package.sh` is also provided for Linux/macOS)
- Python 3.10+ (for `scripts/verify_levipack.py`)

## Build

### Windows / cross-platform PowerShell
```powershell
$env:ANDROID_HOME = "C:/Users/<you>/AppData/Local/Android/Sdk"
./scripts/package.ps1 -Abi arm64-v8a