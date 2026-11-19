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


## ROTFB AUTONOMOUS ENGINEERING REPORT

* Timestamp UTC: 2026-11-18T02:39:03.066Z
* Branch: `gop-rotation-hook`
* HEAD before repair: `4d36e1e4afc820bcb1d531069d565e0c98e7dfcc`
* Source repair commit: `8bd6e933f417322ee47acf9e08c92f66a1210dc0`
* Latest completed target run inspected: Build #64 / Run ID `44218726339` / tested commit `be4b8ab4be49affe9f7dbfba98d9c6b57edd461d` — FAILURE.
  * Job ID: `134514982792`; full decoded job log retrieved (1,597,503 bytes).
  * Artifact ID: `14390221378`; 296,601 bytes; SHA-256 `0389302d178d1a640e6fac7dfe5d446e30ec91bc31f3b6b2ad06adf9f8acc225`.
  * OpenIndiana Git version: `git version 2.56.0`.
  * Exact pre-0007 source captured from the VM: `gfxp_bitmap.c`, 20,636 bytes; POSIX checksum `3495084633 20636`.
  * Patches 0001–0006 applied successfully. The first failure was `git apply --check 0007-gfxp-bitmap-bounds-hardening.patch` at `usr/src/uts/i86pc/io/gfx_private/gfxp_bitmap.c:442`. Compilation and linking were not reached.
* Confirmed root cause: three 0007 hunks (old starts 442, 602, and 644) ended immediately after changed lines and had no trailing unchanged context. Using the exact VM-captured pre-0007 file, the unmodified patch reproduced the failure at hunk 442. Adding trailing context only to hunk 442 advanced the failure to hunk 602; adding context to all three hunks made the complete 0007 patch pass `git apply --check`. This resolves the apparent mismatch between textually matching old-side lines and Git's patch-application failure.
* Minimal repairs:
  * `0007-gfxp-bitmap-bounds-hardening.patch`: added two exact following source-context lines to each of hunks 442, 602, and 644; updated counts to 22/32, 5/5, and 5/5 respectively. No C logic changed.
  * `0004-Makefile-files.patch`: corrected the i86xpv hunk header from 7/7 to 9/9. The pinned upstream block at lines 189–197 contains nine old/new lines; the prior header left the final two context lines outside the declared hunk.
  * `rotfb_model_test.py`: added unified-diff hunk-count validation across all lexical ROTFB patches and explicit trailing-context checks for the three 0007 hunks; preserved existing geometry and 0006 contract checks.
* Validation:
  * ROTFB clockwise mapping, bijection, last-pixel offsets, and clipping model tests: PASS in local validation harness.
  * Python syntax compilation for the updated test logic: PASS.
  * All eight current patch files were inspected in lexical order; after the two metadata repairs, every hunk's declared old/new counts match its body and every patch has a final newline.
  * Complete 0007 patch `git apply --check` and application against the exact Build #64 pre-0007 VM file: PASS; resulting `git diff --check`: PASS. Local Git was 2.47.3; the captured OpenIndiana VM uses Git 2.56.0.
  * Corrected 0004 i86xpv 9/9 hunk: PASS against a fixture containing the exact pinned-upstream lines fetched from commit `098e95820891f8a83fae679daece7fcf58b7b3a4`; `git diff --check`: PASS.
  * Full 0001–0008 application on a complete local upstream clone remains unverified because this execution container cannot resolve outbound GitHub DNS. CI #64 confirms 0001–0006 apply; the next target run must confirm 0007 and 0008 and continue into compile/link.
* Workflow audit: `.github/workflows/build-illumos.yml` remains pinned to `098e95820891f8a83fae679daece7fcf58b7b3a4`, path-filtered to `illumo_rotfb/**` and itself, with `rotfb-single-build` and `cancel-in-progress: false`. Root-level journal is outside the trigger paths.
* Build scheduling: all 64 existing target runs were rechecked immediately before the source commit; zero were queued or active. One atomic source commit was pushed to `gop-rotation-hook`; it changes only the two ROTFB patches and the ROTFB model test. No manual dispatch or concurrent build was issued. The path-filtered push created Build #65 / Run ID `44224788164` with status `in_progress` for HEAD `8bd6e933f417322ee47acf9e08c92f66a1210dc0`. The next cycle must inspect the resulting run and artifacts; do not dispatch while it is queued or active.
* Issues fallback: GitHub Issues remain disabled (HTTP 410); no issue creation was retried and no repository settings were changed.
* Hardware validation: NOT PERFORMED. No physical Chuwi result is claimed.


## ROTFB AUTONOMOUS ENGINEERING REPORT

* Timestamp UTC: 2026-11-18T04:17:38Z
* Branch: `gop-rotation-hook`
* HEAD at inspection before journal update: `1cdfcd05c0629e912e6e2cc041bf2dd84f252387` (journal-only commit; source under test is `8bd6e933f417322ee47acf9e08c92f66a1210dc0`).
* Target workflow inventory: 65 runs found; 2 successful, 63 failed, 0 queued or in progress at recheck.
* Latest completed target run: Build #65 / Run ID `44224788164` / source commit `8bd6e933f417322ee47acf9e08c92f66a1210dc0` — **SUCCESS**. Run URL: https://github.com/Cadiocto/WhateverXe-MiniBook-X-N150/actions/runs/44224788164
  * Job ID: `134535189637`; complete decoded job log retrieved (4,144,641 bytes).
  * Artifact ID: `14395752805`; 7,257,504 bytes; SHA-256 `47bf67f80023c84a497ef81cf8552d3419fd6c15187437a500529d2ef9bee786`; 35 files downloaded and inspected.
  * The artifact's `build-metadata.txt` confirms the exact pinned upstream commit `098e95820891f8a83fae679daece7fcf58b7b3a4`.
  * `patch-apply.log` confirms all eight patches applied in lexical order: 0001 through 0008. `git diff --check` ran in the VM step, which completed successfully.
  * ROTFB geometry/clipping model tests: PASS.
  * `nightly-exit-code.txt=0`; `final-build-exit-code.txt=0`. Full captured gate nightly log is 176,301,477 bytes and ends with “Nightly distributed build completed” at 2026-11-18 03:55:05 UTC (elapsed 1:09:38). No compiler error, fatal error, undefined symbol, syntax error, or dmake failure matched a boundary-aware scan of the full gate log.
  * Compile evidence: `rotfb_boot.o`, `rotfb_runtime.o`, `gfxp_bitmap.o`, and `gfxp_fb.o` each have 3 compile-command records for both i86pc and i86xpv. Link evidence: 2 `unix.o` commands including `rotfb_boot.o`, and 2 `gfx_private` link commands including `rotfb_runtime.o`, `gfxp_bitmap.o`, and `gfxp_fb.o`. Workflow evidence file reports PASS.
  * The upstream patch status/diffstat shows only the expected six files modified by the ROTFB patch series.
* Patch/test/workflow audit at current branch HEAD:
  * Inspected all eight patch files in lexical application order. All hunks have matching declared/actual old and new line counts; all patches end with a newline. The Build #65 VM run is the authoritative confirmation that the full series applies to the pinned source.
  * Inspected `rotfb_model_test.py`; its geometry, bijection, last-pixel, clipping/overflow, patch hunk-count, and patch-contract checks passed in Build #65.
  * Inspected `.github/workflows/build-illumos.yml`: pinned upstream unchanged; push path filters remain `illumo_rotfb/**` and the workflow file; root-level journal is excluded; concurrency remains `rotfb-single-build` with `cancel-in-progress: false`.
* Diagnostic follow-up: the broad `error:` substring scan in the workflow's diagnostic indexes falsely matches the xgettext keyword `directoryError:2`. A boundary-aware grep pattern was tested locally: real compiler-style `error:` and `fatal error:` lines remain matched while `directoryError:2` is excluded. This is diagnostic noise only; Build #65 itself succeeded. A minimal workflow-only correction is the next low-risk change.
* Scheduling: the latest recheck found no queued or active target run. No manual dispatch was used. The source commit tested by #65 is unchanged; current HEAD differs only by the root journal, which is outside path filters. No hardware validation was performed or claimed.
* GitHub Issues remain disabled (HTTP 410); no issue creation or repository-setting changes were attempted.


## ROTFB AUTONOMOUS ENGINEERING REPORT

* Timestamp UTC: 2026-11-18T04:22:25Z
* Branch: `gop-rotation-hook`
* HEAD at inspection: `b2b0021bcc03913b83e5bd9eabd0611ceb7a21c0`.
* Workflow-only diagnostic cleanup: the preceding commit `b2b0021bcc03913b83e5bd9eabd0611ceb7a21c0` changed only `.github/workflows/build-illumos.yml`, tightening three grep call sites to match standalone `error:` / `fatal error:` diagnostics rather than substrings such as xgettext's `directoryError:2`. The exact text replacement was checked (3 lines only); local grep samples confirmed true compiler-style errors remain matched and `directoryError:2` is excluded. No YAML structure, path filters, upstream pin, or concurrency settings changed.
* Build scheduling: the push-triggered validation run appeared after the initial API poll. Current target inventory: 66 runs. Build #66 / Run ID `44236793138` is **IN_PROGRESS** for current HEAD `b2b0021bcc03913b83e5bd9eabd0611ceb7a21c0`: https://github.com/Cadiocto/WhateverXe-MiniBook-X-N150/actions/runs/44236793138
* No second build was dispatched and no build state was changed. The only write in this report is this root-level journal update; the root journal is outside the workflow's push path filters, so it does not trigger another target build. Do not inspect or alter #66 until a later cycle finds it completed.
* Previous source validation remains Build #65 / Run ID `44224788164` — SUCCESS: all eight patches applied to pinned upstream, geometry/clipping tests passed, full nightly exited 0, and compile/link evidence passed for i86pc and i86xpv. No hardware validation was performed or claimed.
* GitHub Issues remain disabled (HTTP 410); no issue creation or repository-setting changes were attempted.


## ROTFB POST-BUILD #66 DEEP ENGINEERING AUDIT

* Timestamp UTC: 2026-11-18T06:01:28.345Z
* Branch: `gop-rotation-hook`
* HEAD/source commit: `f97f44f2e61558cc1866155bc8a04d03356215b2`
* Build #66: Run ID `44236793138`, commit `b2b0021bcc03913b83e5bd9eabd0611ceb7a21c0`, SUCCESS.
* New validation: Build #67 / Run ID `44248223071`, commit `f97f44f2e61558cc1866155bc8a04d03356215b2`, currently IN_PROGRESS. No concurrent run was dispatched.

### Build #66 evidence

* Downloaded artifact ID `14402025444` (7.2 MiB) and inspected the extracted evidence.
* All patches 0001–0008 applied in lexical order; `git diff --check` completed in the VM.
* `nightly-exit-code=0`; `final-build-exit-code=0`.
* Compile evidence: rotfb_boot.o, rotfb_runtime.o, gfxp_bitmap.o and gfxp_fb.o compiled for both i86pc and i86xpv.
* Link evidence: the log contains the i86xpv and i86pc unix link commands with rotfb_boot.o, and the i86xpv and i86pc gfx_private link commands with rotfb_runtime.o, gfxp_bitmap.o and gfxp_fb.o.
* Build #66 remains a CI compile/link checkpoint only; no hardware validation.

### Audit findings by stage

* A — Geometry/memory: fixed ROTFB geometry and mapping remain 1200x1920 / 4800-byte physical pitch and 1920x1200 / 7680-byte logical pitch. The clockwise mapping is unchanged. Existing ROTFB geometry checks use 64-bit size/offset arithmetic and subtraction-based clipping. No target-geometry overflow defect found.
* B — Boot lifecycle: inspected xbi_fb_init -> boot_fb_init -> boot_fb_shadow_init. Patch 0005 explicitly calls boot_fb_init(0) after successful ROTFB shadow allocation, causing import and terminal/cursor geometry recalculation for the logical surface. Boot cursor, blit, erase and scroll flush paths are present. No missing initial boot reinitialization found.
* C — Runtime: inspected init/fini, bitmap_devinit, VIS_DEVFINI and detach. bitmap_devinit's fb-to-shadow copy is a self-memmove in ROTFB runtime (same pointer), which is redundant but safe. Runtime init performs initial physical flush; fini flushes before unmapping and copies the final logical image back to the persistent boot shadow. Callback gate closes and drains active polled callbacks. Potential DDI framework serialization between VIS_DEVFINI and DDI_DETACH remains unproven from this code-only audit; no speculative locking change made.
* D — DDI/mapping: gfxp_map_kernel_space page-aligns the physical base, maps rounded pages with HAT_MERGING_OK | HAT_PLAT_NOCACHE for write-combined access, and gfxp_unmap_kernel_space reverses the offset/page calculation. ROTFB devmap is rejected because the rotated logical view is not physically contiguous. No mapping-size mismatch found for the fixed target geometry.
* E — Polled/cursor: polled callbacks enter/exit the ROTFB callback gate; runtime flushes are issued by the callback paths. The x86 membar_producer implementation is sfence, so the runtime WC write flush has an ordering fence. Boot cursor flush is present in patch 0005.
* F — Build integration: Build #66 logs show per-architecture compile commands and separate i86xpv/i86pc link paths, not merely object-file existence.

### Confirmed defect and repair

* ID: ROTFB-AUD-001
* Severity: Medium (shared gfxp_bitmap framebuffer callback arithmetic; the fixed MiniBook ROTFB dimensions do not reach the overflow threshold, but the affected framebuffer interface permits 64-bit fb_size and uint32_t pitch).
* Location: patched `usr/src/uts/i86pc/io/gfx_private/gfxp_bitmap.c`, bitmap_cons_display(), bitmap_cons_clear(), bitmap_display_cursor().
* Scenario: for a valid large-pitch framebuffer, row * pitch or row * source-line-size was evaluated in 32-bit arithmetic; bitmap_cons_clear also stored uint32_t pitch in signed int. This can wrap row addresses or reject a valid pitch above INT_MAX.
* Repair: added lexical patch `0009-gfxp-bitmap-wide-row-offsets.patch`; row/data offsets are computed as uint64_t and pitch/loop variables retain uint32_t width.
* Test: added a wide-row-offset model case proving 0xffff * 0x20000 = 8,589,803,520 (> UINT32_MAX), while the fixed ROTFB final row remains exactly within the 9,216,000-byte logical allocation.
* Local validation: exact Build #66 pre-0007 gfxp_bitmap.c snapshot was used; current 0007 was applied, then 0009 `git apply --check` and application passed; 0009's 11 hunk counts passed; `git diff --check` passed; isolated wide-offset model passed. Full local upstream checkout remains blocked by container DNS; CI is the full clean-upstream validator.

### Diagnostic matcher audit

* The Build #66 diagnostic index is non-empty because the broad runner scan matches the substring `panic` in object names such as `panic.o`; this is noise, not a build failure.
* The compiler/linker matcher does not explicitly match the illumos linker form `ld: fatal: ...` unless a later line also contains `undefined symbol`. This is a diagnostic coverage gap, not a build-status mask: nightly's exit code and compile/link evidence still determine the job result.
* Controlled samples: standalone `error:` and `fatal error:` are matched; `directoryError:2` is excluded; `ld: fatal: symbol referencing errors` is not matched by the current narrow expression. Record as DIAG-001 for a separate workflow-only repair after Build #67 completes. No workflow change was mixed into the row-offset source fix.

### Commit and scheduling

* Source commit: `f97f44f2e61558cc1866155bc8a04d03356215b2` — `fix(rotfb): widen bitmap row offset arithmetic`.
* Changed files: `illumo_rotfb/patches/0009-gfxp-bitmap-wide-row-offsets.patch`, `illumo_rotfb/tests/rotfb_model_test.py`.
* Before moving the branch ref, rechecked all 66 target runs: zero queued/active. Exactly one push-triggered Build #67 then appeared for this HEAD.
* Current target inventory: 67 runs; Build #67 is IN_PROGRESS. No manual dispatch or second build.
* Root log is outside the workflow path filters; this journal-only update does not start another target build.
* No upstream change, no unrelated subsystem change, no hardware validation.


## ROTFB AUTONOMOUS ENGINEERING REPORT

* Timestamp UTC: 2026-11-18T06:17:30.100Z
* Branch: `gop-rotation-hook`
* Branch HEAD before journal append: `92d972a7caf84e10f5189653775410da719fdf64`; compare against the Build #67 source commit confirms the only intervening change is this root-level engineering journal.
* Target workflow inventory: 67 runs for `.github/workflows/build-illumos.yml` on `gop-rotation-hook`: 3 successful, 63 failed, and 1 in progress; no queued run.
* Latest completed target run remains Build #66 / Run ID `44236793138` — SUCCESS. Its full job log and artifact were inspected in the preceding report; this cycle rechecked the completed job and artifact metadata. No newly completed target run is available.
* Current target run: Build #67 / Run ID `44248223071`, source commit `f97f44f2e61558cc1866155bc8a04d03356215b2`, **IN_PROGRESS**. Job ID `134612250193`. The geometry/clipping model-test step completed successfully; the OpenIndiana VM build step remains in progress. Finalization and artifact upload have not started.
* No manual dispatch, rerun, cancellation, or other build-state operation was issued. No source or workflow change was made while Build #67 is active.

### Patch, test, and workflow audit

* Retrieved and inspected the nine ROTFB patches in lexical application order, 0001–0009. A local unified-diff parser confirmed all 77 hunks have matching declared/actual old and new line counts; all nine patches end with a newline. No malformed hunk was found.
* Patches 0001–0008 are unchanged from the previously audited Build #66 source. Patch 0009 is the new wide-row-offset repair. Its 11 hunks are internally consistent and change only `gfxp_bitmap.c` row/data offset arithmetic and the related unsigned loop/pitch handling.
* Reviewed `rotfb_model_test.py`: it covers clockwise corner mapping, bijection, last-pixel bounds, clipping/overflow inputs, 64-bit wide-row multiplication, and patch hunk/contract assertions. The exact workflow test step passed in Build #67. Independent local mathematical checks of the same geometry, clipping, and wide-offset cases also passed.
* Reviewed `rotfb.h`, boot/runtime ROTFB implementation markers, and `.github/workflows/build-illumos.yml`. The shadow-framebuffer design and fixed 1200x1920 physical / 1920x1200 logical geometry remain unchanged.
* Workflow invariants remain intact: upstream commit `098e95820891f8a83fae679daece7fcf58b7b3a4`, push filters limited to `illumo_rotfb/**` and the workflow itself, concurrency group `rotfb-single-build`, and `cancel-in-progress: false`. The root journal remains outside the trigger paths.
* Local full-upstream patch application and `git diff --check` could not be run: `git ls-remote` failed because this execution environment cannot resolve `github.com`. The CI VM is currently performing the authoritative clean-upstream apply/build validation. No claim of local full-tree patch application is made.

### Deferred diagnostic item

* DIAG-001 remains confirmed but non-blocking: the current workflow compiler-error expression catches standalone `error:` / `fatal error:` forms but does not explicitly catch illumos linker diagnostics such as `ld: fatal: ...`. This is diagnostic-index coverage, not evidence of a build failure or a masked build result. Defer the minimal workflow-only correction until Build #67 completes so no concurrent target run is introduced.

* GitHub Issues remain disabled (HTTP 410); no issue creation was attempted and no repository settings were changed.
* Hardware validation: NOT PERFORMED. No physical Chuwi result is claimed.

## ROTFB POST-BUILD #67 ENGINEERING AUDIT

* Timestamp UTC: 2026-11-18T07:08:00Z
* Branch: `gop-rotation-hook`
* Branch HEAD at inspection: `a4349bb7da0627965339b3dce9501f97211e4655` (journal-only commit after the tested source commit).
* Target workflow inventory: 67 runs for `.github/workflows/build-illumos.yml`; 4 successful, 63 failed, 0 queued, 0 in progress. The repository-wide Actions endpoint returned 96 runs across all workflows; filtering by the exact target workflow path produced the 67-run inventory above.
* Build #67 / Run ID `44248223071` / tested commit `f97f44f2e61558cc1866155bc8a04d03356215b2`: **SUCCESS**. Job ID `134612250193`; attempt 1; completed at 2026-11-18T06:46:21Z.
  * Full decoded job log retrieved and inspected: 15,765 lines / 3,962,643 bytes.
  * Artifact ID `14405031365`, 7,519,616 compressed bytes, SHA-256 `d218f372fade43f510e46f108cee9026757ebe7b26f3f6fa3df861829bdcaeb2`; downloaded and extracted. The archive contains 36 files / 363,460,752 uncompressed bytes.
  * Artifact `build-metadata.txt` confirms pinned upstream `098e95820891f8a83fae679daece7fcf58b7b3a4`.
  * `patch-apply.log` confirms all nine patches, 0001–0009, passed `git apply --check` and were applied in lexical order. VM `git diff --check` completed successfully.
  * ROTFB geometry/clipping model test step: PASS.
  * `nightly-exit-code.txt=0`; `final-build-exit-code.txt=0`. The complete gate log ends with “Nightly distributed build completed” at 2026-11-18 06:44:45 UTC (38:12 build time).
  * Compile evidence: 3 compile-command records each for `rotfb_boot.o`, `rotfb_runtime.o`, `gfxp_bitmap.o`, and `gfxp_fb.o`, for both i86pc and i86xpv.
  * Link evidence: 2 `unix.o` link commands containing `rotfb_boot.o`; 2 `gfx_private` link commands each containing `rotfb_runtime.o`, `gfxp_bitmap.o`, and `gfxp_fb.o`. The workflow's explicit compile/link evidence gate reports PASS.
  * The compiler-error index contains no compiler/linker diagnostics. A boundary-aware scan of the full 176,301,356-byte gate log found no fatal compiler error, undefined symbol, syntax error, dmake failure, or linker fatal diagnostic.
  * The patched upstream diffstat contains only the six expected ROTFB integration targets: i86pc/i86xpv Makefile.files, boot_fb.c, gfxp_bitmap.c, gfxp_fb.c, and framebuffer.h.
* Patch audit at current branch HEAD: inspected all nine patch files in lexical order. The local unified-diff parser found 77 hunks with matching declared/actual old and new line counts; all nine files have final newlines. No malformed hunk was found.
* Test/source audit: reviewed `rotfb_model_test.py`, `rotfb.h`, `rotfb_boot.c`, and `rotfb_runtime.c`. Build #67's model test passed; the 1200x1920 physical / 1920x1200 logical clockwise shadow-framebuffer architecture remains unchanged. No source or patch defect requiring speculative modification was found in this cycle.
* Confirmed non-blocking diagnostic defect DIAG-001: all three workflow diagnostic grep expressions still omit the illumos linker form `ld: fatal:`. This is a coverage gap in diagnostic indexes, not a failure hidden by the build gate; Build #67 itself completed and its full log contains no linker fatal. A local controlled grep test of the proposed additional alternative `ld:[[:space:]]+fatal:` passes for bare and timestamp-prefixed linker errors, continues to match ordinary compiler errors, and does not reintroduce the `directoryError:2` false positive. The three expressions are the only planned workflow change; no source logic or patch is involved.
* Validation limitations: the full clean-upstream patch/build validation is authoritative in Build #67's OpenIndiana VM. A separate full local upstream clone/apply remains unavailable because this execution environment cannot resolve outbound GitHub DNS. The local controlled regex and shell-syntax checks passed.
* Scheduling: the latest target-run recheck found all 67 target runs completed and zero queued/in progress. No rerun, cancellation, manual dispatch, or build-state operation was issued. The root journal is outside workflow path filters.
* GitHub Issues remain disabled (HTTP 410); no issue creation was attempted and no repository settings were changed.
* Hardware validation: NOT PERFORMED. No Chuwi hardware result is claimed.
* Next action: apply the minimal DIAG-001 workflow-only regex extension after one final target-run recheck. The workflow path filter will create at most one validation run; after it appears, stop and wait for the next cycle.

## ROTFB DIAG-001 WORKFLOW REPAIR

* Timestamp UTC: 2026-11-18T07:11:00Z
* Branch: `gop-rotation-hook`
* Branch HEAD at workflow commit: `7126f359baa378e9ef76049c081bb806fac19f20`.
* Confirmed issue: the three workflow diagnostic indexes did not match illumos linker diagnostics beginning with `ld: fatal:`. This left a known diagnostic-coverage gap; it did not affect the build exit code or compile/link evidence gate.
* Minimal repair: added the ERE alternative `ld:[[:space:]]+fatal:` to all three existing diagnostic grep expressions at workflow lines 371, 375, and 405. No source, patch, upstream pin, path filter, concurrency, permissions, or other workflow behavior changed.
* Commit: `7126f359baa378e9ef76049c081bb806fac19f20` (`ci(rotfb): capture illumos linker fatal diagnostics`). GitHub commit inspection confirms the only changed path is `.github/workflows/build-illumos.yml`.
* Local validation: controlled samples confirm bare and timestamp-prefixed `ld: fatal:` messages are matched, ordinary compiler `error:` remains matched, and `directoryError:2` remains excluded. Representative shell block passes `bash -n`; `git diff --check` passes on the exact three-line replacement fixture. The replacement was also checked against the fetched workflow text: exactly three lines changed and the final newline was preserved.
* Build scheduling: immediately before the workflow update, all 67 target runs were completed and none were queued or active. The workflow-only commit triggered exactly one automatic target run: Build #68 / Run ID `44256656761` / HEAD `7126f359baa378e9ef76049c081bb806fac19f20`, currently **IN_PROGRESS**. No manual dispatch, rerun, cancellation, or second build was issued.
* Current target inventory: 68 target runs; Build #68 is the sole active target run. Stop source/workflow changes until it completes. The following journal append is root-level and excluded from the workflow path filters.
* Hardware validation: NOT PERFORMED. No Chuwi hardware result is claimed.


## ROTFB HOURLY ENGINEERING REPORT

* Timestamp UTC: 2026-11-18T08:11:02Z
* Branch: `gop-rotation-hook`
* Branch HEAD before journal append: `10883fe52db353a1fdf81ce4ddbe505267894eaf`.
* Latest commits inspected:
  * `10883fe52db353a1fdf81ce4ddbe505267894eaf` — journal-only report for Build #68.
  * `7126f359baa378e9ef76049c081bb806fac19f20` — DIAG-001 workflow diagnostic update.
  * `746aa415cd13b7ad68e96204e45343e59e1568da` — Build #67 success audit.
* Target Actions inventory: 140 repository runs were returned across two pages. Filtering by the exact target path `.github/workflows/build-illumos.yml` yielded 68 ROTFB runs (all on the target workflow path): 4 success, 63 failure, 1 in progress, 0 queued. Page 2 contained no additional target runs. Other workflow paths were excluded.
* Current target run: Build #68 / Run ID `44256656761` / tested commit `7126f359baa378e9ef76049c081bb806fac19f20` — **IN_PROGRESS**. Job ID `134639857702`. The ROTFB geometry/clipping model-test step is **SUCCESS**; the OpenIndiana VM build step remains in progress. No artifacts are available yet.
* No manual dispatch, rerun, cancellation, or other build-state operation was issued. Build #68 remains the sole active target run. No source or workflow change was made while it is active.

### Latest completed build recheck: #67

* Build #67 / Run ID `44248223071` / tested commit `f97f44f2e61558cc1866155bc8a04d03356215b2` — **SUCCESS**; job ID `134612250193`.
* Retrieved the full decoded job log again: 3,962,643 bytes / 15,765 lines.
* Retrieved artifact ID `14405031365` again: 7,519,616-byte ZIP; SHA-256 `d218f372fade43f510e46f108cee9026757ebe7b26f3f6fa3df861829bdcaeb2`. The archive lists 36 files. Inspected build metadata, patch application log, patched diffstat/status, build evidence, exit-code files, compile/link command counts, and both diagnostic indexes.
* Artifact metadata confirms upstream pin `098e95820891f8a83fae679daece7fcf58b7b3a4`.
* `patch-apply.log` confirms `git apply --check` and application succeeded in lexical order for all nine patches, 0001–0009. The OpenIndiana VM's `git diff --check` passed.
* ROTFB model tests passed. `nightly-exit-code.txt=0`; `final-build-exit-code.txt=0`; `ROTFB/framebuffer compile-link evidence: PASS`.
* Compile evidence records 3 command entries per architecture for each of `rotfb_boot.o`, `rotfb_runtime.o`, `gfxp_bitmap.o`, and `gfxp_fb.o`. Link evidence records two `unix.o` link commands and two `gfx_private` link commands for the ROTFB runtime/bitmap/framebuffer objects.
* `compiler-error-index.txt` contains only its section headers; no compiler diagnostic was indexed. Build #67 has no build failure/root cause because it completed successfully.

### Patch and test audit

* Re-fetched and inspected all nine patches in lexical order. Current raw patch files contain **71** unified-diff hunk headers in total: 0001=7, 0002=8, 0003=1, 0004=4, 0005=12, 0006=10, 0007=17, 0008=1, 0009=11. Every hunk's declared old/new line counts match the actual body counts; all nine files end with a newline.
* Correction to earlier journal entries: the previously recorded total of 77 hunks was an overcount. The verified current total is 71. No patch contents were changed in this audit.
* Reviewed `rotfb_model_test.py`, `rotfb.h`, `rotfb_boot.c`, and `rotfb_runtime.c`. The test covers clockwise corner mapping, bijection, final-pixel bounds, clipping/overflow, wide row offsets, and patch contracts.
* Independent local Python checks passed for the complete 2,304,000-pixel mapping bijection, corner/last-pixel offsets, clipping boundaries, and the 64-bit row-offset case `8,589,803,520 > UINT32_MAX`. Build #68's own model-test step also reports success.

### Workflow diagnostics

* Re-inspected `.github/workflows/build-illumos.yml`. Branch/path filters remain limited to `gop-rotation-hook`, `illumo_rotfb/**`, and this workflow file. Concurrency remains `rotfb-single-build` with `cancel-in-progress: false`. The root journal remains outside the path filters.
* Commit `7126f359baa378e9ef76049c081bb806fac19f20` was inspected through the GitHub commit API: exactly one changed file, the target workflow, with three insertions and three deletions. It adds `ld:[[:space:]]+fatal:` to the three diagnostic expressions (DIAG-001).
* Local controlled grep samples passed for bare and timestamp-prefixed `ld: fatal:`, ordinary compiler `error:`, and exclusion of `directoryError:2`. Representative shell syntax and `git diff --check` fixture checks passed.
* Newly confirmed DIAG-002 from the Build #67 artifact: `runner-error-index.txt` is 2,203,704 bytes because the broad case-insensitive alternative `failed` matches ordinary identifiers and filenames such as `__failed_count.o` and `DES_FAILED.3ext` in the full build logs. This creates substantial false-positive noise in the runner diagnostic index; it is not a build failure. A local controlled test shows the current matcher hits all five samples (including the two false positives), while a word-boundary alternative retains the three real failure samples and excludes both identifiers.
* DIAG-002 is **deferred** until Build #68 completes. Do not modify the workflow while the target run is active. If confirmed against Build #68's artifact as well, make only the minimal diagnostic-regex correction after a fresh no-active-run check.

