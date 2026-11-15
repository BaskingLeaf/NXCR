# NXCR third-party acknowledgements — draft for v0.4.1 Beta 1

**PRE-RELEASE AUDIT — review before public binary distribution.** This file is an attribution map and does not replace the full licences/notices or corresponding source required by applicable licences. See [SOURCE_PROVENANCE.md](SOURCE_PROVENANCE.md).

## SysDVR — exelix11 and contributors

- Project: https://github.com/exelix11/SysDVR
- Licence text: [NXCR distribution `LICENSE`](LICENSE), matching GNU GPL version 2; upstream [SysDVR licence](https://github.com/exelix11/SysDVR/blob/master/LICENSE).
- NXCR's capture implementation shows substantial lineage with the SysDVR `grc:d` capture implementation, including one-time `grcdServiceBegin` retry handling. NXCR adds local microSD MKV recording, Tesla controls and later feature/fix layers.
- Preserve original copyright headers/notices for any adapted code and identify modified files and dates. **Exact file-level contribution inventory remains open.**

## libtesla — WerWolv and contributors

- Project: https://github.com/WerWolv/libtesla
- Source pinned by the NXCR workflow: `f766e9b607a05e9756843cbd62b3bfb98be1646c`
- Licence text: [full pinned libtesla GPL v2](LICENSES/libtesla-GPL-2.0.txt); [upstream original](https://github.com/WerWolv/libtesla/blob/f766e9b607a05e9756843cbd62b3bfb98be1646c/LICENSE).
- Used to build NXCR's Tesla overlay. Corresponding overlay source and build details need to be published alongside binaries in compliance with the applicable terms.

## libnx — libnx authors

- Project: https://github.com/switchbrew/libnx
- Licence text and original copyright/disclaimer: [full libnx licence](LICENSES/libnx-LICENSE.md) (ISC-style).
- Preserve the libnx copyright notice and permission/disclaimer text where applicable.

## stb_truetype.h — Sean Barrett / RAD Game Tools and contributors

- Pinned vendored header: `overlay/libtesla/include/stb_truetype.h` within libtesla's source checkout (v1.24), with contributions listed in its header.
- The header offers **MIT or public-domain terms**. For NXCR's distribution, its MIT alternative and attribution are preserved in [LICENSES/stb_truetype-MIT.txt](LICENSES/stb_truetype-MIT.txt); the entire original notice remains in the bundled header.
- This is a separate third-party component included in the upstream libtesla dependency; libtesla's GPL notice does not erase this header's author and licence notice.

## GCC C++ runtime, Newlib C runtime and math libraries

The pinned **devkitA64 r29.2-1** build uses **GCC 15.2.0-7**, **Newlib 4.6.0.20260123-4**, **libnx 4.12.0-1**, and **switch-tools 1.13.1-1**. Actual NXCR executable link maps show static references to libnx, GCC's `libstdc++.a`, and Newlib's `libg.a`/`libm.a`.

- GCC runtime libraries: the GNU GPL version 3 **with GCC Runtime Library Exception 3.1 where applicable**. The full [GPL v3](LICENSES/gcc-15.2.0-COPYING3) and [GCC runtime exception](LICENSES/gcc-15.2.0-COPYING.RUNTIME) texts from the pinned GCC 15.2.0 release are included. This separate toolchain licence does not mean the NXCR recorder is relicensed to GPL v3.
- Newlib: a multi-author, multi-licence C/math runtime. Its exact original [Newlib 4.6.0.20260123 COPYING.NEWLIB](LICENSES/newlib-4.6.0.20260123-COPYING.NEWLIB) has been preserved. The upstream newlib-4.6.0.20260123 archive and its licence file hashes were verified against their published SHA-256 values.
- The full source/link inventory and outstanding rights questions are recorded in [LEGAL_PROVENANCE_AUDIT.md](LEGAL_PROVENANCE_AUDIT.md). These notices preserve identified licences; they do not independently resolve every licence exception or the ownership of uncredited original prototype code.

## Before publishing

- [x] Inventory all 18 files in the recoverable original prototype and map identified derived components. **Individual contributor ownership remains unverified for some prototype code**; see the detailed audit.
- [x] Verify upstream GPL and other identified licence texts; conservatively retain GPL version 2 for the combined recorder. **This does not establish copyright ownership for every original file**. Do not infer GPL-2.0-or-later permission for all upstream code from the prototype's `LICENSE-NOTE.txt` alone.
- [x] Add full upstream licence texts for the **identified** dependencies (SysDVR GPL v2, pinned libtesla GPL v2, libnx ISC and stb_truetype MIT). Additional authors/third-party files from the original prototype remain to be audited; completeness is **not** certified.
- [x] Publish the readable post-patch source in this repository, pinned libtesla as a submodule, corresponding notices and build instructions. The matching final binary and fully bundled corresponding-source ZIP are still pending prerelease publication.
- [x] Document changed source layers, pinned libtesla commit and devkitA64 image, and publish [build instructions](BUILDING.md). Exact libnx package revision and file-level provenance review remain outstanding.

NXCR branding does not imply ownership of upstream components. No affiliation or endorsement by the original projects is claimed.
