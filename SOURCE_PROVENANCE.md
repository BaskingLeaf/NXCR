# NXCR v0.4.1 source provenance

## What this repository contains

The source code here is the readable, assembled NXCR v0.4.1 release-candidate source, not a copy of the private development repository's complete history or experiments. It was prepared from the code that built the candidate sysmodule and overlay and contains dated modification notices in relevant C/C++/shell files.

The upstream libtesla source is pinned through a Git submodule at commit `f766e9b607a05e9756843cbd62b3bfb98be1646c`. The devkitA64 build environment used for the candidate was `devkitpro/devkita64:20260219`.

## Upstream and copyright

- **SysDVR:** The `grc:d` capture layer has significant implementation similarities to exelix11's [SysDVR](https://github.com/exelix11/SysDVR), including the guarded one-time Begin-and-retry logic and service wrapper. It should be treated as derivative or influenced code for attribution and GPL compliance; this project does **not** claim to have independently invented the capture implementation.
- **libtesla:** The overlay is built using WerWolv/libtesla, which carries GNU GPL version 2 terms.
- **libnx:** Nintendo Switch homebrew SDK; the libnx authors' ISC-style licence text is retained.
- **stb_truetype:** Bundled with pinned libtesla; its separate MIT/public-domain alternatives and authorship are documented in [LICENSES/](LICENSES/).

The first prototype contained a notice expressing an intent to distribute under GPL-2.0-or-later. That note cannot expand permissions granted by other copyright holders, and **this project uses GPL version 2 as the conservative working distribution licence**. Do not assume that all upstream code is GPL-2.0-or-later.

## Scope and remaining verification

The original development line included local MKV muxing, segmentation, runtime configuration, a Tesla UI, markers and later reliability changes. Their complete individual contribution history is not transferred into this clean public repository. The public source is provided as a modification-friendly snapshot, and authors' rights are preserved.

Before publishing a compiled public beta, verify: final exact-source/binary matching, all applicable notices, linked toolchain runtime dependencies, the compatibility and safety disclosures, and any unresolved original-prototype contributor permissions. This page is not legal certification.

See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md), [LICENSE](LICENSE), and [BUILDING.md](BUILDING.md).
