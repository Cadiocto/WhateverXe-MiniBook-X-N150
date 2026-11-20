# ROTFB Autonomous Engineering State

Last updated: 2026-11-20T22:07:31.438Z

## Current repository state

- Repository: Cadiocto/WhateverXe-MiniBook-X-N150
- Branch: gop-rotation-hook
- HEAD observed immediately before this state-file commit: 929e3dfb2d0c63152c73fa10da7375b4a42d97d7
- Packaging workflow: .github/workflows/rotfb-device-packaging.yml
- Packaging workflow blob at that snapshot: 84554b4031145c6464fdf8e8c97ce70e97322748
- ROTFB source revision: 8e2a6e8f246c43c151793046934bcb848371768d
- Pinned upstream illumos: 098e95820891f8a83fae679daece7fcf58b7b3a4

## Verified baseline

- Full build Run 44622365817 on source SHA 8e2a6e8f246c43c151793046934bcb848371768d: completed, conclusion success.
- Same-commit callback-only Run 44622365674: completed, conclusion success.
- Same-commit DDI integration Run 44622365794: completed, conclusion success.
- This proves the historical build baseline only; it does not prove the packaging workflow or offline Install Kit.

## Current release stage

Stage 1 — Workflow recovery: IN PROGRESS / NOT YET VERIFIED.

Latest relevant packaging run:
- Run ID: 44795290315
- Commit: 2214f9f6e1fe33ac89afeb4531274c4199053afb
- Workflow: ROTFB offline device packaging
- Job: package, job ID 136531842357
- Conclusion: failure
- First meaningful error: sh: line 117: syntax error at line 174: '|' unexpected
- Job then reported ssh exited with code 3.
- Root cause located in the guest run block: the find -exec command contained two backslashes before the semicolon; the shell treated the semicolon as a command separator before the pipe.

## Changes applied

Two verified commits on gop-rotation-hook:

1. 0599b94938607ad941c72ef187ff12bc9b7b1184 — ROTFB: repair packaging shell and IPS repository paths
2. 929e3dfb2d0c63152c73fa10da7375b4a42d97d7 — ROTFB: name nightly driver log accurately

Only .github/workflows/rotfb-device-packaging.yml was changed.

Fixes applied:
- Removed the workflow's path-filtered push trigger; retained workflow_dispatch.
- Corrected the guest find -exec terminator to one shell-escaping backslash.
- Corrected both escaped shell parameter expansions used to derive relative paths.
- Corrected PKGDIR to expand PKGARCHIVE_BASE and PKGARCHIVE_SUFFIX.
- Added PKG_REPO at PKGDIR/repo.redist and changed repository verification, listing, manifest access and installer validation to the actual IPS repository root.
- Removed -x from the nightly invocation; retained the real nightly return-code gate.
- Renamed the wrapper output to NIGHTLY_DRIVER_LOG at rotfb-package-logs/nightly-driver.log; the log is required to exist, but may be empty.
- Added stable step ID assemble_kit.
- Gated USB artifact upload on successful assembly and a present ZIP.
- Excluded the final ZIP from the diagnostic artifact.
- Preserved the mandatory offline dependency gate; no false PASS was introduced.

## Validation evidence

- GitHub branch ref was checked after the changes; HEAD was 929e3dfb2d0c63152c73fa10da7375b4a42d97d7.
- Latest workflow blob fetched from GitHub: 84554b4031145c6464fdf8e8c97ce70e97322748.
- GitHub commit diff confirms only the packaging workflow changed in both commits.
- Text-level acceptance checks against the fetched workflow: PASS for dispatch-only trigger, no escaped relative-path or PKGDIR expansions, single-backslash find terminator, no nightly xtrace, no pkg5.repository assumption, correct repo.redist paths, assembly-gated USB upload, diagnostic ZIP exclusion, and preserved offline gate.
- Full YAML parse: NOT RUN.
- actionlint: NOT RUN / tool not available in the current execution environment.
- Full guest sh -n: NOT RUN. The GitHub job's shell parser previously failed before execution; a post-fix guest syntax check has not yet run.
- Target OpenIndiana /bin/sh syntax/runtime validation: NOT RUN.
- No packaging workflow has been dispatched for the new HEAD.

## CI and artifacts

