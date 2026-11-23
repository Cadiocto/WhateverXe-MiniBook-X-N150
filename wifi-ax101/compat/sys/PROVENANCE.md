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
