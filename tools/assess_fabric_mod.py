#!/usr/bin/env python3
"""Assess a Minecraft Java Fabric .jar for manual porting to a LeviLauncher C++ mod.

This tool does NOT convert .jar files into .levipack (which is impossible due to
JVM/Mixin vs Bedrock C++ engine differences). Instead, it inspects fabric.mod.json,
Mixin configurations, and class bytecode references to produce a porting checklist.
"""

from __future__ import annotations

import argparse
import json
import sys
import zipfile
from pathlib import Path

CAPABILITY_HINTS = [
    (b"org/spongepowered/asm/mixin", "Uses Java Mixins (@Inject/@Redirect) -> Must rewrite as C++ pl::memory::hook / writeBytes using unique ARM64 signatures in libminecraftpe.so."),
    (b"net/minecraft/client/gui", "Custom Java GUI / Screens -> No 1:1 native API; rewrite via MinecraftUIRenderContext or pl::modmenu / embedded View."),
    (b"net/minecraft/client/render", "Java Client Rendering / Shaders -> No 1:1 API; requires GLES/EGL boundary hooks or engine tessellation hooks."),
    (b"net/minecraft/world/gen", "World Generation / Custom Dimensions -> High complexity; very limited or no direct native Bedrock hook parity."),
    (b"net/minecraft/entity", "Entity / AI Internals -> Rewrite via Actor hooks (NormalTick, Actor offsets/vtables); layouts differ per Bedrock version."),
    (b"net/minecraft/network/packet", "Networking / Packets -> Hook LoopbackPacketSender::sendToServer or packet handlers; Bedrock protocol differs completely from Java."),
    (b"net/fabricmc/fabric/api/client/keybinding", "Keybinds / Input -> Use pl::input::registerKeyCallback / registerTouchCallback / ModMenu ButtonBuilder."),
    (b"net/fabricmc/fabric/api/client/event/lifecycle", "Client Tick Events -> Hook LocalPlayer::normalTick or eglSwapBuffers via HookWatcher."),
]


def assess_jar(jar_path: Path) -> int:
    if not jar_path.is_file():
        print(f"Error: file not found: {jar_path}", file=sys.stderr)
        return 1

    with zipfile.ZipFile(jar_path, "r") as zf:
        names = zf.namelist()
        print(f"# Fabric Porting Assessment: {jar_path.name}\n")

        if "fabric.mod.json" in names:
            meta = json.loads(zf.read("fabric.mod.json").decode("utf-8", errors="replace"))
            print("## Mod Metadata (`fabric.mod.json`)")
            print(f"- **id**: `{meta.get('id', 'unknown')}` -> CMake `MOD_ID`")
            print(f"- **name**: `{meta.get('name', 'unknown')}` -> CMake `MOD_NAME`")
            print(f"- **version**: `{meta.get('version', 'unknown')}` -> CMake `MOD_VERSION`")
            print(f"- **environment**: `{meta.get('environment', '*')}`")
            mixins = meta.get("mixins", [])
            print(f"- **mixin configs**: `{mixins}`")
            deps = meta.get("depends", {})
            print(f"- **depends**: `{deps}`\n")
        else:
            print("## Warning: `fabric.mod.json` not found (may not be a Fabric mod JAR).\n")

        mixin_files = [
            n for n in names
            if n.endswith(".mixins.json") or ("mixin" in n.lower() and n.endswith(".json"))
        ]
        if mixin_files:
            print("## Mixin Configurations Detected")
            for mf in mixin_files:
                try:
                    mjson = json.loads(zf.read(mf).decode("utf-8", errors="replace"))
                    targets = (mjson.get("mixins", []) or []) + (mjson.get("client", []) or []) + (mjson.get("server", []) or [])
                    print(f"- `{mf}`: {len(targets)} mixin class(es) targeting Java package `{mjson.get('package', '?')}`")
                except Exception:
                    print(f"- `{mf}` (could not parse JSON)")
            print()

        class_blobs = b"\n".join(zf.read(n) for n in names if n.endswith(".class"))
        detected = [msg for needle, msg in CAPABILITY_HINTS if needle in class_blobs]

        print("## Rewrite Mapping & Feasibility Notes")
        if detected:
            for item in detected:
                print(f"- {item}")
        else:
            print("- No standard Minecraft/Fabric class markers matched; inspect decompiled Java sources manually.")

        print("\n## Recommended Next Steps")
        print("1. List which core features are essential vs optional.")
        print("2. Map each behavior using `docs/FEATURE_MAPPING.md`.")
        print("3. Locate & verify target ARM64 signatures in your target `libminecraftpe.so` (`UNIQUE` only).")
        print("4. Reimplement in C++ under `src/mod/` using `HookWatcher` and `pl::` APIs.")
        print("5. Build `.levipack` and verify with `python3 tools/validate_levipack.py`.")

    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("jar", type=Path, help="Path to Fabric mod .jar")
    args = parser.parse_args()
    return assess_jar(args.jar)


if __name__ == "__main__":
    raise SystemExit(main())