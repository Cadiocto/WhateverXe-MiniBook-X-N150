# Compatibility header provenance

`sys/net80211_proto.h` is copied verbatim from illumos-gate:

- Repository: https://github.com/illumos/illumos-gate
- Revision: `098e95820891f8a83fae`
- Path: `usr/src/uts/common/sys/net80211_proto.h`
- License: BSD-style license retained verbatim in the header.

The gate keeps this header under `usr/src/uts/common/sys` for in-gate
consumers; it is not installed into the OpenIndiana proto's
`/usr/include/sys`. The adjacent `mac_wifi.h` includes it directly.
This copy supplies the protocol constants and frame layouts required by
the GLDv3 Wi-Fi adapter; it does not replace OpenBSD net80211.


## OpenBSD allocator / mbuf wait-flag overlay

The local patch `wifi-ax101/patches/0001-openbsd-allocator-flags.patch` is applied
to the pinned solnix checkout during CI; it does not change the solnix pin or
the upstream `if_iwx.c` source.

The pinned `if_iwx.c` is OpenBSD revision 1.229 (2026-05-28). OpenBSD's
`malloc(9)` contract uses `M_WAITOK=0x0001`, `M_NOWAIT=0x0002`, and
`M_CANFAIL=0x0004`; `sys/mbuf.h` aliases `M_WAIT` to `M_WAITOK` and
`M_DONTWAIT` to `M_NOWAIT`. The old local shim redefined `M_NOWAIT` and
`M_CANFAIL` as zero, overriding the allocator values before
`iwx_bsd_malloc()` tested `flags & M_NOWAIT`. That made non-waiting
allocations select `KM_SLEEP` instead of `KM_NOSLEEP`.

The overlay restores the shared flag contract and the source gate checks it.
**It does not yet implement OpenBSD `M_CANFAIL` failure semantics inside
`iwx_bsd_malloc()`; that remains a separate open allocator issue.**


## Native illumos net80211 header closure

Run #18 (GitHub Actions run `45086828528`, job `137407263126`, commit
`04c31c4a758cd11b4c191c0a8290a3abda1f1011`) passed the AX101 and allocator
static gates, then failed in the native GLDv3 translation unit at
`iwm_gld.c:40` because `<sys/net80211.h>` was absent from the OpenIndiana
proto include path. This is a verified missing-header blocker, not a compiler
or source-selection failure.

The following files are exact copies from
`illumos/illumos-gate@098e95820891f8a83fae679daece7fcf58b7b3a4`.
Git blob IDs are recorded to let CI verify byte identity before staging:

| Local path | Upstream path | Git blob | License retained in file |
|---|---|---|---|
| `compat/sys/net80211_proto.h` | `usr/src/uts/common/sys/net80211_proto.h` | `4fe7d40604615e3ffd1adb80d6e061fd4859c546` | BSD-style / GPL-2 dual notice |
| `compat/sys/net80211.h` | `usr/src/uts/common/sys/net80211.h` | `be6b65fe905fabd6ebfd31cf74d7048357510e12` | BSD-3-Clause / GPL-2 dual notice |
| `compat/sys/net80211_crypto.h` | `usr/src/uts/common/sys/net80211_crypto.h` | `72ef3ebec3c9ccc599ba29823d3561198d953252` | BSD-style / GPL-2 dual notice |
| `compat/sys/net80211_ht.h` | `usr/src/uts/common/sys/net80211_ht.h` | `449726aa0feae79d6bcea0f73e0625a7078a150e` | Two-condition BSD license notice |
| `compat/sys/net80211_amrr.h` | `usr/src/uts/common/sys/net80211_amrr.h` | `36bd2fcd0eacd88c7b2c307c32a4db32bff5ffa0` | ISC-style permission notice |
| `compat/net/wpa.h` | `usr/src/uts/common/net/wpa.h` | `82c4fd468a5f623411a98a814c40ba3d9ac4c87b` | CDDL header retained verbatim |