- Latest packaging run inspected: 44795290315 — FAILURE.
- Failed job: package / 136531842357.
- Diagnostic artifact: rotfb-device-packaging-diagnostics-44795290315-8e2a6e8f246c43c151793046934bcb848371768d
- Artifact ID: 14662739027
- Artifact digest reported by GitHub: sha256:2e3608f9d9f6ec7f559fec14a207a315eb1c74d51d98372b040be0c8dd8ec87f
- Diagnostic artifact upload succeeded. USB Install Kit upload was skipped; no validated ZIP exists.

## Release stage ledger

| Stage | State | Evidence / blocker |
|---|---|---|
| 1. Workflow recovery | IN PROGRESS / NOT VERIFIED | Fixes committed; full YAML/actionlint/guest shell validation and post-fix run outstanding |
| 2. Control build | NOT RUN for packaging workflow | Historical full build passed on source SHA, not current packaging workflow |
| 3. IPS repository validation | PARTIAL / NOT VERIFIED | Failed-run archive structure inspected; IPS-native pkgrepo verification still not executed |
| 4. Offline dependency closure | BLOCKED | Target-equivalent OpenIndiana 2026.10 package baseline and offline solver proof absent |
| 5. Kernel payload validation | PASS (i86pc payload scope) | Run 44795246582 artifact: 29 file hashes, 40 mode checks, 11 directories and 1 hardlink match the system/kernel/platform IPS manifest; unix/gfx_private ELF and ROTFB symbols verified. The enclosing nightly run failed at PKGLINT, so this is not a successful full build or release |
| 6. Offline Install Kit assembly | NOT RUN / BLOCKED | Assembly intentionally refuses unless offline closure is PASS; installer preflight has been hardened but not exercised on OpenIndiana |
| 7. ZIP revalidation | NOT RUN | No kit ZIP uploaded |
| 8. Physical-device preparation | NOT RUN | Target-side runbook still required |
| 9. Physical-device verification | NOT VERIFIED | Requires user-performed test-BE installation and hardware evidence |

## Blockers and constraints

- Offline dependency closure is a mandatory release gate and must not be bypassed. A direct dependency lookup against the build repository is not sufficient.
- No target-equivalent OpenIndiana 2026.10 x86_64 baseline snapshot has been supplied or validated.
- The current connected GitHub toolset exposes run/job/artifact inspection and repository writes, but no workflow-dispatch operation. The new workflow has workflow_dispatch; no run was started from this session.
- The local container cannot resolve external GitHub hosts, so it cannot fetch the workflow for local parser execution. The GitHub API fetch remains the source of repository content.
- The prior ROTFB Autonomous Build Watch automation was paused and had an obsolete build-only prompt. This was resolved by updating that existing automation to the offline-kit release mission and enabling its hourly condition-watch schedule.

## Next concrete actions

1. Complete static validation of the current workflow (YAML parser, actionlint, full extracted guest shell syntax check, set -u and heredoc audit).
2. Run one controlled packaging workflow only after Stage 1 is fully verified.
3. Inspect the resulting full logs and actual artifacts; validate the IPS repository and kernel payload.
4. Obtain a target-equivalent OpenIndiana 2026.10 package baseline and run a genuinely offline IPS solver dry-run. Keep Stage 4 BLOCKED until that evidence exists.
5. Continue the remaining release milestones without touching protected production source, patches 0001–0013, upstream pin, runtime core, callback drain, DDI integration or shadow framebuffer architecture.
6. Prepare the separate-test-BE installation/rollback runbook; never install on the user's working BE or physical device autonomously.

## Append-only history

### 2026-11-20T21:58:30.115Z — packaging workflow recovery cycle

- Confirmed live branch HEAD 2214f9f6e1fe33ac89afeb4531274c4199053afb before edits.
- Inspected Run 44795290315, job 136531842357, and its failure logs.
- Identified the double-backslash find -exec syntax defect and related packaging-path defects in the current workflow.
- Committed workflow-only corrections in 0599b94938607ad941c72ef187ff12bc9b7b1184 and 929e3dfb2d0c63152c73fa10da7375b4a42d97d7.
- Confirmed no build, workflow dispatch, rerun, packaging execution, or physical-device action was performed.
- Stage 4 remains BLOCKED; no Install Kit readiness claim is made.


### 2026-11-20T21:59:02.859Z — persistent automation resumed