### Validation limits and project safety

* A separate local upstream checkout/patch application remains blocked: `git ls-remote https://github.com/illumos/illumos-gate.git 098e95820891f8a83fae679daece7fcf58b7b3a4` failed with `Could not resolve host: github.com`. Therefore no local full-upstream apply/build is claimed; the successful clean-upstream VM run is the available authoritative patch/build validation.
* GitHub Issues remain disabled (HTTP 410); no issue creation was attempted and no repository settings were changed.
* Shadow framebuffer architecture and pinned upstream are unchanged.
* Hardware validation: NOT PERFORMED. No physical Chuwi result is claimed.
* Next action: wait for Build #68; when complete, retrieve its full job log and artifact, inspect the first failure if any, and verify DIAG-001/DIAG-002 behavior. No concurrent build is to be dispatched.


## ROTFB POST-BUILD #68 AUDIT

* Timestamp UTC: 2026-11-18T08:34:07.792Z
* Branch: `gop-rotation-hook`
* Branch HEAD before journal append: `d4e1c677b0c797c1f32d543add05759a6337feb2` (root-level journal-only commit).
* Target workflow inventory: 68 runs for `.github/workflows/build-illumos.yml`, all on `gop-rotation-hook`: 5 successful, 63 failed, 0 queued, 0 in progress. The repository-wide Actions API returned 97 runs across all workflows; unrelated workflow paths were excluded.
* Build #68 / Run ID `44256656761` / tested commit `7126f359baa378e9ef76049c081bb806fac19f20`: **SUCCESS**. Job ID `134639857702`; completed at `2026-11-18T08:31:21Z`.
  * Full decoded job log retrieved: 4,948,226 bytes / 17,135 lines.
  * Artifact ID `14410871820` retrieved and downloaded: 7,612,780-byte ZIP; SHA-256 `87c68e72159c035c73271730c3a06a447e5d1a555f4f7bdbcd87c994ad9e77d0`.
  * Artifact contains 36 files / approximately 364 MB uncompressed. `build-metadata.txt` confirms pinned upstream `098e95820891f8a83fae679daece7fcf58b7b3a4`.
  * `patch-apply.log` confirms all nine patches, 0001–0009, passed `git apply --check` and applied in lexical order. OpenIndiana VM `git diff --check` passed.
  * ROTFB geometry/clipping model-test step: PASS. `nightly-exit-code.txt=0`; `final-build-exit-code.txt=0`.
  * Compile evidence: 3 command records for each of `rotfb_boot.o`, `rotfb_runtime.o`, `gfxp_bitmap.o`, and `gfxp_fb.o`, for both i86pc and i86xpv. Link evidence: 2 `unix.o` link commands and 2 `gfx_private` link commands for each ROTFB/runtime/framebuffer object. Workflow evidence gate: PASS.
  * No compiler or linker failure was found in the completed build. Build #68 has no build root cause; it completed successfully.
* No manual dispatch, rerun, cancellation, or other build-state operation was issued. All target runs are complete at this observation; no source/workflow modification had yet been made.

### DIAG-002 confirmed against Build #68

* Build #68's `runner-error-index.txt` is 3,141,306 bytes / 4,809 lines even though the build succeeded. The line-405 runner-side expression still contains unbounded alternatives `cannot|failed|killed|panic`.
* The artifact contains false positives from ordinary source/build names: `__failed_count.o` (66 occurrences), `DES_FAILED.3ext` (4), `tst.exitkilled.ksh` (8), `libvarpd_panic.o` (66), and `/os/panic.c` (18). Aggregate token counts in the index: `failed` 88, `cannot` 2, `killed` 8, and `panic` 768.
* Reproduced the false-positive behavior against the downloaded Build #68 artifact. A candidate one-line expression with POSIX token boundaries for `cannot|failed|killed` and a specific panic marker `panic(:|\\[)` reduced a full simulated scan of the same artifact from 3,141,306 bytes to 5,585 bytes / 36 lines. It retains representative true-positive samples: compiler `error:`, `ld: fatal:`, `cannot open`, standalone `failed`, standalone `killed`, `panic:`, and `panic[cpu0]`; it excludes the filename/path samples above. Two low-volume `cannot set property` comment matches remain; they are harmless and do not recreate the multi-megabyte index.
* The exact candidate shell assignment passes `bash -n`; a controlled sample fixture reports 6 expected matches and 0 of the known false-positive filename matches. The Build #68 artifact provides a second independent confirmation of DIAG-002. This is diagnostic-index noise, not a build failure.
* Minimal repair scope is one expression at workflow line 405 only. No source, patch, upstream pin, path filter, permissions, or concurrency change is required.

### Patch, test, and workflow audit

* Re-fetched and inspected all nine patches in lexical order. Current hunk counts: 0001=7, 0002=8, 0003=1, 0004=4, 0005=12, 0006=10, 0007=17, 0008=1, 0009=11; **71 total**. A local unified-diff parser confirmed all declared old/new hunk counts match the actual bodies and all nine files end with a newline.
* Re-inspected `rotfb_model_test.py`, `rotfb.h`, `rotfb_boot.c`, and `rotfb_runtime.c`. The clockwise 1200x1920 physical / 1920x1200 logical shadow-framebuffer design and boot/runtime lifecycle remain unchanged. No additional confirmed source defect was found.
* Workflow path filters remain restricted to `gop-rotation-hook`, `illumo_rotfb/**`, and `.github/workflows/build-illumos.yml`; concurrency remains `rotfb-single-build` with `cancel-in-progress: false`. The root journal is outside the workflow path filters.

### Next action and limitations

* All 68 target runs were complete at the pre-journal check. The next operation may be the single minimal DIAG-002 workflow update, but only after another fresh branch/run check confirms no queued or active target build. The workflow update itself should trigger at most one automatic validation run; do not manually dispatch a second build. After the new run appears, stop and let the next cycle inspect it.
* Local patch structure and diagnostic-regex checks passed. A separate full local upstream checkout/apply remains blocked: `git ls-remote https://github.com/illumos/illumos-gate.git 098e95820891f8a83fae679daece7fcf58b7b3a4` failed with `Could not resolve host: github.com`. No local full-upstream build is claimed; Build #68's clean OpenIndiana VM run is the authoritative apply/compile/link evidence.
* GitHub Issues remain disabled (HTTP 410); no issue creation was attempted and no repository settings were changed.
* Hardware validation: **NOT PERFORMED**. No physical Chuwi result is claimed.

## ROTFB POST-BUILD #69 AUDIT

