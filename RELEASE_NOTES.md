# NXCR v0.4.1 Beta 1 — Experimental prerelease

NXCR records compatible Nintendo Switch gameplay video and game audio directly to microSD without a PC or capture card.

## Included

- Gameplay recording to MKV (720p / 30 fps when supported) with game audio.
- NXCR Tesla overlay for recording controls, status and markers.
- Configurable recording segments and minimum-free-space protection.
- Recordings saved at `sdmc:/Videos/NXCR/`.
- The pre-input-telemetry v0.4.1 recorder baseline; experimental Bluetooth, phone pairing and controller visualiser features are **not** included.

## Install

Extract the SD-root ZIP to the root of your Switch microSD card and restart Atmosphère. Use **NXCR** in the Tesla overlay menu. The NXCR-only package stores settings and control files under `/switch/NXCR/`. Settings from older development builds are not migrated automatically; back up your existing files.

## Known limitations

- **HOME transitions:** Occasional brief black frames and a possible extra gameplay cut under repeated HOME transitions are reported, not fully resolved.
- **Game compatibility:** Some titles that disallow native Capture-button clips can still record (e.g., Descenders), while FAST RMX, Rock of Ages 2/3 and Snake Pass may remain **Armed** with no playable video on the tested setup. See GAME_COMPATIBILITY.md.
- **Storage:** Low-space recording refusal was observed (about 550 MB free with a 1 GB minimum), but a dedicated warning and recovery after restoring free space are not confirmed.

- **HOME cut/re-sync:** A brief black flash may occur when gameplay capture resumes after HOME. A possible extra cut of gameplay after repeated HOME transitions has also been reported, but is not yet confirmed or quantified. Neither is currently planned as a beta-blocking visual issue unless it leads to substantial footage loss or broken playback.
- **Slower Switch boot:** With NXCR enabled at startup, the Nintendo Switch logo can remain on-screen longer than usual. The recorder source currently includes a **20-second initialisation wait**. This does not necessarily mean a console freeze; the exact additional splash-screen duration has not been measured.
- NXCR requires Atmosphère and a compatible game capture service; it does not record the HOME menu.
- Do not run NXCR simultaneously with another `grc:d` client.
- Recording can fail on incompatible games, low/slow SD storage, or certain system transitions.

**Test evidence:** The maintainer reported **17/17 core regression checks passed** on an earlier candidate; the actual Beta 1 sysmodule and overlay were subsequently tested and produced a playable MKV with picture and game audio. The remaining behaviours were not all retested on the final binary. This is an **experimental prerelease**, not a stable compatibility guarantee.

**Licence and source:** NXCR contains SysDVR-derived GPL-covered capture code and uses libtesla, libnx and stb_truetype. The matching readable source archive, audited notices, and SHA-256 checksums accompany the installer. Individual authorship of parts of the initial MKV/recorder prototype has not been independently established; see [LEGAL_PROVENANCE_AUDIT.md](LEGAL_PROVENANCE_AUDIT.md).