- Rechecked live branch HEAD before this state update: 2dcb15ea6d7a76985f09a9e8c138e78bc861252d.
- Updated existing automation ROTFB Autonomous Build Watch (ID 6aff5a46fff881918dcc37ed37bd0366) to ROTFB Offline Kit Release.
- Replaced its stale build-only prompt with the staged offline-kit mission, current failure evidence, protected-component constraints, mandatory offline closure gate, and truthful status requirements.
- Re-enabled the existing hourly condition-watch schedule in Europe/Helsinki. No duplicate automation was created.
- The automation does not itself prove a workflow run or release artifact; it must inspect live GitHub evidence on each execution.


### 2026-11-20T22:02:38.779Z — failed-run artifact forensic analysis and package selection fix

- Recovered and downloaded diagnostic artifact 14663616959 from Run 44795246582 (commit cc9657aa313760621f5642fb7a0d4afe4a3d5a30). Downloaded ZIP SHA-256: ddbf8793402301e19a3ce23241be5061e6b3981ff6a756e21c94703d67e9900e; 220,263,301 bytes.
- The failed run's nightly exit code is 1. mail_msg.txt shows empty build-error, build-warning and package-build-error sections. pkglint_noise.txt contains only shell xtrace commands (+ cd, + dmake -e pkglint, + tee, + egrep). build_noise shows pkglint_noise nonempty and upstream build_extras_ok=n. This establishes the false PKGLINT noise caused by nightly xtrace; removal of -x was the correct fix.
- Inspected failed-package-repository.tar.gz (217,960,383 bytes; SHA-256 0736fa6e47bc46244d1c9ba1c0cce2f5f676d50e088d5181e250d53883b0b242). It contains packages/i386/nightly-nd/repo.redist with catalog, pkg and file directories; 32,208 archive entries, 557 package manifest files, 1,115 pkg-tree entries and 24,445 file-tree entries. The repository has not passed IPS-native pkgrepo verify/list in this environment.
- Found the actual payload-owning FMRI in the repository manifest: pkg://on-nightly/system/kernel/platform@0.5.11,5.11-999999.1:20261120T195202Z. Its manifest includes platform/i86pc/kernel/amd64/unix and platform/i86pc/kernel/misc/amd64/gfx_private (plus i86xpv counterparts). The package manifest contains no ROTFB-named path; ROTFB is linked into gfx_private and rotfb_info is defined by unix. The prior workflow's search for literal rotfb strings would therefore fail to select the package.
- Corrected package discovery to match the actual i86pc unix and gfx_private manifest paths in commit e8e47e10612dcba79849a39f0a6e59366cdd4677. This is workflow-only; no protected production source or patch changed.
- Extracted the failed-run kernel payload (51 files). unix is ELF64 x86-64 Solaris EXEC, 2,384,808 bytes, SHA-256 1ea83d1d5597d50893aeef6b8d3b79f3be9b439ca509ba31062529bb87d1f300. gfx_private is ELF64 x86-64 Solaris REL, 70,352 bytes, SHA-256 53487c33f17d63b3670f15df4585149da6fea1198380f01a639abbd7e9de9207. nm shows rotfb_runtime_init/fini/flush/callback symbols in gfx_private; gfx_private has U rotfb_info and unix defines global B rotfb_info. Kernel payload archive SHA-256: 80b137cbe005addf098fb25fc120386f8697d3a2253b854736e207695c2405f4.
- Local validation against the recovered actual package manifest: package-selection regex PASS; POSIX shell parameter-expansion fixture PASS; one-backslash find -exec syntax fixture PASS. These are targeted fixtures, not a full workflow YAML or guest-script syntax pass.
- No post-fix packaging run was started. Latest observed Actions runs remain historical failures; the current workflow is dispatch-only.


### 2026-11-20T22:03:48.973Z — i86pc kernel payload manifest-integrity verification

