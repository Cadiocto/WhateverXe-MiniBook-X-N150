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


## ROTFB AUTONOMOUS ENGINEERING REPORT

* Timestamp UTC: 2026-11-17T22:53Z (GitHub API observation window)
* Branch: `gop-rotation-hook`
* Current HEAD before this journal append: `e797c0cfeb65f07c86dbe216bc521cb3f3e53b96`
* Last successful build: #49 / Run ID `44040470164` / `cc0f1399db65a04bcbdacd8d61970522d5966c7a`
* Active build: #61 / Run ID `44193836335` / `c0a906084c311d9971a2e66a827f7fd3c7fb924f`
* Latest build: #61
* Build status: IN_PROGRESS
* Patch validation: NOT VERIFIED for current build #61. Current patch inventory is 0001–0008 in lexical order; every patch has a final newline and well-formed visible hunk headers. Full sequential `git apply --check` is still pending the active OpenIndiana VM job.
* Geometry/clipping tests: PASS on #61, step 3.
* Compilation status: NOT VERIFIED; OpenIndiana VM step is still running.
* Link status: NOT VERIFIED.
* New errors: none available for #61; the in-progress job log endpoint currently returns 404 and no artifact has been published yet.
* Root cause:
  * #60 / Run `44193502726`: `rotfb_model_test.py:87` raised `NameError: name 'Path' is not defined`; build VM was skipped. Commit `c0a9060` added `from pathlib import Path`; #61 model-test step now passes.
  * #59 / Run `44191304244`: the artifact's `patch-apply.log` confirms patches 0001–0003 applied, then patch 0004 failed at `git apply --check` with `corrupt patch ... line 40`. No compilation occurred.
* Files investigated:
  * `.github/workflows/build-illumos.yml` — uses concurrency group `rotfb-single-build`, `cancel-in-progress: false`; triggers on ROTFB path push or manual dispatch; no schedule trigger.
  * `.github/workflows/build-gop-rotation-hook.yml` and `.github/workflows/ci.yml` — separate non-ROTFB workflows; no additional target ROTFB build workflow found in the inspected workflow directory.
  * `illumo_rotfb/tests/rotfb_model_test.py` — imports `Path`; #61 test step passed.
  * `illumo_rotfb/patches/0001-0008`, `rotfb.h`, `rotfb_boot.c`, `rotfb_runtime.c`.
  * Pinned upstream Makefile.files at `098e95820891f8a83fae679daece7fcf58b7b3a4`.
* Changes made: no source or workflow changes. Existing root-level `ROTFB_ENGINEERING_LOG.md` is used as the fallback because repository metadata reports `has_issues=false`; GitHub issue creation returned HTTP 410 ("Issues has been disabled"). No repository settings were changed.
* Commit SHA: journal-only append to this file; resulting commit SHA is returned by GitHub after this update.
* Local verification: no local clone or upstream patch-application run was performed in this cycle. The #61 CI model-test step is the only current test execution evidence.
* CI verification:
  * #61 remains active on commit `c0a9060`; no new target build was dispatched.
  * #60 artifact contains only runner-side index files (528 uncompressed bytes); it confirms no VM diagnostics were collected because the VM step was skipped.
  * #59 artifact contains the patch-apply log and Perl package audit; patch-apply log ends at `CHECK 0004-Makefile-files.patch`.
  * #49 artifact confirms `nightly-exit-code=0`. Its historical build compiled `rotfb_boot.o` for i86pc and i86xpv, and `rotfb_runtime.o` for i86pc; that older patch series did not yet add `rotfb_runtime.o` to the i86xpv object list, so #49 does not prove current i86xpv runtime integration.
* Remaining blockers:
  * Wait for #61 to finish and then inspect complete logs, patch-apply log, and compile/link evidence before another target build.
  * GitHub Issues are disabled; this Markdown file is the durable append-only fallback.
  * Hourly automation `6afcda7b6630819184b8240079e358ee` is enabled, but no completed scheduled agent execution is yet confirmed.
  * Physical Chuwi hardware behavior remains NOT VERIFIED.
* Next autonomous action: re-check all target runs; if #61 is still queued/in progress, do not dispatch or mutate build state. Once completed, retrieve and analyze its logs and artifacts before deciding whether a source/CI repair is justified.
