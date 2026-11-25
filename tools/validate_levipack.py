#!/usr/bin/env python3
"""Validate a .levipack package for LeviLauncher Android native mods.

Checks:
  1. Valid ZIP archive without Java/Fabric artifacts (.jar, .class, fabric.mod.json).
  2. manifest.json exists (either at archive root or inside <mod_id>/).
  3. manifest.json has type == "preload-native", non-empty name, version, entry.
  4. Native library at <prefix><entry> exists and is ELF (AArch64 or ARMv7).
  5. .so exports PLGetModRegistration in .dynsym (non-undefined).
  6. For arm64-v8a, PT_LOAD segments meet Android 16 KB page alignment (>= 0x4000).
  7. Optional config/config.json, config/config.schema.json, and icon exist & parse.
"""

from __future__ import annotations

import argparse
import json
import struct
import sys
import zipfile
from pathlib import Path

EM_ARM = 0x28
EM_AARCH64 = 0xB7
PT_LOAD = 1
SHT_DYNSYM = 11


def find_manifest_prefix(names: list[str]) -> str | None:
    if "manifest.json" in names:
        return ""
    candidates = [n for n in names if n.endswith("/manifest.json") and n.count("/") == 1]
    if len(candidates) == 1:
        return candidates[0][: -len("manifest.json")]
    return None


def inspect_elf(blob: bytes) -> dict:
    if len(blob) < 64 or blob[:4] != b"\x7fELF":
        raise ValueError("Entry binary is not an ELF file")

    ei_class = blob[4]
    ei_data = blob[5]
    if ei_data != 1:
        raise ValueError("Expected little-endian ELF")

    is_64 = ei_class == 2
    if ei_class not in (1, 2):
        raise ValueError(f"Unsupported ELF class: {ei_class}")

    e_type, e_machine = struct.unpack_from("<HH", blob, 16)

    if is_64:
        e_phoff, e_shoff = struct.unpack_from("<QQ", blob, 32)
        e_phentsize, e_phnum, e_shentsize, e_shnum, e_shstrndx = struct.unpack_from("<HHHHH", blob, 54)
        load_aligns = []
        for i in range(e_phnum):
            off = e_phoff + i * e_phentsize
            p_type, _p_flags = struct.unpack_from("<II", blob, off)
            if p_type == PT_LOAD:
                p_align = struct.unpack_from("<Q", blob, off + 48)[0]
                load_aligns.append(p_align)

        sections = []
        for i in range(e_shnum):
            off = e_shoff + i * e_shentsize
            sh_name, sh_type, _flags, _addr, sh_offset, sh_size, sh_link, _info, _align, sh_entsize = (
                struct.unpack_from("<IIQQQQIIQQ", blob, off)
            )
            sections.append((sh_type, sh_offset, sh_size, sh_link, sh_entsize))
    else:
        e_phoff, e_shoff = struct.unpack_from("<II", blob, 28)
        e_phentsize, e_phnum, e_shentsize, e_shnum, e_shstrndx = struct.unpack_from("<HHHHH", blob, 42)
        load_aligns = []
        for i in range(e_phnum):
            off = e_phoff + i * e_phentsize
            p_type = struct.unpack_from("<I", blob, off)[0]
            if p_type == PT_LOAD:
                p_align = struct.unpack_from("<I", blob, off + 28)[0]
                load_aligns.append(p_align)

        sections = []
        for i in range(e_shnum):
            off = e_shoff + i * e_shentsize
            sh_name, sh_type, _flags, _addr, sh_offset, sh_size, sh_link, _info, _align, sh_entsize = (
                struct.unpack_from("<IIIIIIIIII", blob, off)
            )
            sections.append((sh_type, sh_offset, sh_size, sh_link, sh_entsize))

    exports: set[str] = set()
    for sh_type, sh_offset, sh_size, sh_link, sh_entsize in sections:
        if sh_type != SHT_DYNSYM or sh_link >= len(sections):
            continue
        _str_type, str_off, str_size, _, _ = sections[sh_link]
        strtab = blob[str_off : str_off + str_size]
        sym_size = sh_entsize or (24 if is_64 else 16)
        count = sh_size // sym_size
        for idx in range(count):
            entry_off = sh_offset + idx * sym_size
            if is_64:
                st_name, _st_info, _st_other, st_shndx = struct.unpack_from("<IBBH", blob, entry_off)
            else:
                st_name, _st_value, _st_size, _st_info, _st_other, st_shndx = struct.unpack_from(
                    "<IIIBBH", blob, entry_off
                )
            if st_shndx == 0 or st_name >= len(strtab):
                continue
            end = strtab.find(b"\x00", st_name)
            name = strtab[st_name : (end if end >= 0 else len(strtab))].decode("utf-8", "replace")
            if name:
                exports.add(name)

    return {
        "class": 64 if is_64 else 32,
        "e_type": e_type,
        "e_machine": e_machine,
        "load_aligns": load_aligns,
        "exports_registration": "PLGetModRegistration" in exports,
    }


