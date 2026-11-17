# ROTFB Autonomous Engineering Log

> Append-only journal for the ROTFB project on `gop-rotation-hook`. GitHub Issues are disabled for this repository (GitHub API returned HTTP 410 when creating the requested issue), so this file is the durable fallback. This file is intentionally outside `illumo_rotfb/**` and the target build workflow path filters, so journal-only commits do not trigger target builds.

## ROTFB AUTONOMOUS ENGINEERING REPORT

* Timestamp UTC: 2026-11-18 (current session; GitHub API timestamps below are authoritative)
* Branch: `gop-rotation-hook`
* Current HEAD at inspection: `c0a906084c311d9971a2e66a827f7fd3c7fb924f`
* Last successful build: #49 / Run ID `44040470164` / `cc0f1399db65a04bcbdacd8d61970522d5966c7a`
* Active build: #61 / Run ID `44193836335` / `c0a906084c311d9971a2e66a827f7fd3c7fb924f`
* Latest build: #61
* Build status: IN_PROGRESS (checked via GitHub Actions API)
* Patch validation: NOT VERIFIED for HEAD #61; workflow is applying lexical `illumo_rotfb/patches/*.patch` sequence against pinned upstream `098e95820891f8a83fae679daece7fcf58b7b3a4`
* Geometry/clipping tests: PASS on Build #61 (step completed successfully)
* Compilation status: NOT VERIFIED; OpenIndiana VM step still in progress
* Link status: NOT VERIFIED
* New errors: none observed yet for Build #61; logs are not downloadable while job is in progress
* Root cause:
  * Build #59 / Run `44191304244`: `git apply` failed with `corrupt patch` at 0004 line 40. It stopped before compilation.
  * Build #60 / Run `44193502726`: host-side model test failed with `NameError: name 'Path' is not defined`; OpenIndiana VM step was skipped. Subsequent commit `c0a9060` added `from pathlib import Path`.
* Files investigated:
  * `.github/workflows/build-illumos.yml`
  * `.github/workflows/build-gop-rotation-hook.yml`
  * `.github/workflows/ci.yml`
  * `illumo_rotfb/rotfb_model_test.py`
  * `illumo_rotfb/rotfb.h`
  * `illumo_rotfb/rotfb_boot.c`
  * `illumo_rotfb/rotfb_runtime.c`
  * `illumo_rotfb/patches/0001-0008`
  * pinned upstream `usr/src/uts/i86pc/Makefile.files` and `usr/src/uts/i86xpv/Makefile.files`
* Changes made: no ROTFB source or build-workflow changes in this session. A root-level journal file is being created as an Issues-disabled fallback.
* Commit SHA: pending journal-only commit
* Local verification: GitHub source inspection completed; local git clone unavailable in this execution environment because outbound DNS/network access is blocked.
* CI verification: Build #61 is active; no new build dispatched.
* Remaining blockers:
  * Wait for Build #61 to finish, then inspect complete job logs and artifact contents.
  * GitHub Issues are disabled (HTTP 410), so the requested Issue cannot be created through the available GitHub integration. Repository settings are not being changed.
  * Hourly automation is configured, but a successful scheduled agent run has not yet been observed.
  * Physical Chuwi hardware behavior remains NOT VERIFIED.
* Next autonomous action: inspect Build #61 completion, full logs, patch-apply log, compile/link evidence, and artifacts before any further target build.

## Operating invariants

* One active target ROTFB build maximum; check all target workflow runs before dispatch.
* Preserve the pinned upstream commit and clockwise shadow-framebuffer mapping.
* Only modify ROTFB and its dedicated build workflow.
* Do not claim compile/link or physical-hardware success without logs/evidence.
* Append new reports below; do not rewrite earlier entries.
