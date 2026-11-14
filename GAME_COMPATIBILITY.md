# NXCR game compatibility testing

These are **owner-reported physical Switch test observations**, not a guarantee for every firmware, system configuration, SD card, or game version. All eight titles were selected because the owner identified them as lacking Nintendo's ordinary built-in clip recording. NXCR compatibility is a separate question.

**Test date:** 2026-11-13 (reported). **Build family:** NXCR v0.4.1 public beta candidate. Exact system and game versions have not been supplied.

| Game | NXCR result | Notes |
| --- | --- | --- |
| Descenders | **WORKS** | Owner reports playable local recording even though native clips are not supported. |
| FAST RMX | **ARMED / NO PLAYABLE VIDEO** | NXCR remains in **Armed** state; playable footage is not produced (owner-reported). |
| Rock of Ages 2: Bigger & Boulder | **ARMED / NO PLAYABLE VIDEO** | NXCR remains in **Armed** state; playable footage is not produced (owner-reported). |
| Rock of Ages 3: Make & Break | **ARMED / NO PLAYABLE VIDEO** | NXCR remains in **Armed** state; playable footage is not produced (owner-reported). |
| Slime-san | **NOT TESTED** | Pending. |
| Snake Pass | **ARMED / NO PLAYABLE VIDEO** | NXCR remains in **Armed** state; playable footage is not produced (owner-reported). |
| The End Is Nigh | **NOT TESTED** | Pending. |
| World of Goo | **NOT TESTED** | Pending. |

**Snapshot:** 1 working, 4 stuck in **Armed** without a playable recording, 3 untested. The user specifically reports **Armed** status in all four cases. This does **not** establish whether an empty/incomplete MKV is created, whether a capture-service error was raised internally, or whether switching afterward to a supported game is safe; no crash was reported.

## What the results tell us

- NXCR **does not universally bypass** games' capture limitations. In four tested titles, the recorder stays **Armed** rather than reaching an actively recording state. A success in *Descenders* cannot be generalized to every game where native clips are disabled.
- The root reason for the differences is **not established**. The underlying gameplay capture stream or title-specific behavior could differ; further logs/tests would be needed before assigning a cause.
- The owner showed an `atmosphere/exefs_patches/` folder without an obvious DVR-Patches `am` directory. This alone does **not** prove the console has no patches elsewhere.

## Suggested minimal re-test for a failing game

1. Start actual gameplay and attempt a 20–30 second NXCR recording.
2. Record what Tesla says (recording enabled, error/status), whether the game and console continue normally, whether an MKV is produced, and whether the file plays with video and audio.
3. Note Switch firmware, Atmosphère version, NXCR build, game version and any relevant capture patches, where available.
4. Stop recording before changing settings. Avoid enabling both NXCR and SysDVR simultaneously.

**Separate regression gate:** Remaining **Armed** without a playable video is not the same as crashing. To close the failure-handling test, verify that NXCR can be switched OFF afterward and a supported game still records normally, without corrupting earlier files. Consider a future timeout or clear **No capture stream available** status instead of waiting indefinitely in **Armed**.

See [PUBLIC_RELEASE_CHECKLIST.md](PUBLIC_RELEASE_CHECKLIST.md) for the beta acceptance tests.
