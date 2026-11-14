# NXCR third-party acknowledgements — draft for v0.4.1 Beta 1

**PRE-RELEASE DRAFT — review before public binary distribution.** This file is an attribution map and does not replace the full licences/notices or corresponding source required by applicable licences. See [LICENSING_REVIEW.md](LICENSING_REVIEW.md).

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

## Before publishing

- [ ] Inventory all code from the original `project.b64` source archive and subsequent patches, including additional third-party files.
- [ ] Confirm exact upstream copyright notices and GPL version permissions before choosing the final NXCR licence label. Do not infer GPL-2.0-or-later permission for upstream code from the prototype's `LICENSE-NOTE.txt` alone.
- [x] Add full upstream licence texts for the **identified** dependencies (SysDVR GPL v2, pinned libtesla GPL v2, libnx ISC and stb_truetype MIT). Additional authors/third-party files from the original prototype remain to be audited; completeness is **not** certified.
- [x] Build a candidate corresponding-source ZIP from the **post-patch compiled source tree**, including pinned libtesla source, copyright notices, source manifests and build instructions, and independently rebuild it in CI. **Public distribution** of that exact source alongside a final binary is still pending.
- [x] Document changed source layers, the pinned libtesla commit and the devkitA64 image, and publish build steps in [BUILDING.md](BUILDING.md). Exact libnx package revision and file-level provenance review remain outstanding.

NXCR branding does not imply ownership of upstream components. No affiliation or endorsement by the original projects is claimed.
