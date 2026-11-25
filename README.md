# LeviLauncher Fabric Porting Kit

A **native Android mod scaffold, porting guide, and `.levipack` validation toolkit** for rewriting Minecraft Java Edition (Fabric) mods into C++ `preload-native` mods for **LeviLauncher Android** (`LiteLDev/preloader-android`).

> **Important Technical Scope:**  
> This repository is **not** an automatic `.jar` → `.levipack` converter. Fabric mods use JVM bytecode and Mixins targeting Java classes, whereas LeviLauncher `.levipack` native mods are Android NDK C++ shared libraries (`.so`) hooking Bedrock's closed C++ engine (`libminecraftpe.so`). See [`docs/WHY_NO_AUTO_CONVERTER.md`](docs/WHY_NO_AUTO_CONVERTER.md).

## What This Project Provides

1. **Ready-to-Build Native Mod Template** (`CMakeLists.txt`, `src/`, `manifest.json.in`)  
   Follows the official [LeviMC Android Mod Tutorial](https://levimc.org/tutorials/develop/first-android-mod/) and `QYCottage/levilauncher-android-mod-template` using `preloader-android` (`0.2.2`), typed config generation (`config.json` + `config.schema.json`), and 16 KB page alignment (`-Wl,-z,max-page-size=16384`).
2. **Fabric Feasibility Assessor** (`tools/assess_fabric_mod.py`)  
   Inspects a Fabric `.jar` (`fabric.mod.json`, Mixin configs, class references) and outputs a native Bedrock C++ rewrite feasibility report.
3. **`.levipack` Test & Validation Harness** (`tools/validate_levipack.py`)  
   Verifies archive layout, `manifest.json`, `"type": "preload-native"`, ELF64 AArch64 (`arm64-v8a`), `PLGetModRegistration` dynamic export, 16 KB `PT_LOAD` alignment, and config JSON schemas.
4. **Offline Signature Gate** (`tools/check_mod_signatures.py`)  
   Checks every `constexpr std::string_view` byte signature in your C++ source against one or more `libminecraftpe.so` builds (`UNIQUE` vs `MISSING`/`AMBIGUOUS`).
5. **Porting & Crash-Safety Docs** (`docs/`)  
   Feature mapping, safe `RTLD_NOLOAD` attach lifecycle, and BedrockTools hooking patterns.

## Prerequisites (from LeviMC Tutorial)

- **Android SDK** Platform **Android 9.0 (API 28)**
- **Android NDK** **`28.2.13676358`** (or compatible)
- **CMake** `3.22+` and **Ninja**
- **Host C++ Compiler** (Visual Studio *Desktop development with C++* on Windows, or GCC/Clang on Linux/macOS — required to run `levi_config_generator` on the host)
- **PowerShell 7+** (or Windows PowerShell) / Bash on Linux & macOS
- **Python 3.10+** (for `tools/` validation and assessment scripts)
- **ARM64 Android device** (Android 9+) with Minecraft Bedrock & LeviLauncher Android

## Quick Start

### 1. Assess a Fabric Mod (Optional)

```bash
python3 tools/assess_fabric_mod.py path/to/some-fabric-mod.jar