The direct include closure of `sys/net80211.h` is:
`sys/mac.h`, `sys/mac_provider.h`, `sys/ethernet.h`,
`sys/net80211_proto.h`, `sys/net80211_crypto.h`,
`sys/net80211_ht.h`, `sys/net80211_amrr.h`, `net/wpa.h`, and
`sys/atomic.h`. `net/wpa.h` additionally includes
`inet/wifi_ioctl.h` and `sys/net80211_crypto.h`.

The workflow verifies every local header against the pinned Git blob ID,
uses the target's native `/usr/include` header whenever one exists, and
stages only missing private headers into the existing
`iwm/compat-illumos` include root. It explicitly checks the remaining
native dependencies (`sys/mac.h`, `sys/mac_provider.h`,
`sys/ethernet.h`, `sys/atomic.h`, and `inet/wifi_ioctl.h`) rather than
vendoring or shadowing them. This is a narrow GLDv3 include-closure overlay,
not a replacement for the OpenBSD net80211 stack.

## net80211 prepend bounds fix

`wifi-ax101/patches/0002-net80211-prepend-bounds.patch` applies to the
pinned solnix net80211 adapter at
`src/ports/wifi/net80211/n80211_mbuf.c`. It replaces the
`b_rptr - len` bounds test, which forms an out-of-object pointer before
checking the lower bound, with integer-address range validation. It rejects
malformed read/write windows and insufficient headroom before deriving the
new read pointer. The existing userland harness gains exact-headroom and
one-byte-short regression cases. CI compiles and runs the harness, including
its existing negative control, before the kernel-module build.

This source change is a candidate until the patched harness and subsequent
OpenIndiana build pass. It does not establish physical packet-path behavior.


## Userland mbuf harness portability overlay

Run #19 (GitHub Actions run `45092903456`, job `137426241172`, commit
`e65fcba6d9c1cb5ab701dec483031896784a2bca`) failed before the native
header-closure stage. The userland harness did not compile on the OpenIndiana
host because the pinned test stub checked `#ifndef B_TRUE`; illumos exposes
`B_TRUE` and `B_FALSE` as enum constants, not preprocessor macros. The
condition therefore redefined `boolean_t`, conflicting with the native
typedef in `sys/types.h`. The same compile also found that the non-kernel
branch of `n80211_mbuf.c` called `bcopy()` without including
`<strings.h>`.

The new local overlay
`wifi-ax101/patches/0003-mbuf-test-portability.patch` makes the stub define
its fallback boolean type only on non-Solaris hosts (`!__sun`) and includes
`<strings.h>` in the userland test branch so the native declaration of
`bcopy()` is visible. The kernel include branch and production packet path
are unchanged by this test-portability patch.

Run #19 did not reach the mbuf tests, AX101 source gate, native header staging,
or kernel compilation. The prepend bounds change remains FIXED-CANDIDATE
until the harness passes and the OpenIndiana build proceeds through the
relevant translation units.


## Run #20 patch-format correction

Run #20 (GitHub Actions run `45097606012`, job `137440814065`, commit
`3e40f14c8331ac9d1385acb565bb8e42f2295754`) failed while applying
`0003-mbuf-test-portability.patch`, before the userland mbuf harness,
native header staging, or kernel compilation. The job log reports
`error: corrupt patch .../0003-mbuf-test-portability.patch:12`.

The first hunk declared seven old/new lines although its body contains six;
its corrected range is `@@ -24,6 +24,6 @@`. The second hunk was based
on a stale include context: the pinned solnix source at
`atiq-cs/solnix@1cee7021091f3df92b7816ece2902ff59b37f1be` has only
`iwx_mbuf_test_stubs.h`, `string.h`, and `#endif` in the userland
include block (lines 37-39); it does not include `stdint.h` there. The
corrected hunk is `@@ -37,3 +37,4 @@` and inserts `strings.h` after
`string.h`.