- Revalidated live branch HEAD before this update: 7b747ce4eecd72ae057157d2bfad1c9d7698581a. Current packaging workflow blob: c2033bf0e6811d3c977624904dd3882bb9ecaf3e.
- Parsed the actual system/kernel/platform manifest from Run 44795246582 and compared it against the extracted platform/i86pc/kernel payload from that same run's failed-kernel-payload.tar.gz.
- Verified 29 regular-file SHA-512/256 content hashes, 40 file/directory mode values, 11 directory entries and one hardlink inode relationship. Result: zero missing entries, zero content-hash mismatches, zero mode mismatches, zero hardlink mismatches.
- Verified unix: ELF64, x86-64, Solaris EXEC, 2,384,808 bytes; SHA-256 1ea83d1d5597d50893aeef6b8d3b79f3be9b439ca509ba31062529bb87d1f300. Its SHA-512/256 matches the IPS manifest value 7eb15466f9274b330d8ad39cdda78d2d264f0d4a084375611365dc0ddd5dbc4d.
- Verified gfx_private: ELF64, x86-64, Solaris REL, 70,352 bytes; SHA-256 53487c33f17d63b3670f15df4585149da6fea1198380f01a639abbd7e9de9207. Its SHA-512/256 matches the IPS manifest value fd128f5222eb2f17f6bda36ab2b077651da9f5019986988244cc2090638a1e1a.
- Symbol check: gfx_private contains rotfb_runtime_init, rotfb_runtime_fini, rotfb_runtime_flush_all, rotfb_runtime_callback_enter and rotfb_runtime_callback_exit; gfx_private references rotfb_info as undefined and the i86pc unix image defines global B rotfb_info.
- Kernel payload tar SHA-256: 80b137cbe005addf098fb25fc120386f8697d3a2253b854736e207695c2405f4.
- This validates the i86pc kernel payload from the failed packaging run against its own IPS manifest. It does not convert Run 44795246582 into a successful nightly/control build and does not validate the full repository with pkgrepo.


### 2026-11-20T22:07:31.438Z — installer preflight hardening and failure-path harness

- Revalidated live branch HEAD before this update: c00b34f66acca7de08fe0d1d56e8472309fa9783. Current packaging workflow blob: 49c0190e2f122ede8328e81435ca2bef0b68cff0.
- Workflow-only commits: aed244f7db9e064bf2775843ee7a6909a7028bdf (installer preflight) and 50ffd620ae75116ee7e32a4f33aba34b528acbb1 (checksum path validation and BE-selection warning).
- INSTALL.sh now verifies every SHA256SUMS entry using the target OpenIndiana digest utility, rejects malformed hashes and unsafe paths, requires at least one checksum entry, verifies the actual repo.redist with pkgrepo verify/list, verifies each selected FMRI with pkg contents, performs pkg install -n --no-refresh against a named test BE, and only then runs the actual install into that separate BE. It does not call beadm activate.
- INSTALL-README now accurately warns that IPS may change the next-boot BE selection; the running BE's package contents remain separate, and the user must inspect beadm list/boot selection before reboot. Firmware/NVRAM changes are not performed by the script.
- Extracted the exact INSTALL.sh heredoc from the workflow and ran sh -n: PASS (Linux /bin/sh syntax only; not target OpenIndiana runtime validation).
- Installer harness with stubbed digest/pkg/pkgrepo/beadm/uname/grep commands: success path PASS; tampered-file checksum rejection PASS before any pkg/pkgrepo call; solver dry-run failure PASS and no real pkg install call followed. This validates shell control flow only, not real IPS behavior or a physical install.
- OpenIndiana docs confirm pkg install -n is a no-change dry-run and --be-name installs into a new BE while leaving the current BE's package contents unchanged; --no-refresh avoids refreshing configured publishers. The next-boot selection must still be inspected before reboot.
- Official actions/upload-artifact@v4 documentation confirms multiline path inputs support negative glob exclusions; the diagnostic ZIP exclusion syntax is supported.
- Full workflow YAML parse, actionlint, full guest run-block sh -n, and target OpenIndiana IPS runtime tests remain NOT RUN. No workflow dispatch or build was started.


### 2026-11-20T22:14:03Z — workflow static acceptance, inactive-BE installer and physical runbook

