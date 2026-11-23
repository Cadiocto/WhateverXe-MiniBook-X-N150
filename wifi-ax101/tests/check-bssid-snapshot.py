#!/usr/bin/env python3
import pathlib
import sys

gld = pathlib.Path(sys.argv[1]).read_text()
giant = pathlib.Path(sys.argv[2]).read_text()
attach = pathlib.Path(sys.argv[3]).read_text()
struct = gld.split("struct iwm_gld {", 1)[1].split("};", 1)[0]
rx = gld.split("iwm_gld_rx_frame(void *gp, mblk_t *mp)", 1)[1].split("\n}", 1)[0]
set_bss = gld.split("iwm_gld_set_bss(struct iwm_gld *g)", 1)[1].split("\n}", 1)[0]
bssid_hook = gld.split("iwm_gld_bssid_hook(void *gp, int up)", 1)[1].split("\n}", 1)[0]
get_bssid = attach.split("iwm_il_get_bssid(void *arg, uint8_t *bssid)", 1)[1].split("\n}", 1)[0]
link_hook = gld.split("iwm_gld_link_hook(void *gp, int up)", 1)[1].split("\n}", 1)[0]
unregister = gld.split("iwm_gld_unregister(void *gp)", 1)[1].split("\n}", 1)[0]
giant_exit = giant.split("n80211_giant_exit(void)", 1)[1].split("\n}", 1)[0]
disable = giant.split("n80211_gld_callbacks_disable(void)", 1)[1].split("\n}", 1)[0]

checks = {
    "snapshot_is_first_struct_member": struct.index("volatile uint64_t g_bssid_word;") < struct.index("mac_handle_t"),
    "reader_uses_atomic_snapshot": "atomic_cas_64(&g->g_bssid_word, 0, 0)" in rx,
    "reader_does_not_copy_mutable_wifi_data": "g->g_wd.wd_bssid" not in rx,
    "reader_does_not_take_mutex": "mutex_enter" not in rx and "mutex_exit" not in rx,
    "reader_does_not_allocate": "kmem_alloc" not in rx and "kmem_zalloc" not in rx,
    "byte_order_is_explicit": "snapshot >> (i * 8)" in rx and "bssid[5] << 40" in gld,
    "native_atomic_header": "#include <sys/atomic.h>" in gld,
    "interrupt_bssid_hook_is_atomic": "iwm_il_get_bssid" in bssid_hook and "atomic_swap_64" in bssid_hook,
    "interrupt_bssid_reader_has_no_allocation": "kmem_alloc" not in get_bssid and "kmem_zalloc" not in get_bssid,
    "linkhook_no_longer_skips_interrupt": "servicing_interrupt" not in link_hook and "iwm_gld_set_bss(g)" in link_hook,
    "wifi_data_writer_serialized": "mutex_enter(&g->g_bssid_lock)" in set_bss and "mutex_exit(&g->g_bssid_lock)" in set_bss,
    "wifi_data_update_outside_giant": set_bss.index("mutex_exit(&n80211_giant)") < set_bss.index("mac_pdata_update"),
    "bssid_published_before_giant_drop": giant_exit.index("bssid_hook(xlate_arg") < giant_exit.index("mutex_exit(&n80211_giant)"),
    "interrupt_link_notification_deferred": "servicing_interrupt()" in giant_exit and "task_add(systq" in giant_exit,
    "callbacks_snapshotted_before_unlock": giant_exit.index("= n80211_rx_xlate") < giant_exit.index("mutex_exit(&n80211_giant)"),
    "callbacks_drained_before_return": "while (n80211_dispatch_refs != 0)" in disable and "cv_wait(" in disable,
    "callbacks_cleared_on_disable": "n80211_rx_xlate = NULL" in disable and "n80211_link_hook = NULL" in disable and "n80211_bssid_hook = NULL" in disable,
    "disable_preserves_mac_handles_for_rollback": "n80211_rx_mh = NULL" not in disable and "n80211_link_mh = NULL" not in disable,
    "gld_unregister_quiesces_first": unregister.index("n80211_gld_callbacks_disable()") < unregister.index("mac_unregister("),
    "gld_unregister_rolls_back_on_failure": "n80211_gld_callbacks_enable" in unregister,
    "gld_unregister_forgets_handles_only_after_success": unregister.index("if (err == 0)") < unregister.index("n80211_gld_callbacks_forget()") < unregister.index("mutex_destroy"),
}
failed = [name for name, ok in checks.items() if not ok]
for name, ok in checks.items():
    print("{}={}".format(name, "PASS" if ok else "FAIL"))
if failed:
    sys.exit(1)
print("BSSID_AND_CALLBACK_CONTRACT=PASS")