A local `git apply --check` and application test against a minimal file
reconstructed from the exact pinned source context passed. This validates
the unified-diff syntax and context shape, but is not a substitute for
applying the patch in the pinned checkout. Run #21 must prove that.

The `net80211_ht.h` license row above was also corrected after inspecting
the copied header's retained copyright/license text: it is a two-condition
BSD notice, not BSD-3-Clause. No source license text was changed.


## Run #21: portability patch ordering correction

Finding: AX101-TEST-002
First seen: 2026-11-23T02:04:24Z (Run #20)
Latest seen: 2026-11-23T03:10:39Z (Run #21)
Status: VERIFIED / FIXED-CANDIDATE

Run #21 (GitHub Actions run `45103889328`, job `137460654737`,
commit `98ac802b5cd8ff3bb8c35f17888d8e94fff1bcf2`) applied the
allocator overlay `0001` and prepend overlay `0002`, then failed while
applying `0003-mbuf-test-portability.patch` at
`src/ports/wifi/net80211/n80211_mbuf.c:37`. The job log reports:

`error: patch failed: src/ports/wifi/net80211/n80211_mbuf.c:37`

The cause is the ordered overlay dependency. Patch `0002` adds
`#include <stdint.h>` to the userland include block in
`n80211_mbuf.c`. Patch `0003` is applied afterwards, but its second hunk
still expected the original upstream block without that line. The hunk
therefore had stale context even though its first hunk (the illumos
`boolean_t` stub guard) was applicable.

The corrected `0003` hunk now explicitly includes the post-`0002`
`stdint.h` line and inserts `strings.h` after `string.h`. The first
hunk remains unchanged. A targeted local `git apply --check` sequence
against reconstructed exact upstream include contexts, applying the
`0002` include hunk before `0003`, passed. This is not yet a substitute
for applying both complete patches in the pinned checkout; Run #22 must
verify that, then execute the mbuf harness and proceed to native header
closure.

Run #21 did not reach the userland harness, AX101 source gate, native header
staging, or kernel translation-unit compilation. The prepend-bounds fix
remains FIXED-CANDIDATE; no runtime or physical packet-path claim is made.


## net80211 m_free override overlay

The local patch `wifi-ax101/patches/0004-net80211-mfree-override.patch` applies to the pinned solnix header
`src/ports/wifi/net80211/compat/sys/n80211_mbuf.h`. The upstream header documents that the two net80211 call sites use `m_free` only as `return m_free(m)` on a freshly allocated single management mbuf. In this narrow case, illumos `freemsg()` followed by `NULL` preserves the required result.

The shared driver adapter defines `m_free` with a different return contract. The overlay explicitly `#undef`s that macro before defining the net80211 single-mbuf wrapper. It does not change the wrapper's generated behavior or the pinned solnix revision; it makes the intentional API boundary explicit and removes the compiler's accidental-redefinition diagnostic. CI checks the build log and fails if any `m_free` redefinition warning remains. This local compatibility patch imports no third-party code and changes no upstream license text.


## AX101-ALLOC-002 — M_CANFAIL allocator contract

First seen: recorded in the accessible Run #5 engineering report; the original first-seen timestamp was not exposed in this invocation's history.
Latest seen: 2026-11-23T04:29:22Z
Baseline HEAD: `e47ab1086465e66fc16d2ed4b4fad5e9aab4124f`
Baseline parent: `bb375e40666aba39e9f4e37bf3331a6aaa13c9e2`
Pinned source: `atiq-cs/solnix@1cee7021091f3df92b7816ece2902ff59b37f1be`
Relevant workflow evidence: Run #25 `45111068752`, job `137483300319`, commit `e47ab1086465e66fc16d2ed4b4fad5e9aab4124f` completed the 30-TU build/link and existing tests; it did not test M_CANFAIL semantics.

**Observed:** `n80211_kcompat.c:251-263` selected `KM_NOSLEEP` only for `M_NOWAIT`; every other allocation used `KM_SLEEP`. The pinned `if_iwx.c` has five active `M_CANFAIL` allocations at lines 1532-1537, 1565-1571, 4247-4255, 7688-7691 and 7748-7751. Each caller checks for NULL and unwinds with ENOMEM.

**Contract evidence:** OpenBSD `malloc(9)` defines `M_CANFAIL` with `M_WAITOK` as a request that may wait but returns NULL rather than panic; it has no effect with `M_NOWAIT`. The pinned illumos `usr/src/man/man9f/kmem_alloc.9f` states that `KM_SLEEP` allocations may sleep and are guaranteed to succeed, while `KM_NOSLEEP` never sleeps and may return NULL. There is no exact illumos wait-and-fail equivalent.

**Root cause: VERIFIED.** The old wrapper silently mapped `M_WAITOK|M_CANFAIL` to `KM_SLEEP`, so the OpenBSD return-NULL contract was not represented.

**Selected resolution:** local overlay `0005-malloc-canfail-nonsleep.patch` introduces `iwx_bsd_kmem_flags()`: `M_NOWAIT` and `M_CANFAIL` select `KM_NOSLEEP`; ordinary waitable allocations select `KM_SLEEP`. This deliberately prefers a non-panicking, checked ENOMEM path over waiting when M_CANFAIL is requested. It can fail earlier under transient memory pressure than OpenBSD; the five current callers all check NULL. `mallocarray` overflow behavior is unchanged; no active `mallocarray` call site was found in the pinned Wi-Fi source.

**Alternatives:** retaining `KM_SLEEP` preserves waiting but cannot preserve M_CANFAIL's return-NULL behavior; changing flag values would reintroduce the cross-header collision; using `KM_NOSLEEP` for M_CANFAIL is the conservative supported mapping.

**Changes in the candidate commit:**
- `wifi-ax101/patches/0005-malloc-canfail-nonsleep.patch`: adds the shared flag-selection helper and uses it in `iwx_bsd_malloc`.
- `wifi-ax101/tests/n80211-allocator-flags.c`: tests M_WAITOK, M_NOWAIT, M_CANFAIL and M_ZERO combinations against the exact helper.
- `.github/workflows/wifi-ax101-openindiana.yml`: applies patch 0005 and compiles/runs the helper test before the existing mbuf suites.
- `wifi-ax101/check-ax101-source.sh`: checks helper integration, the flag mapping, and all five NULL-checked M_CANFAIL sites.
- `wifi-ax101/README.md` and this file: record Run #25 evidence and the conservative compatibility trade-off.

Lifecycle at source commit: **FIXED-CANDIDATE**. Promotion to FIXED-VERIFIED requires the new helper test, source gate, and subsequent OpenIndiana translation-unit compile/link to pass. This does not prove runtime memory-pressure behavior or physical networking.


## AX101-CI-PATCH-001 — malformed allocator patch hunk

First seen: 2026-11-23T04:44:48Z
Latest seen: 2026-11-23T05:05:16Z
HEAD at failure: `7d59deafc073ae91539e50fc86162f0df6545c1c`
Parent: `e47ab1086465e66fc16d2ed4b4fad5e9aab4124f`
Pinned source: `atiq-cs/solnix@1cee7021091f3df92b7816ece2902ff59b37f1be`
Workflow evidence: Run #26 `45113192316`, job `137489976111`, step 4, commit `7d59deafc073ae91539e50fc86162f0df6545c1c`.

**Observed:** OpenIndiana VM setup and pinned solnix checkout completed, then `git apply --check` rejected `wifi-ax101/patches/0005-malloc-canfail-nonsleep.patch` with `error: corrupt patch ...:45`. The job stopped before the allocator unit test, source gate, native translation-unit compilation, or link. Artifact `14790587546` is a 1,923-byte diagnostic artifact only; it is not a build result.

**Root cause: VERIFIED.** The second hunk declared seven old/new lines (`@@ -251,7 +252,7 @@`) but its body contains five lines on each side. The unified-diff parser reached EOF with two declared lines missing.

**Selected resolution:** correct only the hunk range to `@@ -251,5 +252,5 @@`. No C helper logic, allocator constants, pinned source, or build acceptance condition is changed. Adding unrelated context lines was rejected because the five-line hunk already contains sufficient exact context.

**Validation performed locally:** `git apply --stat` passed; `git apply --check` passed against a temporary fixture reconstructed from the exact pinned include/function context; `cc -std=c99 -Wall -Wextra -Werror` compiled the allocator test and all six checks passed. These are local syntax/context and host-compiler checks, not an OpenIndiana build.

**Lifecycle:** FIXED-CANDIDATE. Run #27 must prove the corrected patch applies to the complete pinned checkout, the six allocator checks pass on OpenIndiana, the source gate passes, and the 30-TU compile/link completes. Run #26 did not test allocator semantics. No runtime memory-pressure behavior or physical networking is established.


Status update (2026-11-23T05:11:16Z): Run #27 parsed the corrected five-line hunk but `git apply --check` still failed at `n80211_kcompat.c:251`. The earlier malformed-hunk-count diagnostic no longer reproduced; the full overlay was not yet applied. The separate context mismatch is tracked as AX101-CI-PATCH-002 below. Lifecycle remains FIXED-CANDIDATE until the complete overlay applies.

## AX101-CI-PATCH-002 — allocator overlay indentation mismatch

First seen: 2026-11-23T05:11:16Z
Latest seen: 2026-11-23T06:00:37Z
HEAD at failure: `6b16dbbb53e9bc3c7378876ab214dfd569d0ed3c`
Parent: `7d59deafc073ae91539e50fc86162f0df6545c1c`
Pinned source: `atiq-cs/solnix@1cee7021091f3df92b7816ece2902ff59b37f1be`
Path: `wifi-ax101/patches/0005-malloc-canfail-nonsleep.patch`, second hunk; upstream target `src/ports/wifi/net80211/n80211_kcompat.c`, `iwx_bsd_malloc()`.
Workflow evidence: Run #27 `45115843531`, job `137498379960`, step 4; log timestamp `2026-11-23T05:11:16.505Z`.

**Observed:** The OpenIndiana 2026.10 job checked out the pinned solnix revision successfully, applied overlays 0001–0004, then failed applying overlay 0005 with `error: patch failed: src/ports/wifi/net80211/n80211_kcompat.c:251`. It stopped before the allocator unit test, mbuf suites, source gate, header staging, native translation-unit compilation and link. Artifact `14791252949` (1,895 bytes) is diagnostic only.

**Root cause: VERIFIED.** At the pinned source revision, the relevant function lines use literal tab indentation: `\tvoid *p;`, `\tint km = ...`, and `\t(void) type;`. Overlay 0005 encoded those context/replacement lines with spaces. Git's exact context matcher therefore rejected the hunk. The hunk-count correction from AX101-CI-PATCH-001 was accepted by the parser; this is a distinct whitespace-context failure.

**Reproduction:** A local fixture with the exact pinned include and allocator-function context failed `git apply --check` with the original space-indented hunk. Changing only those indentation bytes to tabs made `git apply --check` pass. Changing the hunk line offsets was not selected: the tabs-only variant already passed.

**Selected resolution:** change only the three context/deletion/addition indentation prefixes in overlay 0005 to match the pinned source's tabs. Preserve helper logic, allocator flags, wire ABI, the solnix pin, the illumos-gate pin and all acceptance conditions.

**Validation performed locally:** exact-context `git apply --check` fixture passed after the tab-only correction; host `cc -std=c99 -Wall -Wextra -Werror` compiled the helper/test, and all six allocator-flag checks passed. This is not a full pinned-checkout application or an OpenIndiana build.

**Lifecycle:** FIXED-VERIFIED for patch application and compile/link. Run #29 (`45128665119`) applied the corrected overlay sequence to the exact pinned solnix checkout, passed the allocator and mbuf suites, and completed all 30 translation-unit compiler invocations plus the module link. Runtime memory-pressure behavior and physical networking remain NOT TESTED.


## AX101-CI-PATCH-003 — literal backslash-t in allocator overlay

Finding: AX101-CI-PATCH-003
First observed: 2026-11-23T06:08:10.385Z (Run #28)
Baseline HEAD: `afa08480d956a280331e1dc953533c7995659566`
Baseline parent: `6b16dbbb53e9bc3c7378876ab214dfd569d0ed3c`
Path: `wifi-ax101/patches/0005-malloc-canfail-nonsleep.patch`
Target after patch: `src/ports/wifi/net80211/n80211_kcompat.c:255`
Run #28: `45121693091`, job `137516679772`, artifact `14794221085`
Fix commit: `f900e457bfc2ac7b89fb397519e13143d76049d1`
Verification run: Run #29 `45128665119`, job `137538818671`, artifact `14796997103`

**Observed:** Run #28 successfully applied overlays 0001–0005 and passed the allocator tests, net80211 mbuf harness, iwx mbuf/mlist/cookie suites, negative controls and AX101 static source gate. Compilation then failed in `n80211_kcompat.c:255`:
`error: stray '\\' in program`
and
`error: unknown type name 'tint'`.
The job ended with `exit_code=2`. This was a malformed added C source line, not a patch-application failure. Run #28 artifact was 37,873 bytes; uploaded ZIP SHA-256:
`5ef0dce5283c924fa4cfbf3adb9d98dcccc9ec4c230f508a607743bbcb9391a2`.

**Root cause: VERIFIED.** The added line in patch 0005 contained the two literal characters backslash and `t` before `int`. After patch application, the C compiler received `\\tint km = ...` instead of a TAB-indented `int km = ...`.

**Selected correction:** commit `f900e457bfc2ac7b89fb397519e13143d76049d1` changes only that added indentation prefix to an actual TAB. The commit diff confirms the old literal backslash-t representation was replaced by a real tab byte. A byte-level scan of the current patch reports no literal backslash-t sequence; the modified added line contains an actual TAB. No allocator logic, ABI, source pin, firmware identity or acceptance condition was changed.

**Run #29 verification:** the workflow checked and applied overlays 0001–0005 in order against solnix commit `1cee7021091f3df92b7816ece2902ff59b37f1be`. The resulting `n80211_kcompat.c` compiled successfully as part of the 30 compiler invocations, and `ld -r -Breduce` linked the module. Artifact `14796997103` is 816,168 bytes; ZIP SHA-256:
`8a33dcd33e918b27270ae66c85acb3dabbb0bb9db13c474dace0731f54bedea4`.
The OpenIndiana build status is `exit_code=0`.

**Tests in Run #29:**
- allocator helper: 6 checks, 0 failures;
- net80211 mbuf harness: all cases passed, `allocs==frees==20`, negative control caught;
- iwx mbuf suite: 71 checks; mlist: 26; cookie: 23; all passed;
- all 13 iwx sabotage/negative controls were caught;
- AX101 SO/HR1 static source gate passed;
- 30 compiler invocations and the module link completed.

**M_CANFAIL call-site audit:** the five pinned `if_iwx.c` call sites are at lines 1532–1537, 1565–1571, 4247–4255, 7688–7691 and 7748–7751. Each checks the allocation result for NULL. The first three set `err = ENOMEM` and unwind; the final two return `ENOMEM` directly. The async command allocations preserve `M_NOWAIT` precedence; the non-async `M_WAIT | M_CANFAIL` form uses the conservative failure-capable path.

**Allocator limitation remains explicit:** illumos has no exact waitable-and-failable equivalent for OpenBSD `M_WAITOK | M_CANFAIL`. The helper maps `M_CANFAIL` to `KM_NOSLEEP`, so it may fail earlier under transient memory pressure instead of waiting. The six helper tests verify flag-to-mode selection, not real memory-pressure behavior. The mapping is intentionally not described as fully semantically equivalent.

**Artifact/ELF limits:** the artifact contains an ELF64 little-endian AMD64 relocatable object (`ET_REL`), and independent inspection found no `.interp` section. The OpenIndiana `elfdump` report includes `TEXTREL`; 132 undefined symbols are present in the symbol listing and were not reconciled against the target kernel's complete export set. The workflow stores `elfdump`/`nm` output diagnostically with `|| true`; it does not implement a strict independent ELF acceptance parser. Firmware file and license were staged; the firmware file hash is `bed4958236366c3c451b3ec994a56b2ad83c08d70e0a1eae42fb00f2eaabbcc8`. Firmware ALIVE and runtime operation remain NOT TESTED.

**Subsequent branch status (outside PATCH-003):** after Run #29, later commits introduced overlay `0006-net80211-ioctl-macro-closure.patch`. Run #30 `45130051443` failed with `corrupt patch ...0006...:60`; Run #31 `45130849256` then failed applying that patch at `src/ports/wifi/net80211/compat/net80211/n80211_stack.h:122`. Source HEAD before this provenance-only update: `6a4fccf67e04ddd2f3927e538971ce3279b19449`. The provenance-only documentation commit recording this state is `b3922ed7647edb7dfb7793b678aaa630942a9dd7`; no driver source or build inputs changed in that commit. These later failures do not invalidate Run #29's PATCH-003 verification, but the latest branch HEAD is not build-green. No automatic retry is authorized or recorded here.


## AX101-CI-PATCH-004 — malformed hunk count in ioctl macro overlay

Finding: AX101-CI-PATCH-004  
First seen: 2026-11-23T07:22:49.797Z (Run #30)  
Latest seen: 2026-11-23T07:29:00.707Z (Run #31)  
Failure HEAD: `fd1e08ae8fe5c90d922b92ded0490355ff1ca934`  
Failure parent: `f900e457bfc2ac7b89fb397519e13143d76049d1`  
Fix commit: `6a4fccf67e04ddd2f3927e538971ce3279b19449`  
Path: `wifi-ax101/patches/0006-net80211-ioctl-macro-closure.patch`  
Target: second unified-diff hunk for `src/ports/wifi/net80211/compat/net80211/n80211_stack.h`  
Run #30: `45130051443`, job `137543235465`, step 4, log at `2026-11-23T07:22:49.797Z`  
Run #31: `45130849256`, job `137545745809`, step 4.

**Observed:** Run #30 rejected overlay 0006 with `error: corrupt patch ...0006-net80211-ioctl-macro-closure.patch:60`. It stopped before the allocator tests, mbuf suites, source gate and native compilation.

**Root cause: VERIFIED.** The second hunk declared 37 old lines and 7 new lines, while its body contained 36 old and 6 new lines. The unified-diff counts were each one too large.

**Selected correction:** commit `6a4fccf67e04ddd2f3927e538971ce3279b19449` changed only the hunk header from `@@ -122,37 +123,7 @@` to `@@ -122,36 +123,6 @@`. No source, ioctl values, structure layout, pinned revision or acceptance gate changed.

**Validation:** Run #31 no longer reported a corrupt-patch/parser error; it advanced to a context mismatch in the same hunk. `git apply --stat` on the corrected patch parsed successfully and reported one file, 7 insertions and 36 deletions. This verifies hunk syntax only, not full application.

**Lifecycle:** FIXED-VERIFIED for unified-diff syntax/counts only. Full patch application remained blocked by AX101-CI-PATCH-005. No OpenIndiana compilation or runtime ioctl behavior is established by this finding.


## AX101-CI-PATCH-005 — ioctl overlay tab mismatch in deletion context

Finding: AX101-CI-PATCH-005  
First seen: 2026-11-23T07:29:00.707780Z (Run #31)  
Latest seen: 2026-11-23T07:31:23Z  
Failure HEAD: `6a4fccf67e04ddd2f3927e538971ce3279b19449`  
Failure parent: `fd1e08ae8fe5c90d922b92ded0490355ff1ca934`  
Current pre-fix HEAD: `0ad2ffaaeac20c1c5a4de94933f8b30cea08f38c` (parent `b3922ed7647edb7dfb7793b678aaa630942a9dd7`; intervening commits are provenance-only)  
Path: `wifi-ax101/patches/0006-net80211-ioctl-macro-closure.patch`  
Pinned upstream: `atiq-cs/solnix@1cee7021091f3df92b7816ece2902ff59b37f1be`  
Upstream file: `src/ports/wifi/net80211/compat/net80211/n80211_stack.h:155`  
Upstream blob: `8b94c33afe9e378814b070864b34f0faedaa8fca`  
Workflow evidence: Run #31 `45130849256`, job `137545745809`, step 4, failure at `2026-11-23T07:29:00.707Z`.

**Observed:** after overlays 0001–0005 applied, Run #31 failed at `git apply --check` for overlay 0006 with `patch failed: .../n80211_stack.h:122`. The reported line is the hunk start, not the exact mismatching line.

**Root cause: VERIFIED.** A byte-level comparison of every deleted context line against the pinned source found exactly one mismatch: `SIOCS80211KEYRUN`. The pinned upstream line is `#define SIOCS80211KEYRUN<TAB>_IOW(...)` (one TAB), while the deletion context in overlay 0006 contained two TABs. Git therefore rejected the otherwise correctly counted hunk.

**Semantic audit:** the pinned `Makefile.illumos` force-includes `sys/n80211_prelude.h`; that prelude includes `net80211/n80211_stack.h`. The pinned `ieee80211_ioctl.c` includes `net80211/ieee80211_var.h` and `net80211/ieee80211_ioctl.h`; `ieee80211_var.h` also includes the ioctl header. The pinned `ieee80211_ioctl.h` provides the canonical definitions for the 29 duplicated SIOC80211 command names, with their original command numbers and request structures. It does not define `SIOCG80211ZSTATS`, so that one definition remains in `n80211_stack.h`. The overlay removes only the conflicting ifreq-only duplicates; it does not alter wire structures or the pinned ioctl header.

**Selected resolution:** change only the deleted `SIOCS80211KEYRUN` context line in overlay 0006 to use the single TAB present in the pinned source. Alternatives such as changing hunk offsets, adding fuzz, or modifying upstream source were rejected because the exact one-TAB correction is sufficient and preserves the source.

**Local validation:** an isolated fixture reconstructed from the exact pinned upstream lines 108–157 reproduced the failure before correction (`git apply --check` returned 1 at hunk line 122). After changing only that deletion line to one TAB, `git apply --check` passed. `git apply --stat` reported one file changed, 7 insertions and 36 deletions; after application, `SIOCG80211ZSTATS` was the only remaining `SIOC*80211` definition in `n80211_stack.h`. This is a targeted exact-context fixture, not a full pinned-checkout or OpenIndiana build.

**Change in this commit:** `wifi-ax101/patches/0006-net80211-ioctl-macro-closure.patch` changes one TAB byte in the deletion context. Parent: `0ad2ffaaeac20c1c5a4de94933f8b30cea08f38c`. No firmware, PCI identity, ABI structure, solnix pin or illumos-gate pin changed.

**Lifecycle:** FIXED-CANDIDATE pending the next controlled OpenIndiana run. That run must apply all overlays in order, pass allocator/mbuf/source gates, compile and link the native module, and confirm no `SIOC*80211` macro-redefinition warnings remain. Strict ELF acceptance, firmware ALIVE, runtime attach and physical networking remain NOT TESTED.
