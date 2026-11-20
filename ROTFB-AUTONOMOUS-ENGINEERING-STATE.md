# ROTFB Autonomous Engineering State

Last updated: 2026-11-20T21:59:02.859Z

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
| 3. IPS repository validation | NOT VERIFIED | No successfully generated current repository artifact inspected |
| 4. Offline dependency closure | BLOCKED | Target-equivalent OpenIndiana 2026.10 package baseline and offline solver proof absent |
| 5. Kernel payload validation | NOT VERIFIED | No successful current packaging payload inspected |
| 6. Offline Install Kit assembly | NOT RUN / BLOCKED | Assembly intentionally refuses unless offline closure is PASS |
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