- Revalidated live branch HEAD: 4c17935c1318366183910cb8d46cf4b5fc15b206. Current workflow blob: b151fa92aa0db89f22ae846cdb2f2ab25246f70b5. The local validation copy's Git blob hash matched the live GitHub blob exactly.
- Workflow-only changes in c00b34f66acca7de08fe0d1d56e8472309fa9783 close two release defects: the guest snapshot no longer uses unsupported Solaris/OpenSolaris `find -maxdepth`, and the diagnostics artifact now uploads only `rotfb-package-logs/`, not the staging tree. The package repository, kernel and boot payloads are preserved as named diagnostic archives under the logs directory. An incomplete uncompressed kit, ZIP or recheck tree is no longer included in diagnostics.
- The workflow now asserts the exact `$CODEMGR_WS/packages/$MACH/nightly-nd` PKGDIR, records IPS repository integrity as NOT_VERIFIED until both `pkgrepo verify` and `pkgrepo list` succeed, and preserves the offline dependency-closure gate unchanged.
- Installer evolution: ff575663773cd76a61c706e38afe401d295fff50 changed installation to a mounted inactive BE using `beadm create`, `beadm mount`, `pkg -R`, and `beadm unmount`; 9188900356d9feb0fdebcf5c90c93d0a3b1da826 leaves an incomplete inactive test BE for manual inspection after a failed install instead of automatically destroying it; 1aafc75de85325a064c9be74812c762d12cf0897 adds an existing-BE collision precheck and clarifies the README/rollback behavior. The installer does not call `beadm activate` or `beadm destroy`.
- Installer safety harness using stubbed digest/pkg/pkgrepo/beadm/uname/grep commands: success path PASS; checksum tampering rejected before package/BE side effects PASS; current-image solver failure stops before BE creation PASS; inactive-BE solver failure unmounts and retains the BE PASS; actual inactive-BE install failure unmounts and retains the BE PASS; pre-existing BE collision rejected before package solver or BE creation PASS. These are control-flow tests only, not real IPS/BE runtime tests.
- Exact current workflow validation: PyYAML 6.0.3 full YAML parse PASS; seven workflow steps recognized; `sh -n` on the exact extracted 337-line guest run block PASS; `sh -n` on guest prepare block PASS; `bash -n` on all host run blocks PASS; `sh -n` on the exact extracted 87-line INSTALL.sh PASS. Static audit of dispatch-only trigger, step IDs, expressions, PKG_REPO paths, PKGDIR assertion, closure gate, USB assembly gate and diagnostic upload scope PASS.
- `actionlint`, `shellcheck` and `yq` are not installed in the available validation environment. Target OpenIndiana /bin/sh runtime and real IPS/beadm execution remain NOT RUN. No physical device command was run.
- Created `ROTFB-PHYSICAL-TEST-RUNBOOK.md` in e94cbfe331b51f8d8bb33d6a31ed20326eded79d and aligned it with the mounted inactive-BE installer in 4c17935c1318366183910cb8d46cf4b5fc15b206. It is owner-operated and explicitly says not to execute until READY FOR PHYSICAL DEVICE TEST.
- Latest observed packaging Actions runs remain historical failures: 44795290315 and 44795246582. No new control build was dispatched; there is no queued or in-progress packaging run.
- Stage 1 — PASS for static workflow/YAML/shell acceptance; actionlint unavailable is recorded above.
- Stage 2 — BLOCKED: workflow is dispatch-only but exists only on `gop-rotation-hook`; repository default branch is `main`, and GitHub workflow_dispatch requires the workflow to exist on the default branch. No workflow-dispatch write capability is available here. No push trigger was added and `main` was not modified.
- Stage 3 — NOT VERIFIED on a successful control build. Historical failed-run repository structure is known, but no new live `pkgrepo verify/list` evidence exists.
- Stage 4 — BLOCKED: no target-equivalent OpenIndiana 2026.10 x86_64 baseline snapshot with installed FMRIs, publishers, incorporations, variants and facets is available. Direct build-repository dependency resolution is not offline closure.
- Stage 5 — historical failed-run i86pc kernel payload matched its own IPS manifest with no content/mode/hardlink mismatches, but current control-build payload remains NOT VERIFIED.
- Stage 6 — BLOCKED by Stage 4; no kit assembled or published.
- Stage 7 — NOT RUN; no release ZIP exists.
- Stage 8 — PREPARED; owner-side test/rollback runbook committed.
- Stage 9 — NOT VERIFIED; no owner-provided physical-device evidence.
- Current release status: INSTALL KIT NOT READY. READY FOR PHYSICAL DEVICE TEST and DEVICE VERIFIED have not been reached.
- Next concrete action: obtain a target-equivalent OpenIndiana 2026.10 x86_64 baseline snapshot for the mandatory offline solver proof, while preserving the default-branch/dispatch constraint for a later owner-approved release path.
