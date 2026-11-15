# NXCR v0.4.1 Beta 1 test summary

## Latest private test candidate — requires console confirmation

Candidate compiled from public source commit `39dd369a7af6684dcb511cccfbdf91dabb79c30c` on **15 November 2026** using the pinned devkitA64 container and libtesla dependency. Installation and source ZIPs were built in the same private CI run, passed archive checks, and have verified SHA-256 checksums.

- Install archive SHA-256: `bf5debaf7b08fa19b71b87872500d68a6e911f54520181fd9bcc9a4150e94eeb`
- Source archive SHA-256: `49f52474ab0c5f2c29367b85f890ceecf93c9da46350d91fad5d23c2af42225d`
- **Not a public release and not yet hardware-tested.**

### Minimal console smoke test

1. Free sufficient microSD space (several GB recommended), and back up existing videos, settings and the prior recorder sysmodule folder.
2. Stop the old recorder; replace only the NXCR/SysDVR-SD shared Atmosphère title ID `00FF0000A53BB667`. Avoid installing multiple recorders using `grc:d`.
3. Extract the install ZIP onto the **microSD root**, preserving the `atmosphere/` and `switch/` layout; remove a separate obsolete `switch/.overlays/SysDVR-SD.ovl` after backing it up, if present.
4. Reboot Atmosphère, open a previously working game, and make a short NXCR recording with game audio; add a marker if desired.
5. Stop recording, inspect MKV picture/audio and the marker sidecar, then reboot once and confirm NXCR still opens. Avoid changing settings while recording.

Record whether the tested build matches the two hashes above. The earlier 17/17 core regression report applies to a previous candidate and **must not** be represented as testing this newly compiled package.


**17/17 original core regression behaviours were reported working by the project owner on a physical Nintendo Switch** for the candidate development line (November 2026). This does not mean 17/17 independent lab tests or that all advanced safety scenarios have passed.

## Confirmed core behaviours

- Switch boot and NXCR Tesla overlay presence
- Video and game audio recorded as playable MKV files on microSD
- Start/Stop with playable finalised files
- Marker sidecars aligned to recordings
- Live status and timer displayed during testing
- Basic HOME recovery with known black-flash/possible trimming limitations
- Thirty-minute segmentation, persistence across reboot and settings persistence
- R + Album and R + game shortcuts
- Existing recordings remain intact
- A non-recordable game can remain Armed without crashing and normal recording can resume afterward
- Low-space refusal: with about **550 MB free** and a configured **1 GB minimum**, no new recording was produced

## Remaining limitations and unverified edge cases

- Low-space warning wording and successful recording after freeing space
- First-run configuration creation and invalid-file handling
- Mid-recording changes of audio, segment length or storage threshold
- Interruptions during writes and repeated fast Start/Stop
- Broad Switch firmware, game and SD-card compatibility
- Potential extra gameplay cut or black frames around HOME changes

**For Beta 1, stop recording before changing configuration settings**, and do not deliberately fill an SD card or interrupt its power during recording.

See [GAME_COMPATIBILITY.md](GAME_COMPATIBILITY.md).
