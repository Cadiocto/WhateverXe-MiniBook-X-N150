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
