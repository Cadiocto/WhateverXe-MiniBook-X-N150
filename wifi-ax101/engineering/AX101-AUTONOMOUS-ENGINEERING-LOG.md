# AX101 Autonomous Engineering Log

Append-only engineering record. Historical entries are not rewritten.

## RUN #65 — Dedicated GLDv3 metadata publisher candidate

- Date: 2026-11-24 (exact wall-clock timestamp not exposed by the execution environment)
- Branch: `ax101-run64-validation`
- Initial HEAD: `5b3cdf5eea7d4decbdd655c77ab4c57a73f8f2c3`
- Pinned solnix: `atiq-cs/solnix@1cee7021091f3df92b7816ece2902ff59b37f1be`
- Pinned illumos-gate: `illumos/illumos-gate@098e95820891f8a83fae679daece7fcf58b7b3a4`
- Firmware: API 77, `iwx_cfg_so_a0_hr_b0`, `iwx-so-a0-hr-b0-77`
- Main branch was inspected and left unchanged.

### Candidate changes

- Added production overlay `wifi-ax101/patches/0013-bssid-gld-publication.patch`.
- Replaced direct callback MAC publication with a dedicated kernel worker.
- Added pending, published and failed generation tracking; generation overflow is fail-closed.
- Worker snapshots `wifi_data_t` under publisher synchronization and calls `mac_pdata_update()` followed by `mac_tx_update()` after dropping its locks.
- Detach candidate drains n80211 callbacks, drains publisher generation, calls `mac_unregister()` unlocked, resumes callbacks and publisher on unregister failure, and joins the worker before destroying synchronization objects.
- Replaced substring-based BSSID checker with a comment/string-aware lexical function/lock-state checker and built-in independent controls.
- Added a deterministic 16-interleaving publisher/lifecycle model harness.
- Updated the OpenIndiana workflow to apply overlay 0013, execute the checker and harness, and trigger on the validation branch.
- Added pinned solnix tree/blob inventory and SHA-256 snapshots for the relevant source files to CI artifacts.

### Verification status at commit preparation

- Remote HEADs and active workflow runs were checked; no active AX101 run was found.
- Production overlay application: NOT YET VERIFIED by the OpenIndiana workflow.
- Checker self-tests: NOT YET EXECUTED in CI.
- 16 interleaving model scenarios: NOT YET EXECUTED in CI.
- Full patch stack: NOT YET VERIFIED.
- Per-TU compilation, linking, ELF acceptance: NOT TESTED.
- Native OpenIndiana module load/unload: NOT TESTED.
- Physical PCI attach, firmware ALIVE, scan, WPA2, TX/RX, DHCP/DNS and internet: NOT TESTED.
- CI Run ID, Job ID, Artifact ID and artifact SHA-256: pending.

### Engineering caveat

The 16-case harness is a deterministic interleaving model, not a substitute for an actual kernel-thread race test. No P0 closure or BUILD VERIFIED claim is made by this entry. The production overlay must pass the pinned-source OpenIndiana workflow before acceptance.


### RUN #65 CI trigger follow-up

- The validation commit was created through the Git data API and the branch ref was advanced with a compare-and-swap lease.
- No Actions run was created for that ref update; this append-only journal update uses the Contents API to generate a normal branch push event for the same validation workflow.
- The follow-up commit changes no production code.

### RUN #65 verification addendum

- Initial implementation commit: `38a85595b6f6e38b3cf4bdb9b005a9d6e3d8c1e5`.
- Checker parser-depth correction commit: `1e59f24642a451a2f194f299ea7afc300f16b7ff`.
- Publisher generation-overflow/drain correction commit: `2a6c07c595524d677724370815c2eca8d5d7f4fa`.
- Validation branch HEAD immediately before this addendum: `2a6c07c595524d677724370ac014a5bcd9102bc5`.
- Primary branch HEAD remained `5b3cdf5eea7d4decbdd655c77ab4c57a73f8f2c3`.
- Actions query for the validation commits returned zero workflow runs; no active run was present. The available GitHub integration exposes workflow-run reads and reruns but no workflow-dispatch operation. Commits created through this integration did not trigger the push workflow.
- Therefore patch application, checker execution, C harness execution, OpenIndiana compilation, linking, ELF acceptance, and native module load/unload remain NOT VERIFIED / NOT TESTED.
- P0 status: IMPLEMENTATION CANDIDATE ONLY — BUILD BLOCKED pending an Actions run on the validation branch.
- Physical PCI attach and all real network acceptance gates remain NOT TESTED.

### RUN #65 journal correction

- The publisher overflow/drain correction commit SHA in the preceding addendum was mistyped. The correct commit is `2a6c07c595524d677724370ac014a5bcd9102bc5`.
- Branch HEAD before this correction entry: `bdde9144c1bbb3b9b1a815feec39231d751c39e1`.
