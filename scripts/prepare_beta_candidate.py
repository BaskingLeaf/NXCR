#!/usr/bin/env python3
"""Prepare UNPUBLISHED v0.4.1 Beta 1 install/source bundles from public source.

Build both recorder and overlay first. This script does NOT publish a release.
It packages the exact binaries just compiled with the corresponding, readable
source, pinned libtesla checkout, notices and SHA-256 checksums.
"""
from __future__ import annotations

import hashlib
import os
from pathlib import Path
import subprocess
from zipfile import ZipFile, ZIP_DEFLATED

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "release-candidate"
RAW = ROOT / "NXCR.zip"
INSTALL = OUT / "NXCR-v0.4.1-beta.1-install.zip"
SOURCE = OUT / "NXCR-v0.4.1-beta.1-source.zip"
PINNED_TESLA = "f766e9b607a05e9756843cbd62b3bfb98be1646c"
SOURCE_BASE = "NXCR-v0.4.1-source"
DOCS = [
    "README.md", "RELEASE_NOTES.md", "BUILDING.md", "SOURCE_PROVENANCE.md",
    "TESTING.md", "GAME_COMPATIBILITY.md", "THIRD_PARTY_NOTICES.md", "LICENSE",
]
SOURCE_PATHS = [
    ".gitmodules", ".github/workflows/build.yml", "Makefile",
    "default_config.ini", "exefs.npdm", "sysmodule.json", "toolbox.json",
    "scripts", "source", "overlay/Makefile", "overlay/source",
    "overlay/libtesla", "LICENSES", *DOCS,
]
SKIP_PARTS = {".git", "build", "__pycache__", "release", "release-candidate"}
SKIP_SUFFIXES = {".o", ".d", ".elf", ".nso", ".nsp", ".nro", ".ovl", ".map", ".a", ".pyc"}
REQUIRED_INSTALL = {
    "atmosphere/contents/00FF0000A53BB667/exefs.nsp",
    "switch/.overlays/NXCR.ovl",
    "switch/NXCR/config.example.ini",
}


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def git(*args: str, cwd: Path = ROOT) -> str:
    return subprocess.check_output(["git", *args], cwd=cwd, text=True).strip()


def check_path(p: str) -> None:
    if p.startswith("/") or "\\" in p or any(seg in {"", ".", ".."} for seg in p.split("/")):
        raise ValueError(f"Unsafe archive path: {p!r}")


def main() -> None:
    if not RAW.is_file():
        raise FileNotFoundError("Build first: make, make -C overlay, make package")
    if git("rev-parse", "HEAD") == "":
        raise RuntimeError("Missing exact Git revision")
    if git("rev-parse", "HEAD", cwd=ROOT / "overlay/libtesla") != PINNED_TESLA:
        raise RuntimeError("libtesla checkout is not the pinned revision")
    for path in DOCS + ["LICENSES/libnx-LICENSE.md",
                        "LICENSES/libtesla-GPL-2.0.txt",
                        "LICENSES/stb_truetype-MIT.txt"]:
        if not (ROOT / path).is_file():
            raise FileNotFoundError(f"Missing legal/build documentation: {path}")
    OUT.mkdir(exist_ok=True)
    with ZipFile(RAW) as z:
        raw_names = set(z.namelist())
        missing = REQUIRED_INSTALL - raw_names
        if missing:
            raise RuntimeError(f"Install ZIP missing required paths: {missing}")
        with ZipFile(INSTALL, "w", ZIP_DEFLATED, compresslevel=9) as w:
            for info in z.infolist():
                if info.is_dir():
                    continue
                check_path(info.filename)
                w.writestr(info.filename, z.read(info))
            for name in DOCS:
                w.write(ROOT / name, name)
            for path in sorted((ROOT / "LICENSES").rglob("*")):
                if path.is_file():
                    w.write(path, path.relative_to(ROOT).as_posix())
            w.writestr(
                "README-FIRST.txt",
                "NXCR v0.4.1 Beta 1 — UNPUBLISHED TEST CANDIDATE\n"
                "Install only on Atmosphere with compatible Tesla loader.\n"
                "Back up the prior recorder and your settings/recordings first.\n"
                "Do not install beside another recorder using grc:d.\n"
                "Read README.md, RELEASE_NOTES.md and THIRD_PARTY_NOTICES.md.\n"
                "Status: experimental beta; source/licensing and final hardware sign-off pending.\n",
            )

    paths: dict[str, Path] = {}
    for item in SOURCE_PATHS:
        p = ROOT / item
        if p.is_file():
            paths[p.relative_to(ROOT).as_posix()] = p
        elif p.is_dir():
            for f in p.rglob("*"):
                if not f.is_file():
                    continue
                rel = f.relative_to(ROOT)
                if any(part in SKIP_PARTS for part in rel.parts):
                    continue
                if f.suffix.lower() in SKIP_SUFFIXES or f.name == ".git":
                    continue
                paths[rel.as_posix()] = f
        else:
            raise FileNotFoundError(f"Source input missing: {item}")

    with ZipFile(SOURCE, "w", ZIP_DEFLATED, compresslevel=9) as w:
        for name, path in sorted(paths.items()):
            check_path(name)
            w.write(path, f"{SOURCE_BASE}/{name}")
        w.writestr(
            f"{SOURCE_BASE}/RELEASE_BUILD_ID.txt",
            f"Public source commit: {git('rev-parse', 'HEAD')}\n"
            f"libtesla commit: {PINNED_TESLA}\n"
            "Toolchain CI image: devkitpro/devkita64:20260219\n"
            "Both source and install ZIPs were packaged in the same run.\n"
            "Binaries were built from this working source. Not a legal sign-off.\n",
        )

    with ZipFile(INSTALL) as z:
        assert REQUIRED_INSTALL.issubset(set(z.namelist()))
        assert z.testzip() is None
    with ZipFile(SOURCE) as z:
        required_source = {
            f"{SOURCE_BASE}/source/main.c",
            f"{SOURCE_BASE}/source/mkv.c",
            f"{SOURCE_BASE}/overlay/source/main.cpp",
            f"{SOURCE_BASE}/overlay/libtesla/include/tesla.hpp",
            f"{SOURCE_BASE}/LICENSE",
        }
        assert required_source.issubset(set(z.namelist()))
        assert z.testzip() is None

    sums = OUT / "SHA256SUMS.txt"
    sums.write_text(
        f"{sha(INSTALL.read_bytes())}  {INSTALL.name}\n"
        f"{sha(SOURCE.read_bytes())}  {SOURCE.name}\n",
        encoding="utf-8",
    )
    print("CANDIDATE_ONLY: not published or declared legally cleared")
    print("Source commit:", git("rev-parse", "HEAD"))
    print("libtesla:", PINNED_TESLA)
    print("Packaged", len(paths), "public source/dependency files")
    print(sums.read_text())


if __name__ == "__main__":
    main()
