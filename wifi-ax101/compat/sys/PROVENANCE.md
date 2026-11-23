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
