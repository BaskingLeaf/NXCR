# NXCR v0.4.1 Beta 1 — source and licensing provenance audit

**Audit date: 15 November 2026. Scope: pinned candidate built from public source commit `39dd369a7af6684dcb511cccfbdf91dabb79c30c`.**

**Verdict:** All **identified** original/third-party components have licences permitting use in a GPL-2.0-covered NXCR derivative when their conditions are followed, and the matched public-source build and its packaged source are technically verifiable. **This is not a guarantee of legal clearance**: the copyright ownership of every line in the original prototype (particularly the local MKV writer) cannot be conclusively proven from a source archive or similarity analysis.

## Evidence inspected

1. The **complete private build-baseline archive** reconstructed from `payload/project.part.00`–`04`, SHA-256 `2e28b36dbab4ce17afcc6ecf77e0991725eb965ab2d3eb6476894b422fd19427`. It contains **18 entries**. All 13 prototype C/C++ headers and implementation files lacked individual copyright/SPDX headers; the archive included `LICENSE-NOTE.txt`.
2. A separate legacy `project.b64` Git blob **contains an ellipsization placeholder and cannot decode as a complete ZIP**. It must not be used as release provenance. The complete `payload/` archive is the usable early baseline.
3. NXCR v0.4.1 source files and build instructions recovered from the exact private candidate's assembled source ZIP, then imported unchanged into a clean public repository.
4. Current [exelix11/SysDVR](https://github.com/exelix11/SysDVR) `sysmodule/source/grcd.c`, `sysmodule/source/capture.c`, the repository's [GPL v2 text](https://github.com/exelix11/SysDVR/blob/master/LICENSE), and the relevant upstream code logic.
5. The pinned [WerWolv/libtesla](https://github.com/WerWolv/libtesla) revision `f766e9b607a05e9756843cbd62b3bfb98be1646c`, its GPL v2-or-later source header and bundled `stb_truetype.h` dual-licence header.
6. The exact pinned `devkitpro/devkita64:20260219` toolchain's package list and two compiled link maps, showing the static archives linked into the NXCR sysmodule and overlay.
7. Matching install and corresponding-source ZIPs built from public commit `39dd369a` and checked with SHA-256; a user subsequently reported successful MKV picture/audio playback using the provided candidate.

## Identified source provenance

| File or source | Finding | Distribution handling |
| --- | --- | --- |
| `source/grcd.c`, `source/grcd.h` | Service wrappers are substantially similar to SysDVR's `grcd` implementation; the baseline contains the same wrapper as the current release | Treat as SysDVR-derived GPL v2 work; keep upstream author/project credit and dated NXCR change notice |
| `source/capture.c`, `source/capture.h` | One-time mutex-protected `grc:d` Begin-and-retry sequence is distinctive and closely parallels SysDVR; only limited changes since the baseline | Treat as SysDVR-derived GPL v2 work; preserve modification notices |
| `source/main.c` | Original baseline uses SysDVR-style 20-second startup handling; latest project adds recorder threads, HOME handling, status, diagnostics, and controls | GPL v2 on the combined work; acknowledge upstream lineage and later NXCR additions |
| `source/mkv.c`, `source/mkv.h` | A self-contained local EBML/Matroska writer exists in the earliest complete project archive and is substantially retained in v0.4.1; it is not a copy of SysDVR's normal streaming implementation | No additional embedded MKV library identified in a source/dependency inspection; **original individual copyright ownership not independently established** |
| `source/recorder.c`, `source/recorder.h` | Earliest archive contains the local SD recorder and segmentation/free-space logic, later extended for markers and runtime config | GPL v2 working licence; **individual original authorship not independently established** |
| `source/runtime_config.c`, `source/runtime_config.h` | Introduced later in the project development line | NXCR development modifications under the project's GPL v2 working licence |
| `overlay/source/main.cpp` | Tesla UI from the original prototype, substantially expanded for NXCR status, options and controls | Uses pinned libtesla under GPL-compatible terms, retain credit |
| `Makefile`, `overlay/Makefile`, `scripts/package.sh` | Build/packaging inputs preserved and modified through the project | Supplied in the corresponding source archive |

The original `LICENSE-NOTE.txt` states the prototype's intention to be distributed under **GPL-2.0-or-later** because of its SysDVR-family interfaces, with upstream attribution. That is **evidence of intention, not a relicensing grant from exelix11 or other copyright holders**. The published project conservatively uses **GPL version 2**, preserving the actual third-party notices.

## External code and static linked runtime

| Dependency | Exact audited revision | Licence and retained notice |
| --- | --- | --- |
| SysDVR by exelix11 and contributors | Reference implementation of the captured `grc:d` flow | [GPL v2](LICENSE), with project credit in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) |
| libtesla | Git `f766e9b607a05e9756843cbd62b3bfb98be1646c` | GPL v2-or-later statement in `tesla.hpp`, [full GPL v2 text](LICENSES/libtesla-GPL-2.0.txt); corresponding pinned source bundled in the source ZIP |
| libnx | `4.12.0-1` | [ISC notice](LICENSES/libnx-LICENSE.md); [upstream v4.12.0 source](https://github.com/switchbrew/libnx/tree/v4.12.0) |
| stb_truetype | v1.24, included in pinned libtesla | MIT or public domain; [MIT alternative and copyright](LICENSES/stb_truetype-MIT.txt) |
| devkitA64 / GCC | `r29.2-1`; `devkita64-gcc 15.2.0-7` | GCC's libstdc++ and other runtime components use GPL v3 **with GCC Runtime Library Exception 3.1** as applicable; [full GCC GPL v3](LICENSES/gcc-15.2.0-COPYING3) and [exception](LICENSES/gcc-15.2.0-COPYING.RUNTIME) are retained, and do **not** change NXCR's GPL v2 licence |
| Newlib C and math libraries | `devkita64-newlib 4.6.0.20260123-4` | Multi-author/multi-licence `COPYING.NEWLIB` retained in [LICENSES](LICENSES/newlib-4.6.0.20260123-COPYING.NEWLIB). Exact source archive is [upstream](https://sourceware.org/pub/newlib/newlib-4.6.0.20260123.tar.gz), SHA-256 `6ff27e3bf022666f43f7802255be680eeff722ac181b1725d21e2e8318604ee3`. |
| switch-tools | `1.13.1-1` | Build-time packaging tool; not an identified embedded library |

The audited link maps contain object members of `libnx.a`, Newlib's `libg.a`, `libm.a` and GCC's `libstdc++.a`. This is why the runtime notices were added rather than omitted. The Newlib `COPYING.NEWLIB` content and official upstream tarball checksum were verified. **This audit does not independently adjudicate every object-file-specific exception in the toolchain.**

## GPL obligations and practical review result

- **Licence:** Publish the NXCR-derived work under GPL v2 terms; credit SysDVR authors and preserve its licence and modifications; do not claim a clean-room implementation.
- **Notices:** Include [LICENSE](LICENSE), [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md), and the full identified [LICENSES/](LICENSES/) in the **actual distributable**.
- **Source:** Supply the complete, readable source and build/packaging scripts with the matching binary. Include the pinned libtesla source, and make the exact devkitPro package/version and source information available.
- **Matching:** The original private install/source candidate pair passed ZIP integrity and SHA-256 tests. Source commit and source/libtesla revisions are recorded. Rebuilds were successful; byte-identical output on different build dates was **not** demonstrated.
- **Hardware:** The maintainer reported playable MKV picture and audio on the supplied candidate, not a second full 17-point regression.

### Remaining legal/technical uncertainty

1. **Individual authorship:** The early self-contained MKV and local recorder modules have no historical per-file copyright headers and cannot be assigned to any particular human contributor from the archive alone. No named external MKV library was found, but absence of a match is not proof of original authorship. Review of the September source-generation conversation or author confirmation could strengthen this evidence.
2. **Toolchain licences:** The exact Newlib/GCC notices are now public, but whether to redistribute further exact toolchain sources in the final source ZIP is an application-specific GPL source-distribution judgement; the full notices alone are not a legal opinion on the GPL system-library exception.
3. **Candidate package:** The previously tested private Beta 1 ZIP predates the newly added Newlib/GCC notices. A **repacked distribution with the exact same recorder/overlay binary bytes** and updated notices is needed before publication; verify executable hashes are unchanged.
4. **Formal sign-off:** A qualified legal reviewer/copyright holder would be required to guarantee rights for every original-prototype contribution. The available evidence supports **a credible GPL v2 compliance approach, not unconditional legal certification**.

No binary release should be described as legally verified solely on the basis of this technical audit.
