# NXCR — Switch Continuous Recording

**Record compatible Nintendo Switch gameplay directly to microSD**, without a PC or external capture card while recording.

NXCR is an experimental Nintendo Switch homebrew background recorder for **Atmosphère**, with a **Tesla overlay** for controls and status. Compatible games can be recorded as MKV files with game audio, generally up to 720p / 30 fps as provided by the Switch's internal capture service.

> **Experimental Beta 1:** Get the installation archive and matching corresponding source from [GitHub Releases](../../releases). Gameplay support is game-dependent, and this is not a stable release. The automatic GitHub “Source code (zip)” is **not** the SD-card installation package. See [licensing and provenance limitations](LEGAL_PROVENANCE_AUDIT.md).

## Features

- Video and game audio recorded to `/Videos/NXCR/` on the microSD card
- Start/Stop controls, live status and marker requests via Tesla overlay
- Segment rollover (30 minutes by default), with configurable segment length
- Minimum-free-space recording protection and diagnostic settings
- Settings and control files under `/switch/NXCR/`

The Android companion, network remote control and controller-input visualiser are **not** in v0.4.1.

## Installation (once the beta is released)

1. Back up existing video recordings, settings and any previous recorder sysmodule.
2. Download the **NXCR installation ZIP** from [Releases](../../releases), not GitHub's automatic source archive.
3. Copy the ZIP's `atmosphere/` and `switch/` folders to the root of your Switch microSD.
4. Restart Atmosphère, start a compatible game and choose **NXCR** in Tesla.
5. Test a short recording, turn recording off, and verify the resulting MKV plays with picture and audio.

**Recorder conflict:** NXCR uses Atmosphère title ID `00FF0000A53BB667`; back up and replace *only* that folder when updating an older build, and disable competing applications using the exclusive `grc:d` capture service. Do not delete your entire `atmosphere/contents/` directory.

## Known limitations

- Some games are unsupported; on certain titles recording remains **Armed** without producing a playable video.
- The HOME menu is not recorded. Resuming gameplay can cause brief black frames and possibly cut extra gameplay around repeated HOME transitions.
- A fixed initialisation wait may slow the Switch startup logo.
- Recording can fail on full/slow microSD cards. Stop recording before adjusting live configuration settings.
- Broad compatibility across firmware versions, console models and games has **not** been established.

Early physical Switch testing reported **17/17 original core regression checks** passed, but advanced edge cases, final packaging and independent testing are not fully complete.

## Licence and credits

NXCR's `grc:d` capture implementation has substantial lineage with [SysDVR](https://github.com/exelix11/SysDVR) by exelix11 and contributors. The Tesla overlay uses [libtesla](https://github.com/WerWolv/libtesla); the recorder uses [libnx](https://github.com/switchbrew/libnx). These original authors retain their rights.

See [LICENSE](LICENSE), [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md), and the detailed [licence/provenance audit](LEGAL_PROVENANCE_AUDIT.md). NXCR is not affiliated with or endorsed by Nintendo or the upstream authors.

## Source and issues

The [readable source](source/), [overlay source](overlay/source/), [pinned libtesla submodule](overlay/libtesla), [building instructions](BUILDING.md), [source provenance](SOURCE_PROVENANCE.md) and [test summary](TESTING.md) are public here. The older development repository remains separate and private. Report problems with your NXCR version, system/Atmosphère version, game and reproducible steps, without uploading private logs or recordings.
