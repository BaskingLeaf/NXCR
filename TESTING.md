# NXCR v0.4.1 Beta 1 test summary

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