* Timestamp UTC: 2026-11-18T10:23:51.212Z
* Branch: `gop-rotation-hook`
* Branch HEAD before journal append: `e609963f2a92733edb2cfbfca210c62a51017f10`.
* Latest branch commits inspected: `e609963f2a92733edb2cfbfca210c62a51017f10` (DIAG-002 runner diagnostic filter repair), `544e0714b5fb0a9aa75e916b9b8551bd6f6fd682` (Build #68 audit journal), and `d4e1c677b0c797c1f32d543add05759a6337feb2` (Build #68 in-progress journal).
* Target Actions inventory: repository-wide branch query returned 98 runs; filtering the exact workflow path `.github/workflows/build-illumos.yml` yielded 69 ROTFB runs. Results: 6 success, 63 failure, 0 cancelled, 0 queued/in progress. No target run is queued or active.
* Build #69 / Run ID `44268167584` / tested commit `e609963f2a92733edb2cfbfca210c62a51017f10`: **SUCCESS**. Job ID `134677782252`; completed at `2026-11-18T09:22:51Z`. All nine job steps completed successfully, including geometry/clipping tests, OpenIndiana VM build, runner diagnostic finalization, and artifact upload.
  * Full decoded job log retrieved: 1,666,178 characters / 12,261 lines. The job log contains no failed step or build failure. Both full gate-log copies were scanned for compiler/linker failure forms (`error:`, `ld: fatal:`, undefined symbol, syntax error, dmake failure, and missing make target); zero matches.
  * Artifact ID `14413194199`, 7,371,055 compressed bytes; SHA-256 `dc372190dc67c63c7772018a7031af2920d8ec62f5ba0ecd0b6bae9ccaccf85c`. Downloaded and extracted; 36 files / 361,478,298 uncompressed bytes.
  * `build-metadata.txt` confirms pinned upstream `098e95820891f8a83fae679daece7fcf58b7b3a4`.
  * `patch-apply.log` confirms all nine patches (0001–0009) passed `git apply --check` and were applied in lexical order. The VM build script executes `git diff --check`; the job completed successfully.
  * ROTFB model-test step: PASS. `nightly-exit-code.txt=0`; `final-build-exit-code.txt=0`.
  * Compile evidence: 3 compile-command records for each of `rotfb_boot.o`, `rotfb_runtime.o`, `gfxp_bitmap.o`, and `gfxp_fb.o`, for both i86pc and i86xpv. Link evidence: 2 `unix.o` boot-object link commands and 2 `gfx_private` link commands for each runtime/bitmap/framebuffer object. The workflow evidence gate reports PASS.
  * Build #69 has no build root cause because it completed successfully; no failed compile, link, patch application, or model-test step was found.
* DIAG-002 verification: `runner-error-index.txt` is 5,761 bytes / 37 lines, down from 3,141,306 bytes / 4,809 lines in Build #68 (99.8166% smaller). The five previously confirmed false-positive filename/path patterns (`__failed_count.o`, `DES_FAILED.3ext`, `tst.exitkilled.ksh`, `libvarpd_panic.o`, and `/os/panic.c`) are absent. The only remaining matches are two copies of the benign upstream comment “we cannot set property”; no build failure is indicated. `compiler-error-index.txt` contains only its section headings and no diagnostics. The one-line boundary-aware filter repair is therefore validated by the real Build #69 artifact.
* Patch audit at current branch HEAD: fetched all nine patches in lexical application order. A local unified-diff parser verified 71 total hunks with declared old/new counts matching actual bodies; all nine patch files end with a newline. The earlier correction from 77 to 71 hunks remains accurate.
* Test/source audit: fetched `rotfb_model_test.py`, `rotfb.h`, `rotfb_boot.c`, and `rotfb_runtime.c`. The model test covers clockwise corner mapping, full mapping bijection, last-pixel bounds, clipping/overflow inputs, wide row-offset arithmetic, and patch contracts. Independent local Python model checks passed for all 2,304,000 pixels, 9,216,000-byte framebuffer geometry, clipping boundaries, and the 64-bit offset `8,589,803,520 > UINT32_MAX`.
* Workflow audit: the branch/path filters remain limited to `gop-rotation-hook`, `illumo_rotfb/**`, and `.github/workflows/build-illumos.yml`; concurrency remains `rotfb-single-build` with `cancel-in-progress: false`; the upstream pin is unchanged. The root journal remains outside the workflow path filters.
* No source, patch, or workflow changes were required in this cycle. No build was dispatched, rerun, cancelled, or otherwise modified. The journal-only update does not match workflow path filters.
* Local full-upstream checkout/patch application remains unavailable because the execution environment previously failed outbound GitHub DNS resolution. No separate local illumos build is claimed; the successful OpenIndiana VM build is the authoritative clean-upstream patch/compile/link evidence.
* GitHub Issues remain disabled (HTTP 410); no issue creation was attempted and no repository settings were changed.
* Hardware validation: **NOT PERFORMED**. No physical Chuwi result is claimed.

## ROTFB BOOT IMPORT CONTRACT FIX / BUILD #70 STATUS

* Timestamp UTC: 2026-11-18T10:44:31.801Z
* Branch: `gop-rotation-hook`
* Branch HEAD before this journal append: `d0d53a475f63e10239a2046d8f0fb967a10dc63f`.
* Previous HEAD: `efbff3414f0197cc504a80033a7b96e57bb09a06`.
* Target workflow inventory: 70 runs for `.github/workflows/build-illumos.yml` on `gop-rotation-hook`: 6 success, 63 failure, 0 cancelled, 0 queued, 1 in progress. The only active target run is Build #70.
* Build #70 / Run ID `44288142299` / tested commit `d0d53a475f63e10239a2046d8f0fb967a10dc63f`: **IN_PROGRESS** (created `2026-11-18T10:43:53Z`). Job ID `134746050977`. The geometry/clipping model-test step completed **SUCCESS**; the OpenIndiana VM step is still running. No logs or artifact are available for this active job yet.
* Confirmed boot-path root cause:
  * `rotfb_boot_import()` explicitly rejects the call unless `fb->fb == rotfb_info.logical_fb`.
  * The `boot_fb_init()` path first resets `fb_info.fb` to the physical GOP address, calls `rotfb_boot_init()`, and then calls `rotfb_boot_import()`; the old `rotfb_boot_init()` populated only `fb->physical.fb` and left `fb->fb` pointing at the physical address.
  * The assignment to `rotfb_info.logical_fb` in the caller occurred only after `rotfb_boot_import()`. Therefore the import precondition was unsatisfied and the failure branch in patch 0006 disabled ROTFB and restored the physical geometry. This was a boot-path logic defect not exercised by the prior compile/link-only evidence.
* Minimal repair committed as `d0d53a475f63e10239a2046d8f0fb967a10dc63f`:
  * `illumo_rotfb/rotfb_boot.c`: `rotfb_boot_init()` now assigns `fb->fb = rotfb_info.logical_fb` after validating/publishing the physical mapping and before setting `boot_ready`. This makes the state satisfy the existing import contract; the physical mapping remains separately recorded in `fb->physical.fb`.
  * `illumo_rotfb/tests/rotfb_model_test.py`: added a regression assertion that the logical framebuffer is installed by `rotfb_boot_init()` before `boot_ready`, and that `rotfb_boot_import()` enforces the matching pointer precondition.
  * GitHub compare confirms exactly two changed files, 14 insertions, 0 deletions. No patch files, workflow, upstream pin, unrelated subsystem, or settings were changed.
* Pre-commit validation:
  * Confirmed the branch had no queued or active target run immediately before the update; the branch ref was advanced with expected-HEAD protection.
  * Local Python syntax check for the added regression assertion: PASS.
  * Independent model checks: PASS for all 2,304,000 logical pixels (bijective clockwise mapping), four corners, clipping edges, and 64-bit row offset `8,589,803,520 > UINT32_MAX`.
  * Re-audited all nine patches in lexical order: 71 hunks total, 0 declared-count mismatches, all patch files end with newline. Existing patch contents were not changed.
  * Local source/test whitespace and final-newline checks: PASS. A separate local upstream apply/build is still unavailable because this execution environment cannot resolve `github.com`; Build #70's OpenIndiana VM is the authoritative next patch/build validation.
* Workflow invariants rechecked: push branch/path filters still target `gop-rotation-hook` and `illumo_rotfb/**`; concurrency remains `rotfb-single-build` with `cancel-in-progress: false`; pinned upstream remains `098e95820891f8a83fae679daece7fcf58b7b3a4`. The journal remains outside the target path filters.
* No manual dispatch, rerun, cancellation, or other build-state operation was issued. The single Build #70 was automatically created by the real source/test commit. No second build will be started while it is active.
* GitHub Issues remain disabled (HTTP 410); no issue creation was attempted and no repository settings were changed.
* Hardware validation: **NOT PERFORMED**. No physical Chuwi result is claimed.
* Next action: wait for Build #70 to complete, then retrieve its full job log and artifact; verify patch application, the VM `git diff --check`, compile/link evidence for i86pc and i86xpv, and the new boot-import regression test. Do not modify source/workflow or start another target build while #70 is queued or active.


## ROTFB POST-BUILD #70 AUDIT / DIAG-003

* Timestamp UTC: 2026-11-18T12:11:55Z.
* Branch: `gop-rotation-hook`.
* Branch HEAD before this journal append: `612516f4a1a1b5f964aaec22290f2b2df351a2d8` (journal-only commit; the tested source commit remains `d0d53a475f63e10239a2046d8f0fb967a10dc63f`).
* Target workflow inventory: repository branch query returned 99 runs across workflows (99 on this page); filtering the exact path `.github/workflows/build-illumos.yml` and branch `gop-rotation-hook` yielded 70 target runs: 7 success, 63 failure, 0 queued, 0 in progress. No target run is active.
* Build #70 / Run ID `44288142299` / tested source commit `d0d53a475f63e10239a2046d8f0fb967a10dc63f`: **SUCCESS**. Job ID `134746050977`; completed at `2026-11-18T11:30:57Z`. Run URL: https://github.com/Cadiocto/WhateverXe-MiniBook-X-N150/actions/runs/44288142299.
* Full job log retrieved: 1,661,627 characters / 12,208 lines. Artifact `illumos-framebuffer-build-logs-44288142299` downloaded and inspected: 7,077,052 bytes, 35 files, SHA-256 `4ce583a2c4dff92166a63a3222dc3de0a91b8f14f1b8d3b4d06a35bd80ec6d00`.
* Build evidence:
  * ROTFB model geometry/clipping test step: SUCCESS.
  * `patch-apply.log` confirms patches 0001–0009 each passed `git apply --check` and were applied in lexical order.
  * VM ran `git diff --check` before build; the VM job completed successfully.
  * Nightly exit code: `0`; final build exit code: `0`; nightly distributed build completed at `2026-11-18T11:29:29Z`.
  * Compile command counts: 3 each for `rotfb_boot`, `rotfb_runtime`, `gfxp_bitmap`, and `gfxp_fb` on both i86pc and i86xpv. Link command counts: 2 each for boot `unix.o` and the three gfx_private objects. `ROTFB/framebuffer compile-link evidence: PASS`.
  * `compiler-error-index.txt` is empty apart from section headers. Targeted scan of both full nightly logs found no compiler/linker failure signature. Build #70 has no build root cause; it completed successfully.
  * The boot-import regression assertion was exercised by the successful model-test step; this is source/model validation, not a physical boot test.
* Re-audited all nine patch files in lexical order: 71 hunks total, all declared old/new hunk counts match their actual bodies, all nine files end with a newline. No patch content changed.
* Workflow invariants: branch/path filters remain limited to `gop-rotation-hook`, `illumo_rotfb/**`, and `.github/workflows/build-illumos.yml`; concurrency remains `rotfb-single-build` with `cancel-in-progress: false`; upstream checkout remains pinned to `098e95820891f8a83fae679daece7fcf58b7b3a4`. Root journal remains outside target path filters.
* Confirmed DIAG-003 in the completed artifact: the runner diagnostic's generic standalone `cannot` alternative matches the benign illumos source comment `Since we cannot set property values by applying a profile yet` in both nightly log copies. This creates two false-positive source matches in the runner-side scan. A narrower `cannot` verb list was replayed against every text file in the Build #70 artifact: 0 matches on the clean artifact, while 13/13 synthetic failure examples (fatal error, linker fatal, undefined symbol, syntax error, failed target, cannot find/open, OOM, panic, segmentation fault, SSH exit, killed process) remained detectable. This is a diagnostic-index quality defect only; it did not affect build success or build gating.
* No source or patch changes were made in this cycle. The only planned code change is the single runner diagnostic regex in the dedicated workflow, to be committed only after a fresh all-runs check confirms no queued or active target run. No manual dispatch, rerun, cancellation, or issue creation was performed.
* The OpenIndiana VM successfully cloned the pinned upstream and performed patch application plus the full nightly build. The separate local container still cannot resolve `github.com`; no local full-upstream clone/build is claimed.
* GitHub Issues remain disabled (HTTP 410); no issue creation was attempted and no repository settings were changed.
* Hardware validation: **NOT PERFORMED**. No physical Chuwi result is claimed.


### AUDIT CYCLE 2026-11-18T12:48:06.342Z — HEAD f0cebdfd2f2877b83a3b082e507ab73627c5b6b8 — RUN 44302559938

* Scope: read-only source/lifecycle audit on `gop-rotation-hook`. Current branch HEAD and Build #71 source SHA both remain `f0cebdfd2f2877b83a3b082e507ab73627c5b6b8`; no source, patch, or workflow files were changed.
* Target run inventory was refreshed: Build #71 (`44302559938`) is the sole queued/running target workflow run; status `in_progress`, job `134795747200`. The ROTFB model-test step completed successfully; the OpenIndiana VM build step remains active. No artifact is published yet. The job-log download endpoint currently returns BlobNotFound while the job is still running, so no partial log or build result is claimed. No rerun, dispatch, cancellation, or build-state mutation was performed.
* Completed-run review: Build #70 (`44288142299`) remains the latest completed target run and its full log/artifact audit is already recorded above. There is no newer completed target run in this cycle.
* Patch inventory re-read from GitHub in lexical application order: 0001–0009. Hunk counts are respectively 7, 8, 1, 4, 12, 10, 17, 1, 11 (71 total); every hunk's declared old/new line counts match its body and every patch ends with a newline. No patch content was modified.
* Independent host-side geometry replay: clockwise map `physical_x = 1199 - logical_y`, `physical_y = logical_x` maps all 2,304,000 logical pixels bijectively onto the 1200×1920 physical surface. All four corner mappings and the 9,216,000-byte equal-size surface bounds pass.
* Boot source-level sequence review: `rotfb_boot_shadow_init()` allocates and installs the logical shadow and logical geometry; the patched `boot_fb_shadow_init()` calls `boot_fb_init(0)` again; the boot-init path calls `rotfb_boot_init()` and `rotfb_boot_import()` before continuing console setup. `rotfb_boot_init()` assigns `fb->fb = rotfb_info.logical_fb` before publishing `boot_ready`; import validates that pointer and copies the physical image into the logical surface. This is source-level call-path evidence, not an executed firmware boot.
* Runtime source-level sequence review: `rotfb_runtime_init()` maps the physical surface, allocates a distinct runtime shadow, copies the boot shadow, publishes runtime pointers, performs an initial full flush, and only then opens the callback gate. `rotfb_runtime_fini()` closes the gate, drains counted callbacks, flushes while the mapping is valid, copies the runtime image back to the boot shadow, restores saved framebuffer state, then unmaps/frees runtime allocations and leaves the gate closed. Static ordering checks pass.
* Polled callback review: all three polled entry points use callback enter/exit wrappers. The framebuffer ioctl path serializes `VIS_DEVINIT`/`VIS_DEVFINI` under `softc->lock`; `VIS_DEVINIT` publishes `data->polledio` only after `bitmap_setup_fb()` has completed. This supports the normal first-init ordering, but does not replace a concurrent callback/teardown stress test. No confirmed callback UAF or deadlock was established in this source-only pass.
* Integration evidence remains limited to Build #70's successful OpenIndiana VM compile/link for i86pc and i86xpv. DIAG-001 was exercised by Build #68, DIAG-002 by Build #69; DIAG-003's latest regex change is on Build #71's commit and awaits its completed artifact for real-log confirmation.
* Status: no newly confirmed source defect; no code changes recommended from this static pass. Next action is to wait for Build #71, then retrieve its complete job log and artifact, verify DIAG-003 false-positive count and build exit codes, and only then consider further changes.
* Limitations: local container DNS still prevents a separate local upstream clone/build; OpenIndiana VM build evidence is available from completed runs. Physical Chuwi hardware validation has not been performed.


## ROTFB POST-BUILD #71 INDEPENDENT AUDIT — INTERIM, AUDIT-ONLY

* Timestamp UTC: 2026-11-18T12:59:41.967Z
* Branch: `gop-rotation-hook`
* HEAD at report time: `5d014620e42956c1b0fd631cc47919ae3807c4d6` (journal-only commit; source tree remains the Build #71 source at `f0cebdfd2f2877b83a3b082e507ab73627c5b6b8`)
* Pinned upstream: `098e95820891f8a83fae679daece7fcf58b7b3a4`
* Build #71 / Run ID: `44302559938`, still IN_PROGRESS at the time of this entry. Model/clipping step PASS; OpenIndiana VM step still running; no artifact or completed job log available. No new build dispatched.
* Build #70 / Run ID `44288142299`: SUCCESS; full job log and artifact `14423481233` downloaded and inspected. 35 artifact files, 12,208 job-log lines, SHA-256 `4ce583a2c4dff92166a63a3222dc3de0a91b8f14f1b8d3b4d06a35bd80ec6d00`. Patches 0001–0009 applied; VM `git diff --check`; model tests PASS; nightly and final exit codes 0.
* Build #70 compile/link evidence: both i86pc and i86xpv ROTFB boot/runtime objects compile; the `unix.o` commands include `rotfb_boot.o`; the `gfx_private` commands include `rotfb_runtime.o`, `gfxp_bitmap.o`, and `gfxp_fb.o`. Artifact command context distinguishes i86xpv and i86pc builds. This is compile/link evidence, not hardware validation.
* Commit history from Build #66 to current HEAD: only ROTFB source, ROTFB patches, the dedicated `build-illumos.yml`, tests, and the root engineering log changed. No unrelated subsystem changes. Source commits: `f97f44f` (0009 wide row offsets; Build #67 SUCCESS), `7126f35` (DIAG-001 linker fatal capture; Build #68 SUCCESS), `e609963` (DIAG-002 token boundaries; Build #69 SUCCESS), `d0d53a4` (publish logical fb before boot import; Build #70 SUCCESS), `f0cebdf` (DIAG-003 cannot matcher; Build #71 pending). Other commits in this range are journal-only.
* Patch structure audit: nine patches in lexical order, 71 hunks total; declared hunk counts match; every patch ends with newline. Full clean-upstream application is confirmed by Build #70; no local full upstream clone available because this environment cannot resolve GitHub DNS.

### Confirmed runtime alias defects — no code changed

* `ROTFB-LIFE-001`, Severity Medium: patched `gfxp_bitmap.c::bitmap_cons_copy()` in patch 0007 performs both `memmove(dst + increment, src + increment, width)` and `memmove(sdst + increment, src + increment, width)`. ROTFB runtime deliberately aliases `fb->fb == fb->shadow_fb`, so `dst == sdst`. For valid overlapping horizontal VIS_CONSCOPY rectangles, the second memmove repeats the move using already modified source bytes and corrupts the result. A local isolated memmove simulation reproduced corruption for left and right overlap. Standard TEM scrolling currently copies full-width rows vertically, for which the duplicate is normally idempotent; the generic VIS_CONSCOPY rectangle contract remains broken for horizontal overlap. Proposed minimal fix: skip the second copy when destination pointers alias; add left/right overlap tests.
* `ROTFB-LIFE-002`, Severity Medium: patched `gfxp_bitmap.c::bitmap_display_cursor()` in patch 0007 applies the same XOR transform to both framebuffer and shadow pointers for 8/16/24/32 bpp. ROTFB aliases these pointers, so each pixel is transformed twice and restored to its original value. A local isolated XOR simulation reproduced the no-op. This affects runtime VIS_DISPLAY_CURSOR and VIS_HIDE_CURSOR. Proposed minimal fix: perform the second buffer update only when the pointers differ; add alias tests at all supported depths.

### Boot WC ordering gap

* `ROTFB-BOOT-001`, Severity Medium / hardware manifestation unverified: dboot maps the framebuffer using `PT_NOCACHE | PT_PAT_4K`; `PT_PAT_4K` is documented in `mach_mmu.h` as the write-combining PAT bit. `rotfb_boot_flush_rect()` writes to that WC mapping but returns without `membar_producer()`; the runtime flush explicitly calls `membar_producer()`, implemented as `sfence` on amd64. Proposed minimal fix: issue one producer barrier at the end of each successful boot rectangle flush. The code-level ordering gap is confirmed; visible stale/delayed output on target hardware has not been tested.

### Other lifecycle findings

* Boot call path verified against pinned upstream: `_start()` -> `bcons_init()` -> `boot_fb()` -> initial physical `boot_fb_init()`; after boot allocator initialization, `boot_fb_shadow_init()` allocates logical shadow and re-enters `boot_fb_init(0)`. Build #70 change `d0d53a4` sets `fb->fb = rotfb_info.logical_fb` before `boot_ready`; `rotfb_boot_import()` then validates and imports physical pixels into the logical surface. Source-level PASS; no real boot test.
* Runtime init/fini and callback gate are source-level coherent: allocation/mapping occur before publication, initial flush precedes opening the gate, fini closes gate and drains active callbacks before restoring boot pointers and freeing runtime resources. Polled callbacks do not take mutexes; wait occurs in fini process/ioctl/detach context. No dynamic callback stress test.
* `bitmap_devinit()` retains a redundant exact self-`memmove` through `bitmap_copy_fb()` because fb and shadow alias. Exact same-pointer memmove is safe/no-op; this is not the overlapping duplicate-copy defect above.
* DDI detach versus ioctl serialization still relies on framework/lifecycle guarantees; no independent stress test proves the interaction. No teardown race is claimed as confirmed.
* Boot cursor boundary validation retains upstream `>` rather than `>=` checks. A one-past terminal cursor position could be unsafe if supplied by invalid boot metadata; the dboot cursor-position contract must be verified before treating this as a confirmed defect.

### Diagnostic filter audit

* DIAG-001: `ld: fatal:` is present in both compiler and runner scans; controlled linker-fatal sample matches.
* DIAG-002: boundary-aware `failed|killed` matching excludes identifier/file-name substrings; controlled filename samples do not match.
* DIAG-003: current `cannot` matcher limits matches to known failure verbs. Replaying the current expression against the full Build #70 artifact produced zero matches; 13/13 controlled positive samples matched and known benign comment/file-name samples did not. Build #70 itself used the pre-DIAG-003 expression and contained two benign `cannot set property` matches. Build #71 is the real-artifact confirmation and remains pending.
* No committed automated diagnostic-regex fixture test exists in `build-illumos.yml`; the workflow's model test does not test diagnostic regex behavior. Add controlled fixtures only in a later authorized cycle.
* Diagnostic indexes are observational; nightly and final exit codes determine build success. No grep filter masks the actual nightly exit status.

### Automation control

* Hourly automation `ROTFB hourly engineering` remains enabled. Its prompt does not yet contain AUDIT-ONLY restrictions and still authorizes low-risk source/workflow repairs after a build completes.
* The latest automation cycle created commit `5d014620e42956c1b0fd631cc47919ae3807c4d6`, changing only `ROTFB_ENGINEERING_LOG.md`; no source, patch, or workflow files changed, and no additional target run appeared.
* The prompt was not modified while Build #71 is active. Once #71 completes, switch the automation prompt to read-only audit/journal mode while keeping its hourly schedule enabled, so it cannot apply fixes before explicit user approval.

### Status and next steps

* Geometry/memory arithmetic for fixed target dimensions: source/model PASS; C behavior not dynamically exercised.
* Boot lifecycle: SOURCE-LEVEL PASS; actual boot BLOCKED pending hardware.
* Runtime lifecycle: FAIL for the two alias defects above; lifecycle ownership/rollback otherwise source-level PASS.
* Shadow synchronization: SOURCE-LEVEL PASS; runtime self-copy is harmless; no real framebuffer round-trip test.
* Polled callbacks: lifetime gate SOURCE-LEVEL PASS; functional cursor/copy defects apply to polled paths; no stress test.
* i86pc/i86xpv: Build #70 compile/link PASS; Build #71 still running and changes only the diagnostic workflow.
* Hardware: NOT TESTED on Chuwi MiniBook X N150.
* No source, patch, or workflow modifications were made during this audit. This journal-only commit is outside the dedicated workflow path filters and is not intended to trigger a build.


## ROTFB POST-BUILD #71 AUDIT CORRECTIONS — AUDIT-ONLY

* Timestamp UTC: 2026-11-18T13:18:30Z
* Branch HEAD before this journal-only update: `3cd393fdbfc2bedef2163aae598f61b4b99c9a5e`.
* Build #71 / Run ID `44302559938` remains IN_PROGRESS after approximately 60 minutes. Job `134795747200` is still in the OpenIndiana VM step; GitHub returns `BlobNotFound` for its active job log, and no artifact is published. No cancellation or new build was attempted.
* The only active target workflow remains Build #71. This entry is root-level journal content, outside the workflow path filters; it must not trigger a new build.

### Confirmed findings

* `ROTFB-LIFE-001`, Severity Medium — overlap corruption in `gfxp_bitmap.c::bitmap_cons_copy()` (patch 0007). Runtime init sets `fb->fb == fb->shadow_fb`. The function first memmoves from the shadow source to the framebuffer destination, then repeats the same move to the shadow destination. Those destinations alias. For horizontal overlapping moves, the first memmove modifies source bytes that the second memmove reads, corrupting the result. This is reachable through normal TEM insert/delete-character operations: `tem_safe.c::tem_safe_shift()` issues horizontal `tem_safe_copy_area()` operations, which invoke `tem_safe_callback_copy()` for an active terminal. A local overlapping-memmove simulation reproduced the corruption. Minimal fix after authorization: skip the second copy when `sdst == dst`; test left and right overlap.
* `ROTFB-MEM-002`, Severity High — signed input validation gap in `gfxp_bitmap.c::bitmap_cons_copy()`, `bitmap_cons_display()`, and `bitmap_display_cursor()`. The VIS interface uses signed `screen_pos_t` and `screen_size_t` fields (`short`). The patched functions reject zero and upper-bound values but do not consistently reject negative coordinates and dimensions before casting to `uint64_t`/ `uint32_t` or doing pointer arithmetic. In `bitmap_cons_display()` and `bitmap_display_cursor()`, patch 0009 also changes loop indices to `uint32_t`; a negative signed height is converted to a large unsigned loop bound. `bitmap_cons_copy()` can form huge offsets from negative coordinates and does not independently validate those offsets against fb_size before pointer arithmetic. Normal TEM has its own input bounds checks, and the VIS ioctls reject user-mode requests; the driver callback still needs defensive validation for malformed kernel-origin calls. Minimal fix after authorization: reject every negative coordinate/size before arithmetic; add tests for -1, SHRT_MIN, negative width/height, and signed-to-unsigned conversion.
* `ROTFB-BOOT-WC-001`, Severity Medium — boot flush ordering gap. Pinned upstream `dboot_startkern.c` maps Multiboot framebuffer pages with `PT_NOCACHE` and, when PAT is supported, `PT_PAT_4K`; `mach_mmu.h` documents `PT_PAT_4K` as the write-combining PAT bit. `rotfb_boot_flush_rect()` writes to this mapping but has no producer fence, while `rotfb_runtime_flush_rect()` calls `membar_producer()` (amd64 implementation is `sfence`). The code-level ordering asymmetry is confirmed; actual stale-output manifestation on hardware is untested. Minimal fix after authorization: add a producer barrier after the completed boot rectangle writes.
* `ROTFB-BOOT-GEO-001`, Severity Blocker / hardware precondition — `physical_fb_info` is populated directly from the Multiboot framebuffer tag width/height/pitch in `xbi_fb_init()`; `rotfb_boot_configure()` requires width 1200 and height 1920. If the actual boot tag reports 1920x1200, configuration returns failure and ROTFB silently remains disabled. The actual Chuwi tag values have not been captured in this audit. Do not infer physical panel dimensions from the panel specification; obtain the real boot tag width/height/pitch/address and pixel masks before declaring boot activation possible.
* `ROTFB-TEST-001`, Severity Medium — current host tests exercise a Python geometry model and patch-text/hunk contracts, not the actual C functions. The clipping model does not reject negative coordinates. There is no test for overlapping aliased VIS_CONSCOPY, negative signed VIS arguments, boot import/rollback, runtime allocation failures, or callback-versus-fini concurrency. Build #71's green host-test step does not cover these paths.

### Correction to the preceding interim journal entry

* The earlier claim `ROTFB-LIFE-002` that `bitmap_display_cursor()` applies the XOR transform twice when framebuffer pointers alias is **NOT CONFIRMED and is withdrawn**. Source inspection shows the function transforms `sfb*` once, then assigns the transformed value to `fb*`; when pointers alias, that assignment is a self-assignment, not a second XOR. Do not implement or test a cursor-alias fix based on that claim. Cursor behavior still needs a normal display/hide regression test.
* The exact same-pointer `memmove` in `bitmap_devinit()` remains redundant but safe. It is distinct from the overlapping double-copy defect in `bitmap_cons_copy()`.

### Verified lifecycle and remaining limits

* Boot order source-level PASS: `_start()` -> `bcons_init()` -> `boot_fb()` -> initial physical `boot_fb_init()`; later `boot_fb_shadow_init()` allocates logical shadow and re-enters `boot_fb_init(0)`. Build #70 change `d0d53a4` sets `fb->fb = rotfb_info.logical_fb` before `boot_ready`, then imports physical pixels. No real boot test.
* Runtime init/fini and callback gate are source-level coherent. Polledio is returned only after `bitmap_setup_fb()` completes; callbacks use the gate; fini closes it and drains active calls before freeing mappings. No dynamic concurrency test; DDI_DETACH versus VIS_DEVFINI serialization still relies on framework lifecycle guarantees.
* DIAG-001 and DIAG-002 are supported by commits/builds #68 and #69. DIAG-003's current expression passed 14 positive and 8 benign negative synthetic samples in this audit and produced zero matches when replayed against the full Build #70 artifact. Build #71 remains the required real-artifact confirmation. Diagnostic regex fixtures are not committed into the workflow.
* Build #70 remains the latest completed source build: all nine patches applied; nightly and final exit codes 0; compile/link evidence for i86pc and i86xpv. Build #71 changes only the diagnostic workflow and remains pending.
* No source, patch, Makefile, interface, or workflow changes were made. No manual build was dispatched. Hardware validation on Chuwi MiniBook X N150 remains NOT TESTED.
* Hourly automation remains enabled. Its prompt still permits low-risk repairs; it has not been switched to audit-only while Build #71 is active. After #71 completes, convert the prompt to audit/journal-only mode while preserving its hourly schedule, and require explicit user approval before source changes.


## ROTFB POST-BUILD #71 — VERIFIED RESULT AND LOW-RISK REPAIR PREPARATION

* Timestamp UTC: 2026-11-18T13:38:08Z
* Branch HEAD before this journal entry: `c3ddcd3110e7f88df3efc66db2ce0c3f46dbb1f2`.
* Build #71 / Run ID `44302559938`, Job ID `134795747200`, Artifact ID `14430996873`: **SUCCESS**. The run completed at 2026-11-18T13:18:45Z on source commit `f0cebdfd2f2877b83a3b082e507ab73627c5b6b8`.
* Full job log and artifact were retrieved. The artifact is 7,077,031 bytes (SHA-256 `319c4c6e6d41e86d95583878b38aa5bf78c27460856a91e8784a11481a1e0fd3`). Its captured illumos nightly log is 176,301,459 bytes / 310,262 lines.
* All nine patches applied in lexical order to pinned upstream `098e95820891f8a83fae679daece7fcf58b7b3a4`. The VM ran `git diff --check`; nightly exit code and final build exit code are both 0.
* Compile evidence: 3 compile commands each for ROTFB boot, ROTFB runtime, gfxp_bitmap, and gfxp_fb on both i86pc and i86xpv; 2 link commands each for unix boot object, runtime module, bitmap module, and gfxp_fb module.
* The DIAG-003 workflow expression is confirmed against the real Build #71 artifact: `runner-error-index.txt` is header-only (38 bytes); `compiler-error-index.txt` is 307 bytes with no matched error context. A scan of the captured nightly log found no compiler errors, linker fatals, undefined symbols, panics, segmentation faults, or matched operational failures.
* Current target-run inventory: 71 total, 8 successful, 63 failed historical runs, 0 queued, 0 active. The complete first page plus page 2 was inspected and filtered to `.github/workflows/build-illumos.yml`.

### Confirmed source defects and validated candidate fixes

* `ROTFB-LIFE-001` — confirmed alias-overlap corruption in `bitmap_cons_copy()`. Runtime init sets `fb->fb == fb->shadow_fb`; the KD_TEXT path performs a framebuffer memmove and then repeats the same memmove to the aliased shadow destination. A local C harness reproduced `abababghi` instead of the correct `ababcdghi` for an overlapping move. Candidate patch `0010-gfxp-bitmap-vis-safety.patch` skips the second copy only when the destinations alias in KD_TEXT, preserving the non-alias and KD_GRAPHICS cases.
* `ROTFB-MEM-002` — confirmed signed VIS validation gap. `screen_pos_t` / `screen_size_t` are signed short, while framebuffer dimensions are `uint16_t`; both operands promote to signed int, so negative values pass the existing upper-bound comparisons. A local C harness showed the old guard accepts `col=-1` and a later uint64 offset becomes `18446744073709551612`. Candidate patch 0010 rejects negative source/destination coordinates and negative display/cursor coordinates or dimensions before arithmetic.
* `ROTFB-BOOT-WC-001` — confirmed ordering asymmetry. Pinned dboot maps the Multiboot framebuffer using `PT_NOCACHE` and `PT_PAT_4K` when PAT is available; the latter is the write-combining PAT entry. Runtime flush already calls `membar_producer()`, but boot flush did not. `sys/atomic.h` declares the barrier and Build #71's unix link command contains `atomic.o`; candidate boot fix mirrors the runtime barrier after the completed rectangle writes.
* `ROTFB-BOOT-GEO-001` remains unresolved: actual Multiboot framebuffer tag width/height/pitch/address and pixel masks from the Chuwi are not captured. Do not claim boot activation or hardware rotation until these values are obtained.

### Local validation before source commit

* Reconstructed the Build #71 pre-0007 bitmap source from the published artifact. The exact workflow patch order 0007, 0009, then candidate 0010 passed local `git apply --check` and application; `git diff --check` passed.
* Local Python model tests passed for negative VIS values (`-1`, `SHRT_MIN`) across all signed coordinate/size fields and for forward/backward overlapping copies with aliased and separate surfaces.
* The local C promotion/memmove harness reproduced both old defects and verified the explicit signed guard.
* The complete illumos build is not available in this runner: DNS resolution to github.com remains unavailable. The OpenIndiana VM build is the authoritative compile/link check.
* No hardware validation was performed. No issue creation, repository settings, other branches, or unrelated subsystems were touched.

* This entry is root-level journal content outside the workflow path filters and must not trigger a build. Candidate source changes will be committed together in one ROTFB-only commit after a fresh branch/run recheck; at most one Build #72 may be created.


## ROTFB BUILD #71 EVIDENCE CORRECTION — PATCH 0010 AND BOOT BARRIER NOT INCLUDED

* Timestamp UTC: 2026-11-18T13:57:30Z.
* Read-only post-build audit retrieved Build #71 / Run ID `44302559938`, Job ID `134795747200`, Artifact ID `14430996873`. Run conclusion: SUCCESS; source commit: `f0cebdfd2f2877b83a3b082e507ab73627c5b6b8`.
* The complete job log (1,662,383 characters) and artifact ZIP (7,077,031 bytes; SHA-256 `319c4c6e6d41e86d95583878b38aa5bf78c27460856a91e8784a11481a1e0fd3`) were retrieved. Artifact contains the original nightly log (176,301,459 bytes), patch-apply log, compile/link command evidence, and actual exit-code files.
* Actual `nightly-exit-code.txt=0`; `final-build-exit-code.txt=0`. Original nightly log records distributed build completion at 2026-11-18 13:18:12 UTC and OS-Net source build completion at 13:17:59 UTC. These exit codes and original logs are primary evidence; diagnostic indexes are secondary.
* Patch application log records CHECK and APPLIED for exactly the nine patches 0001–0009 in lexical order. All nine applied to pinned upstream `098e95820891f8a83fae679daece7fcf58b7b3a4`; `git diff --check` was part of the VM build step.
* Compile evidence records three compile commands per architecture for each of rotfb_boot, rotfb_runtime, gfxp_bitmap, and gfxp_fb (i86pc and i86xpv); link evidence records two commands each for unix boot object, runtime module, bitmap module, and gfxp_fb module. The run completed successfully.
* Diagnostic indexes: runner index is header-only (38 bytes); compiler index is 307 bytes. This only reports no matches for those configured patterns and does NOT establish absence of all possible errors.

### Important source/build boundary correction

* Build #71 was built from `f0cebdfd2f2877b83a3b082e507ab73627c5b6b8`. Its `patch-apply.log` proves only patches 0001–0009 were applied. It does NOT validate patch `0010-gfxp-bitmap-vis-safety.patch`.
* Commit `ebac4d529814ce7e9e46a8f8e9f914a73647bf4a` subsequently introduced patch 0010 and the boot `membar_producer()` change. Therefore Build #71 must not be cited as compile/link validation for ROTFB-LIFE-001 / ROTFB-MEM-002 candidate changes or ROTFB-BOOT-WC-001 barrier change. Build #72 / Run ID `44318770892` is IN_PROGRESS; its VM job is active and no artifact is published yet.

### Known issue status after read-only source inspection

* ROTFB-LIFE-001: candidate guard exists in patch 0010 for the two aliased memmove sites. Not validated by Build #71; await Build #72 patch application and compile/link evidence. Python model is not a C-level test.
* ROTFB-MEM-002: patch 0010 adds negative-coordinate/positive-dimension guards to VIS copy/display/cursor paths. Not validated by Build #71; await Build #72 evidence and direct C-level tests remain absent.
* ROTFB-BOOT-WC-001: current branch `rotfb_boot.c` contains `sys/atomic.h` and `membar_producer()` after boot flush writes. Commit `ebac4d5` introduced this change after Build #71, so #71 does not validate it; await Build #72 compile/link evidence.
* ROTFB-BOOT-GEO-001: unresolved hardware/input-data blocker. Actual Chuwi Multiboot framebuffer width, height, pitch, physical address, and pixel masks remain uncollected.
* ROTFB-TEST-001: unresolved test-coverage limitation. Current tests model behavior in Python and assert patch text; they do not execute the C implementation or exercise real lifecycle/callback concurrency.

* The dedicated workflow path filter includes only `illumo_rotfb/**` and `.github/workflows/build-illumos.yml`; root-level `ROTFB_ENGINEERING_LOG.md` is outside those push path filters. This is a journal-only correction; no source, patch, Makefile, test, or workflow file was changed.
* No build was dispatched, rerun, cancelled, or otherwise modified during this audit.


## ROTFB BUILD #72 POST-BUILD AUDIT — DBOOT LINK FAILURE

* Audit timestamp UTC: 2026-11-18T15:16:25.707Z.
* Branch: `gop-rotation-hook`; pre-journal HEAD: `36fa1f636360f2ec9455b5859b03fb449d60cede`.
* Recent commits and paths: `36fa1f636360f2ec9455b5859b03fb449d60cede` changes only `ROTFB_ENGINEERING_LOG.md`; its parent `ebac4d529814ce7e9e46a8f8e9f914a73647bf4a` changes `illumo_rotfb/patches/0010-gfxp-bitmap-vis-safety.patch`, `illumo_rotfb/rotfb_boot.c`, and `illumo_rotfb/tests/rotfb_model_test.py`; preceding `ada201915c395f1055365c17dde06fcd0b326280` changes only the engineering log.
* Pinned upstream remains `098e95820891f8a83fae679daece7fcf58b7b3a4`.

### Build #72 — completed failure

* Run ID: `44318770892`; job ID: `134851820291`; source commit: `ebac4d529814ce7e9e46a8f8e9f914a73647bf4a`; artifact ID: `14437498860`.
* Run conclusion: FAILURE. Actual `nightly-exit-code.txt=1`; actual `final-build-exit-code.txt=1`. Model test step passed; OpenIndiana VM build step failed; diagnostic finalization and artifact upload completed.
* Full job log: 1,717,843 characters. Artifact: 6,791,138 bytes, SHA-256 `9f28b40212531d2610354b3ab0677dce9ebf25fc59cf04466d6309831748490f`, 34 files. Original captured gate nightly log: 176,283,272 bytes; second gate copy: 176,271,533 bytes.
* First root cause in chronological nightly output: i86xpv dboot final link fails at log line 50570 with `Undefined ... membar_producer ... dboot/obj64/rotfb_boot.o`; second occurrence is i86pc dboot final link at line 53086 with the same unresolved symbol. Both fail to produce `dboot/obj64/dboot`. This is the direct cause of the nightly and final exit code 1; later dmake errors are consequences.
* Root-cause explanation verified against pinned upstream headers and actual dboot compiler/link commands: `rotfb_boot.c` calls `membar_producer()`. `sys/atomic.h` declares it as an external function. Its `asm/atomic.h` include is conditional on `_KERNEL && _ASM_INLINES`; the dboot compile command uses `-U_KERNEL`, so no kernel inline implementation is included. The dboot link input list does not include kernel `atomic.o`. Thus `rotfb_boot.o` retains an unresolved external symbol in both dboot targets.
* Build #72 is the first completed build that applied the new boot barrier and patch 0010. The barrier candidate currently blocks both architecture boot-image links. ROTFB-BOOT-WC-001 is therefore NOT fixed; it is a confirmed current build blocker.

### Patch application and compile/link evidence

* Actual current patch set: 10 files in lexical order: 0001-framebuffer-metadata; 0002-boot-fb-rotation; 0003-gfxp-bitmap-tem-rotation; 0004-Makefile-files; 0005-rotfb-lifecycle-hardening; 0006-rotfb-console-contract-and-copy-safety; 0007-gfxp-bitmap-bounds-hardening; 0008-gfxp-detach-failure-safety; 0009-gfxp-bitmap-wide-row-offsets; 0010-gfxp-bitmap-vis-safety.
* Patch hunk counts are respectively 7, 8, 1, 4, 12, 10, 17, 1, 11, and 5: 10 patches / 76 hunks total. Build #72 `patch-apply.log` contains CHECK and APPLIED for all ten in that lexical order, followed by `git diff --check` in the VM script.
* The artifact records three compile command occurrences per architecture for each of `rotfb_boot`, `rotfb_runtime`, `gfxp_bitmap`, and `gfxp_fb`. The boot objects compile far enough to reach the final dboot link; the dboot link then fails for both i86pc and i86xpv on `membar_producer`.
* The artifact records two `ld -r -o obj64/unix.o` command occurrences and two `ld -ztype=kmod ... gfx_private` command occurrences. The latter are followed by CTF merge commands in the original log. However, the full build is failed and no final dboot image is produced for either architecture. Do not describe overall i86pc/i86xpv build or final boot link as PASS.
* The workflow's `rotfb-build-evidence.txt` says `ROTFB/framebuffer compile-link evidence: PASS`, but source inspection shows that this status is based only on grep counts of command lines. `BOOT_LINK_COUNT` counts `ld -r -o obj64/unix.o`, not the final `ld ... -o dboot/obj64/dboot`; runtime/module link counters also count command lines rather than checking command exit status. This PASS label is evidence-presence only and contradicts the actual failed dboot link if read as a successful-link result.

### New confirmed CI evidence issue — ROTFB-CI-EVIDENCE-001

* Classification: confirmed reporting/evidence semantics defect in `.github/workflows/build-illumos.yml` (read-only finding; no workflow change made).
* Scenario: a ROTFB object/module command is present in the nightly log, but a later architecture-specific dboot link fails. The counter-based script still emits a generic compile-link `PASS` because it checks command occurrence counts, not successful return status or final dboot link outputs.
* Recommended owner-authorized remediation: distinguish `COMMANDS_PRESENT` from `LINK_SUCCESS`; explicitly capture both i86pc and i86xpv dboot final-link status and verify expected output files; retain nightly/final exit codes as authoritative. Add a fixture regression using the Build #72 pattern: all ROTFB compile commands present plus unresolved `membar_producer` at dboot link must never produce an overall link PASS.

### Known issue status

* ROTFB-LIFE-001: patch 0010 is present and was applied. It guards the two aliased memmove sites. Build #72 records gfxp_bitmap compile commands for both architectures, but the model test is Python-only and the complete build fails later at dboot link. Status: fix candidate applied; NOT verified fixed under the required direct-C-test criterion. Proposed direct test: compile/run the actual C copy routine with aliased source/destination and overlapping forward/backward ranges in text and graphics modes.
* ROTFB-MEM-002: patch 0010 adds negative-coordinate and non-positive-dimension checks to VIS copy/display/cursor paths and was applied. Build #72 model step passed; no C-level test exists. Status: fix candidate applied; NOT verified fixed. Proposed tests: exercise actual VIS handlers with -1, SHRT_MIN, zero/negative sizes, upper-bound coordinates, and verify rejection occurs before unsigned conversion/pointer arithmetic.
* ROTFB-BOOT-WC-001: current `rotfb_boot_flush_rect()` contains `membar_producer()`, but the call is unresolved in both dboot links. Status: confirmed current build blocker; candidate is not linkable in the boot environment. Proposed remediation plan: identify a dboot-supported store-ordering primitive or boot-linked implementation with correct WC semantics for both i86pc and i86xpv; verify no undefined external remains in either dboot link and retain runtime kernel barrier separately.
* ROTFB-BOOT-GEO-001: unresolved hardware/input-data blocker. Actual Chuwi Multiboot framebuffer address, width, height, pitch, bpp/depth, RGB masks, and reported framebuffer size remain unknown.
* ROTFB-TEST-001: unresolved coverage limitation. Python model tests now cover mapping, alias-copy model, and signed VIS input model, but do not execute the C implementation or cover complete boot import/rollback, runtime allocation failures, callback races, detach, and concurrent fini/flush teardown.

### Diagnostic indexes and build inventory

* `compiler-error-index.txt` is 12,698 bytes and `runner-error-index.txt` is 47,448 bytes; both include the unresolved `membar_producer` dboot link failures and associated dmake failures. The indexes are secondary aids; original full logs and exit-code files are authoritative.
* Fresh run inventory: 72 runs for `.github/workflows/build-illumos.yml` on this branch; 8 success, 64 failure, 0 queued/active. Latest completed run is #72 FAILURE; latest successful run is #71 SUCCESS (Run ID `44302559938`, nightly/final exit 0, nine patches 0001–0009 applied).
* No hardware validation was performed. Actual Multiboot parameters remain a device-data blocker.

### Journal/write boundary

* The workflow path filter contains only `illumo_rotfb/**` and `.github/workflows/build-illumos.yml`; root-level `ROTFB_ENGINEERING_LOG.md` is outside those push filters.
* At the pre-write recheck there were no queued or active target builds. This entry changes only the root-level engineering journal. No source, patch, test, Makefile, script, workflow, branch, or build state was changed; no run was dispatched, rerun, cancelled, or approved.


## ROTFB AUTONOMOUS REPAIR CYCLE — DBOOT SFENCE AND REGRESSION TEST

* Timestamp UTC: 2026-11-18T15:28:40Z.
* Branch: `gop-rotation-hook`. Source HEAD: `dd3155227799a2ea0479d93e351b7a719f316cb4`. Pinned upstream remains `098e95820891f8a83fae679daece7fcf58b7b3a4`.
* Root cause from Build #72 is confirmed against pinned illumos source: x86 `membar_producer()` is implemented as `sfence` in `usr/src/uts/intel/ml/lock_prim.S`; the dboot link does not include kernel atomic objects, so an external `membar_producer` reference cannot resolve. The boot AMD64 cpufunc header also defines an inline `sfence()` with a compiler memory clobber.
* Commit `f71907dc6c40f6afbc7d99304d7678057963341d` removes `<sys/atomic.h>` from `rotfb_boot.c`, adds a local boot helper containing the hardware `sfence` instruction plus compiler memory clobber, and calls it after physical framebuffer stores. This preserves the WC store-ordering semantics of the pinned x86 `membar_producer` implementation without an external kernel symbol.
* A regression test was added to extract and compile the actual helper body for `-Di86pc` and `-D__i86xpv`, disassemble the object to require `sfence`, and require no undefined symbols. An independent local GCC probe with `-m64 -O2 -ffreestanding -fno-pic -mno-sse -mno-mmx` produced `mov; sfence; ret` for both defines; `nm -u` was empty. This is a host-toolchain probe, not an illumos dboot build.
* Build #73: Run ID `44335478142`, source `f71907dc6c40f6afbc7d99304d7678057963341d`, FAILURE before OpenIndiana VM. First cause: old `test_patch_contracts()` still asserted that `<sys/atomic.h>` and `membar_producer()` must be present. No upstream build was attempted.
* Commit `177e60cb038a5907c05bd27e8b9d1d1723510397` updated those stale source-contract assertions. Build #74: Run ID `44335650407`, FAILURE before VM because the new Python assertion contained over-escaped quote sequences and caused a SyntaxError at line 342. No upstream build was attempted.
* Commit `dd3155227799a2ea0479d93e351b7a719f316cb4` fixes the Python assertion using a single-quoted Python literal. Build #75: Run ID `44335792313`, Job ID `134912017496`, is IN_PROGRESS. Geometry/clipping and the full Python test step completed PASS; OpenIndiana VM build is currently running. No artifact or nightly/final exit code is available yet.
* The current target run list was rechecked: #75 is the only active target build; no other queued or active target run exists. No further source/workflow changes will be made until #75 completes.
* Current patch set remains 10 patches (0001–0010) in lexical order. Build #72 applied all 10, but failed both final dboot links due the unresolved external. Build #75 is the first build candidate containing the inline SFENCE fix; its patch-apply and i86pc/i86xpv compile/link results remain pending.
* CI evidence issue ROTFB-CI-EVIDENCE-001 remains open: current evidence counts linker command lines but does not validate fresh final dboot outputs or actual linker success. Build #72 artifact confirms this weakness. Proposed workflow correction: remove expected outputs before nightly; after nightly require fresh non-empty `i86pc/unix/dboot/obj64/dboot`, `i86xpv/unix/dboot/obj64/dboot`, both `unix/obj64/unix.o`, and both `unix/obj64/gfx_private` outputs; fail evidence on linker fatal/undefined-symbol diagnostics and preserve actual nightly/final exit codes. This workflow change is deferred until #75 completes to avoid editing during an active build.
* Workflow push path filters are exactly `illumo_rotfb/**` and `.github/workflows/build-illumos.yml`; root `ROTFB_ENGINEERING_LOG.md` is outside them. This commit updates only the root journal while #75 runs and does not trigger another build. No run was dispatched, rerun, cancelled, or approved by this journal operation.

### Readiness after this cycle

* BUILD VERIFIED: NO — Build #72 failed; #75 is still running.
* CODE/TEST VERIFIED: NO — SFENCE helper probe and Python regression step pass, but full C/lifecycle coverage remains incomplete.
* DEVICE VERIFIED: NO — no Chuwi boot or real Multiboot parameter capture has been performed.

### Next action

Wait for Build #75. On completion retrieve the complete job log and artifact, verify patch application, both dboot final links, both `unix.o` and `gfx_private` outputs, and actual nightly/final exit codes. Then correct ROTFB-CI-EVIDENCE-001 in the dedicated workflow only after confirming no active target run, and validate that change with a single subsequent build.


## ROTFB-MEM-002 — VIS CURSOR WRAPPER VALIDATION GAP

* Timestamp UTC: 2026-11-18T15:35:26.624Z.
* Inspected HEAD: `75386a7948a8b95efc9489eea4718c0a511d9fed`; source under test in Build #75 remains `dd3155227799a2ea0479d93e351b7a719f316cb4`. Pinned upstream remains `098e95820891f8a83fae679daece7fcf58b7b3a4`.
* Build #75 Run ID `44335792313`, Job ID `134912017496`, remains IN_PROGRESS. Model/regression test step passed; OpenIndiana VM step is active. No source or workflow files were changed while that build runs.
* Confirmed signed-dimension gap: the VIS ABI defines `screen_size_t` as signed `short`. In patched `bitmap_cons_cursor()`, the VIS_DISPLAY_CURSOR guard checks `ca->width == 0` and `ca->height == 0`, not `<= 0`. Because `console->fb.screen.x/y` are unsigned 16-bit values promoted to signed `int`, negative widths/heights can pass the upper-bound comparison. The function then updates cursor position/origin and sets `cursor.visible = B_TRUE`; the lower-level `bitmap_display_cursor()` rejects nonpositive dimensions before pointer arithmetic. This is a confirmed cursor-state inconsistency, not a confirmed out-of-bounds framebuffer write.
* Proposed minimal fix after #75 completes: reject negative col/row and `width <= 0` / `height <= 0` in `bitmap_cons_cursor()` before any cursor-state mutation. Add a C-level regression asserting position, origin, and visibility remain unchanged for `-1` and `SHRT_MIN` dimensions. Keep the lower-level `bitmap_display_cursor()` guard.
* ROTFB-MEM-002 remains partially unresolved until this wrapper path is fixed and tested.
* Correction to the previous proposed CI output paths: `gfx_private` outputs are `usr/src/uts/i86pc/gfx_private/obj64/gfx_private` and `usr/src/uts/i86xpv/gfx_private/obj64/gfx_private`, not under each architecture's `unix/obj64` directory. The `unix.o` outputs are under `usr/src/uts/{i86pc,i86xpv}/unix/obj64/unix.o`; dboot outputs are under `usr/src/uts/{i86pc,i86xpv}/unix/dboot/obj64/dboot`.
* The workflow push path filters were verified as `illumo_rotfb/**` and `.github/workflows/build-illumos.yml`. This commit changes only the root journal and does not dispatch a new target build.


## ROTFB-CI-EVIDENCE-001 — Build #75 post-build audit and linker-status instrumentation

* Audit timestamp UTC: 2026-11-18T16:58:53.265Z.
* Audited branch HEAD before changes: `2ca9fcecc5e9310b46ca57a7453cfdb4713962ce`; pinned upstream remains `098e95820891f8a83fae679daece7fcf58b7b3a4`.
* Build #75: Run ID `44335792313`, Job ID `134912017496`, source `dd3155227799a2ea0479d93e351b7a719f316cb4`, workflow conclusion `success`, nightly exit `0`, final build exit `0`, artifact ID `14445417546`, artifact SHA256 `f4581a9bcd0d95e09bc581620edaf8239832b066f508273726de53ed5875b8e3`. The artifact archive hash was independently recomputed and matched.
* Build #75 artifact confirms all ten patches `0001`–`0010` were applied. The original nightly log contains dboot final-link command lines for both architectures with `dboot/obj64/rotfb_boot.o`, unix.o relocatable-link commands for both architectures with `obj64/rotfb_boot.o`, and gfx_private link commands for both architectures with `obj64/rotfb_runtime.o`. The gfxp_bitmap/gfxp_fb compile command records also exist for both architecture defines.
* Evidence limitation: Build #75's `rotfb-build-evidence.txt` only counted command-line matches. It did not preserve per-linker process exit statuses, target output sizes/timestamps, or final target files. Therefore the Build #75 workflow/nightly success is VERIFIED, but individual dboot/unix.o/gfx_private target freshness and per-linker exit statuses are PARTIAL, not independently VERIFIED from that artifact.
* Root cause: the previous evidence implementation used grep counts for linker command presence and then emitted one aggregate PASS. It never observed the return code of the linker process or checked the actual target file after the link.
* Changes in this commit: add `illumo_rotfb/rotfb_link_evidence.sh`; replace command-count link checks in `.github/workflows/build-illumos.yml` with a temporary wrapper around the exact onbld amd64 linker path used by the build. The wrapper preserves the original linker, records target-specific argv/object membership, captures each real linker exit status and diagnostics, and passes the original arguments through unchanged. The collector removes only six exact ROTFB final link products (dboot, unix.o, gfx_private for i86pc and i86xpv), records pre-clean state and build start time, then checks nonzero output size, post-start mtime, expected ROTFB object argument, linker status, and target-specific diagnostics separately for all six outputs. Any missing proof yields INCOMPLETE.
* The helper also records compile-command evidence, patch count/order, nightly exit status, per-target command files and linker diagnostics. It does not remove source, common build outputs, or caches.
* Local helper validation: shell syntax check PASS; synthetic six-target run PASS; injected linker exit code 9 plus `ld: fatal` diagnostic correctly produced `linker_exit_status=NONZERO`, `linker_diagnostics=FOUND`, and overall INCOMPLETE. These are harness tests, not an OpenIndiana build.
* No C source, rotation formula, framebuffer geometry, shadow-buffer architecture, or unrelated subsystem was changed in this CI-evidence commit. C-level ROTFB tests remain the next separate engineering stage after this evidence workflow is validated by one complete build.


## ROTFB-CI-EVIDENCE-001 — Live audit of Build #76 instrumentation

* Audit timestamp UTC: 2026-11-18T17:09:09Z.
* Branch HEAD: `5da8f521836211f43bd85bf54ac17457f56745d1`, branch `gop-rotation-hook`; pinned upstream remains `098e95820891f8a83fae679daece7fcf58b7b3a4`.
* Build #76: Run ID `44353489911`, Job ID `134975230572`, source `5da8f521836211f43bd85bf54ac17457f56745d1`, status IN_PROGRESS. The geometry/clipping model test step is PASS; the OpenIndiana VM step is still running. No artifact is available yet. This is the only active target build; no second build was started.
* The live workflow now calls `illumo_rotfb/rotfb_link_evidence.sh`. Its `prepare` mode removes only six exact target files: dboot, unix.o, and gfx_private for i86pc and i86xpv. It records the pre-clean state and checks that each path is absent afterward; it does not remove source, object directories, common build outputs, or caches.
* Its `watch` mode waits for the exact onbld amd64 linker path used by the build, preserves the original linker as `.rotfb-real`, and installs a temporary pass-through wrapper. For each recognized dboot, unix.o, and gfx_private invocation the wrapper writes a command record before executing the original linker, captures combined diagnostics, writes the actual linker return code afterward, and returns that same code to dmake. This is materially stronger than Build #75's grep-only evidence.
* The `finalize` mode checks the six final target paths for nonzero size and mtime after the recorded build start, verifies the expected ROTFB object argument in the wrapper status, and scans each target's captured linker output for fatal/undefined/relocation diagnostics. The command and status files are preserved separately in `linker-output/`.
* Remaining evidence defects identified by static review, to correct only after Build #76 completes:
  1. `linker_command_detected_count` is currently derived from completed `.status` records, so command observation and completed linker status are still conflated in the evidence summary even though separate `.cmd` files exist. Report command count from `.cmd` and actual completion/exit status from `.status` independently.
  2. A nonzero linker exit and nonzero nightly exit currently collapse to INCOMPLETE. A known nonzero exit must be FAIL; reserve INCOMPLETE for missing or ambiguous proof.
  3. Patch validation checks only that ten patches applied, not that the exact expected `0001`–`0010` names applied in lexical order.
  4. The build-start epoch is recorded before target pre-clean. Move the start marker to immediately after successful cleanup, before nightly begins.
  5. Require the expected invocation count and verify the wrapper's recorded cwd/output path matches each of the six expected targets.
* Pinned-upstream WC barrier semantics independently verified: `usr/src/uts/intel/ml/lock_prim.S` at upstream `098e95820891f8a83fae679daece7fcf58b7b3a4`, lines 648–651, implements `membar_producer` as `sfence; ret`. Current `rotfb_boot.c` implements the same SFENCE with a compiler memory clobber, and `rotfb_boot_flush_rect()` calls it after completing the pixel-store loops. Therefore the instruction semantics and placement are VERIFIED; actual framebuffer mapping attributes and Chuwi device behavior remain PARTIAL. Do not close BOOT-WC-001 as a device-level issue.
* Patch 0010 currently contains negative cursor coordinate guards and `ca->width <= 0` / `ca->height <= 0` guards in the cursor callback hunk. This conflicts with the older journal note claiming that only zero dimensions were rejected. Treat that older claim as requiring re-verification against the post-patch source in the completed VM workspace; do not add a duplicate fix or close MEM-002 without a C-level regression.
* ROTFB-TEST-001 remains PARTIAL: the existing Python geometry/alias/clipping tests and SFENCE assembly probe are not a direct C implementation or lifecycle test. No C-level harness has been added in this cycle. The next test step remains direct C coverage of boot import/flush, VIS callback guards/alias copy, runtime allocation rollback, callback gate, and teardown.
* No source or workflow files were changed during this live audit. This entry changes only the root engineering journal, which is outside the workflow path filters and does not trigger a build.

### Current readiness during Build #76

* BUILD VERIFIED: NO — Build #76 is still running; its final linker evidence and artifact are not yet available.
* CODE/TEST VERIFIED: NO — critical C-level lifecycle and callback tests are still missing.
* DEVICE VERIFIED: NO — no Chuwi boot or real Multiboot capture has been performed.

### Next action

Wait for Build #76 to finish. Retrieve its full artifact and nightly log, verify all six linker status records against the six fresh target files, and inspect the exact per-architecture results. Then make one minimal evidence-only correction for the five items above, add regression fixtures for nonzero/missing linker statuses and patch order, and run exactly one subsequent build after confirming no active target run.


## ROTFB-CI-EVIDENCE-001 — Build #76 exit-code serialization defect

* Audit timestamp UTC: 2026-11-18T17:32:21.256Z.
* Branch HEAD before journal update: `f492d42f3b9e8a0299a27f0b4df1fb04f2c5733e`; branch `gop-rotation-hook`. Pinned upstream remains `098e95820891f8a83fae679daece7fcf58b7b3a4`.
* Build #76 remains active: Run ID `44353489911`, Job ID `134975230572`, source `5da8f521836211f43bd85bf54ac17457f56745d1`. GitHub reports the geometry/clipping model-test step completed SUCCESS; the OpenIndiana VM step is IN_PROGRESS; diagnostic finalization and artifact upload are pending. No artifact or final build exit code is available yet.
* Additional confirmed defect in `.github/workflows/build-illumos.yml`: three shell printf formats contain a doubled backslash before `n`: the `ld-wrapper-watch-exit-code.txt` write, the `final-build-exit-code.txt` write, and the appended `final_build_exit_code` evidence field. In a YAML literal block Bash receives both backslashes; `printf '%s\\n' 0` emits bytes `30 5c 6e` (the text `0\\n`), not `30 0a` (the numeric value followed by a newline). This was reproduced locally with `od`. The correct format is a single backslash: `printf '%s\n'`.
* Consequence: the workflow process still exits using the numeric `BUILD_RC`, so the GitHub job conclusion remains authoritative; however, the final exit-code artifact and appended evidence field are malformed and cannot be consumed as plain numeric/line-oriented evidence. This is a confirmed workflow evidence defect, not evidence that the active build itself failed.
* Static review also confirms the current helper's `linker_command_detected_count` is calculated from completed `.status` files rather than `.cmd` files; it therefore conflates observed commands with completed linker processes. It records expected ROTFB object arguments but does not inspect the final ELF/object output for ROTFB symbols. The previously logged five evidence corrections (separate command/status counts, explicit FAIL vs INCOMPLETE, exact patch order, cleanup/start ordering, and exact per-target invocation/cwd validation) remain open.
* The dboot barrier fix remains present in `illumo_rotfb/rotfb_boot.c`: the local `sfence` plus compiler memory clobber runs after the WC pixel-store loop. The pinned upstream `membar_producer` implementation in `usr/src/uts/intel/ml/lock_prim.S` is `sfence; ret`; the dboot final-link inputs in Build #72 omitted the kernel atomic implementation, explaining the original unresolved external. The current Python regression extracts the actual helper body, compiles it for `-Di86pc` and `-D__i86xpv`, disassembles for SFENCE, and checks for no undefined symbol. Build #76's model-test step passed, but its VM compile/link evidence remains pending.
* Current patch set remains 10 patches in lexical order: 0001-framebuffer-metadata, 0002-boot-fb-rotation, 0003-gfxp-bitmap-tem-rotation, 0004-Makefile-files, 0005-rotfb-lifecycle-hardening, 0006-rotfb-console-contract-and-copy-safety, 0007-gfxp-bitmap-bounds-hardening, 0008-gfxp-detach-failure-safety, 0009-gfxp-bitmap-wide-row-offsets, 0010-gfxp-bitmap-vis-safety.
* No source, patch, test, Makefile, or workflow file was changed. This is a root-level journal-only update outside the workflow push path filters; it does not trigger a target build. No active run was dispatched, rerun, cancelled, or modified.

### Readiness and next action

* BUILD VERIFIED: NO — Build #76 is still running; its real per-linker statuses and final artifact have not been retrieved.
* CODE/TEST VERIFIED: NO — direct C/lifecycle, allocator rollback, callback-gate, and teardown coverage remains insufficient.
* DEVICE VERIFIED: NO — no Chuwi boot or actual Multiboot framebuffer capture has been performed.
* Wait for Build #76 to complete. Inspect the full job log and artifact, verify all six link results and output freshness, then fix the printf escaping and remaining evidence semantics in one minimal workflow/helper commit. Recheck the run list and branch HEAD immediately before that build-triggering commit; do not edit source/workflow while any target run is active.


## ROTFB-MEM-002 — Correction: VIS cursor wrapper guard remains incomplete

* Audit timestamp UTC: 2026-11-18T17:34:30.953Z. Branch HEAD before journal update: `3485981c02378ed239315d0e8d042fc8bea45f7f`. Build #76 Run ID `44353489911` / Job ID `134975230572` remains IN_PROGRESS on source `5da8f521836211f43bd85bf54ac17457f56745d1`.
* Rechecked the exact pinned-upstream VIS ABI at `usr/src/uts/common/sys/visual_io.h`, commit `098e95820891f8a83fae679daece7fcf58b7b3a4`: both `screen_pos_t` and `screen_size_t` are signed `short`. The cursor's `col`, `row`, `width`, and `height` therefore accept negative values at the ABI boundary.
* Correction to the later Build #76 audit note above: patch `0010-gfxp-bitmap-vis-safety.patch` does NOT add `<= 0` checks to the state-mutating `bitmap_cons_cursor()` wrapper. Patch 0010's cursor hunk is in the lower-level `bitmap_display_cursor()` and rejects negative coordinates/nonpositive dimensions before pointer arithmetic. The wrapper's guard was added earlier by patch 0007 and still checks only `ca->width == 0 || ca->height == 0` before updating cursor position/origin; it then calls the lower-level routine and sets `cursor.visible = B_TRUE`. A negative width/height can therefore mutate cursor state and mark it visible even though the lower-level renderer refuses to draw it.
* The older `ROTFB-MEM-002 — VIS CURSOR WRAPPER VALIDATION GAP` entry dated 2026-11-18T15:35:26Z is correct. The conflicting statement in the 17:09 Build #76 instrumentation audit is withdrawn. This remains a confirmed cursor-state inconsistency, not a demonstrated out-of-bounds write.
* Minimal fix after Build #76 completes: in `bitmap_cons_cursor()`, reject negative `ca->col`/`ca->row` and `ca->width <= 0`/`ca->height <= 0` before cursor state changes; add a direct C regression that verifies position, origin, and visibility remain unchanged for `-1` and `SHRT_MIN`. Preserve the existing lower-level checks.
* No source, patch, test, Makefile, or workflow file was changed. This is a root-level journal-only correction outside the workflow path filters; it does not trigger a build.


## ROTFB-CI-EVIDENCE-001 — Build #76 forensic completion and Build #77 live audit

* Audit timestamp UTC: 2026-11-18T18:38:01.038Z.
* Branch before this journal-only update: `9ff1c2312227f2ce88fbbea5ddf27d18bb1870cb`, branch `gop-rotation-hook`. The build-triggering commit `9ff1c2312227f2ce88fbbea5ddf27d18bb1870cb` changed only `.github/workflows/build-illumos.yml`, `illumo_rotfb/rotfb_link_evidence.sh`, and the new `illumo_rotfb/tests/rotfb_link_evidence_test.sh`. Pinned upstream remains `098e95820891f8a83fae679daece7fcf58b7b3a4`.
* Build #76 is COMPLETE/FAILURE: Run ID `44353489911`, Job ID `134975230572`, source `5da8f521836211f43bd85bf54ac17457f56745d1`. Artifact ID `14456277180`; downloaded archive SHA256 `f17de5eda3fe841d11444eed8bd9ef215c624ee6556b48a0249b5aff1cfc2df3`. The SHA256 was independently recomputed from the downloaded ZIP and matched the Actions upload digest.
* The full job log is 1,639,243 bytes. The artifact contains 43 files, including the original nightly log (176,288,210 bytes) and the timestamped nightly log (176,301,419 bytes), patch application log, build-noise capture, exit-code files, evidence file, and diagnostic indexes.
* Actual exit-code evidence from the downloaded artifact:
  * `nightly-exit-code.txt`: bytes `30 0a`, exact value `0\n`. The OpenIndiana nightly itself completed with exit code 0 at 2026-11-18 17:52:28 UTC after approximately 46 minutes 16 seconds.
  * `final-build-exit-code.txt`: bytes `31 5c 6e`, literal text `1\\n`, not a valid plain numeric line.
  * `ld-wrapper-watch-exit-code.txt`: bytes `30 5c 6e`, literal text `0\\n`, not a valid plain numeric line.
* The workflow job failed after nightly completed. The full Actions log contains `sed: \\: No such file or directory`, followed by `ssh exited with code 1`. The cause is the doubled backslash at the end of the multiline `sed` command in the YAML shell block; Bash passes a literal backslash argument to sed. The three doubled-backslash printf formats also serialize literal backslash-n rather than a newline. These were corrected in commit `9ff1c2312227f2ce88fbbea5ddf27d18bb1870cb` using single-backslash printf formats and a single-line sed command.
* Build #76 patch application log confirms all 10 patches applied in exact lexical order: `0001-framebuffer-metadata.patch`, `0002-boot-fb-rotation.patch`, `0003-gfxp-bitmap-tem-rotation.patch`, `0004-Makefile-files.patch`, `0005-rotfb-lifecycle-hardening.patch`, `0006-rotfb-console-contract-and-copy-safety.patch`, `0007-gfxp-bitmap-bounds-hardening.patch`, `0008-gfxp-detach-failure-safety.patch`, `0009-gfxp-bitmap-wide-row-offsets.patch`, `0010-gfxp-bitmap-vis-safety.patch`.
* The original nightly log contains all six expected linker command lines, with ROTFB inputs present: i86xpv dboot at line 50566 and i86pc dboot at line 53074 both include `dboot/obj64/rotfb_boot.o`; i86xpv/i86pc unix.o links at lines 50511/53018 include `obj64/rotfb_boot.o`; i86xpv/i86pc gfx_private links at lines 51221/53927 include `obj64/rotfb_runtime.o`. The log also contains ROTFB boot/runtime and gfxp_bitmap/gfxp_fb compile commands for both architecture defines. However, the wrapper produced zero command/status/log records, so these command lines are not per-linker exit-status proof. All six targets remain INCOMPLETE, not independently verified.
* Root cause of the missing linker records: the old watcher installed the wrapper once and exited immediately; it did not monitor the linker path for replacement during nightly's tools/build phases. The wrapper reported ready at 17:07:53 UTC, while actual target link commands occurred later, and the artifact contains zero linker status records. A tool-install replacement is the likely explanation, but the old run did not preserve enough path-state data to prove the exact replacement event. Commit `9ff1c231` changes the watcher to remain alive through nightly, detect/reinstall the wrapper if the linker path changes, and restore the original linker after nightly exits.
* Local regression tests for commit `9ff1c231`: `sh -n` passes for the helper and test script. The synthetic harness passed all six success-path target invocations, wrapper reinstallation after simulated linker replacement, command/status/log pairing, ELF magic/freshness checks, and exact 10-patch order. An injected linker failure (exit 7 plus `ld: fatal`) was propagated and classified as `linker_exit_status=NONZERO`, target `FAIL`, overall `FAIL`. GitHub Build #77 also reports both the existing geometry/model test and the new linker-evidence regression test as SUCCESS.
* Important correction discovered after commit `9ff1c231`: the pinned upstream has Makefiles at `usr/src/uts/i86pc/gfx_private/Makefile` and `usr/src/uts/i86xpv/gfx_private/Makefile`. The i86pc Makefile's `GFXP_VGATEXT_SRC` points to the source directory `i86pc/io/gfx_private`, but the actual build/output directory is `uts/i86pc/gfx_private`; the i86xpv output directory is likewise `uts/i86xpv/gfx_private`. Commit `9ff1c231` incorrectly changed the two gfx_private final-output paths and wrapper cwd classifier to `.../io/gfx_private`, and the synthetic test fixture mirrors that incorrect path. This is a confirmed defect in the new evidence commit. Do not hide it or claim the current test proves the real gfx_private path. Correct the helper's target paths, wrapper cwd matching, and fixture to the Makefile output directories only after Build #77 completes.
* Build #77 is the sole active target run: Run ID `44370911917`, Job ID `135037112179`, source `9ff1c2312227f2ce88fbbea5ddf27d18bb1870cb`, status IN_PROGRESS. Steps completed: checkout, geometry/model tests PASS, linker-evidence regression tests PASS, artifact staging PASS. OpenIndiana VM build step remains IN_PROGRESS; final logs and artifact are not yet available. No other target run is queued or active.
* This entry changes only the root engineering journal. The root file is outside the workflow push path filters; this journal-only update does not trigger another build. No C source, patch, Makefile, or additional workflow change was made while Build #77 is active.

### Readiness after Build #76 / during Build #77

* BUILD VERIFIED: NO — Build #76's nightly exit was 0, but the GitHub job failed during evidence finalization; all six per-linker status records were missing. Build #77 is still running and its evidence paths contain the known gfx_private path defect.
* CODE/TEST VERIFIED: NO — the new tests exercise the evidence harness only. Direct C/lifecycle, allocation rollback, callback gate, and teardown coverage remains insufficient.
* DEVICE VERIFIED: NO — no Chuwi boot, real Multiboot capture, orientation, cursor, or stability test has been performed.

### Next action

Wait for Build #77 to complete and retrieve its full logs/artifact. Inspect whether the watcher reinstalled after tool bootstrap and whether dboot/unix.o records now contain real linker statuses. Then make one minimal correction for the gfx_private build/output directories and fixture, recheck branch HEAD and all target runs immediately before committing, and run exactly one subsequent build. Do not edit source/workflow while Build #77 is queued or active.


### 2026-11-18T20:22:00.102Z — Build #77 post-build audit and recovery commit

**Repository:** `Cadiocto/WhateverXe-MiniBook-X-N150`  
**Branch:** `gop-rotation-hook`  
**Pinned illumos upstream:** `098e95820891f8a83fae679daece7fcf58b7b3a4`

#### Build #77 authoritative result

- Run ID: `44370911917`
- Job ID: `135037112179`
- Source SHA: `9ff1c2312227f2ce88fbbea5ddf27d18bb1870cb`
- Artifact ID: `14463159509`
- Artifact SHA256: `8427f5368f3a... (see GitHub artifact metadata for full digest)`
- Workflow conclusion: `failure`
- OpenIndiana nightly exit code: `0`
- Final workflow exit code: `1`
- Patch application: `10/10`, lexical order confirmed.

The artifact contains four valid linker status records: dboot i86pc/i86xpv and unix.o i86pc/i86xpv. All four report exit status 0, fresh non-empty ELF targets and the required `rotfb_boot.o` argument.

Both gfx_private targets are **INCOMPLETE**, not PASS. The original nightly log contains their link commands (i86xpv around line 51240 and i86pc around line 53926), with `obj64/rotfb_runtime.o` in the argument list and subsequent ctfmerge/install commands. The linker wrapper did not capture those calls, so no actual linker exit status or independent freshness evidence exists for either target.

#### Confirmed root cause

The pinned upstream has the gfx_private Makefiles and final targets under:

- `usr/src/uts/i86pc/gfx_private/Makefile`
- `usr/src/uts/i86xpv/gfx_private/Makefile`
- `usr/src/uts/<arch>/gfx_private/obj64/gfx_private`

Build #77's evidence helper and its test fixture incorrectly used `usr/src/uts/<arch>/io/gfx_private` for the final target, cleanup path and wrapper working-directory classifier. The fixture duplicated the same wrong path, allowing the regression test to pass while the production build evidence missed both link calls.

#### Recovery commit

Commit: `3b58a6a350325ed660a1cb13de56cb2ed0a665e8`

Changed paths (all ROTFB-authorized):

- `.github/workflows/build-illumos.yml`
- `illumo_rotfb/rotfb_link_evidence.sh`
- `illumo_rotfb/tests/rotfb_link_evidence_test.sh`
- `illumo_rotfb/rotfb_vis_cursor.h`
- `illumo_rotfb/tests/rotfb_vis_cursor_test.c`
- `illumo_rotfb/patches/0011-gfxp-bitmap-cursor-state.patch`

The evidence helper now uses the actual gfx_private paths and requires one matching command/status/log tuple per target, exact working directory and output path, expected ROTFB object, zero linker exit status, fresh non-empty output and ELF magic. Missing or stale outputs remain INCOMPLETE; a non-zero linker status is FAIL.

Patch 0011 moves VIS cursor validation/state preparation into a portable helper used by the production gfxp_bitmap patch. It rejects negative signed VIS coordinates and non-positive dimensions before unsigned conversion, validates terminal/screen/pitch/framebuffer bounds, and commits position/origin only after all checks succeed. The caller changes visible state only after validation and rendering.

#### Local validation

- Host C regression test compiled with `cc -std=c99 -Wall -Wextra -Werror` and passed.
- Updated linker evidence test shell syntax: `sh -n` passed.
- The full linker evidence regression suite and all eleven patch applications remain pending Build #78; no OpenIndiana success is claimed for this commit.

#### Build #78

- Run ID: `44390180897`
- Source SHA: `3b58a6a350325ed660a1cb13de56cb2ed0a665e8`
- URL: https://github.com/Cadiocto/WhateverXe-MiniBook-X-N150/actions/runs/44390180897
- Initial state after push: queued.

No additional source/workflow changes are permitted until Build #78 completes and its full logs/artifact are audited. No device verification is claimed.


### 2026-11-18T20:22:16.591Z — Build #77 artifact digest correction and Build #78 observation

The full SHA256 of Build #77 artifact `14463159509` (`build77-diagnostics.zip`) is:

`8427f53600c0d64b81caff62e228eb3209d8d0c86b822421aa53d03566556330`

This replaces the abbreviated digest in the preceding entry; no artifact contents or conclusions changed.

Build #78 remains active:
- Run ID: `44390180897`
- Job ID: `135105385542`
- Source SHA: `3b58a6a350325ed660a1cb13de56cb2ed0a665e8`
- Current step: ROTFB linker evidence regression tests.

No source, patch, test, Makefile or workflow changes were made after Build #78 started. Only this journal correction was committed.


### 2026-11-18T22:00:57.739Z — Build #79 patch-context forensic result and 0011 recovery

**Repository:** `Cadiocto/WhateverXe-MiniBook-X-N150`  
**Branch:** `gop-rotation-hook`  
**HEAD before recovery:** `2c25c6dedb948ccbf72afeb3d0972a868cac6d78`  
**Pinned upstream:** `098e95820891f8a83fae679daece7fcf58b7b3a4`

#### Build #79 authoritative result

- Run ID: `44399431071`; Job ID: `135138083340`; source SHA: `2c25c6dedb948ccbf72afeb3d0972a868cac6d78`.
- Workflow conclusion: FAILURE. Geometry/model, linker-evidence, and VIS cursor C regression host steps all PASS.
- Artifact ID: `14472319186`; verified downloaded ZIP SHA256: `b9afb13cd37c5cbbf70ed0090070ff8ee016d75e48d9b24d5de1d15dc17d5cce` (matches GitHub artifact digest).
- Original `patch-series-regression.log` confirms patches 0001–0010 applied in lexical order. The altered-offset negative case failed at `gfxp_bitmap.c:714`; the actual 0011 from this source revision failed at `gfxp_bitmap.c:730`.
- OpenIndiana VM stopped in patch-series regression and SSH exited 1. Nightly was not invoked. Nightly/final exit codes and compile/link evidence are NOT AVAILABLE; no runtime-link failure is established.

#### Exact 0011 root cause

The pinned-upstream `usr/src/uts/i86pc/io/gfx_private/gfxp_bitmap.c` has a blank line between `console->fb.cursor.origin.y = ca->row;` and `bitmap_display_cursor(softc, ca);`. The 0011 hunk deleted the origin assignment and immediately expected the display call, but omitted representation of the unchanged blank line. Consequently the hunk could not match the actual post-0001–0010 source even with old-side start 730. The 14 VIS guard lines added by 0007 match the corresponding removed lines in 0011 byte-for-byte; they are not the context mismatch. This is a patch-context defect, not a runtime or linker defect.

The same hunk's new-side start was one line short. Previous hunk deltas are +1 (include), +45 (helper insertion), and -9 (lower guard replacement), net +37; old start 730 therefore maps to new start 767, not 766.

#### Minimal recovery committed

- `illumo_rotfb/patches/0011-gfxp-bitmap-cursor-state.patch`: explicitly replace the old blank line with a new blank line after the cursor update helper, avoiding a whitespace-only context line; update final hunk counts to old 34/new 15 and new-side start to 767. No behavior or architecture changed.
- `illumo_rotfb/tests/fixtures/0011-before-blank-context-fix.patch`: preserve the exact pre-fix 0011 from parent revision `2c25c6dedb948ccbf72afeb3d0972a868cac6d78` as a negative regression fixture.
- `illumo_rotfb/tests/rotfb_patch_series_test.sh`: remove the misleading sed-based offset rewrite; apply the exact pre-fix fixture after patches 0001–0010 and require its recorded failure at `gfxp_bitmap.c:730`, then apply corrected 0011. Assert exactly 11 patch files and their exact lexical order.

#### Local validation

- Build #79 artifact SHA256 independently recomputed and matched: PASS.
- Full Build #79 job log and original patch-series regression log inspected: PASS.
- Exact pinned-upstream blank-line context verified: PASS.
- Corrected unified-diff counts old 34/new 15 and new start 767 verified: PASS.
- Disposable synthetic source-context test: exact pre-fix fixture fails at `gfxp_bitmap.c:730`; corrected patch passes `git apply --check` and applies: PASS.
- Updated patch-series test `sh -n`: PASS.
- Full application of all 11 patches against the actual pinned upstream remains pending Build #80. No compile, nightly, or linker result is claimed for Build #79.
- No device commands were run. The supplied Chuwi MDB snapshot remains accepted as measured evidence; physical paddr, BAR2 aperture, kernel fb mapping, and shadow_fb remain distinct.

#### Readiness and next step

- BUILD VERIFIED: NO — patch 0011 did not apply in Build #79; nightly was not run.
- CODE/TEST VERIFIED: NO — existing VIS C and host evidence tests pass, but lifecycle/alias/allocation/rollback/callback/teardown coverage remains incomplete.
- DEVICE VERIFIED: NO — no physical-device test was performed.
- Next: run one controlled build for the recovery source revision, inspect the full patch-series log first, then nightly and all six independent linker results. Do not make another source/workflow change while that run is active.


### 2026-11-18T23:47:13Z — Build #80 BUILD VERIFIED; LIFE-001 direct C regression work

**Repository:** `Cadiocto/WhateverXe-MiniBook-X-N150`  
**Branch:** `gop-rotation-hook`  
**Source HEAD:** `8eb2a6fffc02a638c18a313469cd4aed78052b1f`  
**Pinned illumos upstream:** `098e95820891f8a83fae679daece7fcf58b7b3a4`

#### Build #80 authoritative result

- Run ID: `44407397142`; Job ID: `135166157203`; source SHA: `8eb2a6fffc02a638c18a313469cd4aed78052b1f`.
- Workflow conclusion: SUCCESS.
- Artifact ID: `14478343298`; SHA256: `0f073d60532ad522b10811e9edd81dcb4817b3df68713c30415c1b84249e29d7`.
- Artifact: 66 files, 7,378,698 bytes compressed; downloaded archive SHA256 independently recomputed and matched GitHub digest.
- Original OpenIndiana nightly log is present (176,301,411 bytes) and its completed tail shows nightly completion at 2026-11-18T22:50:57Z and `exit 0`.
- `nightly-exit-code.txt`, `final-build-exit-code.txt`, and `ld-wrapper-watch-exit-code.txt` each contain exactly bytes `30 0a` (ASCII `0\n`).
- Nightly exit code: `0`; final build/evidence exit code: `0`; linker watcher exit code: `0`.
- Patch application: **11/11**, lexical order confirmed in both `patch-apply.log` and `patch-series-regression.log`. The patch regression intentionally reproduced the old 0011 failure and applied corrected 0011; `PATCH_SERIES_STATUS=PASS`.
- All three host regression steps passed: geometry/clipping model, linker evidence, VIS cursor C tests.

#### Six real linker results

All six targets have exactly one command, one status record, and one log file associated by the same process ID. All status records report exit status 0, no command/status pair mismatches, no linker diagnostics, fresh outputs after the 2026-11-18T22:08:40Z build start, non-zero output size and valid ELF magic. The pre-clean evidence records all six targets absent before the build.

| Target | PID | Exit | Fresh output | Size |
|---|---:|---:|---|---:|
| dboot i86pc | 87357 | 0 | PASS | 77,508 |
| dboot i86xpv | 88062 | 0 | PASS | 65,104 |
| unix.o i86pc | 86354 | 0 | PASS | 19,013,320 |
| unix.o i86xpv | 87095 | 0 | PASS | 18,587,480 |
| gfx_private i86pc | 95467 | 0 | PASS | 70,360 |
| gfx_private i86xpv | 96182 | 0 | PASS | 70,904 |

Actual linker commands include `rotfb_boot.o` in both dboot and both unix.o links, and `rotfb_runtime.o` in both gfx_private links. The corresponding original nightly log includes the compiler/CTF steps and actual linker invocations. Per-target linker log files exist and are zero bytes because the successful linkers emitted no diagnostics.

#### Runtime audit correction

A prior report incorrectly listed `ROTFB-LIFE-002 — DETACH ERROR PROPAGATION` as a confirmed defect. Full patch-series review disproves that finding: patch 0005 explicitly checks `rotfb_runtime_fini()` in `gfxp_bm_detach()` and returns `DDI_FAILURE` on failure; patch 0008 propagates the error through `gfxp_fb_detach()` and defers property removal/mutex destruction until success. **LIFE-002 is not an open defect and must not be carried forward.** The earlier diagnosis was made from patch 0002 without accounting for the later 0005/0008 changes.

#### LIFE-001 direct test improvement

The production copy path in patch 0010 already skips the second shadow `memmove` when `dst == sdst` in text mode. The missing piece is a direct C regression test of the production copy-order/alias logic. A portable helper is being extracted into `illumo_rotfb/rotfb_copy.h`, called by the patched `bitmap_cons_copy()`, and tested for forward/backward overlapping moves, aliased destinations, distinct framebuffer/shadow destinations, and graphics-mode shadow-only copies.

Local disposable test:
```
cc -std=c99 -Wall -Wextra -Werror -I/tmp -o /tmp/rotfb-copy-test /tmp/rotfb_copy_test.c
/tmp/rotfb-copy-test
ROTFB bitmap copy C tests: PASS
```
The 0012 patch also passed `git apply --check` and apply against a disposable synthetic source-context fixture. Full application after patches 0001–0011 remains pending the next OpenIndiana build.

#### Readiness

- **A. BUILD VERIFIED: YES for source revision `8eb2a6fffc02a638c18a313469cd4aed78052b1f` and pinned upstream `098e958...`.** All 11 patches, nightly/final exits, six linker targets, ROTFB link inputs, output freshness and artifact digest are verified.
- **B. CODE/TEST VERIFIED: NO.** Direct alias-copy coverage is being added; allocation failure, rollback, callback and complete teardown coverage remain incomplete.
- **C. DEVICE VERIFIED: NO.** No physical Chuwi boot or display test was performed. The supplied MDB snapshot remains the only device evidence; paddr, BAR2 aperture, kernel fb mapping and shadow_fb remain distinct.

#### Next controlled change

The next source/workflow commit adds patch 0012 and a C test for the existing alias-safe row-copy logic. It also corrects the earlier LIFE-002 false positive in this journal. Before committing, recheck branch HEAD and all target runs; do not change source/workflow if any run is queued or active. One Build #81 is permitted for the resulting source SHA.

### 2026-11-18T23:48:46.775Z — LIFE-001 C regression commit and Build #81 start

- HEAD before: `8eb2a6fffc02a638c18a313469cd4aed78052b1f`.
- Source commit: `38ce3f43610c6eb96665b8a92452735d303b396c` (`test(rotfb): cover aliased bitmap row copies`).
- HEAD after: `38ce3f43610c6eb96665b8a92452735d303b396c`.
- Changed paths are limited to the ROTFB workflow, ROTFB patch series, ROTFB C/header tests, and this journal: `.github/workflows/build-illumos.yml`, `illumo_rotfb/patches/0012-gfxp-bitmap-copy-helper.patch`, `illumo_rotfb/rotfb_copy.h`, `illumo_rotfb/tests/rotfb_bitmap_copy_test.c`, `illumo_rotfb/tests/rotfb_patch_series_test.sh`, `ROTFB_ENGINEERING_LOG.md`.
- Local C regression command: `cc -std=c99 -Wall -Wextra -Werror -I/tmp -o /tmp/rotfb-copy-test /tmp/rotfb_copy_test.c && /tmp/rotfb-copy-test`; output: `ROTFB bitmap copy C tests: PASS`.
- The test calls the same `rotfb_copy_rows()` helper used by the production `bitmap_cons_copy()` patch and checks forward/backward overlap ordering, aliased destination copy count, separate framebuffer/shadow copies, graphics-mode shadow-only behavior, and invalid arguments.
- Local patch-format test: `git apply --check /tmp/0012-gfxp-bitmap-copy-helper.patch`, followed by apply and `git diff --check` on a disposable source-context fixture; result: PASS. Full exact pinned-upstream patch-series application is pending Build #81.
- Patch series is now expected to contain 12 patches. The updated VM regression checks exact order, replays the pre-fix 0011 failure, applies corrected 0011 and then 0012, and asserts the production source includes/calls `rotfb_copy_rows()`.
- Build #81: Run ID `44423856535`; Job ID `135223486150`; source SHA `38ce3f43610c6eb96665b8a92452735d303b396c`; URL https://github.com/Cadiocto/WhateverXe-MiniBook-X-N150/actions/runs/44423856535. Job is `in_progress`; no artifact yet. Nightly/final exit codes and link results are not yet available.
- Build #80 remains BUILD VERIFIED for its exact source SHA `8eb2a6fffc02a638c18a313469cd4aed78052b1f`; the new revision awaits its own validation.
- No device commands or physical Chuwi operations were performed. Next: observe Build #81 only; inspect the full patch-series log first, then nightly and all six linker records. Do not edit source/workflow while it is active.


### 2026-11-18T23:50:18.079Z — Runtime integration audit after Build #80

- Re-read pinned-upstream `gfxp_bitmap.c`, `gfxp_fb.c`, `framebuffer.h`, and `visual_io.h`, plus the complete ROTFB patches 0002, 0005, 0006, 0007, 0008, 0010 and 0011.
- Confirmed the detach-error path is already correct after the full series: `gfxp_bm_detach()` checks `rotfb_runtime_fini()` under `rotfb_info.configured`; patch 0008 makes `gfxp_fb_detach()` return on failure before removing `primary-controller` or destroying the mutex. The earlier LIFE-002 diagnosis remains retracted.
- Confirmed runtime initial synchronization is not a self-copy: `rotfb_runtime_init()` copies the boot logical shadow into a newly allocated runtime shadow, publishes the separate WC mapping, and calls `rotfb_runtime_flush_all()` before opening the polled callback gate.
- Confirmed a redundant but harmless self-copy after runtime initialization: `bitmap_devinit()` calls `bitmap_copy_fb(softc, console->fb.fb, console->fb.shadow_fb)`; ROTFB sets both pointers to the same runtime shadow. `bitmap_copy_fb()` therefore performs row-wise `memmove(p,p,pitch)`. This is a no-op/performance observation, not a correctness failure; no behavior change made.
- Confirmed the polledio function pointers are installed by `gfxp_bm_attach()`, but are returned to the terminal emulator only at the end of `bitmap_devinit()`, after `rotfb_runtime_init()` has set the callback gate to managed/open. This removes the suspected pre-init callback race for the normal attach path; no race defect established.
- LIFE-001 production guard in patch 0010 skips the second shadow copy when `dst == sdst` in text mode. Commit `38ce3f43610c6eb96665b8a92452735d303b396c` factors this exact row-copy decision into `rotfb_copy_rows()` and adds direct C coverage. Build #81 is still active, so the exact pinned-upstream application and VM compile remain unverified.
- No source/workflow changes made during Build #81. The only write in this audit was this root journal update; it is outside workflow path filters.
- Next: wait for Run `44423856535` / Job `135223486150`; verify patch 0012, 12/12 application, host C output, nightly/final exit bytes, all six real link tuples and artifact digest before considering any further source change.


### 2026-11-19T00:52:29.786Z — Build #81 failure: 0012 omitted the 0005 errno include context; minimal recovery

- Repository: `Cadiocto/WhateverXe-MiniBook-X-N150`; branch: `gop-rotation-hook`; pinned upstream: `098e95820891f8a83fae679daece7fcf58b7b3a4`.
- Live HEAD before recovery: `55ac30933ebe6b76a32d8f173e420ac2b4345112`. The only commits after Build #81 source `38ce3f43610c6eb96665b8a92452735d303b396c` were journal-only; no unexpected source changes were found. The target workflow run list contained no queued or active run before the source change.
- Build #81: Run `44423856535`, Job `135223486150`, source `38ce3f43610c6eb96665b8a92452735d303b396c`, conclusion FAILURE. Artifact ID `14484520671`, SHA256 `7e9a19e84f3760a7f81b38e685b545c0907f7e3005922417f1ccde7e5c06a38a`; downloaded archive was independently hashed and matched the GitHub digest.
- Full job log and original `patch-series-regression.log` inspected. Host geometry/model, linker-evidence, VIS cursor C, and bitmap-copy C tests all reported PASS. In the pinned OpenIndiana patch-series test, patches 0001–0010 applied, corrected 0011 applied, then 0012 failed at `usr/src/uts/i86pc/io/gfx_private/gfxp_bitmap.c:29`. SSH returned 1. Nightly was not started; nightly/final exit files and all compile/link evidence are absent. This is a patch-context failure, not a compiler or runtime-link failure.
- Exact root cause: patch 0005 adds `#include <sys/errno.h>` between `#include <sys/rgb.h>` and `#include <sys/rotfb.h>` in `gfxp_bitmap.c`. After 0002/0005/0011, the include order is rgb, errno, rotfb, rotfb_vis_cursor, gfx_private. Patch 0012's first hunk incorrectly required rgb to be followed immediately by rotfb, omitting the errno line. The failure is a missing context line, not an offset-only problem.
- Minimal correction: update only the 0012 include hunk to include the existing errno line and use the seven-line post-0011 context; add fixture `illumo_rotfb/tests/fixtures/0012-before-errno-context.patch` containing the exact failing pre-fix patch; extend the real pinned-upstream patch-series test to require that fixture to fail at line 29 and corrected 0012 to apply. Correct the stale patch-count diagnostic from “exactly 11” to “exactly 12” (the numeric assertion already required 12).
- Files changed in this recovery: `illumo_rotfb/patches/0012-gfxp-bitmap-copy-helper.patch`, `illumo_rotfb/tests/rotfb_patch_series_test.sh`, new pre-fix fixture, and this journal. No production C behavior, geometry, mapping, or upstream pin is changed.
- Build #80 remains BUILD VERIFIED only for source SHA `8eb2a6fffc02a638c18a313469cd4aed78052b1f`. Build #81 source SHA is not BUILD VERIFIED. Full 12-patch application, nightly, and six linker results for the corrected revision remain pending its one push-triggered build.


### 2026-11-19T00:56:23.831Z — Build #81 failure diagnosed; 0012 context corrected; Build #82 active; runtime audit evidence

- Branch HEAD before journal update: `6200b38994f332428f85263a627142531ebbd76a`; source commit: `6200b38994f332428f85263a627142531ebbd76a`; upstream pin unchanged: `098e95820891f8a83fae679daece7fcf58b7b3a4`. The source recovery commit was fast-forwarded from `55ac30933ebe6b76a32d8f173e420ac2b4345112`; the branch now has one active target run, Build #82. No source/workflow edits are being made while it runs.
- Build #81 (Run `44423856535`, Job `135223486150`, source `38ce3f43610c6eb96665b8a92452735d303b396c`) failed in the OpenIndiana patch-series regression. Artifact `14484520671`, SHA256 `7e9a19e84f3760a7f81b38e685b545c0907f7e3005922417f1ccde7e5c06a38a`; downloaded archive SHA256 independently matched. Original log confirms host geometry, linker-evidence, VIS C and bitmap-copy C tests PASS; patches 0001–0010 and corrected 0011 applied, then 0012 failed at `gfxp_bitmap.c:29`. SSH exit 1; nightly was not started; nightly/final exit codes and linker evidence are absent.
- Root cause is confirmed from pinned upstream and patch sequence: 0005 inserts `#include <sys/errno.h>` between `sys/rgb.h` and `sys/rotfb.h`; 0012 omitted that existing line from its include context. Commit `6200b38994f332428f85263a627142531ebbd76a` corrects the 0012 hunk to the exact post-0005/post-0011 include order, adds the exact pre-fix 0012 patch as a regression fixture, and makes the pinned-upstream patch-series test assert that old 0012 fails at line 29 while corrected 0012 applies. The stale patch-count error text now correctly says 12. Changed source/test paths: `illumo_rotfb/patches/0012-gfxp-bitmap-copy-helper.patch`, `illumo_rotfb/tests/rotfb_patch_series_test.sh`, `illumo_rotfb/tests/fixtures/0012-before-errno-context.patch`; no production C behavior or geometry changed in this fix.
- Build #82: Run `44432538778`, Job `135253242559`, source SHA `6200b38994f332428f85263a627142531ebbd76a`, URL https://github.com/Cadiocto/WhateverXe-MiniBook-X-N150/actions/runs/44432538778. At this entry, run is IN_PROGRESS. Geometry/model, linker-evidence, VIS cursor C and bitmap-copy C host steps all completed PASS; OpenIndiana VM step is still running. Patch-series outcome, nightly/final exit codes, six linker tuples, artifact ID and SHA256 are not yet available. No additional run was created by this journal update (verified after commit).
- WC ordering audit against exact upstream: `usr/src/common/atomic/amd64/atomic.S:564-567` implements `membar_producer()` with `sfence; ret`; `usr/src/man/man9f/membar_ops.9f:56-60` defines the producer barrier as ordering prior stores before later stores. `usr/src/uts/i86pc/Makefile.files:145-190` defines `BOOT_DRIVER_OBJS` as compiled for dboot and unix, but `DBOOT_OBJS` contains dboot assembly/startup plus those boot-driver objects and does not include the common amd64 atomic assembly object. Thus the dboot unresolved `membar_producer` is expected; the local `__asm__ __volatile__("sfence" ::: "memory")` in `rotfb_boot.c` matches the pinned amd64 producer-barrier instruction and compiler ordering constraint. Runtime code uses the kernel `membar_producer()` implementation. Build #80 previously confirmed both dboot links with this fix, for its exact source SHA only.
- Physical mapping audit: pinned `gfxp_map_kernel_space()` in `usr/src/uts/i86pc/io/gfx_private/gfxp_vm.c:85-132` maps the supplied physical address through `hat_devload()`; WC mode selects `HAT_MERGING_OK | HAT_PLAT_NOCACHE`. It does not validate the address against a PCI BAR resource. `gfxp_check_for_console()` in `gfxp_fb.c` treats nonzero bootloader-provided `fb_info.paddr` as framebuffer metadata and does not establish BAR identity. Therefore the Chuwi equality `paddr == BAR2 base` remains correlation only; physical paddr, BAR2 aperture, kernel WC mapping and shadow pointer remain distinct. No claim that they are identical is made.
- Runtime integration audit: `rotfb_runtime_init()` validates boot metadata, maps the physical paddr WC, allocates a separate runtime shadow, copies the imported boot logical image into it, publishes the mapping/pointers, flushes the full image, and only then opens the polled callback gate. `gfxp_bm_attach()` stores polledio callbacks, but `bitmap_devinit()` exposes `data->polledio` only after `bitmap_setup_fb()` returns; in ROTFB mode that setup calls runtime_init first. The normal attach path therefore does not expose callbacks before initialization. `rotfb_runtime_fini()` validates state, closes the gate, drains active callbacks, flushes WC writes, copies the runtime shadow back to the boot-owned shadow, restores pointers/geometry and unmaps/frees runtime resources. No new lifecycle defect is established by this read-only pass.
- Remaining B-level test gaps: direct production C tests for allocation failure/unmap rollback, initial logical-to-physical synchronization and clipping, callback gate/quiesce/drain, and complete fini/teardown rollback. Existing VIS test checks position/origin unchanged through the shared helper, but its `visible` field is not passed into that helper; a future callback-level test should explicitly prove visible state remains unchanged on rejected display/hide requests. No source changes for these gaps while Build #82 is active.
- Readiness: A BUILD VERIFIED remains YES only for source SHA `8eb2a6fffc02a638c18a313469cd4aed78052b1f` (Build #80). Build #81 and current Build #82 revisions are not BUILD VERIFIED until their own full evidence. B CODE/TEST VERIFIED is NO; C DEVICE VERIFIED is NO. No physical Chuwi commands were run.
- Next: observe Build #82 only; inspect its full original patch-series log, then nightly/final exit bytes, all six process-correlated linker command/status/log records, fresh ELF outputs and artifact digest. Do not change source/workflow until Build #82 completes.


### 2026-11-19T01:04:48Z — Build #82 diagnosed; 0012 flush-call context corrected; Build #83 active

- Branch HEAD before this journal update: `a9312ec431ef8f15e356f5bf88390543f2fe0b76`; source fix commit: `a9312ec431ef8f15e356f5bf88390543f2fe0b76`; parent: `42d8e48613ddfab3be5a3192f1dfe9b1a2d7a08e`; upstream pin remains `098e95820891f8a83fae679daece7fcf58b7b3a4`. The only source changes in the commit are the ROTFB patch, patch-series test and one regression fixture.
- Build #82: Run `44432538778`, Job `135253242559`, source `6200b38994f332428f85263a627142531ebbd76a`, FAILURE. Artifact `14486488932`; SHA256 `adf39c2cf97f58aca717097c2af83fc1aad64fc994a2bc95ad3c5edb31013c5d`. Downloaded archive independently hashed to the same value. Original job log and `patch-series-regression.log` inspected. Geometry/model, linker-evidence, VIS cursor C and bitmap-copy C host tests PASS. Patches 0001–0010 and corrected 0011 applied; 0012 then failed at `usr/src/uts/i86pc/io/gfx_private/gfxp_bitmap.c:379`. SSH exited 1. The artifact contains only collector-started, patch-series-regression, Perl audit, runner indexes and VM-started files; no nightly log, patch-apply completion log, exit-code files or linker evidence. Nightly/final exit codes are therefore NOT RUN / unavailable, not zero.
- Exact root cause established by comparing pinned upstream patch 0002 with 0012's failed hunk: patch 0002 adds the `rotfb_runtime_flush_rect()` call with the width and height arguments on separate continuation lines. Patch 0012's third hunk expected those two expressions collapsed onto one line. Its include hunk and declaration hunk were compatible; the copy-loop hunk's trailing context was not. This is a formatting/context mismatch, not a compiler, linker or runtime defect.
- Minimal correction committed as `a9312ec431ef8f15e356f5bf88390543f2fe0b76`: change only the 0012 trailing context to preserve the two-line call and update the third hunk counts from old/new 31/9 to 32/10. Add exact pre-fix fixture `illumo_rotfb/tests/fixtures/0012-before-flush-context-fix.patch` (the previous 0012 with correct errno include but collapsed flush-call context). Extend `illumo_rotfb/tests/rotfb_patch_series_test.sh` to require that fixture to fail at line 379 and then require corrected 0012 to apply. Existing pre-fix errno-context fixture remains, so both historical 0012 failures are independently reproduced.
- Pre-commit checks: source hunk parser reported 0012 hunks old/new counts 7/8, 7/6, 32/10. Commit diff confirms only `illumo_rotfb/patches/0012-gfxp-bitmap-copy-helper.patch`, `illumo_rotfb/tests/fixtures/0012-before-flush-context-fix.patch`, and `illumo_rotfb/tests/rotfb_patch_series_test.sh` changed. No local full pinned-upstream patch-series run was possible in this runner; the authoritative test is in the OpenIndiana VM run below.
- Build #83: Run `44434077768`, Job `135258410111`, source `a9312ec431ef8f15e356f5bf88390543f2fe0b76`, URL https://github.com/Cadiocto/WhateverXe-MiniBook-X-N150/actions/runs/44434077768. At this journal entry it is IN_PROGRESS. Geometry/model step PASS; linker-evidence step IN_PROGRESS; VIS and bitmap-copy C steps pending; OpenIndiana VM pending. Patch count/order, nightly/final exit codes, six link tuples, artifact ID and SHA256 are not yet available.
- Readiness: A BUILD VERIFIED remains YES only for source SHA `8eb2a6fffc02a638c18a313469cd4aed78052b1f` (Build #80). Build #82 source SHA failed before nightly; source SHA `a9312ec` is pending its own Build #83 evidence. B CODE/TEST VERIFIED remains NO; allocation failure, initial sync, callback drain/quiesce and complete teardown tests are incomplete. C DEVICE VERIFIED remains NO.
- Next: observe Build #83 only. Inspect its original full job log and artifact, require all 12 patches to apply in lexical order, then verify nightly/final exit bytes, six process-correlated linker command/status/log records and fresh ELF outputs. No source/workflow edits while Build #83 is queued or active.


### 2026-11-19T01:06:11Z — Read-only runtime copy, flush and callback audit during Build #83

- Branch HEAD before journal update: `c678a599ee540b17404e594bee57c4dda807f892`; source under validation: `a9312ec431ef8f15e356f5bf88390543f2fe0b76`; upstream pin unchanged. Build #83 Run `44434077768`, Job `135258410111` remains IN_PROGRESS; all four host test steps have completed PASS and the OpenIndiana VM step is active. No source/workflow files changed during this audit.
- Reviewed `rotfb_copy_rows()` and its production caller in patch 0012. With separate physical and shadow mappings in text mode it copies both destinations; when the pointers alias it skips the duplicate shadow write; in graphics mode it updates only shadow. Row traversal direction follows `toffset <= soffset`, while each row uses memmove for horizontal overlap. Caller bounds in patch 0007 constrain width, height, source/destination coordinates, pitch and framebuffer size before helper pointer arithmetic. No additional alias-ordering defect was proven in this pass.
- Reviewed boot/runtime flush clipping. Both flush routines reject zero/out-of-range origins, clip width/height using subtraction from fixed logical bounds before forming end coordinates, and use 64-bit row/column offsets. Runtime flush checks owner, published pointers, geometry, mapping sizes and runtime_ready before writes; its producer barrier follows the pixel loop. No new clipping/offset defect was established.
- Reviewed runtime init/fini sequence. Init validates metadata, maps WC physical paddr, allocates the runtime shadow, copies the boot logical image, saves original state, marks callbacks managed, publishes runtime pointers, flushes the full image, then opens the callback gate. Fini validates state before quiescing, closes the gate, drains the callback count, flushes while mapping is live, copies runtime shadow back to the boot-owned logical buffer, restores pointers/geometry, then unmaps/frees. The normal `bitmap_devinit()` path assigns `data->polledio` only after `bitmap_setup_fb()` returns, and ROTFB setup completes runtime_init before that return. This supports the existing callback-publication ordering analysis; it is not a stress test.
- Test gap refined: `rotfb_vis_cursor_test.c` includes a `visible` member in its state snapshot, but `rotfb_vis_cursor_update()` does not accept or mutate that member. Its unchanged value therefore does not prove production `bitmap_cons_cursor()` preserves visible state on rejected DISPLAY/HIDE callbacks. B-level coverage still needs a callback-level production helper/test. Runtime allocation failure, rollback, initial-sync pixel equivalence, callback quiesce/drain and complete teardown are also not directly exercised by C tests.
- The `bitmap_devinit()` call to `bitmap_copy_fb(fb, shadow_fb)` is a self-copy in ROTFB mode because runtime_init publishes both pointers as the same runtime shadow. It is redundant but no functional failure is established; no change made.
- No new source defect was confirmed, so no source commit was created. Next: continue observing Build #83, then use its actual patch-series/nightly/link evidence before considering a separate C-testability refactor. Physical paddr/BAR2 identity remains unproven; no device or MMIO access performed.


### 2026-11-19T01:07:58Z — CI-EVIDENCE-001 follow-up: patch-count finalizer is stale at 11 while series is 12

- Branch HEAD before journal update: `639ee2c70a2a9ff3a89de22e7338d1f9d30ab163`; Build #83 source SHA: `a9312ec431ef8f15e356f5bf88390543f2fe0b76`; upstream pin unchanged. Build #83 Run `44434077768`, Job `135258410111` remains IN_PROGRESS in the OpenIndiana VM. No source/workflow changes made.
- Confirmed a second deterministic evidence defect by reading the exact current helper and workflow. The VM workflow applies every `illumo_rotfb/patches/*.patch` file and the patch-series test now requires 12 patches. The current `rotfb_link_evidence.sh finalize` still hardcodes `APPLIED == 11` and an expected order ending at `0011-gfxp-bitmap-cursor-state.patch`. It will therefore set `overall_status=FAIL` when the full 12-patch series is successfully applied, even if nightly and all six links succeed. The same stale 11-entry list remains in `rotfb_link_evidence_test.sh::write_patch_evidence()`, so its synthetic PASS does not cover the actual 12-patch series.
- Exact relevant code: helper `finalize` counts `^APPLIED ` from `patch-apply.log`, compares to 11 and the 0001–0011 order; workflow loop at `.github/workflows/build-illumos.yml` applies the lexical `*.patch` glob and writes one APPLIED line per patch before nightly. This is a confirmed CI evidence mismatch, not a compile/link defect. Build #83 may reach nightly and six real links but still be marked FAIL at final evidence until the helper and its test fixture are updated to 12.
- No changes are permitted while Build #83 is active. After it completes, inspect original log/artifact first, then minimally update only `illumo_rotfb/rotfb_link_evidence.sh` and `illumo_rotfb/tests/rotfb_link_evidence_test.sh` (and journal): expected count/order must include `0012-gfxp-bitmap-copy-helper.patch`; synthetic patch evidence must contain all 12 entries. Add a test that fails if helper's expected patch count/order drifts from the actual patch directory.
- Current Build #83 host tests are PASS, but this does not prove the OpenIndiana patch series or final evidence. Nightly/final/link results remain pending. A BUILD VERIFIED remains only for Build #80 source SHA `8eb2a6fffc02a638c18a313469cd4aed78052b1f`.


### 2026-11-19T01:09:01Z — Synthetic hunk regression for Build #82 line-379 context completed

- During Build #83 (Run `44434077768`, Job `135258410111`, source `a9312ec431ef8f15e356f5bf88390543f2fe0b76`, still IN_PROGRESS), ran a disposable local git-apply regression using the exact 0012 third-hunk preimage and the two-line flush-call formatting from patch 0002.
- Command outcome: pre-fix patch with collapsed flush-call context failed at `gfxp_bitmap.c:379`; corrected 0012 patch passed `git apply --check`, applied successfully, and the resulting file contained `rotfb_copy_rows()`. Output: `SYNTHETIC_FLUSH_CONTEXT_REGRESSION_PASS`.
- This validates the specific context correction only. It is a synthetic hunk fixture, not the full pinned-upstream patch-series test; Build #83's OpenIndiana VM remains the authoritative 12-patch validation.
- No source/workflow files changed. The 11-versus-12 patch-count finalizer defect recorded above remains unresolved until Build #83 completes. No nightly, final build or linker result is claimed for source `a9312ec` yet.


## ROTFB ENGINEERING — Build #83 finalizer diagnosis, Build #84 harness regression, Build #85 validation

* Timestamp UTC: 2026-11-19T02:19:19.581Z
* Branch: `gop-rotation-hook`
* Pinned upstream: `098e95820891f8a83fae679daece7fcf58b7b3a4`
* HEAD before journal entry: `b30b33152bda3215068dbfa8c20aec1cb0c0a8ef`
* Source revisions and commits:
  * Build #83 source: `a9312ec431ef8f15e356f5bf88390543f2fe0b76`
  * Build #84 source: `24471962a3116d2042b4cc0d59732d964222e14d`
  * Corrected test source: `b30b33152bda3215068dbfa8c20aec1cb0c0a8ef`

### Build #83 — run 44434077768 / job 135258410111

* Result: workflow FAILURE; source patch series and nightly did complete.
* Artifact: `14490329910`; SHA256 `14f1a0a6198ac40eec57ceb0bcfcb92a5698e513e5c2230bb6e4102e5488178e` (downloaded and independently hashed).
* Patch application: 12/12 in lexical order, all `APPLIED`; patch-series regression ended `PATCH_SERIES_STATUS=PASS`.
* Nightly exit code: `0`; file bytes `30 0a`.
* Linker watcher exit code: `0`; file bytes `30 0a`.
* Final build/finalizer exit code: `1`; file bytes `31 0a`.
* Six linker targets: all independently reported PASS, exit status 0, exactly one command/status/log tuple each, matching PID, expected working directory/output, ROTFB object argument present, fresh nonzero ELF output with valid ELF magic:
  * dboot i86pc — PASS; PID 91188; `rotfb_boot.o` present.
  * dboot i86xpv — PASS; PID 91666; `rotfb_boot.o` present.
  * unix.o i86pc — PASS; PID 90268; `rotfb_boot.o` present.
  * unix.o i86xpv — PASS; PID 90841; `rotfb_boot.o` present.
  * gfx_private i86pc — PASS; PID 100634; `rotfb_runtime.o` present.
  * gfx_private i86xpv — PASS; PID 100698; `rotfb_runtime.o` present.
* Each linker .log file exists but is zero bytes; the captured linker invocations completed silently. The .cmd and .status files are the primary per-process evidence, paired by PID.
* Compile command evidence recorded three matching commands for each of the eight ROTFB/gfxp compile selectors (boot, runtime, gfxp_bitmap, gfxp_fb × i86pc/i86xpv).
* Confirmed root cause: `rotfb_link_evidence.sh` still expected 11 patches and an order ending at 0011. The actual patch log correctly contained 12 and ended at `0012-gfxp-bitmap-copy-helper.patch`. The finalizer wrote `patch_order_match=NO`, `overall_status=FAIL` despite six PASS links and nightly exit 0. This is an evidence/finalizer defect, not a compiler/linker failure.
* Build #83 does NOT satisfy A because the workflow/finalizer and final-build exit code failed.

### Build #84 — run 44443244672 / job 135289032633

* Result: workflow FAILURE before OpenIndiana VM.
* Artifact: `14492665486`; SHA256 `b0043ae7d6a4ee68699216395ae90ef08a71b912e7a658f12243810f869b1ed9` (downloaded and independently hashed; 751-byte runner-only diagnostic archive).
* Geometry/model test: PASS.
* Linker evidence regression test: FAIL at line 188 with `Syntax error: ")" unexpected (expecting "fi")`, exit code 2.
* VIS C test, bitmap-copy C test, OpenIndiana VM: SKIPPED.
* Nightly/final build and linker results: NOT RUN / UNAVAILABLE.
* Root cause: the test file committed in `24471962a3116d2042b4cc0d59732d964222e14d` was malformed by a replacement-string expansion; the fetched file had 1344 lines and duplicated/truncated shell blocks. This was an error in the autonomous edit, not an upstream defect. No attempt was made to hide or reinterpret this failure.

### Corrective source commit for Build #85

* Commit: `b30b33152bda3215068dbfa8c20aec1cb0c0a8ef`.
* Changed path only: `illumo_rotfb/tests/rotfb_link_evidence_test.sh`.
* Rebuilt the test from the clean 220-line parent version, then applied bounded replacements. Current file is 244 lines.
* New test reads `EXPECTED_ORDER` from the production helper, compares it against the sorted actual `illumo_rotfb/patches/*.patch` directory, and requires exactly 12 names in lexical order. The fixture now emits APPLIED records from that same checked order rather than a stale literal list.
* Added a negative `shortpatch` scenario: truncate the 12-entry evidence to 11 and require the real finalizer to return failure with `patches_applied=11`, `patch_order_match=NO`, and `overall_status=FAIL`.
* Static content validation before commit: 244 lines; helper expected count 12; actual patch directory count 12; exact order equality; dynamic fixture, drift guard and short-patch negative case present. A local shell execution was not available; actual shell execution is pending Build #85.

### Build #85 — active

* Run: `44443394283`; job: `135289541389`; source SHA: `b30b33152bda3215068dbfa8c20aec1cb0c0a8ef`.
* At journal time: run IN_PROGRESS; geometry/model test PASS; linker evidence regression step IN_PROGRESS; later C tests and OpenIndiana VM not yet completed.
* No nightly/final/link/artifact claims are made for #85.

### Readiness and next step

* A BUILD VERIFIED remains YES only for Build #80 source `8eb2a6fffc02a638c18a313469cd4aed78052b1f`.
* Build #83 has six confirmed successful link outputs but failed finalizer; Build #84 failed in host shell syntax; Build #85 must independently pass the corrected tests and full OpenIndiana acceptance.
* B CODE/TEST VERIFIED remains PARTIAL. Allocation failure, rollback, initial-sync equivalence, callback gate/drain and full teardown tests remain.
* C DEVICE VERIFIED remains NOT VERIFIED. No physical Chuwi operation was performed.
* Next action: observe Build #85, fetch full job log/artifact on completion, then audit all 12 patches, nightly/final exit bytes and six link tuples before any further source change.


## ROTFB RUNTIME TEST DESIGN — production runtime C harness (read-only audit)

* Timestamp UTC: 2026-11-19T02:20:00.320Z
* Branch: `gop-rotation-hook`
* HEAD before journal entry: `519c8bec9bc1744fbc941155fe877fd70f6b8a52`
* Active source: `b30b33152bda3215068dbfa8c20aec1cb0c0a8ef`
* Active Build #85: run `44443394283`, job `135289541389`; host geometry, linker evidence, VIS C and bitmap-copy C steps are PASS; OpenIndiana VM is IN_PROGRESS. No nightly/link/artifact claims yet.

### Files inspected

* `illumo_rotfb/rotfb_runtime.c`, blob `8d6e8000262fd97aee1325f0067a0f6871f39db5`.
* `illumo_rotfb/rotfb.h`, blob `2dfdda3c14f957e5666fc9f5bf9ece9598ea7208`.
* `illumo_rotfb/rotfb_copy.h`, blob `f57759cdfc881278563e152707e150aaf6303f6c`.
* `illumo_rotfb/rotfb_vis_cursor.h`, blob `bf828cf0d8636585eea3d92f22b5569c68f8901e`.
* `illumo_rotfb/tests/rotfb_bitmap_copy_test.c` (111 lines) and `rotfb_vis_cursor_test.c` (102 lines).
* Lifecycle patches 0005 and 0008; VIS patch 0010; bitmap-copy patch 0012.

### Confirmed code behavior and remaining coverage gaps

* `rotfb_runtime_init()` maps the physical WC surface first, allocates a separate shadow, unmaps the just-created mapping if shadow allocation fails, copies the boot logical framebuffer into the runtime shadow, publishes pointers, flushes the full logical surface, then opens the callback gate.
* `rotfb_runtime_flush_rect()` validates the published framebuffer state, rejects zero/out-of-range origins, clips width/height using subtraction before end-coordinate addition, applies clockwise mapping `px = physical_width - 1 - y`, `py = x`, writes four bytes per pixel, then calls `membar_producer()`.
* `rotfb_runtime_fini()` validates owner/pointers before teardown, sets the quiescing bit, waits for active callback count to drain, flushes while the WC mapping is still valid, copies runtime shadow back to the boot-owned framebuffer, restores pointers/geometry, then unmaps and frees allocations. The callback gate remains managed and closed after fini.
* Existing `rotfb_bitmap_copy_test.c` exercises the production `rotfb_copy_rows()` helper, including overlap direction and aliased destinations.
* Existing `rotfb_vis_cursor_test.c` exercises the production validation/update helper and compares position/origin/visible state in its local struct. However, `visible` is not passed to `rotfb_vis_cursor_update()`; this does not prove that the production VIS callback leaves its visible flag unchanged after rejection.
* No direct C test currently executes the production `rotfb_runtime.c` lifecycle. Allocation failure/unmap rollback, initial-sync pixel equivalence, clipping canaries, callback gate/drain and full fini/teardown are not yet directly tested.

### Proposed next test implementation (not changed in this run)

* Compile the actual `rotfb_runtime.c` into a host C test translation unit using minimal illumos compatibility headers/stubs for atomic primitives, kmem, gfxp map/unmap, bcopy, delay and framebuffer types. Do not create a parallel model of the lifecycle.
* Inject map failure and shadow-allocation failure; assert return status, exact map/unmap/free counts, unchanged published pointers and readiness flags.
* Run successful init with the real fixed 1200x1920 / 1920x1200 geometry, patterned boot shadow and canary-protected physical/shadow buffers; assert selected/all pixels follow the production clockwise transform and row pitches.
* Exercise clipped and overflowing rectangles through the production `rotfb_runtime_flush_rect()`; assert canaries and untouched pixels remain unchanged.
* Exercise callback enter/exit, quiescing rejection and fini drain; a deterministic stub `delay()` can complete an injected active callback so the real production wait loop is executed without a timing race.
* Exercise pointer-mismatch rollback: fini must return failure without freeing mappings or changing published pointers. Then run successful fini and assert restored pointers, copied-back shadow contents, one unmap/free, and rejection of stale callbacks.

### Validation status

* This entry records source inspection and a test design only. No runtime C harness was added or executed; no new PASS is claimed.
* Build #85 remains the only active target run. No source/workflow change was made during it.
* Next step: inspect Build #85 original log/artifact when complete; only then add the production-runtime harness and one source commit if its design can be kept isolated and testable.


## ROTFB RUNTIME C TEST HARNESS DESIGN — additional source audit

* Timestamp UTC: 2026-11-19T03:09:45.355Z
* Branch: `gop-rotation-hook`
* HEAD before journal entry: `f1bf05f4b3a467500b863829bc2c9d35e09f736a`
* Active source revision: `b30b33152bda3215068dbfa8c20aec1cb0c0a8ef`
* Build #85: run `44443394283`, job `135289541389`; geometry/model, linker-evidence, VIS cursor C and bitmap-copy C host steps PASS; OpenIndiana VM remains IN_PROGRESS. No completed nightly, final-build, six-link or artifact result is claimed.

### Production runtime audit (read-only)

Inspected `illumo_rotfb/rotfb_runtime.c`, `rotfb_boot.c`, patch 0007 callback wrappers, patches 0005/0008 lifecycle handling, patch 0011 cursor state and the current C tests.

* Runtime init maps the physical WC surface, allocates a separate logical shadow, unmaps the just-created mapping if shadow allocation fails, copies the boot logical image, saves the boot-owned pointers/geometry, publishes the runtime surfaces, performs a full flush, then opens the callback gate.
* Runtime flush clips width/height by subtraction before calculating end coordinates. With the fixed 1920x1200 logical geometry, the clockwise mapping is `px = 1199 - y`, `py = x`; byte offsets use 64-bit intermediates. The function ends with `membar_producer()`.
* Runtime fini validates ownership and all published pointers before closing the gate. It then sets QUIESCING, waits for active callback references to drain, flushes while the WC mapping is valid, copies the runtime shadow back to the boot-owned logical shadow, restores saved pointers/geometry, then unmaps and frees the runtime allocations. Validation failure occurs before gate closure and leaves the runtime state intact.
* The three polled callback wrappers in patch 0007 call `rotfb_runtime_callback_enter()`, reject a negative result, invoke the production copy/display/cursor callback, and call `rotfb_runtime_callback_exit()` after return. The unmanaged state returns zero and permits the legacy path; after runtime management is enabled, the gate controls callback entry.
* No new functional runtime defect was established in this read-only pass. No production source was changed.

### Direct production C harness design (not yet implemented)

The next useful test is to compile the actual `rotfb_runtime.c` translation unit against minimal host compatibility headers, not a parallel model. The harness can provide stubs for framebuffer types, atomic operations, `gfxp_map_kernel_space`/`gfxp_unmap_kernel_space`, `kmem_zalloc`/`kmem_free`, `bcopy`, `delay` and `membar_producer`.

Planned direct cases:

1. Inject map failure; require DDI_FAILURE, no shadow allocation, no pointer publication and no unmap of an uncreated mapping.
2. Inject shadow allocation failure; require exactly one unmap and unchanged boot pointers/readiness.
3. Successful init with a patterned 1920x1200 logical shadow; compare all 2,304,000 BGRA pixels against the 1200x1920 physical buffer using the production flush routine and clockwise transform.
4. Exercise edge clipping with UINT32_MAX width/height and invalid origins against guard bytes around the fake mapping.
5. Corrupt one published pointer before fini; require DDI_FAILURE without gate closure, unmap/free or pointer mutation, then restore it and complete fini.
6. Hold a real callback reference, run fini on a pthread, wait until new callback entries are rejected, release the held reference, and verify fini drains before freeing resources.
7. Verify fini copies the runtime image back to the boot shadow, restores geometry/pointers, unmaps/frees exactly once, rejects stale callbacks, and permits a later clean re-init.

The existing VIS C test checks the helper's position/origin state but does not execute `bitmap_cons_cursor()`; its local `visible` member is not passed to `rotfb_vis_cursor_update()`. A callback-level test still needs either a host harness around the production callback or a production state-transition helper used by that callback.

### Validation and next step

* This is source inspection and test design only; no runtime harness was added or executed and no new PASS is claimed.
* No source/workflow changes were made while Build #85 is active.
* Next: retrieve Build #85's full original job log and artifact after completion; verify 12-patch application, nightly/final exit codes, and six real linker command/status/log/output tuples. Only then implement the direct runtime C harness if the build is complete and the branch remains unchanged.


## 2026-11-19T04:03:59.480Z — Build #85 BUILD VERIFIED; production runtime C harness added

* Branch: `gop-rotation-hook`; pinned upstream unchanged: `098e95820891f8a83fae679daece7fcf58b7b3a4`.
* HEAD before this change: `e7dac153447a67ccef4e1d156ca83ddcde3853fc`. The three commits after Build #85 source `b30b33152bda3215068dbfa8c20aec1cb0c0a8ef` change only this journal; GitHub compare reports no source/workflow path changes.
* Build #85: Run `44443394283`, Job `135289541389`, source `b30b33152bda3215068dbfa8c20aec1cb0c0a8ef`, workflow SUCCESS. Artifact `14494733806`; SHA256 `2a013a41e91e3320e0f6563440e6b548c405775b37a601b4f80b7ae2fccbac36`. The artifact was downloaded and its SHA256 independently matched GitHub's digest.
* Patch-series regression on the exact pinned upstream: patches 0001–0012 applied in lexical order; pre-fix 0011 context failure and both pre-fix 0012 context failures were reproduced; corrected 0011/0012 applied; `PATCH_COUNT=12`, `UPSTREAM_PIN=098e95820891f8a83fae679daece7fcf58b7b3a4`, `PATCH_PATH_ALLOWLIST=PASS`, `PATCH_SERIES_STATUS=PASS`.
* Host regression steps: geometry/model, linker evidence, VIS cursor C, and bitmap-copy C all PASS. OpenIndiana VM, runner finalization and artifact upload all completed successfully.
* Exit-code files were independently inspected: `nightly-exit-code.txt`, `final-build-exit-code.txt`, and `ld-wrapper-watch-exit-code.txt` each contain exactly bytes `30 0a` (value 0 plus LF).
* Nightly's original gate log ends with `Ended OS-Net source build` at 2026-11-19 03:22:50 UTC; targeted scan found no compiler/build/link error diagnostics. Nightly exit is 0; final build exit is 0.
* All six real link invocations have one matching command/status/log tuple, identical process IDs, zero pair mismatches, object argument present, linker exit 0, no linker diagnostics, fresh nonempty ELF output:
  * dboot i86pc PID 91677, 77,508 bytes, `rotfb_boot.o`: PASS.
  * dboot i86xpv PID 92607, 65,104 bytes, `rotfb_boot.o`: PASS.
  * unix.o i86pc PID 90215, 19,013,320 bytes, `rotfb_boot.o`: PASS.
  * unix.o i86xpv PID 91952, 18,587,480 bytes, `rotfb_boot.o`: PASS.
  * gfx_private i86pc PID 100170, 70,352 bytes, `rotfb_runtime.o`: PASS.
  * gfx_private i86xpv PID 101257, 70,896 bytes, `rotfb_runtime.o`: PASS.
* Compile-command capture contains three lines per architecture for each of rotfb_boot, rotfb_runtime, gfxp_bitmap and gfxp_fb. This is supplementary evidence; link acceptance is based on the actual linker status records and fresh ELF outputs above.
* A BUILD VERIFIED is now YES for source revision `b30b33152bda3215068dbfa8c20aec1cb0c0a8ef`. It was not inferred from host tests or command presence. This does not establish Chuwi runtime/device behavior.
* Production runtime C harness was developed locally against the exact production `illumo_rotfb/rotfb_runtime.c` blob `8d6e8000262fd97aee1325f0067a0f6871f39db5`. Command: `sh illumo_rotfb/tests/rotfb_runtime_test.sh`. It compiles the production C file with minimal host illumos/DDI stubs and tests invalid geometry rejection before mapping, mapping failure, allocation failure plus unmap rollback, initial shadow import, all 2,304,000 logical pixels mapped clockwise to physical coordinates, clipping/canaries including UINT32_MAX dimensions, pointer-mismatch rollback, callback gate quiesce/drain using pthread, stale callback rejection, successful fini/re-init and copy-back teardown. Result: `ROTFB production runtime C tests: PASS`. Repeated ten consecutive runs: PASS. Local ASan+UBSan run: PASS. These are host-stub tests, not an illumos kernel execution or proof of the platform memory model.
* Files added in this commit: `illumo_rotfb/tests/rotfb_runtime_test.c`, `illumo_rotfb/tests/rotfb_runtime_test_compat.h`, `illumo_rotfb/tests/rotfb_runtime_test.sh`. `.github/workflows/build-illumos.yml` adds one host step to run the production runtime harness. No production C source, patch, Makefile, geometry or mapping behavior is changed.
* Readiness: A BUILD VERIFIED YES for the Build #85 source SHA; B CODE/TEST VERIFIED remains PARTIAL until this harness passes in CI and production callback-level cursor visibility plus remaining lifecycle coverage are complete; C DEVICE VERIFIED NOT VERIFIED.
* Next: validate this test/workflow commit with exactly one Build #86, inspect its full log/artifact, then continue B-level tests. Keep paddr/BAR2/framebuffer mapping identity unresolved and do not touch the physical Chuwi.


## 2026-11-19T05:21:44Z — Build #86 BUILD VERIFIED; AUTONOMOUS_HANDOFF_BACK

* Branch: `gop-rotation-hook`; pinned upstream verified unchanged: `098e95820891f8a83fae679daece7fcf58b7b3a4`.
* Live branch HEAD before this journal-only handoff entry: `12c07ec04828da8a2b1b7922f01971244470682d`. Its parent is `e7dac153447a67ccef4e1d156ca83ddcde3853fc`; its commit tree is `8b420323692c6205fc41d14a53310a609648daad`. Commit changes only `.github/workflows/build-illumos.yml`, `ROTFB_ENGINEERING_LOG.md`, and the three production-runtime host test files `illumo_rotfb/tests/rotfb_runtime_test.c`, `rotfb_runtime_test.sh`, `rotfb_runtime_test_compat.h`. No unrelated subsystem paths are present.
* Build #86: Run `44455747294`, Job `135330215494`, source SHA `12c07ec04828da8a2b1b7922f01971244470682d`, event `push`, workflow conclusion SUCCESS, attempt 1. Job steps 1–11 (checkout, five host regression steps, artifact staging, OpenIndiana VM, runner finalization and artifact upload) completed successfully.
* Artifact `14500198763`, name `illumos-framebuffer-build-logs-44455747294`, size 7,097,224 bytes. GitHub digest: `sha256:430b5366df545c88cece2a6b008a70098329a90a9ad199838182a6fb18bb298d`. Downloaded to the analysis container; independent `sha256sum` returned exactly `430b5366df545c88cece2a6b008a70098329a90a9ad199838182a6fb18bb298d`.
* Original job log was fetched in full (1,675,529 characters / 12,430 lines). It records PASS for geometry/model, linker evidence, VIS cursor C, bitmap copy C, and the new production runtime lifecycle C regression step. The runtime step printed `ROTFB production runtime C tests: PASS`. This is a host build of the actual production `rotfb_runtime.c` against minimal illumos/DDI compatibility stubs; it is not execution in the illumos kernel or on the Chuwi.
* Exact pinned-upstream patch regression: `PATCH_COUNT=12`, `UPSTREAM_PIN=098e95820891f8a83fae679daece7fcf58b7b3a4`, `PATCH_PATH_ALLOWLIST=PASS`, `PATCH_SERIES_STATUS=PASS`. The 12 patches applied in lexical order: 0001 framebuffer metadata; 0002 boot framebuffer rotation; 0003 gfxp bitmap TEM rotation; 0004 Makefile files; 0005 lifecycle hardening; 0006 console contract/copy safety; 0007 bitmap bounds hardening; 0008 detach failure safety; 0009 wide row offsets; 0010 VIS safety; 0011 cursor state; 0012 bitmap copy helper. The separate actual OpenIndiana patch-apply log also records CHECK/APPLIED for every patch in this order.
* Nightly original log: `gate-logs/log.2026-11-19.05_06/nightly.log` (176,301,436 bytes). It records `Ended OS-Net source build` at 2026-11-19 05:06:33 UTC and `Nightly distributed build completed` at 05:06:50 UTC. Targeted scan of the original log found zero occurrences of fatal compiler errors, undefined symbols, `ld: fatal:`, dmake failures, missing make targets, or syntax errors. This scan supplements, and does not replace, the actual exit and linker records.
* Exit-code files were inspected byte-for-byte: `nightly-exit-code.txt`, `final-build-exit-code.txt`, and `ld-wrapper-watch-exit-code.txt` each contain exactly `30 0a` (ASCII `0\n`); all three values are 0.
* All six real linker command/status/log tuples independently PASS. Every tuple has one command, one status, one log record, zero pair mismatches, the expected object argument, exit status 0, no linker diagnostics, a fresh-after-build-start output and valid ELF magic:
  * dboot i86pc — PID 92736, output 77,508 bytes, `dboot/obj64/rotfb_boot.o`: PASS.
  * dboot i86xpv — PID 94019, output 65,104 bytes, `dboot/obj64/rotfb_boot.o`: PASS.
  * unix.o i86pc — PID 91380, output 19,013,320 bytes, `obj64/rotfb_boot.o`: PASS.
  * unix.o i86xpv — PID 92514, output 18,587,480 bytes, `obj64/rotfb_boot.o`: PASS.
  * gfx_private i86pc — PID 102457, output 70,352 bytes, `obj64/rotfb_runtime.o`: PASS.
  * gfx_private i86xpv — PID 102914, output 70,896 bytes, `obj64/rotfb_runtime.o`: PASS.
* Compile-command capture contains three entries per architecture for `rotfb_boot`, `rotfb_runtime`, `gfxp_bitmap` and `gfxp_fb`. These are supplementary; the six link results above are based on matching process command/status/log records and fresh ELF outputs, not command presence alone.
* Result: A BUILD VERIFIED = YES for source revision `12c07ec04828da8a2b1b7922f01971244470682d`. This is a new independent build proof for #86, not merely inherited from Build #85.
* B CODE/TEST VERIFIED remains PARTIAL, not complete. Direct production `rotfb_runtime.c` host coverage now passes: invalid geometry before mapping; map failure; allocation failure and WC unmap rollback; boot-shadow import; all 2,304,000 logical pixels checked against clockwise physical mapping; clipped flush and guard regions; `UINT32_MAX` dimensions; pointer-mismatch rollback; callback gate quiesce/drain with pthread; stale callback rejection; successful fini/re-init and copy-back teardown. The runtime test uses host stubs; its `membar_producer()` stub is a C atomic release fence, not the actual x86 WC/SFENCE execution.
* Remaining B-level gaps: (1) execute the production `bitmap_cons_cursor()` callback and prove invalid DISPLAY/HIDE requests leave the actual `cursor.visible` field and position/origin unchanged; the existing VIS test exercises the validation/update helper only. (2) exercise production `bitmap_cons_copy()` end-to-end around the `rotfb_copy_rows()` helper, including mode and flush integration; current direct C test targets the helper. (3) retain/extend attach/detach integration tests in the actual driver lifecycle beyond the runtime-core host stubs. No new production runtime defect was established in this final read-only audit.
* C DEVICE VERIFIED = NOT VERIFIED. No commands were run on the physical Chuwi and no device boot/orientation/console/cursor/stability test was performed. The owner-supplied MDB values remain unchanged: physical paddr `0x4000000000`, fb_size `0x8ca000`, pitch `4800`, 1200x1920, kernel fb and shadow_fb distinct. Whether paddr is actually framebuffer memory or merely overlaps BAR2 remains unresolved.
* Latest Actions list at handoff: Build #86 Run `44455747294` completed SUCCESS for source `12c07ec...`; #85 `44443394283` SUCCESS; #84 `44443244672` FAILURE; #83 `44434077768` FAILURE; #82 `44432538778` FAILURE; #81 `44423856535` FAILURE; #80 `44407397142` SUCCESS. The latest ten runs were all completed; no queued/in_progress target run existed at handoff. The branch ref still pointed to `12c07ec04828da8a2b1b7922f01971244470682d` immediately before this journal update.
* AUTONOMOUS_HANDOFF_BACK: the scheduled ten-run cycle is complete. Return write ownership to the primary project agent/owner. Final source revision handed back: `12c07ec04828da8a2b1b7922f01971244470682d`; final verified build: Run `44455747294`, Job `135330215494`, Artifact `14500198763`, SHA256 `430b5366df545c88cece2a6b008a70098329a90a9ad199838182a6fb18bb298d`. Remote branch tree at the handoff source revision is the committed tree `8b420323692c6205fc41d14a53310a609648daad`; no uncommitted remote working-tree changes exist. This final update changes only this journal file and is outside the workflow path filters; verify no new Actions run is triggered after the update.
* Next recommended engineering task for the primary agent: implement callback-level C coverage around production `bitmap_cons_cursor()` and integration coverage around production `bitmap_cons_copy()`, then continue attach/detach fault-injection tests. Do not modify framebuffer geometry, clockwise mapping, WC mapping semantics, or BAR/framebuffer assumptions without new evidence.


## 2026-11-19 — Production bitmap callback harness: Build #87 compile failure correction

* Starting source revision: `e95495722f44443996f9b6615994db4b805fa7de` on `gop-rotation-hook`; pinned illumos upstream remains `098e95820891f8a83fae679daece7fcf58b7b3a4`.
* Build #87 (`44472385618`, job `135384743650`) reached the callback integration stage after all 12 patches applied and production callback extraction passed. The host C compiler then rejected signed/unsigned comparison diagnostics in the extracted, unchanged production callback bodies because the harness used `-Werror`. The actual VIS API defines `screen_pos_t` and `screen_size_t` as signed `short`; the harness compatibility header correctly preserves those types. The warnings are not evidence of a runtime defect and production source is not to be rewritten to silence the host compiler.
* Harness compile policy is adjusted to retain `-Wall -Wextra -Werror`, explicitly enable `-Wsign-compare`, and exempt only that warning class from becoming an error via `-Wno-error=sign-compare`. All other compiler warnings remain fatal; the real signed VIS types remain unchanged.
* Cursor coverage is strengthened to inspect every pixel in each 8x16 cursor rectangle on both the runtime shadow and physical surface. A direct call to production `bitmap_display_cursor()` verifies that it changes the shadow only; a subsequent call to production `rotfb_runtime_flush_rect()` verifies publication of the entire rectangle to the physical surface. The production VIS callback path separately verifies display, hide, move, edge placement, invalid requests, and KD_GRAPHICS behavior.
* Copy callback tests are now gated on a completely passing cursor stage; they do not run after a cursor failure. The copy stage continues to exercise the extracted production `bitmap_cons_copy()`, production `rotfb_copy_rows()`, and production runtime flush path.
* No production callback, patch, geometry, mapping, or shadow architecture is changed. This entry records the harness correction only; no PASS is claimed here. The corrected callback binary has not yet been executed, and no new OpenIndiana build has been started.


## 2026-11-19 — Callback harness assertion review before execution

* During pre-CI review of the newly added direct `bitmap_display_cursor()` case, found that the rectangle assertion helper initially compared both surfaces against one expected value. That is incorrect for the intended intermediate state: the production cursor drawing primitive updates the runtime shadow, while the separate physical surface must remain unchanged until `rotfb_runtime_flush_rect()` is called.
* Corrected the test helper to accept independent expected values for shadow and physical memory. The direct primitive case now requires shadow=cursor-inverted and physical=original before flush; after the production runtime flush, both must equal cursor-inverted. Hide/restore and KD_GRAPHICS cases check the full rectangle on both surfaces.
* This was a test expectation correction found before execution, not a production defect. No production source or patch changed; no PASS is claimed. The harness still awaits its first execution after the Build #87 host-compile correction.


## 2026-11-19 — Build #88: production bitmap callback integration and OpenIndiana build

* Run: `44484097492`; Job: `135423155355`; Artifact: `14513261641`; artifact SHA-256: `b48a60e75ee60ee05b47233b05cdabd2a6a5509087c642ad5a80db1b82a8a2ad`.
* Run conclusion: SUCCESS. Source commit: `a61c2ffb1f41d651a11c7f1cf9e719cad250a924`; branch: `gop-rotation-hook`; pinned illumos upstream: `098e95820891f8a83fae679daece7fcf58b7b3a4`. Build started at `2026-11-19T08:08:55Z`; final workflow update at `2026-11-19T08:57:03Z`.
* The callback harness applied all 12 patches and extracted the production `gfxp_bitmap.c` bodies. Patched source SHA-256: `d4e3c4bdffffd2c2417e4f3610bb6527bb912a987b4bbdddfeeaa3e8bab0fcef`. Extracted functions: `bitmap_cons_copy`, `bitmap_cursor_geometry`, `bitmap_cursor_valid`, `bitmap_cursor_update`, `bitmap_display_cursor`, `bitmap_cons_cursor`, and `bitmap_polled_cursor`.
* Host compile command uses `-Wall -Wextra -Werror -Wno-error=sign-compare -Wsign-compare` only for the callback harness. The eight signedness diagnostics remain visible as warnings; other warnings remain fatal. The real signed VIS types are retained. The command successfully compiled and ran the binary; no global CI flags or production C were modified.
* Callback log reports `ROTFB production cursor callback tests: PASS`, `ROTFB production copy callback tests: PASS`, `ROTFB production bitmap callback tests: PASS`, and `ROTFB_PATCHED_PRODUCTION_CALLBACK_TEST=PASS`. Explicit execution record: `bitmap_polled_cursor`, `bitmap_cons_cursor`, `bitmap_display_cursor`, `bitmap_cons_copy`, `rotfb_runtime_flush_rect`, and `rotfb_copy_rows`. The runtime source `rotfb_runtime.c` was compiled into the harness; the flush function was not stubbed.
* Cursor readback: the harness checked all 128 pixels in each tested 8x16 region in the distinct runtime shadow and physical allocations. Direct `bitmap_display_cursor()` changed shadow pixels while physical pixels stayed at their previous values; a subsequent production `rotfb_runtime_flush_rect()` made the physical rectangle match. VIS display and hide callback paths also matched both surfaces; cursor visible/position/origin state and invalid-request no-write behavior passed. Edge placement at logical (1912,1184) was checked through physical readback using the clockwise transform `physical_x = 1199 - logical_y`, `physical_y = logical_x`.
* Copy readback: production `bitmap_cons_copy()` and `rotfb_copy_rows()` passed non-overlap, downward overlap, upward overlap, horizontal overlap, KD_TEXT/KD_GRAPHICS, row move-count, flush-count, and invalid-coordinate checks. Physical readback was explicitly asserted for non-overlap, downward overlap, and horizontal overlap; the upward-overlap case asserted shadow pixels and one flush but did not independently assert the corresponding physical pixels. Guard bytes were checked at the end of the harness.
* Coverage qualification for follow-up: the scenario labelled cursor move hides the old cursor before displaying at the new origin; a direct visible-to-visible relocation without an intervening hide is not yet covered. `VIS_GET_CURSOR` checks returned coordinates and zero flushes but does not currently compare full framebuffer checksums around the request. These are test-coverage gaps, not observed production failures. No production fix was warranted by Build #88.
* OpenIndiana VM/nightly did run after the callback tests passed. The artifact records `nightly_exit_code=0`, `final_build_exit_code=0`, `ld-wrapper-watch-exit-code=0`, and `overall_status=PASS`. Six independent final linker records passed: `dboot.i86pc`, `unix.o.i86pc`, `gfx_private.i86pc`, `dboot.i86xpv`, `unix.o.i86xpv`, and `gfx_private.i86xpv`. Each has one matched linker invocation/status/output record, fresh ELF output, linker exit status 0, no nonzero invocation, and no linker diagnostics. The six targets include the expected ROTFB boot/runtime object arguments. Patch regression reports all 12 patches applied in the expected order.
* Workflow job steps 3–12 all completed successfully, including the patched production callback integration test, OpenIndiana VM build, diagnostics finalization, and artifact upload. The original Build #87 job remains a failure at callback host compilation; its VM step was skipped. Build #88 is the successful recovery run.
* Diff from Build #87 source `e95495722f44443996f9b6615994db4b805fa7de` to Build #88 source `a61c2ffb1f41d651a11c7f1cf9e719cad250a924` contains only `ROTFB_ENGINEERING_LOG.md`, `illumo_rotfb/tests/rotfb_bitmap_callback_test.c`, and `illumo_rotfb/tests/rotfb_bitmap_callback_test.sh`. No production source, patch, framebuffer geometry, mapping, upstream pin, firmware or unrelated subsystem changed.
* Classification: **BUILD VERIFIED** for Build #88 and the six recorded link targets; **CODE/TEST VERIFIED** for the production callbacks and scenarios listed above, subject to the explicit coverage gaps; **DEVICE VERIFIED: NOT VERIFIED**. No physical Chuwi MiniBook X N150 test was performed. No further build was started by this journal update.


## 2026-11-19 — Callback-only Run #1 follow-up: TEM lifecycle and independent scenarios

* Owner decision: direct `VIS_DISPLAY_CURSOR(A) → VIS_DISPLAY_CURSOR(B)` without `VIS_HIDE_CURSOR` is exploratory only and is not a required ROTFB contract. No production callback change is authorized or made on that basis.
* Required cursor scenario A explicitly runs `VIS_DISPLAY_CURSOR(A) → VIS_HIDE_CURSOR(A) → VIS_DISPLAY_CURSOR(B)`. It checks every pixel of both 8×16 rectangles in runtime shadow and physical framebuffer, cursor state/coordinates, neighboring pixels, one production flush per callback operation, and guard regions.
* Scenario B (`VIS_GET_CURSOR`) is independent of A. It establishes a known visible B position, checks returned coordinates, compares complete shadow and physical framebuffer checksums before/after, and verifies that GET adds no flush.
* Scenario C (upward-overlap `bitmap_cons_copy()`) is invoked unconditionally after A and B, regardless of their failure counts. It executes extracted production `bitmap_cons_copy()`, production `rotfb_copy_rows()`, and production `rotfb_runtime_flush_rect()`; validates source, destination and neighboring pixels by shadow and physical readback, plus guard regions.
* Direct DISPLAY→DISPLAY relocation remains exploratory. The complete old and new 8×16 rectangles are classified independently on shadow and physical surfaces; pixel readbacks are emitted and a stale old cursor image is reported rather than made a CI failure. Exploratory results do not contribute to the required aggregate exit code. Test-only cleanup restores the test surfaces after recording the observation.
* The C harness emits separate `SCENARIO_A_RESULT`, `SCENARIO_B_RESULT`, and `SCENARIO_C_RESULT` records. The shell runner extracts them into separate artifact files while retaining original compiler/runtime stdout, stderr and exit codes.
* Scope: only the callback C harness, its shell runner and this append-only journal are changed. No production source, any of the 12 production patches, workflow YAML, geometry, clockwise mapping, shadow architecture or upstream pin is changed. The callback-only workflow path filter matches; the main Build #89 workflow path filter does not.
* Validation state before CI: pending the single callback-only push run. No second callback-only run is authorized. Build #89 must not run. No PASS is claimed before the original CI artifact is inspected.


## 2026-11-19 — Callback-only Run #2: TEM lifecycle and independent copy verification PASS

* Source commit under test: `29a4031495bb6cd3fc956be1d65aabc0d5e263d4`; callback-only Run #2: `44566559986`; Job: `135709802692`; event: push; conclusion: SUCCESS; attempt: 1. No second attempt or follow-up callback run was started.
* Artifact: `14552182368`, `rotfb-callback-only-44566559986`, 23,793 bytes. GitHub SHA-256 and independent local `sha256sum` both equal `3937846c23f91ec315993bd0ffa2d1d9169eb233c5f8de53908727df252df543`. The original artifact was downloaded and inspected. It contains original job/harness console output, compiler/runtime stdout and stderr, compiler/runtime/harness exit-code files, all 12 patch check/apply logs and exit codes, and separate A/B/C/exploratory result files.
* Static validation: workflow YAML PASS; shell syntax PASS; Python extractor syntax PASS. Exact illumos upstream pin fetched and verified as `098e95820891f8a83fae679daece7fcf58b7b3a4`. All 12 production patches passed `git apply --check` and applied in order. `PATCH_COUNT=12`.
* Production callback extraction PASS against patched `gfxp_bitmap.c`, SHA-256 `d4e3c4bdffffd2c2417e4f3610bb6527bb912a987b4bbdddfeeaa3e8bab0fcef`. The extracted production functions include `bitmap_cons_copy`, `bitmap_display_cursor`, `bitmap_cons_cursor`, and `bitmap_polled_cursor`. The harness compiled unchanged production `rotfb_runtime.c`; it did not mock `rotfb_runtime_flush_rect()`.
* Compiler exit 0, runtime exit 0, callback harness exit 0. Eight signed/unsigned comparison warnings from the extracted production bodies remain visible; only `-Wsign-compare` is non-fatal in this host harness. No production warning suppression or source modification was introduced.
* **Scenario A — REQUIRED TEM CURSOR LIFECYCLE: PASS, 0 failures.** Actual production sequence: `VIS_DISPLAY_CURSOR(A) → VIS_HIDE_CURSOR(A) → VIS_DISPLAY_CURSOR(B)`. Full 8×16 shadow and physical readback passed for A display, A restore, and B display. After HIDE A, first pixel was `0x11223344` on both shadow and physical; after DISPLAY B, first pixel was `0xeeddccbb` on both. Cursor visible state and character/pixel coordinates passed; neighboring pixels and 64-byte guard regions passed. Each callback operation issued exactly one expected production flush.
* **Scenario B — VIS_GET_CURSOR: PASS, 0 failures.** Returned coordinates were B = (24, 48). Shadow checksum before/after was identical: `0x7c23ea07acd9f383`. Physical checksum before/after was identical: `0xf2947c82195df383`. GET issued no additional flush.
* **Scenario C — UPWARD-OVERLAP COPY: PASS, 0 failures.** Production `bitmap_cons_copy()`, `rotfb_copy_rows()`, and `rotfb_runtime_flush_rect()` executed. Target shadow and physical readback both equalled `0x00003001, 0x00003002, 0x00003003, 0x00003004`. Original lower source row, all seeded left/right/upper/lower neighbors, and guard regions passed on shadow and physical surfaces. The callback performed exactly one destination flush; the independent test setup flush published sentinels before the operation.
* **Exploratory DISPLAY A → DISPLAY B without HIDE: STALE_OLD_CURSOR_REMAINS.** The complete old A rectangle remained cursor-inverted on both shadow and physical surfaces; the new B rectangle was also cursor-inverted on both. First-pixel readbacks for old/new shadow/physical were all `0xeeddccbb`. This observation is retained in its own artifact result file and did not affect the required aggregate exit code. It is not classified as a production defect under the owner's current VIS/TEM contract decision.
* Explicit execution record: `bitmap_polled_cursor`, `bitmap_cons_cursor`, `bitmap_display_cursor`, `bitmap_cons_copy`, `rotfb_runtime_flush_rect`, and `rotfb_copy_rows`.
* Scope audit: source diff from the previous commit contains only the callback C harness and shell runner plus this journal. No production C, production patch, workflow YAML, framebuffer geometry, clockwise mapping, shadow framebuffer architecture, or upstream pin changed. The callback-only workflow ran; the main illumos build workflow did not run. **Build #89 was not started.** No OpenIndiana VM/nightly build was part of this callback-only run.
* Classification: callback scenarios A/B/C **CODE/TEST VERIFIED on host harness** for the exact tested production callbacks and runtime flush; callback-only CI **PASS**. Direct DISPLAY→DISPLAY is exploratory only and remains observed stale. **DEVICE VERIFIED: NOT VERIFIED** — no physical Chuwi execution was performed.


## 2026-11-19 — Runtime lifecycle audit: production callback race and reattach fault-injection coverage

* Read-only audit baseline: branch HEAD before this test-only change was `dddc36a17a9dd5eed36e2de8189cca1c25bdc8bb`. Build #88 production source and all 12 production patches are unchanged since `a61c2ffb1f41d651a11c7f1cf9e719cad250a924`; the four intervening commits only add callback-only CI/harness files and journal entries. Upstream pin remains `098e95820891f8a83fae679daece7fcf58b7b3a4`.
* Existing Build #88 runtime C test passed geometry validation, map failure, allocation failure/unmap rollback, initial full CW sync, clipping/canaries, pointer-mismatch rollback, callback gate drain, stale callback rejection before reinit, and one fini/reinit/fini cycle. Its drain test holds a raw callback reference token; it does not execute a real production callback concurrently with fini.
* The callback-only harness is extended test-only to run scenario D: block a real extracted production `bitmap_polled_copy()` inside its production `memmove`, start the real production `rotfb_runtime_fini()` on another thread, prove the gate closes and fini cannot unmap/free while the callback is held, release the callback, then prove fini completes and restores boot pointers.
* Scenario D also tests repeated fini rejection without double release; invokes the real production polled callback after fini and verifies it is rejected without framebuffer writes; fault-injects WC map failure and shadow allocation failure on a subsequent reattach; verifies map/unmap/allocation/free counts and boot metadata after each failure; then verifies successful reinit, idempotent init, second fini, and repeated fini.
* These additions compile the exact production `rotfb_runtime.c` and extracted patched production callback bodies with host compatibility stubs. The synchronization thread, allocator/map stubs and memory barrier remain host-only; this does not execute illumos DDI or physical WC instructions.
* No production source, production patch, framebuffer geometry, clockwise mapping, shadow architecture, VIS contract, upstream pin, main build workflow or unrelated subsystem is changed. Only the isolated `.github/rotfb-callback-tests` harness/runner and this append-only journal are in the write set. The push should trigger callback-only CI only; Build #89 must not run.
* CI result pending. No production defect is asserted before this regression test executes.


## 2026-11-19 — Runtime lifecycle audit: Run #3 harness compile correction

* Callback-only Run #3 (`44575044354`, commit `c72d81c94a73bf217122beb06b4a73c75f12b19c`) failed at host C compilation before runtime execution. All 12 production patches applied and production callback extraction passed.
* Exact failure: the added test called `bitmap_polled_copy()`, but the isolated callback extractor intentionally exports only the production callback entry points already selected for this harness; `bitmap_polled_copy` is not in that extraction set. This is a test harness symbol-availability error, not a production source failure.
* Corrected the test-only race injection to use the already-extracted real `bitmap_polled_cursor()` callback and block inside the host `membar_producer()` implementation reached through the real production `rotfb_runtime_flush_rect()`. The callback remains active at the WC barrier while the production fini thread closes the gate and waits. No extractor expansion or production modification is required.
* CI Run #3 artifact: `14556501827`, digest `62e63eae245aa5a55fd647cd3470ae6e406a83e440ffed6b52859ab983f6dff9`. Compiler exit 1 due the undefined test-only symbol above; no runtime test executed. No production defect or PASS is claimed from Run #3.
* Next validation is a single corrected callback-only run. Build #89 remains prohibited.


## 2026-11-19 — ROTFB runtime lifecycle, attach/detach and memory-safety audit

### Revision and scope

* Audit baseline before lifecycle test extension: `dddc36a17a9dd5eed36e2de8189cca1c25bdc8bb`. Latest production-code revision remains `a61c2ffb1f41d651a11c7f1cf9e719cad250a924` (Build #88 source). Test-only lifecycle commits: `c72d81c94a73bf217122beb06b4a73c75f12b19c`, `ff3b60610340251fbc7d266ff9741a6beb48e69d`, and `0dc1e2cec97db81f207709df587fb965303bf0a9`.
* Compare Build #88 source to `0dc1e2c`: only `.github/rotfb-callback-tests/**` and `ROTFB_ENGINEERING_LOG.md` differ. No `illumo_rotfb/**` production source or patch changed. All 12 production patches and upstream pin `098e95820891f8a83fae679daece7fcf58b7b3a4` are unchanged.
* Current callback-only Run #5: `44575376064`, Job `135741359480`, commit `0dc1e2cec97db81f207709df587fb965303bf0a9`, SUCCESS, attempt 1. Artifact `14555293610`, 24,076 bytes, SHA-256 `fa14d42a30aca13941770a58a577840375df7793018341fbe2612b710e67c0bc`; independently downloaded and verified with local `sha256sum`. Compiler, runtime and harness exit codes are all 0. YAML, shell and Python syntax checks PASS; exact upstream pin verified; all 12 patches passed check/apply.
* Run #3 (`44575044354`) failed before runtime because the new test called `bitmap_polled_copy()`, which was not part of the callback extractor's selected function set. Run #4 (`44575289699`) failed before runtime because the test variable still had `struct vis_conscopy` type after switching to `bitmap_polled_cursor()`. Both were test-only compile defects; neither reached runtime nor indicated a production failure. Run #5 is the corrected successful execution.

### Lifecycle state diagram

```text
BOOT_CONFIGURED
  -> BOOT_SHADOW_ALLOCATED
  -> BOOT_READY
  -> DDI_ATTACH: gfxp_bm_attach installs polledio function pointers
  -> VIS_DEVINIT: bitmap_devinit -> bitmap_setup_fb -> rotfb_runtime_init
       -> validate boot fb metadata
       -> WC map acquired
       -> runtime shadow allocated
       -> original fb/rotfb pointers and metadata saved
       -> callback gate marked managed and remains closed
       -> runtime pointers/metadata published; runtime_ready = true
       -> initial full shadow-to-physical flush
       -> callback gate opened
  -> RUNTIME_READY / TEM may use polledio
  -> VIS_DEVFINI (ioctl serialized by softc->lock) or DDI_DETACH
       -> validate live ownership and pointer state
       -> close callback gate
       -> drain active polled callbacks
       -> final full flush while WC mapping is valid
       -> runtime_ready = false; copy runtime image back to boot shadow
       -> restore fb_info and rotfb_info boot pointers/metadata
       -> clear runtime ownership globals
       -> unmap WC mapping
       -> free runtime shadow
       -> callback gate remains managed and closed
  -> BOOT_READY / DETACHED
  -> a later VIS_DEVINIT may acquire resources and reopen the gate
```

* DDI_ATTACH itself does not allocate the ROTFB runtime shadow or WC mapping. The actual runtime resource acquisition occurs later in VIS_DEVINIT. `gfxp_bm_attach()` assigns polledio callbacks and ops; `bitmap_devinit()` returns `data->polledio` only after `bitmap_setup_fb()` and runtime initialization succeed.
* TEM's `VIS_DEVFINI` path is present in the production patch and calls `gfxp_bm_detach()`. The generic ioctl path holds `softc->lock`; therefore VIS_DEVINIT and VIS_DEVFINI are serialized against other ioctls. Polled callbacks do not take that mutex; they use the atomic gate. The DDI_DETACH path relies on DDI device detach/open-handle serialization and is not independently executed in the host harness.

### Resource ownership

| Resource | Acquisition | Release / restoration | Owner and audit result |
|---|---|---|---|
| Boot logical shadow | bootops `bsys_alloc()` in `rotfb_boot_shadow_init()` | Not freed by runtime fini; runtime image is copied back into it | Boot allocator / firmware-lifetime memory; intentionally retained |
| Runtime shadow | `kmem_zalloc(logical_size, KM_NOSLEEP)` | `kmem_free(shadow, shadow_size)`, only after callback drain and pointer restoration | ROTFB runtime; allocation/free counts verified |
| WC physical mapping | `gfxp_map_kernel_space(physical_paddr, physical_size, GFXP_MEMORY_WRITECOMBINED)` | `gfxp_unmap_kernel_space(mapping, map_size)`, only after callback drain and final flush | ROTFB runtime; map/unmap counts and mapping canaries covered by Build #88 runtime C test |
| Framebuffer metadata | Existing boot-owned `fb_info_t`; no separate allocation | Saved changed fields restored by `rotfb_runtime_fini()` | Global framebuffer/boot state; pointer and geometry restoration checked |
| Callback state | Static gate/managed atomics; polledio function pointers stored in softc | Gate closed after fini; function pointers are not explicitly cleared | No separately allocated callback resource; stale callbacks during the closed interval are rejected |

### Attach and initialization failure matrix

| Stage / failure | Observed/structural behavior | Evidence |
|---|---|---|
| Invalid physical/logical geometry or metadata | Rejected before mapping; no allocation or published pointer change | Build #88 runtime C test |
| WC mapping returns NULL | Init fails; no shadow allocation and no unmap of a nonexistent mapping | Build #88 and Run #5 fault injection |
| Shadow allocation fails after successful mapping | Init fails and the just-acquired WC mapping is unmapped exactly once; no shadow free | Build #88 and Run #5 fault injection |
| Failure after successful shadow allocation | No later fallible operation currently returns an init error: pointer publication and initial flush are non-failing/void. There is therefore no post-allocation rollback branch to exercise in current code | Static production review; residual observability limitation |
| Callback registration failure | `gfxp_bm_attach()` only assigns callback/function pointers and has no allocation or failure-returning registration operation. `VIS_DEVINIT` exports polledio only after runtime init succeeds | Static review; no fault point exists in current integration |
| VIS_DEVINIT runtime init failure | `bitmap_setup_fb()` returns failure; runtime init's map/allocation rollback leaves boot pointers and readiness intact | Static integration review plus runtime C fault injection |
| TEM rejects parameters after successful VIS_DEVINIT | Upstream TEM calls VIS_DEVFINI on version/depth/mode rejection after successful device init; VIS_DEVFINI routes through the runtime fini path | Pinned upstream `tem.c` review and production patch review |

### Rollback, detach and reinitialization

* `rotfb_runtime_init()` validates geometry, sizes, paddr and all expected boot pointers before resource acquisition. Mapping and shadow allocation happen before any live framebuffer metadata is changed. The allocation-failure branch unmaps the acquired WC mapping and returns without publishing runtime pointers.
* After both resources exist, the routine snapshots the fields it changes, marks callback management active while the gate is closed, publishes the runtime pointers and metadata, marks runtime_ready, performs the initial flush, and only then opens the gate.
* `rotfb_runtime_fini()` validates live pointers and sizes before closing the gate. Pointer mismatch returns failure without releasing resources; Build #88 test verified the live mapping/shadow remain usable after that rollback refusal.
* Successful fini closes the gate, waits for active references, flushes the full runtime image while the WC mapping remains valid, copies the logical image to the boot-owned shadow, restores `fb_info_t` and `rotfb_info` pointers/metadata, clears runtime globals, unmaps, then frees the runtime shadow.
* `gfxp_bm_detach()` invokes runtime fini only when ROTFB is configured and runtime_ready is true. After VIS_DEVFINI has already completed, a subsequent DDI_DETACH sees runtime_ready false and is a no-op success. Patch 0008 makes generic DDI_DETACH return immediately on bitmap detach failure before removing the primary-controller property or destroying the softc mutex.
* Direct repeated `rotfb_runtime_fini()` is not idempotent-success: it returns DDI_FAILURE when runtime_ready is already false. The caller-level detach path is idempotent because it checks runtime_ready before calling fini. Run #5 verified repeated direct fini does not double-unmap or double-free.
* Run #5 verified init idempotence while ready, init -> fini, stale callback rejection after fini, failed reattach at both map and shadow-allocation stages, successful reinit after both failures, a second fini, and repeated fini. Resource totals over the test lifecycle: `map=4, unmap=3, alloc=3, free=2` (one failed map has no matching unmap; one failed shadow allocation has a matching unmap but no free; two successful lifecycles each release one mapping and shadow).

### Callback synchronization and memory safety

* Production polled callback wrappers acquire a reference through `rotfb_runtime_callback_enter()` and release it through `rotfb_runtime_callback_exit()`. The gate is lock-free: the callback increments the count only if the quiescing bit is clear; fini atomically sets quiescing, so a callback either wins the increment and is drained or observes the closed bit and is rejected.
* Run #5 adds a real extracted production `bitmap_polled_cursor()` callback on a worker thread and holds it inside the host `membar_producer()` reached through the real production `rotfb_runtime_flush_rect()`. A separate thread runs the real production `rotfb_runtime_fini()`. The test observed gate closure, confirmed fini did not complete and did not unmap/free while the callback reference was held, then released the callback and confirmed fini completed and restored boot pointers. Scenario D PASS, zero failures.
* A production polled cursor callback invoked after fini but before reinitialization was rejected without framebuffer writes or an extra flush. No mutex is used by the polled callback.
* VIS ioctl callback paths run under `gfxp_fb_ioctl()`'s `softc->lock`; VIS_DEVFINI uses the same lock. This serializes the ioctl callback path against VIS_DEVFINI. The host test does not compile or exercise the generic gfxp_fb ioctl/DDI attach/detach wrappers themselves.
* Deadlock was not observed in the host callback/fini race. Actual illumos scheduler, interrupt context, DDI detach framework, WC cache attributes and hardware memory ordering remain untested on device.

### Test execution classification

* **Production C compiled/executed on host:** exact repository `illumo_rotfb/rotfb_runtime.c`; extracted patched upstream production functions `bitmap_polled_cursor`, `bitmap_cons_cursor`, `bitmap_display_cursor`, `bitmap_cons_copy`; runtime `rotfb_runtime_flush_rect()`, `rotfb_runtime_flush_all()`, `rotfb_runtime_fini()`, callback gate enter/exit, and `rotfb_copy_rows()`.
* **Host-only test substitutes:** pthread scheduler/threads; `kmem_zalloc/free`; `gfxp_map_kernel_space/unmap`; `membar_producer` release-fence stub; `delay`; illumos atomic/DDI compatibility declarations. The host mapping stub does not reproduce actual WC mapping attributes or physical MMIO behavior.
* **Static integration review only:** `gfxp_bm_attach()`, `gfxp_bm_detach()`, `bitmap_setup_fb()`, `bitmap_devinit()`, `gfxp_fb_attach()`, `gfxp_fb_detach()`, VIS_DEVFINI dispatch and TEM call path. These driver/DI DDI entry points are not compiled into the host harness.
* **Build #88:** Run `44484097492`, Job `135423155355`, Artifact `14513261641`, SHA-256 `b48a60e75ee60ee05b47233b05cdabd2a6a5509087c642ad5a80db1b82a8a2ad`, SUCCESS. The OpenIndiana VM/nightly ran; six independent final linker targets passed. Its runtime C lifecycle test passed geometry validation, mapping failure, allocation rollback, 2,304,000-pixel initial CW sync, clipping/canaries, pointer-mismatch refusal, callback gate drain, stale callback rejection, and one fini/reinit/teardown cycle.
* **Callback-only Run #5:** Run `44575376064`, Job `135741359480`, Artifact `14555293610`, SHA-256 `fa14d42a30aca13941770a58a577840375df7793018341fbe2612b710e67c0bc`, SUCCESS. Scenarios A/B/C from the prior callback suite and new lifecycle scenario D all PASS with zero failures. No OpenIndiana VM or kernel build ran in this callback-only workflow.

### Findings and remaining risks

* No reproducible production defect was found in the exercised runtime init/fini and callback gate paths. No production source or production patch was changed.
* Runtime lifecycle concurrency is serialized at the integration boundary by the softc ioctl mutex for VIS_DEVINIT/VIS_DEVFINI; DDI_DETACH additionally relies on the DDI framework's no-open/busy-device serialization. The generic DDI_DETACH path itself was not stress-tested in host code.
* The callback gate is global and not generation-tagged. It rejects stale callbacks while the gate remains closed after fini. A stale polledio pointer from a previous softc/lifecycle invoked only after a later successful init reopens the global gate would not be distinguishable by generation. The VIS_DEVFINI contract says the framebuffer is no longer used as a console; this cross-generation stale-pointer sequence was not established as a supported TEM call sequence and was not tested. Keep it as an explicit residual risk, not a claimed production defect.
* There is no injectable failure after successful runtime shadow allocation because no later operation currently returns an error. The initial flush is void and cannot report a WC/device failure.
* Callback registration itself has no failure-returning operation in the current gfxp bitmap attach path. The host harness cannot inject a failure into simple function-pointer assignments.
* The real x86 WC barrier, actual DDI map/unmap, interrupt/polled execution context, actual driver detach, firmware boot allocator behavior under exhaustion, and Chuwi device stability have not been verified on hardware.
* No Build #89 was started. No production patch, framebuffer geometry, clockwise mapping, shadow framebuffer architecture, VIS contract, upstream pin, firmware/GOP/DSI, touchpad or unrelated subsystem changed.
* Readiness: **BUILD VERIFIED** for Build #88; **CODE/TEST VERIFIED** for the listed production runtime/callback paths on host; **DEVICE VERIFIED: NOT VERIFIED**.


## 2026-11-19 — DDI ATTACH/DETACH integration audit: TEM ownership, polledio lifetime and detach error propagation

### Revision and scope

* Branch: `gop-rotation-hook`. Branch HEAD before this journal-only append was verified as `a15793321196e81ea9690b58d2ced4c041e832ed`; comparing that SHA with the branch ref returned `identical`.
* Build #88 production baseline: `a61c2ffb1f41d651a11c7f1cf9e719cad250a924`. Compare Build #88 to pre-audit HEAD `a157933` reports only six test/journal files changed: `.github/rotfb-callback-tests/**`, `.github/workflows/rotfb-callback-tests.yml`, and `ROTFB_ENGINEERING_LOG.md`. No `illumo_rotfb/**` production source or patch changed.
* Upstream pin remains `098e95820891f8a83fae679daece7fcf58b7b3a4`. This audit is static source analysis only; no production code, patch, ABI, geometry, CW mapping, shadow architecture, workflow or upstream pin changed.
* No Build #89 was started. Build #88 and callback-only Run #5 were not rerun.

### Pinned source material inspected

* `usr/src/uts/common/io/tem.c` — blob `f35db313856eaf9305c535b83176297429d18052`.
* `usr/src/uts/common/io/tem_safe.c` — blob `6ff2f9ee0cd9a6f5fad0ad1a90193b0a9d4e1038`.
* `usr/src/uts/common/sys/tem_impl.h` — blob `5b4006fa748bb8debe5cab635101729e9e983044`.
* `usr/src/uts/common/sys/visual_io.h` — blob `c894bb48bcb9b0b820360745009a7b5b0b9b81a0`.
* `usr/src/uts/i86pc/io/gfx_private/gfxp_fb.c` — blob `0d2d1fe1defaeeb69bc706e24903759951b78801`.
* `usr/src/uts/i86pc/io/gfx_private/gfxp_bitmap.c` — blob `e2668b24db5fc71d31f7894d791b17f409ee233b`.
* `usr/src/uts/i86pc/io/gfx_private/gfxp_fb.h` — blob `6c6cc5e8bf42cd63ddd781ebc77a05465a2a5e0a`.
* `usr/src/uts/intel/io/vgatext/vgatext.c` — blob `b18a3707176034e42837f041bd0702fdf3ac3a65`.
* ROTFB `illumo_rotfb/rotfb_runtime.c` at Build #88 source — blob `8d6e8000262fd97aee1325f0067a0f6871f39db5`.
* ROTFB patch `0005-rotfb-lifecycle-hardening.patch` adds the VIS_DEVFINI dispatch and error-returning bitmap detach behavior. Patch `0008-gfxp-detach-failure-safety.patch` changes only generic `gfxp_fb_detach()`; the patch allowlist does not include `vgatext.c`.

### Production call chain and ownership

```text
vgatext_attach(DDI_ATTACH)
  -> ddi_soft_state_zalloc(vgatext_softc)
  -> gfxp_fb_softc_alloc()                         [inner gfxp softc]
  -> gfxp_fb_attach()
       -> softc->devi = devi
       -> softc->polledio.arg = (vis_polledio_arg_t *)softc
       -> mutex_init(&softc->lock)
       -> select GFXP_BITMAP
       -> gfxp_bm_attach()
            -> polledio.display/copy/cursor = static bitmap callbacks
            -> softc->gfxp_ops = &gfxp_bitmap_ops
  -> create minor node
  [No ROTFB runtime WC mapping or runtime shadow allocation at DDI_ATTACH]

tem_info_init()
  -> ldi_open_by_name()                             [open reference retained on success]
  -> ldi_ioctl(VIS_DEVINIT) while tems.ts_lock held
       -> gfxp_fb_ioctl() locks softc->lock
       -> bitmap_devinit()
            -> bitmap_setup_fb()
                 -> rotfb_runtime_init()
            -> data->polledio = &softc->polledio
       -> unlock softc->lock
  -> tems.ts_fb_polledio = temargs.polledio
  -> validate depth/mode
  -> tems.ts_initialized = 1
  -> tem_internal_init() sets each VT's tvs_initialized

TEM initialization rejection after successful VIS_DEVINIT:
  -> tems_failed(finish_ioctl = true), still under tems.ts_lock
  -> ldi_ioctl(VIS_DEVFINI)
       -> gfxp_fb_ioctl() locks softc->lock
       -> gfxp_bm_detach()
            -> rotfb_runtime_fini()
       -> unlock softc->lock
  -> ldi_close()
  -> tems.ts_hdl = NULL

DDI_DETACH after the device has no open handles:
  -> vgatext_detach()
       -> gfxp_fb_detach()
            -> gfxp_bm_detach()
                 -> rotfb_runtime_fini() if configured and runtime_ready
            -> remove property / destroy softc->lock on success
       -> gfxp_fb_softc_free()
       -> ddi_soft_state_free()
```

* The outer `vgatext_softc` is owned by the vgatext driver soft-state allocator. The inner `gfxp_fb_softc` is allocated by `gfxp_fb_softc_alloc()` (`kmem_zalloc`) and freed by `gfxp_fb_softc_free()`.
* `struct vis_polledio` is embedded inside `gfxp_fb_softc`; it is not separately allocated. Its `display/copy/cursor` members are static production callback addresses. `polledio.arg` is the same `gfxp_fb_softc *`, not a separate callback object.
* `bitmap_devinit()` exports the address of the embedded `softc->polledio` only after `bitmap_setup_fb()` succeeds. The callback argument was initialized earlier by `gfxp_fb_attach()` to the same softc.
* `softc->console` points at global boot-owned `fb_info`. ROTFB runtime mapping and runtime shadow ownership are held in ROTFB runtime globals; they are not owned by the TEM or by the `polledio` structure.
* On successful TEM initialization, `tems.ts_hdl` remains open. The `tem_state_t` retains `ts_fb_polledio`, and the per-VT state is initialized. There is no normal-success-path `ldi_close()` in `tem_info_init()`.

### TEM proof: when can a saved polledio callback be called?

* Pinned `tem.c` has one kernel `VIS_DEVFINI` call site: `tems_failed()`. It calls FINI only when `finish_ioctl` is true, which is used for post-DEVINIT version, depth, or mode validation failures. A failed VIS_DEVINIT itself uses `tems_failed(..., B_FALSE)` and does not issue FINI.
* `tems_failed()` is reached before `tems.ts_initialized = 1` and before the loop calls `tem_internal_init()`. The latter is where each `tem_vt_state.tvs_initialized` becomes true.
* Pinned `tem_safe_write()` checks `!tem->tvs_initialized` and returns before any display/copy/cursor callback when the VT is not initialized. Thus after TEM's own VIS_DEVFINI failure-cleanup path, TEM does not issue a polled callback: no VT was initialized. `tems.ts_fb_polledio` may retain the pointer assigned before a later depth/mode validation failure, but that stale value is not dereferenced by an uninitialized VT.
* The standalone callback path is real: `tem_safe.c` calls `tems.ts_fb_polledio->display/copy/cursor` with `tems.ts_fb_polledio->arg` when `called_from == CALLED_FROM_STANDALONE`. Those calls are reachable for initialized VTs in standalone/quiesced contexts.
* After successful TEM initialization, the shared LDI handle remains open. The illumos `detach(9E)` contract says DDI_DETACH is called only once there are no outstanding opens. Therefore normal TEM operation prevents DDI_DETACH from freeing the softc while TEM retains and uses its polledio pointer.
* No upstream TEM path calls VIS_DEVFINI after successful initialization. The source search found the kernel call in `tem.c` only in `tems_failed()`; the separate boot TEM has its own early initialization failure path. The successful kernel TEM path does not clear `tems.ts_fb_polledio` or close `tems.ts_hdl`.
* If some other kernel client manually issues VIS_DEVFINI while TEM is already initialized, TEM does not clear `ts_fb_polledio`; an initialized VT can still call that saved pointer in standalone mode. That sequence is outside the observed TEM call path and conflicts with the VIS_DEVFINI contract (“no longer being used as a console”). It is recorded as an external-client contract hazard, not as a supported TEM sequence.

### Old polledio pointer across a subsequent VIS_DEVINIT

* With the same attached `gfxp_fb_softc`, the polledio object is embedded at the same address and `polledio.arg` still points to that same live softc. VIS_DEVFINI does not free the softc or clear the function pointers; a later VIS_DEVINIT on that same instance reuses the same polledio object and argument.
* During the interval after successful FINI and before the next INIT, the global callback gate remains closed; the existing callback wrapper rejects calls before dereferencing the argument. After successful re-INIT, the gate opens and a callback through the old pointer would target the same still-live softc and current runtime framebuffer state. It is not a stale softc address in this same-instance case.
* A pointer from a genuinely detached old softc would be unsafe if called after a new attach/re-INIT, because the global gate is not generation-tagged and would admit it once reopened. However, the pinned TEM success path keeps the LDI open handle and therefore prevents DDI_DETACH; the failed-init path leaves all VTs uninitialized and does not call the retained pointer. No supported TEM sequence was found that carries a callable old-softc polledio pointer across detach/re-attach.
* Conclusion: the global gate alone is not a general generation/lifetime mechanism, but the pinned TEM open-handle and initialization contract prevents the specific old-softc-after-reattach callback sequence in normal TEM operation. No Generation ID is justified by current TEM evidence. External clients retaining polledio pointers beyond their open/console lifetime remain outside the proven contract.

### Lock ordering and deadlock review

| Path | Locks / synchronization | Result |
|---|---|---|
| TEM initialization / failure cleanup | `tems.ts_lock` -> `softc->lock` through LDI ioctl | Established order |
| Normal TEM layered display/copy/cursor | TEM locks -> LDI ioctl -> `softc->lock` | Same order |
| VIS_DEVINIT / VIS_DEVFINI | `gfxp_fb_ioctl()` holds `softc->lock` around `do_gfx_ioctl()` | Serializes ioctl operations |
| Polled TEM callbacks | No mutex; lock-free ROTFB callback gate | No attempt to acquire `softc->lock` or `tems.ts_lock` |
| Runtime FINI | Called under `softc->lock`; closes gate and waits for active callbacks | No lock cycle in current callback bodies because callbacks do not acquire the mutex |
| DDI_DETACH | DDI_DETACH is only called after outstanding opens are gone; wrapper does not acquire `softc->lock` | No concurrent TEM ioctl under the normal open-handle contract |

* The inspected bitmap driver does not invoke the TEM mode-change callback while holding `softc->lock`; the bitmap devinit mode-change fields are not used by the driver. No reverse `softc->lock -> tems.ts_lock` edge was found in this call chain.
* No deadlock was found in the inspected lock graph. This is static reasoning; the DDI framework and actual illumos scheduler were not executed in this audit.

### Confirmed DDI detach error-propagation defect

* ROTFB patch `0005-rotfb-lifecycle-hardening.patch` changes `gfxp_bm_detach()`: when ROTFB is configured and runtime is ready, it returns `DDI_FAILURE` if `rotfb_runtime_fini()` fails.
* `rotfb_runtime_fini()` validates the live framebuffer pointers and metadata before closing the callback gate. A pointer/ownership mismatch in this preflight returns `DDI_FAILURE` while the callback gate remains open and runtime resources remain owned/active. Build #88's runtime test already exercised the pointer-mismatch refusal at the runtime-core level.
* ROTFB patch `0008-gfxp-detach-failure-safety.patch` correctly changes `gfxp_fb_detach()` to return immediately on bitmap detach failure, before removing the primary-controller property or destroying `softc->lock`.
* But the actual pinned upstream caller `vgatext_detach()` does not check that return value. It calls `(void) gfxp_fb_detach(...)`, then unconditionally calls `gfxp_fb_softc_free()`, removes the minor node, frees the outer soft-state entry, and returns `DDI_SUCCESS`.
* Therefore patch 0008's local refusal is discarded by the top-level DDI_DETACH entry point. On a reachable `rotfb_runtime_fini()` preflight failure, vgatext still reports successful detach and frees the inner softc even though ROTFB runtime resources remain active. The softc mutex is also not destroyed because `gfxp_fb_detach()` returned before that point.
* If a polled callback is still active or an external client retained the old polledio pointer, freeing the softc without closing/draining the gate creates a use-after-free risk. This UAF was not executed; it is the consequence of the source-level ownership violation. Under the pinned TEM failure path, TEM itself has not initialized any VT and does not issue such a callback, so the audit does not claim a TEM-reproduced UAF.
* The unconditional DDI_SUCCESS is independently a confirmed contract violation: illumos `detach(9E)` requires returning `DDI_FAILURE` when the instance cannot be safely detached, leaving per-instance state intact. The actual vgatext wrapper does the opposite on the error path.
* This finding is in the upstream `vgatext.c` wrapper, which is not included in the 12-patch ROTFB allowlist. No production change is made under the current instruction.

### Failure scenarios

| Scenario | Expected/current behavior | Audit result |
|---|---|---|
| DDI_ATTACH succeeds; VIS_DEVINIT succeeds; TEM succeeds | LDI handle remains open; TEM owns a borrowed polledio pointer; DDI_DETACH is blocked while open | Safe under TEM contract |
| VIS_DEVINIT fails before publishing polledio | TEM calls `tems_failed(..., B_FALSE)`; closes LDI handle; no runtime resources remain after init rollback | Safe by inspected rollback path |
| VIS_DEVINIT succeeds, TEM rejects version/depth/mode | TEM calls VIS_DEVFINI then closes handle; VT states remain uninitialized, so no polled callbacks | Safe for TEM callback lifetime |
| VIS_DEVFINI succeeds, then DDI_DETACH | Runtime fini releases mapping/shadow; later bitmap detach sees runtime_ready false and succeeds; softc is freed | Normal sequence |
| VIS_DEVFINI preflight fails, then DDI_DETACH | bitmap detach returns failure; gfxp_fb_detach returns failure; vgatext ignores it and frees softc anyway | **Confirmed detach error-propagation defect** |
| External initialized client calls VIS_DEVFINI then continues standalone callbacks | TEM retains polledio pointer and can still call it; contract says device is no longer used | External contract violation / residual risk |
| Same live softc: FINI -> INIT -> old polledio address called | Same embedded polledio and same softc argument; gate admits callback against current runtime | No generation mismatch; not a stale address |
| Old detached softc pointer called after a new attach reopens global gate | Global gate cannot distinguish old generation; old argument could be freed | Unsafe if an external client violates pointer lifetime; not established for TEM |

### Execution classification

* **Executed in this audit:** no new tests. The existing Build #88 and callback-only Run #5 evidence is referenced but was not repeated.
* **Previously executed production code:** Run #5 executed the exact production runtime core and extracted production bitmap polled callbacks, including callback drain/fini race. It did not compile or execute `gfxp_fb_attach()`, `gfxp_fb_detach()`, `gfxp_bm_attach()`, `gfxp_bm_detach()`, `bitmap_devinit()`, `vgatext_attach()`, or `vgatext_detach()`.
* **Static-only in this audit:** the complete DDI/TEM call chain, softc allocation/free, polledio pointer/arg ownership, ioctl mutex ordering, vgatext detach return handling, and illumos detach contract.
* **Why no DDI integration harness was executed:** the existing callback harness intentionally extracts bitmap callback bodies and supplies host stubs; it does not compile the vgatext/gfxp DDI entry points or emulate illumos devinfo, LDI open accounting, soft-state management, mutex lifecycle, and DDI detach dispatch. A host wrapper test with mocked DDI services would only test the stubs unless it is extended to compile the actual production entry-point bodies. No such integration harness was added or run in this audit.
* **Build #88 remains:** BUILD VERIFIED for the pinned OpenIndiana VM/nightly and six link targets.
* **Run #5 remains:** CODE/TEST VERIFIED for the exact runtime and extracted callback paths listed in the lifecycle entry; it is not DDI attach/detach integration evidence.
* **DEVICE VERIFIED:** NOT VERIFIED.

### Audit conclusion and next safe step

* TEM's supported initialization/failed-initialization paths do not demonstrate an old callable polledio pointer crossing a successful VIS_DEVFINI and a later reattach. The same-instance FINI/INIT case reuses the same live embedded polledio object and softc. No Generation ID is warranted by the pinned TEM contract.
* The upstream `vgatext_detach()` return-value discard is a concrete integration defect in the error path introduced/exposed by making `gfxp_bm_detach()` fail on unsafe runtime fini. It must be addressed before treating detach-failure safety as complete. Do not change production code until the owner authorizes it.
* The next validation should be a test-only integration harness that compiles the actual patched `gfxp_fb_detach()`, patched `gfxp_bm_detach()`, actual `vgatext_detach()`, and actual `rotfb_runtime_fini()` against narrowly scoped DDI stubs; inject the existing runtime pointer-mismatch condition and record whether the top-level detach frees softc and reports success. This is a separate DDI integration test, not a repeat of Run #5.
* Journal update only. Build #89 not started; production code and all 12 production patches unchanged.


## 2026-11-19 — DDI detach host integration execution follow-up

### Test-only harness and revisions

* Static DDI audit was followed by a separate host integration harness under `.github/rotfb-ddi-integration-tests/` and workflow `.github/workflows/rotfb-ddi-integration.yml`. This is intentionally separate from the callback-only workflow; it does not rerun Run #5 and does not invoke the OpenIndiana/nightly build.
* Harness source commit: `3a126f77ae24471a9486e8f6bebcab46b0858fb7`. The first workflow run `44587830535` failed before extraction/compilation because the runner passed a relative patch directory to `git -C <worktree> apply`; the patch path was consequently resolved inside the upstream worktree. This was a harness path bug, not a source/patch failure. No production code was compiled or executed in that first attempt.
* Test-only runner correction commit: `fec74244cf2e048fe2fddade7dd28a7775065c25`. The runner canonicalizes the patch directory before entering the upstream worktree. The test's `rotfb_runtime_fini()` success stub also clears the simulated `runtime_ready` flag so the success path models the expected post-FINI state.
* Successful workflow run: `44588127908`, run number 2, job `135786521899`, commit `fec74244cf2e048fe2fddade7dd28a7775065c25`, conclusion SUCCESS.
* Artifact: ID `14561369810`, 11,685 bytes, SHA-256 `9f59c9f32ebd930ac486e89374b4e4d3734ef14b6c42078e78b2b13497836090`. Workflow URL: `https://github.com/Cadiocto/WhateverXe-MiniBook-X-N150/actions/runs/44588127908`.
* Exact upstream pin `098e95820891f8a83fae679daece7fcf58b7b3a4` verified. All 12 production patches passed `git apply --check` and apply in the disposable upstream worktree. Python extraction passed. C compilation passed with `-std=c11 -Wall -Wextra -Werror`. Harness exit 0.

### Production function bodies actually compiled and executed

The harness extracted verbatim function bodies from the pinned upstream worktree after applying the 12 ROTFB patches, then compiled and invoked:

* `gfxp_bm_detach()` — patched production function body.
* `gfxp_fb_detach()` — patched production function body, including patch 0008's failure guard.
* `vgatext_detach()` — unmodified pinned upstream production DDI entry point.

The harness supplies host stubs for DDI services, property removal, mutex destruction, soft-state/free operations, and `rotfb_runtime_fini()`. It does **not** execute the illumos DDI framework or the real `rotfb_runtime_fini()` body in this binary. The fini result is injected as DDI_FAILURE/DDI_SUCCESS; the underlying pointer-mismatch refusal of the real runtime fini was already exercised in the Build #88 runtime test.

### Executed outcomes

* Failure injection:
  * Direct call to actual patched `gfxp_fb_detach()` returned `DDI_FAILURE`; it did not remove the primary-controller property or destroy the softc mutex.
  * Then actual `vgatext_detach()` was called with the same injected FINI failure.
  * Actual wrapper returned `DDI_SUCCESS`, invoked both `gfxp_fb_softc_free()` and `ddi_soft_state_free()`, removed the minor node, did not remove the primary-controller property, did not destroy the mutex, and left simulated `rotfb_info.runtime_ready = 1`.
  * Harness output:
    ```text
    GFXP_FB_DETACH_FAILURE_PROPAGATION=PASS
    VGATEXT_DETACH_FAILURE_HANDLING=DEFECT_REPRODUCED
    FAILURE_PATH_RESULT=DDI_SUCCESS_SOFTC_FREED_RUNTIME_STILL_READY
    FAILURE_PATH_MUTEX_DESTROYED=NO
    ```
* Success injection:
  * Actual `vgatext_detach()` returned `DDI_SUCCESS`.
  * The expected cleanup calls were observed: fini success, primary-controller property removal, mutex destruction, inner softc free call, minor-node removal and outer soft-state free call.
  * Harness output: `VGATEXT_DETACH_SUCCESS_PATH=PASS`.
* Final harness result: `ROTFB_DDI_DETACH_INTEGRATION_HARNESS=PASS`.

### Evidence boundaries

* This is stronger than the previous static-only review: the actual patched detach function bodies and actual upstream vgatext detach body were compiled and executed in a host harness.
* The host stubs count free calls rather than freeing the test objects, allowing the harness to inspect the post-call state. The source-level free calls are real; physical allocator behavior is not being claimed as host evidence.
* The harness does not execute `vgatext_attach()`, `gfxp_fb_attach()`, `gfxp_bm_attach()`, `bitmap_devinit()`, `bitmap_setup_fb()`, `gfxp_fb_ioctl()`, `do_gfx_ioctl()`, `tem_info_init()`, LDI open accounting, the real DDI framework, or actual illumos mutex/soft-state services. Those remain static-only.
* It also does not reproduce an actual concurrent callback racing with this failing DDI_DETACH. The source path proves the callback gate remains open when the real runtime FINI preflight rejects a pointer mismatch; the DDI harness proves the wrapper proceeds to free-call sites despite the returned failure. A concurrent UAF is therefore a credible conditional risk, not an executed UAF.
* The result confirms the previously identified defect at the real production function-body level. No Generation ID was added. No production code or production patch was changed.
* No callback-only Run #5 rerun and no Build #89. Build #88 remains the last OpenIndiana VM/nightly build; device validation remains NOT VERIFIED.


## 2026-11-19 — Source-name clarification for TEM standalone callback guard

* Correction to the preceding DDI audit text: the function is named `tem_safe_polled_write()`, not `tem_safe_write()`.
* At pinned `usr/src/uts/common/io/tem_safe.c` line 245, `tem_safe_polled_write()` returns immediately when `tem->tvs_initialized` is false. Only after that guard does it call `tem_safe_check_first_time()` and `tem_safe_terminal_emulate(..., CALLED_FROM_STANDALONE)`, whose display/copy/cursor helpers dereference `tems.ts_fb_polledio`.
* This confirms the earlier lifecycle conclusion with the exact function name: the TEM-owned VIS_DEVFINI failure path occurs before any VT is initialized, so TEM's standalone polled-write entry point does not invoke the retained callback after that failed initialization. Conversely, an externally initiated VIS_DEVFINI after a VT is initialized does not clear the pointer or reset `tvs_initialized`; a later standalone polled write can still invoke it, outside the normal TEM-owned VIS_DEVFINI path.
* Source: `https://github.com/illumos/illumos-gate/blob/098e95820891f8a83fae679daece7fcf58b7b3a4/usr/src/uts/common/io/tem_safe.c#L232-L253`.


## 2026-11-20 — vgatext DDI_DETACH failure propagation

* Branch baseline before this work: `b12a41577cdfddb2e4f4e0e0c99e2661a64f0dd1`. The branch ref was independently verified against this commit; the unrelated default-branch history was not used as the ROTFB baseline.
* Production source verified against pinned illumos `098e95820891f8a83fae679daece7fcf58b7b3a4`: `usr/src/uts/intel/io/vgatext/vgatext.c`, `vgatext_detach()`. The original DDI_DETACH path discarded `gfxp_fb_detach()`'s return, then freed inner gfxp soft state, removed the minor node, freed outer soft state, and returned DDI_SUCCESS.
* Failure-safety chain verified: patch 0005 makes `gfxp_bm_detach()` return DDI_FAILURE when runtime FINI refuses; the real `rotfb_runtime_fini()` performs all pointer/geometry/owner preflight checks before closing the callback gate or mutating runtime/framebuffer state; patch 0008 makes `gfxp_fb_detach()` return immediately on lower-level failure before removing the primary-controller property or destroying the mutex. Therefore a FINI preflight failure leaves the resources and ownership state required for retry intact.
* Added only the vgatext production change in `illumo_rotfb/patches/0013-vgatext-detach-failure-propagation.patch`: capture the `gfxp_fb_detach()` result and return it immediately unless DDI_SUCCESS. Successful cleanup order is unchanged. No existing production patch was modified.
* Test-only changes: DDI harness now checks failure, retry on the same instance, and ordinary success; extractor asserts that the error guard precedes cleanup; runner expects 13 ordered production patches.
* Final branch HEAD: `94c4469bc30bb56bc787d73daafb6f69e39b72d4` (three commits after baseline: production patch + harness update, then two extractor assertion corrections). Diff from baseline contains exactly one new production patch and three DDI harness files; no other production patch changed.
* DDI integration Run #5: `44594573801`, SUCCESS, job `135809304101`, commit `94c4469bc30bb56bc787d73daafb6f69e39b72d4`. Upstream pin verified; all 13 patches applied; extracted and executed production bodies `gfxp_bm_detach()`, `gfxp_fb_detach()`, and `vgatext_detach()`. Scenario A failure, Scenario B retry, and Scenario C ordinary success all PASS.
* DDI artifact ID `14565456255`; artifact ZIP SHA-256 `001bcc334803b912f634e00f1846b0778cdda93219d86b38ba1247995041a770`.
* Host stubs in the DDI harness: `rotfb_runtime_fini()` (injected success/failure; not the real runtime body in this harness), `ddi_prop_remove()`, `mutex_destroy()`, `gfxp_unmap_kernel_space()`, `kmem_free()`, `ddi_get_instance()`, `getsoftc()`, `gfxp_fb_softc_free()`, `ddi_remove_minor_node()`, `ddi_soft_state_free()`, `gfxp_fb_suspend()`, `gfxp_vga_detach()`, and `cmn_err()`. The host harness counts ownership/cleanup calls instead of freeing its test objects. Real runtime FINI preflight and its no-mutation-on-refusal path were source-inspected; this DDI binary does not execute that body.
* CI path-filter side effect: the push that added patch 0013 also automatically triggered Build #89 / Run `44594478464`, despite the instruction not to run it. It failed at the existing linker-evidence regression step because the patch manifest still expected 12 patches while the directory now contains 13; OpenIndiana VM/nightly and link stages were skipped. Artifact ID `14564452651`, SHA-256 `ee9023358e48cf337370b8579759b50aece6291b5a368b49647d53438a98c74e`. No cancellation action was available through the connected GitHub interface. No Build #90 was started.
* The same harness-path push automatically triggered callback-only Run #6 / `44594478462`, which failed with callback harness exit code 1 after applying all 13 patches. Run #5 was not manually repeated; this was an unintended path-filter trigger. No callback-only rerun was requested.
* DDI Run #3 failed because the first extractor assertion used an over-escaped regex; Run #4 was superseded/cancelled by the subsequent correction. Final Run #5 used literal source-order checks and passed.
* Remaining limitations: host stubs do not emulate illumos DDI scheduling, devinfo/soft-state allocators, real mutex implementation, or a concurrent callback racing DDI_DETACH. The real runtime FINI body was not linked into this DDI harness. No physical Chuwi hardware test was performed; DEVICE VERIFIED remains NOT VERIFIED.
* Build recommendation: do not start a subsequent OpenIndiana kernel build yet. The linker-evidence patch manifest must first be updated to include patch 0013; then a separately authorized full build can validate patch application, compile, and link. The DDI detach integration fix itself is host-harness verified.


## 2026-11-20 — CI/CD restoration audit (pre-change baseline)

* Audited branch `gop-rotation-hook` directly through GitHub before changing CI: HEAD was `4d90aa811ad29e888c1d667051c10617818b8997` (tree `9084eb9a381bb937d0a188dfd6d4bd3bcc4b76ad`). No source or patch changes were made during this audit.
* Confirmed `illumo_rotfb/patches/` contains exactly 13 ordered patches, `0001` through `0013`. The current 0013 blob is `687a3898d881c88fba8944b09666209b2e123f47`. The complete manifest is also asserted by `rotfb_patch_series_test.sh` and `rotfb_link_evidence.sh`.
* Confirmed current workflows: `.github/workflows/build-illumos.yml`, `rotfb-callback-tests.yml`, `rotfb-ddi-integration.yml`, `build-gop-rotation-hook.yml`, and `ci.yml`. The latter two are independent UEFI/macOS project workflows and are not substitutes for the OpenIndiana kernel build.
* Root cause of disabled automatic full build is commit `698fcfc39b56850e4b80d0476f7925395437c57b`, authored by Cadiocto on 2026-11-19 19:11:30Z, message `ROTFB: reconcile patch manifest and isolate full build trigger`. Its exact diff removed the prior `push` trigger (branch `gop-rotation-hook`, paths `illumo_rotfb/**` and `.github/workflows/build-illumos.yml`) and retained `workflow_dispatch`. It preserved concurrency group `rotfb-single-build` and `cancel-in-progress: false`.
* The callback-only workflow triggers on callback harness/workflow, `illumo_rotfb/patches/*.patch`, `illumo_rotfb/tests/**`, and `rotfb_link_evidence.sh`; DDI integration triggers on its harness/workflow and `illumo_rotfb/patches/*.patch`. Both remain distinct from full OpenIndiana VM/nightly.
* Build #88 independently verified from GitHub: Run `44484097492`, job `135423155355`, commit `a61c2ffb1f41d651a11c7f1cf9e719cad250a924`, conclusion SUCCESS. Artifact `14513261641`, name `illumos-framebuffer-build-logs-44484097492`, downloaded ZIP SHA-256 `b48a60e75ee60ee05b47233b05cdabd2a6a5509087c642ad5a80db1b82a8a2ad`. Its evidence shows upstream `098e95820891f8a83fae679daece7fcf58b7b3a4`, 12/12 patches (pre-0013 series), nightly exit 0, and all six linker targets PASS. This is a valid historical baseline but not verification of the current 13-patch series or current HEAD.
* Build #89 independently verified: Run `44594478464`, job `135808960880`, commit `f6f45bcd52679173a516e887454ed6b4ccc0ada7`, conclusion FAILURE. Step `ROTFB linker evidence regression tests` exited 1 because the helper still expected 12 patches while the tree contained 13. OpenIndiana VM/nightly was SKIPPED. Artifact `14564452651` was only 752 bytes; its uploaded ZIP SHA-256 was `ee9023358e48cf337370b8579759b50aece6291b5a368b49647d53438a98c74e`. Failure is CI manifest drift, not an OpenIndiana compiler/linker failure.
* Current linker evidence helper defines exactly six required targets: `dboot.i86pc`, `unix.o.i86pc`, `gfx_private.i86pc`, `dboot.i86xpv`, `unix.o.i86xpv`, and `gfx_private.i86xpv`.
* Current full workflow deficiencies recorded for remediation: no push trigger; test commands before diagnostics initialization are not individually persisted to files with exit codes; one generic artifact name with 14-day retention; no Job Summary; no separate diagnostics/test-results artifacts; no reliable same-run DDI status in the full-build summary. Existing VM/Nightly/linker collection is substantial and must be preserved.
* GitHub connector audit: repository permissions report admin/push, and read access to runs/jobs/logs/artifacts is available. The exposed GitHub action set contains no workflow-dispatch/write-to-Actions operation. No successful POST dispatch capability was established; do not claim workflow_dispatch can be initiated by this connection. A restored, narrowly scoped push trigger is the available autonomous route for a legitimate configuration commit.
* No production source, framebuffer ABI, runtime architecture, upstream pin, or any of the 13 production patch files were modified during this audit. No physical device test has been performed; DEVICE VERIFIED remains NOT VERIFIED.


## 2026-11-20 — CI/CD restoration implementation

* Restored a branch-scoped `push` trigger for `gop-rotation-hook` alongside `workflow_dispatch` in `.github/workflows/build-illumos.yml`. Its paths are limited to the 13-patch series, ROTFB production sources/headers, production link-evidence and patch-series scripts/fixtures, and the full-build workflow itself. Documentation-only and unrelated test-harness-only changes are excluded.
* Preserved full-build concurrency `rotfb-single-build` with `cancel-in-progress: false`; no nightly commands, upstream pin, production patches, framebuffer ABI, runtime core, or production architecture were changed.
* Host model, linker-evidence, VIS cursor, bitmap-copy, runtime lifecycle, and callback harness commands now persist stdout/stderr and exit-code evidence in `rotfb-test-results/`. OpenIndiana IPS setup, patch apply/check, git diff check, linker finalization, nightly and existing VM diagnostic collection are persisted in `rotfb-build-logs/`.
* Added runner outcome metadata, failure context and error/file indexes under `rotfb-build-diagnostics/`. The full workflow now uploads separate diagnostics, build logs and test results artifacts with run ID + commit SHA in their names, `if: always()`, and 30-day retention.
* Added an always-run full-build Job Summary with explicit PASS/FAIL/SKIPPED rows for patch application, host regression, callback harness, DDI integration, OpenIndiana VM, nightly, six-target linker verification and artifact upload. The acceptance summary fails closed if any mandatory stage is not PASS.
* The full-build summary queries callback-only and DDI workflow runs by exact `head_sha`; the callback and DDI workflows are independently triggered by their own workflow-file changes in the same configuration commit, preserving their separation from the full build. Both now emit Job Summaries and commit-scoped test artifact names.
* Added Ruby YAML syntax validation for every workflow file to the callback-only workflow. The full-build workflow retains manual dispatch, but this GitHub connector still has no Actions dispatch/write operation; the real, scoped push trigger is the autonomous launch path.
* This configuration change is intended to cause exactly one full build on this commit, plus the independent callback-only and DDI regression workflows. No production source or any of the 13 patch files is part of the change.
