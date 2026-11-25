#!/usr/bin/env python3
"""Verify a built .levipack offline (Python stdlib only).

Checks:
1. Top-level directory in the zip matches manifest structure.
2. manifest.json has type='preload-native' and an entry matching a packaged .so.
3. Packaged .so is ELF32/ELF64 little-endian with matching machine (0xb7 arm64 / 0x28 arm).
4. Dynamic symbol table (.dynsym/.dynstr) exports PLGetModRegistration.
5. config/config.json and config/config.schema.json exist and parse as JSON.
"""

from __future__ import annotations

import argparse
import json
import struct
import sys
import zipfile
from pathlib import Path


def check_elf(so_bytes: bytes, expected_abi: str) -> None:
    if len(so_bytes) < 64 or so_bytes[:4] != b"\x7fELF":
        raise RuntimeError("Entry library is not a valid ELF file")
    ei_class = so_bytes[4]
    ei_data = so_bytes[5]
    if ei_data != 1:
        raise RuntimeError("Expected little-endian ELF")
    e_machine = struct.unpack_from("<H", so_bytes, 18)[0]

    if expected_abi == "arm64-v8a":
        if ei_class != 2 or e_machine != 0xB7:
            raise RuntimeError(f"Expected ELF64 AArch64 (0xb7), got class={ei_class} machine={hex(e_machine)}")
    elif expected_abi == "armeabi-v7a":
        if ei_class != 1 or e_machine != 0x28:
            raise RuntimeError(f"Expected ELF32 ARM (0x28), got class={ei_class} machine={hex(e_machine)}")

    if ei_class == 2:
        e_shoff = struct.unpack_from("<Q", so_bytes, 0x28)[0]
        e_shentsize, e_shnum, e_shstrndx = struct.unpack_from("<HHH", so_bytes, 0x3A)
        sh_fmt = "<IIQQQQIIQQ"
        sym_fmt, sym_size = "<IBBHQQ", 24
    else:
        e_shoff = struct.unpack_from("<I", so_bytes, 0x20)[0]
        e_shentsize, e_shnum, e_shstrndx = struct.unpack_from("<HHH", so_bytes, 0x2E)
        sh_fmt = "<IIIIIIIIII"
        sym_fmt, sym_size = "<IIIBBH", 16

    if not e_shoff or not e_shnum:
        raise RuntimeError("Missing section headers; cannot verify PLGetModRegistration")

    sections = [struct.unpack_from(sh_fmt, so_bytes, e_shoff + i * e_shentsize) for i in range(e_shnum)]
    shstr = sections[e_shstrndx]
    shstr_bytes = so_bytes[shstr[4]:shstr[4] + shstr[5]]

    def sec_name(off: int) -> str:
        end = shstr_bytes.find(b"\x00", off)
        return shstr_bytes[off:end].decode("ascii", "replace")

    by_name = {sec_name(s[0]): s for s in sections}
    dynsym = by_name.get(".dynsym")
    dynstr = by_name.get(".dynstr")
    if not dynsym or not dynstr:
        raise RuntimeError("Missing .dynsym or .dynstr")

    str_bytes = so_bytes[dynstr[4]:dynstr[4] + dynstr[5]]
    sym_bytes = so_bytes[dynsym[4]:dynsym[4] + dynsym[5]]
    exported = False
    for i in range(len(sym_bytes) // sym_size):
        fields = struct.unpack_from(sym_fmt, sym_bytes, i * sym_size)
        st_name = fields[0]
        st_shndx = fields[3] if ei_class == 2 else fields[5]
        if st_shndx == 0:  # SHN_UNDEF
            continue
        end = str_bytes.find(b"\x00", st_name)
        if str_bytes[st_name:end] == b"PLGetModRegistration":
            exported = True
            break

    if not exported:
        raise RuntimeError("PLGetModRegistration is NOT exported in .dynsym")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("levipack", type=Path)
    parser.add_argument("--abi", default="arm64-v8a", choices=("arm64-v8a", "armeabi-v7a"))
    args = parser.parse_args()

    if not args.levipack.is_file():
        print(f"File not found: {args.levipack}", file=sys.stderr)
        return 1

    with zipfile.ZipFile(args.levipack, "r") as zf:
        names = zf.namelist()
        manifests = [n for n in names if n.endswith("/manifest.json") and n.count("/") == 1]
        if len(manifests) != 1:
            raise RuntimeError(f"Expected exactly one <mod_id>/manifest.json, found: {manifests}")

        mod_dir = manifests[0].split("/", 1)[0]
        manifest = json.loads(zf.read(manifests[0]).decode("utf-8"))

        if manifest.get("type") != "preload-native":
            raise RuntimeError(f"Invalid manifest type: {manifest.get('type')!r}")
        entry = manifest.get("entry")
        if not entry:
            raise RuntimeError("Manifest missing 'entry'")

        so_member = f"{mod_dir}/{entry}"
        if so_member not in names:
            raise RuntimeError(f"Packaged library missing: {so_member}")

        check_elf(zf.read(so_member), args.abi)

        for cfg_rel in ("config/config.json", "config/config.schema.json"):
            cfg_member = f"{mod_dir}/{cfg_rel}"
            if cfg_member not in names:
                raise RuntimeError(f"Missing {cfg_member}")
            json.loads(zf.read(cfg_member).decode("utf-8"))

        print(f"OK: {args.levipack.name} ({args.abi}) -> {mod_dir}/{entry}, PLGetModRegistration exported")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())