def validate_levipack(path: Path) -> list[str]:
    errors: list[str] = []
    if not path.is_file():
        return [f"File not found: {path}"]

    try:
        zf = zipfile.ZipFile(path, "r")
    except zipfile.BadZipFile as exc:
        return [f"Invalid ZIP archive: {exc}"]

    with zf:
        names = [info.filename for info in zf.infolist() if not info.is_dir()]

        # Reject Fabric / Java artifacts inside a native .levipack
        for name in names:
            low = name.lower()
            if low.endswith(".jar") or low.endswith(".class") or low.endswith("fabric.mod.json"):
                errors.append(
                    f"Unsupported Java/Fabric artifact in .levipack ({name}): "
                    "LeviLauncher preload-native packs cannot run JVM bytecode."
                )

        prefix = find_manifest_prefix(names)
        if prefix is None:
            errors.append("Missing manifest.json (expected at root or <mod_id>/manifest.json)")
            return errors

        try:
            manifest = json.loads(zf.read(prefix + "manifest.json").decode("utf-8"))
        except Exception as exc:
            errors.append(f"Invalid manifest.json: {exc}")
            return errors

        if manifest.get("type") != "preload-native":
            errors.append(f"manifest.json 'type' must be 'preload-native', got {manifest.get('type')!r}")

        for req in ("name", "version", "entry"):
            if not isinstance(manifest.get(req), str) or not manifest[req].strip():
                errors.append(f"manifest.json missing non-empty string field '{req}'")

        entry = manifest.get("entry", "")
        entry_path = prefix + entry
        if entry_path not in names:
            errors.append(f"Declared entry library missing from archive: {entry_path}")
        else:
            try:
                elf = inspect_elf(zf.read(entry_path))
                if elf["e_machine"] not in (EM_AARCH64, EM_ARM):
                    errors.append(f"Unsupported ELF e_machine: {hex(elf['e_machine'])}")
                if not elf["exports_registration"]:
                    errors.append(
                        f"{entry_path} does not export 'PLGetModRegistration' "
                        "(ensure PL_REGISTER_MOD is linked and not stripped)"
                    )
                if elf["e_machine"] == EM_AARCH64 and any(a < 0x4000 for a in elf["load_aligns"]):
                    errors.append(
                        f"{entry_path} has PT_LOAD alignment < 16KB ({elf['load_aligns']}); "
                        "link with -Wl,-z,max-page-size=16384"
                    )
            except Exception as exc:
                errors.append(f"ELF inspection failed for {entry_path}: {exc}")

        icon = manifest.get("icon", "")
        if isinstance(icon, str) and icon.strip():
            icon_path = prefix + icon
            if icon_path not in names:
                errors.append(f"Declared icon missing from archive: {icon_path}")

        for cfg_name in ("config/config.json", "config/config.schema.json"):
            cfg_path = prefix + cfg_name
            if cfg_path in names:
                try:
                    json.loads(zf.read(cfg_path).decode("utf-8"))
                except Exception as exc:
                    errors.append(f"Invalid JSON in {cfg_path}: {exc}")

    return errors


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("levipack", type=Path, help="Path to .levipack archive")
    args = parser.parse_args()

    errors = validate_levipack(args.levipack)
    if errors:
        print(f"FAILED: {args.levipack}")
        for err in errors:
            print(f"  - {err}")
        return 1

    print(f"OK: {args.levipack} passed all LeviLauncher preload-native checks.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())