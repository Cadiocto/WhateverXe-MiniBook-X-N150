#!/usr/bin/env python3
"""Fail-closed source contract and negative-control gate for RUN87."""
from __future__ import annotations
import argparse
import re
import shutil
import tempfile
from pathlib import Path


def clean(s: str) -> str:
    return re.sub(r'/\*.*?\*/|//[^\n]*', lambda m: '\n' * m.group(0).count('\n'), s, flags=re.S)


def validate(root: Path) -> list[str]:
    w = root / 'src/ports/wifi'
    gen = clean((w/'iwx/build/if_iwx_illumos.c').read_text())
    mbuf = clean((w/'iwx/iwx_mbuf.c').read_text())
    test = (w/'iwx/iwx_mbuf_test.c').read_text()
    gld = (w/'iwm/iwm_gld.c').read_text()
    glue = clean((w/'iwx/iwx_glue.c').read_text())
    kernel = (w/'iwx/compat/sys/iwx_kernel.h').read_text()
    giant = clean((w/'net80211/n80211_giant.c').read_text())
    adapter = (w/'iwx/iwx_mbuf_adapter.h').read_text()
    make = (w/'iwx/Makefile.illumos').read_text()
    generator = (w/'iwx/gen-illumos-src.sh').read_text()
    patch = (w/'iwx/patches/if_iwx.illumos.patch').read_text()
    checks = [
      (not re.search(r'->\s*m_(?:data|len|pkthdr)\b|\bm_(?:defrag|copym)\s*\(', gen), 'generated IWX source has no BSD mbuf field/API dereferences'),
      ('-include iwx_mbuf_adapter.h' in make and 'build/if_iwx_illumos.c' in make, 'native Makefile compiles generated source with mblk adapter'),
      ('iwx_m_set_window(m, (uint8_t *)pktdata + desc_size, len)' in gen and 'iwx_m_shift_right(m, (size_t)hdrlen, 2)' in gen and gen.count('iwx_m_len(m) >= sizeof(struct ieee80211_qosframe') == 2, 'RX MPDU window, padding and segment-length conversions'),
      ('m = iwx_m_copy(m0);' in gen and 'size_t pktdata_offset = minsz;' in gen and 'iwx_m_at(m, pktdata_offset, maxlen)' in gen and 'iwx_rx_mpdu_mq(sc, m, copy_pktdata, maxlen, ml)' in gen and 'err = EPROTO;' in gen, 'multi-MPDU clone uses clone-relative packet data pointer and fails closed'),
      ('b_wptr - m->b_rptr' in mbuf and 'b_cont' in mbuf and 'DB_BASE(m)' in mbuf and 'DB_LIM(m)' in mbuf and 'msgpullup(m, -1)' in mbuf and 'copymsg((mblk_t *)m)' in mbuf, 'mblk adapter uses STREAMS windows, chains, linearization and deep copy'),
      ('#include <sys/callo.h>' in gld and 'CALLOUT_FLAG_ABSOLUTE' in gld and 'cv_timedwait_hires' in gld, 'callout flag declaration included without changing absolute timeout'),
      ('iwx_rw_destroy(&sc->ioctl_rwl)' in glue and 'iwx_rw_destroy' in kernel and 'iwx_rw_destroy' in giant and not re.search(r'(?<!iwx_)rw_destroy\s*\(&sc->ioctl_rwl', glue), 'typed rwlock adapter used for init/destroy type contract'),
      ('iwx_m_take_cookie(m)' in gen and '#include "iwx_mcookie.h"' in adapter, 'management-frame cookie taken through mblk adapter'),
      ('--update' in generator and 'patches/if_iwx.illumos.patch' in generator and 'copy_pktdata' in patch and 'iwx_m_copy' in patch, 'generator canonical patch contains RUN87 source rewrites'),
      ('copy_payload != original_payload' in test and 'iwx_m_at(dup, 4, 8)' in test, 'regression harness proves copied payload pointer does not alias DMA source'),
    ]
    failed = [label for ok, label in checks if not ok]
    if failed:
        raise AssertionError('; '.join(failed))
    return [label for _, label in checks]


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument('--source-root', required=True, type=Path)
    ap.add_argument('--self-test', action='store_true')
    args = ap.parse_args()
    root = args.source_root.resolve()
    passed = validate(root)
    print('RUN87_ILLUMOS_ADAPTATION_STATIC={}/{} PASS'.format(len(passed), len(passed)))
    for i, label in enumerate(passed, 1): print('{:02d} PASS {}'.format(i, label))
    if args.self_test:
        with tempfile.TemporaryDirectory(prefix="ax101-run87-mutants-") as td:
            mutant_root = Path(td)
            source_paths = [
                "iwx/build/if_iwx_illumos.c", "iwx/iwx_mbuf.c", "iwx/iwx_mbuf_test.c",
                "iwm/iwm_gld.c", "iwx/iwx_glue.c", "iwx/compat/sys/iwx_kernel.h",
                "net80211/n80211_giant.c", "iwx/iwx_mbuf_adapter.h", "iwx/Makefile.illumos",
                "iwx/gen-illumos-src.sh", "iwx/patches/if_iwx.illumos.patch",
            ]
            for rel in source_paths:
                src = root / "src/ports/wifi" / rel
                dst = mutant_root / "src/ports/wifi" / rel
                dst.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(src, dst)
            mutants = [
                ("iwx/build/if_iwx_illumos.c", "iwx_m_len(m)", "m->m_len", "reintroduced BSD m_len"),
                ("iwx/build/if_iwx_illumos.c", "iwx_rx_mpdu_mq(sc, m, copy_pktdata, maxlen, ml)", "iwx_rx_mpdu_mq(sc, m, pkt->data, maxlen, ml)", "reused original DMA pointer after deep copy"),
                ("iwx/iwx_mbuf.c", "copymsg((mblk_t *)m)", "dupmsg((mblk_t *)m)", "changed deep copy to shared duplicate"),
            ]
            for rel, old, new, label in mutants:
                path = mutant_root / "src/ports/wifi" / rel
                data = path.read_text()
                if old not in data:
                    raise SystemExit("cannot install negative-control mutant: " + label)
                path.write_text(data.replace(old, new, 1))
                try:
                    validate(mutant_root)
                except AssertionError:
                    print("PASS negative control caught: " + label)
                else:
                    raise SystemExit("FAIL negative control escaped: " + label)
                shutil.copy2(root / "src/ports/wifi" / rel, path)
        print("RUN87_SOURCE_MUTANTS=3/3 PASS")
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
