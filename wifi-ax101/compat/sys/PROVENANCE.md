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
