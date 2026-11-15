#!/usr/bin/env python3
"""Assemble release assets from an already hardware-tested installation ZIP.

This script does not compile or modify executables. It validates the exact
tested bytes and supplies human-readable corresponding source with third-party
runtime source archives. GitHub Actions performs publication after validation.
"""
from __future__ import annotations
from pathlib import Path
from zipfile import ZIP_DEFLATED, ZipFile
import hashlib
import os
import subprocess
import shutil

HERE = Path(__file__).resolve().parent.parent
CODE = HERE / "sourcebase"
TOOLCHAIN = HERE / "toolchain-source"
STAGING = HERE / "release-staging" / "NXCR-v0.4.1-beta.1-audited-install.zip"
OUT = HERE / "release-output"
INSTALL = OUT / "NXCR-v0.4.1-beta.1-install.zip"
SOURCE = OUT / "NXCR-v0.4.1-beta.1-source.zip"
SOURCE_COMMIT = "39dd369a7af6684dcb511cccfbdf91dabb79c30c"
TESLA_COMMIT = "f766e9b607a05e9756843cbd62b3bfb98be1646c"
EXPECTED_INSTALL = "1f3003453a1ade8ed6653ce1a4b581220da3da0195c9be515e96fb9289513420"
EXPECTED_SYS = "1e80280a1088aca52ca5280c4ebae8170d6065fc122686c45ad50b3635680f5f"
EXPECTED_OVL = "62db335aafb8e4a47beebd915263f01d019c7d245133dae701d42267a4edf73d"
BASE = "NXCR-v0.4.1-source/"
DOCS = (
    "README.md", "RELEASE_NOTES.md", "LICENSE", "BUILDING.md",
    "THIRD_PARTY_NOTICES.md", "SOURCE_PROVENANCE.md",
    "LEGAL_PROVENANCE_AUDIT.md", "GAME_COMPATIBILITY.md", "TESTING.md",
)
SKIP_PARTS = {".git", "build", "__pycache__", "release-candidate", "release", "release-output"}
SKIP_EXT = {".o", ".d", ".elf", ".nsp", ".nso", ".nro", ".ovl", ".map", ".pyc", ".a"}
REQUIRED_TOOLCHAIN = {
    "libnx-v4.12.0.tar.gz": "1665209994d67b4b236d80fa200499853473a7e2bb502d5045d00fc0170f90ef",
    "newlib-4.6.0.20260123.tar.gz": "6ff27e3bf022666f43f7802255be680eeff722ac181b1725d21e2e8318604ee3",
    "newlib-4.6.0.20260123-4.patch": "6d5aba4356553c727765c98e7553d3784f31d361c856e220ef27cc26353fb88f",
}

def sha(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()

def git_hash(path: Path) -> str:
    return subprocess.check_output(
        ["git", "-C", str(path), "rev-parse", "HEAD"], text=True
    ).strip()

def add(entries: dict[str, Path], source_root: Path, rel_prefix: str = "") -> None:
    for p in sorted(source_root.rglob("*")):
        if not p.is_file() or p.is_symlink():
            continue
        rel = p.relative_to(source_root)
        if any(part in SKIP_PARTS for part in rel.parts):
            continue
        if p.suffix.lower() in SKIP_EXT or p.name == ".git":
            continue
        arc = rel_prefix + rel.as_posix()
        if arc.startswith("/") or ".." in Path(arc).parts:
            raise RuntimeError(f"Unsafe path in source: {arc}")
        entries[arc] = p

def main() -> None:
    assert STAGING.is_file(), f"Missing staged installer {STAGING}"
    assert sha(STAGING) == EXPECTED_INSTALL, "Installer differs from tested audited ZIP"
    assert git_hash(CODE) == SOURCE_COMMIT, "Wrong corresponding source commit"
    assert git_hash(CODE / "overlay" / "libtesla") == TESLA_COMMIT, "Wrong libtesla version"
    with ZipFile(STAGING) as z:
        assert z.testzip() is None, "Corrupt test installer"
        for name, expected in (
            ("atmosphere/contents/00FF0000A53BB667/exefs.nsp", EXPECTED_SYS),
            ("switch/.overlays/NXCR.ovl", EXPECTED_OVL),
        ):
            actual = hashlib.sha256(z.read(name)).hexdigest()
            assert actual == expected, f"Altered hardware-tested executable: {name}"
        for doc in ("LICENSE", "THIRD_PARTY_NOTICES.md",
                    "LICENSES/gcc-15.2.0-COPYING.RUNTIME",
                    "LICENSES/newlib-4.6.0.20260123-COPYING.NEWLIB"):
            assert doc in z.namelist(), f"Installer missing required notice {doc}"
    for filename, expected in REQUIRED_TOOLCHAIN.items():
        p = TOOLCHAIN / filename
        assert p.is_file() and sha(p) == expected, f"Invalid toolchain source {filename}"
    for rel in DOCS:
        assert (HERE / rel).is_file(), f"Missing release document {rel}"

    OUT.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(STAGING, INSTALL)
    entries: dict[str, Path] = {}
    add(entries, CODE)
    # Current licensing and documentary review supersedes the original
    # pinned source checkout's older pre-audit document versions.
    for rel in DOCS:
        entries[rel] = HERE / rel
    for p in sorted((HERE / "LICENSES").rglob("*")):
        if p.is_file():
            entries[p.relative_to(HERE).as_posix()] = p
    for name in REQUIRED_TOOLCHAIN:
        entries["toolchain-source/" + name] = TOOLCHAIN / name
    # The source package needs to be buildable when extracted.
    for required in ("Makefile", "source/main.c", "source/mkv.c",
                     "source/capture.c", "source/recorder.c",
                     "overlay/Makefile", "overlay/source/main.cpp",
                     "overlay/libtesla/include/tesla.hpp",
                     "scripts/package.sh", ".gitmodules"):
        assert required in entries, f"Missing corresponding source: {required}"
    with ZipFile(SOURCE, "w", ZIP_DEFLATED, compresslevel=8) as z:
        for rel, path in sorted(entries.items()):
            z.write(path, BASE + rel)
        z.writestr(BASE + "RELEASE_BUILD_ID.txt",
            f"NXCR version: v0.4.1-beta.1\n"
            f"Tested source commit: {SOURCE_COMMIT}\n"
            f"Pinned libtesla commit: {TESLA_COMMIT}\n"
            "Original compiler image: devkitpro/devkita64:20260219\n"
            f"Tested sysmodule sha256: {EXPECTED_SYS}\n"
            f"Tested overlay sha256: {EXPECTED_OVL}\n"
            "Additional third-party source and audit documents retained.\n")
        z.writestr(BASE + "toolchain-source/README.txt",
            "Verified original libnx 4.12.0 and Newlib 4.6.0.20260123 sources,\n"
            "with the devkitPro Newlib patch for devkitA64 r29.2.\n"
            "GCC 15.2.0 Runtime Library Exception and GPL text in LICENSES/.\n")
    assert ZipFile(SOURCE).testzip() is None, "Corrupt corresponding source ZIP"
    sums = OUT / "SHA256SUMS.txt"
    sums.write_text(f"{sha(INSTALL)}  {INSTALL.name}\n"
                    f"{sha(SOURCE)}  {SOURCE.name}\n", encoding="utf8")
    print("Verified hardware-tested sysmodule:", EXPECTED_SYS)
    print("Verified hardware-tested Tesla overlay:", EXPECTED_OVL)
    print("Pinned source:", SOURCE_COMMIT)
    print("Source archive inputs:", len(entries))
    print(sums.read_text())
if __name__ == "__main__":
    main()
