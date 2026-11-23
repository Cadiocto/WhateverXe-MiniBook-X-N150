#!/usr/bin/env python3
import pathlib
import re
import sys

source = pathlib.Path(sys.argv[1]).read_text()
struct = source.split("struct iwm_gld {", 1)[1].split("};", 1)[0]
rx = source.split("iwm_gld_rx_frame(void *gp, mblk_t *mp)", 1)[1].split("\n}", 1)[0]
writer = source.split("iwm_gld_set_bss(void *gp, const uint8_t *bssid)", 1)[1].split("\n}", 1)[0]

checks = {
    "snapshot_is_first_struct_member": re.sub(r"/\\*.*?\\*/", "", struct, flags=re.S).lstrip().startswith("volatile uint64_t g_bssid_word;"),
    "writer_serialized": "mutex_enter(&g->g_bssid_lock)" in writer and "mutex_exit(&g->g_bssid_lock)" in writer,
    "writer_uses_atomic_publication": "atomic_swap_64(&g->g_bssid_word" in writer,
    "reader_uses_atomic_snapshot": "atomic_cas_64(&g->g_bssid_word, 0, 0)" in rx,
    "reader_does_not_copy_mutable_wifi_data": "g->g_wd.wd_bssid" not in rx,
    "reader_does_not_take_mutex": "mutex_enter" not in rx and "mutex_exit" not in rx,
    "reader_does_not_allocate": "kmem_alloc" not in rx and "kmem_zalloc" not in rx,
    "byte_order_is_explicit": "snapshot >> (i * 8)" in rx and "bssid[5] << 40" in source,
    "native_atomic_header": "#include <sys/atomic.h>" in source,
}
failed = [name for name, ok in checks.items() if not ok]
for name, ok in checks.items():
    print("{}={}".format(name, "PASS" if ok else "FAIL"))
if failed:
    sys.exit(1)
print("BSSID_SOURCE_CONTRACT=PASS")
