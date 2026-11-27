#!/usr/bin/env python3
"""Apply the deterministic RUN87 post-generation BSD-to-illumos adaptation.

This runs only after the pinned Solnix source is regenerated/replayed. Every
rewrite is exact-count checked so source drift or partial application fails
closed. gen-illumos-src.sh --update then persists the canonical source patch.
"""
from __future__ import annotations

import argparse
from pathlib import Path


def replace_exact(path: Path, old: str, new: str, count: int = 1) -> None:
    data = path.read_text(encoding="utf-8")
    seen = data.count(old)
    if seen != count:
        raise SystemExit("{}: expected {} matches, found {} for {!r}".format(path, count, seen, old[:90]))
    path.write_text(data.replace(old, new), encoding="utf-8")


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--source-root", required=True, type=Path, help="pinned Solnix checkout root")
    root = ap.parse_args().source_root.resolve()
    wifi = root / "src/ports/wifi"
    gen = wifi / "iwx/build/if_iwx_illumos.c"
    gld = wifi / "iwm/iwm_gld.c"
    glue = wifi / "iwx/iwx_glue.c"
    mbuf_test = wifi / "iwx/iwx_mbuf_test.c"
    for p in (gen, gld, glue, mbuf_test):
        if not p.is_file():
            raise SystemExit("missing expected pinned source file: " + str(p))

    replace_exact(gen,
        "\tm->m_data = pktdata + desc_size;\n\tm->m_pkthdr.len = m->m_len = len;",
        "\tif (!iwx_m_set_window(m, (uint8_t *)pktdata + desc_size, len)) {\n"
        "\t\tIC2IFP(ic)->if_ierrors++;\n\t\tm_freem(m);\n\t\treturn;\n\t}")
    replace_exact(gen, "\t\tmemmove(m->m_data + 2, m->m_data, hdrlen);",
        "\t\tif (!iwx_m_shift_right(m, (size_t)hdrlen, 2)) {\n"
        "\t\t\tIC2IFP(ic)->if_ierrors++;\n\t\t\tm_freem(m);\n\t\t\treturn;\n\t\t}")
    replace_exact(gen, "m->m_len >= sizeof(struct ieee80211_qosframe_addr4)", "iwx_m_len(m) >= sizeof(struct ieee80211_qosframe_addr4)")
    replace_exact(gen, "m->m_len >= sizeof(struct ieee80211_qosframe)", "iwx_m_len(m) >= sizeof(struct ieee80211_qosframe)")
    replace_exact(gen, "totlen = m->m_pkthdr.len;", "totlen = (int)iwx_m_pktlen(m);")
    replace_exact(gen,
        "\t\tif (m_defrag(m, M_DONTWAIT)) {\n\t\t\tm_freem(m);\n\t\t\treturn ENOBUFS;\n\t\t}",
        "\t\t/* msgpullup() may replace the head; it consumes m on failure. */\n"
        "\t\tm = iwx_m_defrag(m);\n\t\tif (m == NULL)\n\t\t\treturn ENOBUFS;")
    replace_exact(gen, "\t\t\tni = m->m_pkthdr.ph_cookie;",
        "\t\t\tni = iwx_m_take_cookie(m);\n"
        "\t\t\tif (ni == NULL) {\n\t\t\t\tifp->if_oerrors++;\n\t\t\t\tm_freem(m);\n\t\t\t\tcontinue;\n\t\t\t}")
    replace_exact(gen, "m->m_len < sizeof (*eh)", "iwx_m_len(m) < sizeof (*eh)")
    replace_exact(gen,
        "\t\tcase IWX_REPLY_RX_MPDU_CMD: {\n\t\t\tsize_t maxlen = IWX_RBUF_SIZE - offset - minsz;",
        "\t\tcase IWX_REPLY_RX_MPDU_CMD: {\n\t\t\tvoid *copy_pktdata;\n"
        "\t\t\tsize_t maxlen = IWX_RBUF_SIZE - offset - minsz;\n"
        "\t\t\tsize_t pktdata_offset = minsz;")
    replace_exact(gen, "m = m_copym(m0, 0, M_COPYALL, M_DONTWAIT);", "m = iwx_m_copy(m0);")
    replace_exact(gen,
        "\t\t\t\tm_adj(m, offset);\n\t\t\t\tiwx_rx_mpdu_mq(sc, m, pkt->data, maxlen, ml);",
        "\t\t\t\tm_adj(m, offset);\n"
        "\t\t\t\t/* copymsg() relocates the packet to a new dblk. */\n"
        "\t\t\t\tcopy_pktdata = iwx_m_at(m, pktdata_offset, maxlen);\n"
        "\t\t\t\tif (copy_pktdata == NULL) {\n"
        "\t\t\t\t\tifp->if_ierrors++;\n\t\t\t\t\tm_freem(m);\n"
        "\t\t\t\t\terr = EPROTO;\n\t\t\t\t\tbreak;\n"
        "\t\t\t\t}\n"
        "\t\t\t\tiwx_rx_mpdu_mq(sc, m, copy_pktdata, maxlen, ml);")
    replace_exact(gen,
        "\t\t}\n\n\t\toffset += roundup(len, IWX_FH_RSCSR_FRAME_ALIGN);",
        "\t\t}\n\t\tif (err != 0)\n\t\t\tbreak;\n\n"
        "\t\toffset += roundup(len, IWX_FH_RSCSR_FRAME_ALIGN);")

    replace_exact(gld, "#include <sys/condvar.h>\n", "#include <sys/condvar.h>\n#include <sys/callo.h>\t\t/* CALLOUT_FLAG_ABSOLUTE */\n")
    replace_exact(glue, "rw_destroy(&sc->ioctl_rwl);", "iwx_rw_destroy(&sc->ioctl_rwl);")
    replace_exact(mbuf_test,
        "\tmemset(iwx_mtod(dup), 0x55, 16);\n"
        "\tCHECK(((unsigned char *)iwx_mtod(m))[0] == 0xAA,\n"
        "\t    \"writing the copy must not change the original\");\n"
        "\tCHECK(iwx_m_adj(dup, 4) && iwx_m_len(m) == 16,\n"
        "\t    \"trimming the copy must not change the original's length\");",
        "\tCHECK(iwx_m_adj(dup, 4) && iwx_m_len(m) == 16,\n"
        "\t    \"trimming the copy must not change the original's length\");\n"
        "\t{\n\t\tunsigned char *copy_payload = iwx_m_at(dup, 4, 8);\n"
        "\t\tunsigned char *original_payload = iwx_m_at(m, 8, 8);\n"
        "\t\tCHECK(copy_payload != NULL && original_payload != NULL,\n"
        "\t\t    \"payload offset must remain addressable after copy head trim\");\n"
        "\t\tif (copy_payload != NULL && original_payload != NULL) {\n"
        "\t\t\tCHECK(copy_payload != original_payload,\n"
        "\t\t\t    \"copied packet payload pointer must not alias DMA source\");\n"
        "\t\t\tCHECK(memcmp(copy_payload, original_payload, 8) == 0,\n"
        "\t\t\t    \"relative packet-data offset must preserve copied bytes\");\n"
        "\t\t}\n\t}\n"
        "\tmemset(iwx_mtod(dup), 0x55, iwx_m_len(dup));\n"
        "\tCHECK(((unsigned char *)iwx_mtod(m))[0] == 0xAA,\n"
        "\t    \"writing the copy must not change the original\");")
    print("RUN87_POSTGEN_ADAPTATION=PASS")
    print("FILES_CHANGED=iwx/build/if_iwx_illumos.c,iwm/iwm_gld.c,iwx/iwx_glue.c,iwx/iwx_mbuf_test.c")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
