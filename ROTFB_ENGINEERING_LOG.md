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


## ROTFB AUTONOMOUS ENGINEERING REPORT

* Timestamp UTC: 2026-11-17T23:28:58Z
* Branch: `gop-rotation-hook`
* HEAD before repair: `958a1710a6c05ecd8bd435f50df9474a998c3994`
* Repair commit: `40d901eee5fc5fd2ffb94af0b5e492b7095191ad`
* Latest completed target run inspected: #61 / Run `44193836335` / `c0a906084c311d9971a2e66a827f7fd3c7fb924f`
* Target workflow inventory: 61 runs (#1–#61), all completed at the pre-push recheck; no queued or in-progress target build remained.
* Build #61 result: FAILURE. Geometry/clipping model tests passed. OpenIndiana VM step failed before compilation or linking.
* First root cause: `git apply --check` rejected the i86xpv hunk in `0004-Makefile-files.patch` at `usr/src/uts/i86xpv/Makefile.files:192`. The hunk had only two leading context lines and no trailing context. The pinned upstream block at lines 192–194 matched textually, but this short context was not accepted by `git apply`. The earlier boot-object insertion in the same file also shifts the new-side position.
* Minimal repair: expanded the i86xpv hunk to include the preceding `BALLOON_OBJS`, `DOMCAPS_OBJS`, and `EVTCHN_OBJS` lines and following `IOAT_OBJS`, `ISANEXUS_OBJS`, and `PCI_E_NEXUS_OBJS` lines; updated the hunk header to `@@ -189,7 +190,7 @@`. Updated the model-test contract assertion to expect the corrected hunk header. No runtime source or shadow-framebuffer behavior changed.
* Patch audit: all eight patches in lexical order were checked. All hunk old/new line counts are internally consistent, all files end with a newline, and the 0006 console-contract guards are present.
* Local validation: ROTFB geometry/clipping model tests PASS; corrected 0004 patch `git apply --check` PASS against a local fixture containing the exact pinned-upstream context for both Makefile.files targets; `git diff --check` PASS for the two-file change. A full local upstream clone/whole-series apply was not possible because outbound DNS resolution is unavailable in this execution container.
* Commit contents: only `illumo_rotfb/patches/0004-Makefile-files.patch` and `illumo_rotfb/tests/rotfb_model_test.py`.
* Build scheduling: rechecked all 61 target runs immediately before moving the branch ref; zero were queued/in progress. Pushed one source commit, which is covered by the workflow's `illumo_rotfb/**` path filter and should create one automatic target run. No manual dispatch or concurrent build was issued. The next cycle must inspect that run and confirm patch application through 0008 before assessing compilation/linking.
* Issues fallback: GitHub Issues remain disabled (HTTP 410); this append-only root log remains the durable engineering record and is outside the target workflow path filters.
* Hardware validation: NOT PERFORMED. No claim of Chuwi hardware success.


## ROTFB AUTONOMOUS ENGINEERING REPORT

* Timestamp UTC: 2026-11-18T00:23Z
* Branch: `gop-rotation-hook`
* HEAD before repair: `7645f5971a6d580d0c7bcb33860c5cb0e26d99f1`
* Repair commit: `ee13395843657c844abdcb682db4575156db45ba`
* Target run inventory: 62 ROTFB runs (#1–#62); all 62 were completed at the pre-push recheck. No queued or active target run remained.
* Build #62 / Run `44200842413` / commit `40d901eee5fc5fd2ffb94af0b5e492b7095191ad`: FAILURE.
  * Full job log retrieved: job `134454182028`, 11,372 lines / 1,597,331 bytes.
  * Diagnostic artifact retrieved: artifact `14380134859`, 287,643 bytes.
  * `patch-apply.log`: patches 0001–0005 applied; patch 0006 failed at `usr/src/uts/i86pc/boot/boot_fb.c:680` with `patch does not apply`.
  * First root cause: the boot-console shiftline hunk in 0006 could not match its one-line context in the file after patches 0001–0005. The VM stopped in patch application; no compilation or linking occurred.
* Build #61 / Run `44193836335` was also rechecked with its full job log and artifact `14379285258`: patches 0001–0003 applied, then 0004 failed. Build #62 confirms the preceding 0004 context repair worked: 0004 now applies, as does 0005.
* Minimal repair: expanded the 0006 shiftline hunk from a single context line to the surrounding `s_col`, `s_row`, `e_col`, and `t_col` function-local context; updated its hunk range to `@@ -677,8 +721,8 @@`. The two corrected off-by-one assignments are unchanged. Added model-test assertions for the expanded hunk anchor. Only `illumo_rotfb/patches/0006-rotfb-console-contract-and-copy-safety.patch` and `illumo_rotfb/tests/rotfb_model_test.py` changed.
* Patch/workflow audit: all eight patches were inspected in lexical application order; ROTFB sources and the model test were reviewed. `.github/workflows/build-illumos.yml` remains pinned to upstream `098e95820891f8a83fae679daece7fcf58b7b3a4`, uses the `illumo_rotfb/**` and workflow path filters, and has non-cancelling single-build concurrency. Root-level journal remains outside those path filters.
* Local validation: geometry/clipping model tests PASS; Python syntax compilation PASS; expanded shiftline hunk `git apply --check` and application PASS against a local fixture carrying the pinned-upstream function context; fixture `git diff --check` PASS. Full sequential application of 0001–0008 against a complete local upstream checkout remains unavailable because outbound DNS is blocked in this execution container.
* Build scheduling: immediately before updating the branch ref, rechecked all 62 target runs and confirmed zero queued/in progress. Moved the branch once to `ee13395843657c844abdcb682db4575156db45ba`; the two source/test paths match the automatic workflow filter. No manual dispatch or concurrent build was issued. The next cycle must inspect the resulting run and its artifact; do not dispatch another build while it is queued or active.
* Issues fallback: GitHub Issues remain disabled (HTTP 410); this append-only file remains the durable record. No settings were changed.
* Hardware validation: NOT PERFORMED. No Chuwi hardware result is claimed.


## ROTFB AUTONOMOUS ENGINEERING REPORT

* Timestamp UTC: 2026-11-18T01:40:38Z
* Branch: `gop-rotation-hook`
* HEAD at inspection: `be4b8ab4be49affe9f7dbfba98d9c6b57edd461d`
* Previous source commit tested by Build #63: `ee13395843657c844abdcb682db4575156db45ba`
* Target workflow inventory: 64 runs (#1–#64); Build #64 is the only queued/in-progress target run at the final check.
* Latest completed run: #63 / Run ID `44209198579` / commit `ee13395843657c844abdcb682db4575156db45ba` — FAILURE.
  * Job ID: `134482907109`; full decoded job log retrieved.
  * Artifact ID: `14384543969`; 287,663 bytes; SHA-256 `9991bcb0be4ee74e3530ebcf0e6c69ee1161737f241a404dcc1a3927c58de755`.
  * Geometry/clipping model test step: PASS.
  * OpenIndiana VM started and checked out pinned upstream `098e95820891f8a83fae679daece7fcf58b7b3a4`.
  * `patch-apply.log` confirms 0001–0006 applied. The first failing operation was `git apply --check 0007-gfxp-bitmap-bounds-hardening.patch`, which returned `error: patch failed: usr/src/uts/i86pc/io/gfx_private/gfxp_bitmap.c:442` / `patch does not apply`.
  * No compiler or linker stage was reached. The VM step exited with code 1.
* Root-cause status: the first CI failure is confirmed as patch-application failure in 0007 at the `gfxp_bitmap.c` hunk starting at line 442. The exact source/context discrepancy is not yet proven. A text-level reconstruction using the pinned raw upstream file plus patches 0002, 0005, and 0006 found exact old-context matches for all 17 hunks in 0007 at their declared positions; an isolated local `git apply --check` fixture for the failing display hunk also passed. This does not reproduce the OpenIndiana VM's failure, so no speculative 0007 edit was made.
* Additional patch audit: all eight patches were inspected in lexical order; all end with a newline. Patch 0007 has 17 hunks with internally consistent declared counts. A separate non-blocking formatting defect remains in 0004: the i86xpv Makefile hunk declares 7/7 lines but contains 9/9 lines. Local Git accepts the current hunk while ignoring its final two context lines; changing the header to 9/9 passes the isolated fixture. This was not changed while the primary 0007 failure remains unresolved and the diagnostic build is active.
* Tests and workflow inspected: `.github/workflows/build-illumos.yml`, `illumo_rotfb/tests/rotfb_model_test.py`, `rotfb.h`, `rotfb_boot.c`, `rotfb_runtime.c`, and patches 0001–0008. The workflow remains pinned to the required upstream commit, retains the non-cancelling `rotfb-single-build` concurrency group, and its path filters still exclude this root-level journal.
* Local checks: geometry/clipping model functions PASS; Python syntax compilation PASS; isolated 0007 display-hunk fixture `git apply --check` PASS; pre-0007 diagnostic shell snippet `sh -n` PASS; workflow static checks confirmed unchanged path filters/concurrency and no trailing whitespace. A full local upstream checkout and the OpenIndiana-packaged Git executable are unavailable here because outbound DNS to GitHub is blocked.
* Diagnostic workflow change: commit `be4b8ab4be49affe9f7dbfba98d9c6b57edd461d` modifies only `.github/workflows/build-illumos.yml`. Immediately before attempting 0007 it now captures the OpenIndiana Git version, checksum, exact pre-0007 `gfxp_bitmap.c`, relevant context lines, and the exact 0007 patch into the diagnostic artifact. This is intended to resolve the mismatch between the CI result and the local context reconstruction.
* Build scheduling: before the workflow commit, all 63 existing target runs were completed and zero were queued/in progress. The workflow commit triggered exactly one new target run: #64 / Run `44218726339` / HEAD `be4b8ab4be49affe9f7dbfba98d9c6b57edd461d`. It is currently IN_PROGRESS. No manual dispatch or second build was issued. This report is journal-only and does not match the workflow path filters.
* Issues fallback: GitHub Issues remain disabled (HTTP 410); no issue creation was retried and no repository settings were changed.
* Hardware validation: NOT PERFORMED. No physical Chuwi result is claimed.
* Next action: after #64 completes, inspect its full job log and artifact, compare the captured pre-0007 file and Git version against the pinned-source reconstruction, then make only a proven minimal patch repair. Do not dispatch another build while #64 is active.
