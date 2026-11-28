#pragma D option quiet

/* RUN128 physical AX101 scan capture.
 * Run with: dtrace -q -s physical-ax101-scan.d -c '/usr/sbin/dladm scan-wifi'
 */

sdt:::iwx_rx_packet_header
{
	@rx_by_header[args[3], args[4], args[0], args[1]] = count();
	@rx_by_length[args[5]] = count();
}

sdt:::iwx_rx_packet_validation
{
	@rx_validation[args[1], args[2]] = count();
}

sdt:::iwx_rx_packet_rejected
{
	@rx_rejected[args[0], args[1], args[2]] = count();
}

sdt:::iwx_rx_packet_opcode_normalized
{
	@rx_normalized[args[0], args[1], args[2], args[3]] = count();
}

sdt:::iwx_rx_packet_unknown
{
	@rx_unknown[args[0], args[1], args[2], args[3]] = count();
}

sdt:::iwx_scan_complete_notification
{
	@scan_complete[args[0], args[1], args[2], args[3]] = count();
}

sdt:::iwx_scan_iteration_notification
{
	@scan_iteration[args[0], args[1], args[2], args[3], args[4]] = count();
}

sdt:::iwx_scan_complete_malformed
{
	@scan_complete_malformed[args[0]] = count();
}

sdt:::iwx_scan_end_enter
{
	@scan_end_enter[args[0], args[1], args[2], args[3]] = count();
}

sdt:::iwx_scan_end_ignored
{
	@scan_end_ignored[args[0], args[1], args[2], args[3]] = count();
}

sdt:::iwx_scan_state_publish
{
	@scan_state_publish[args[0], args[1], args[2], args[3]] = count();
}

sdt:::iwx_init_task_schedule
{
	@init_task_schedule[args[0], args[1], args[2], args[3], args[4], args[5], args[6]] = count();
}

sdt:::iwx_init_task_enter
{
	@init_task_enter[args[0], args[1], args[2], args[3], args[4], args[5]] = count();
}

sdt:::iwx_init_task_exit
{
	@init_task_exit[args[0], args[1], args[2], args[3]] = count();
}

sdt:::iwx_rfkill_state
{
	@rfkill[args[0], args[1], args[2]] = count();
}

sdt:::iwx_rx_mpdu_valid
{
	@rx_mpdu_valid[args[0]] = count();
}

sdt:::iwx_rx_beacon_or_probe_response
{
	@rx_beacon_probe[args[0], args[1], args[2]] = count();
}

sdt:::ieee80211_bss_node_update
{
	@bss_node_update[args[0], args[1], args[2], args[3], args[4]] = count();
}

sdt:::iwx_bss_tree_snapshot
{
	@bss_tree[args[0], args[1], args[2]] = count();
}

sdt:::iwx_il_scan_enter
{
	@il_scan_enter[args[0], args[1], args[2], args[3]] = count();
}

sdt:::iwx_il_scan_exit
{
	@il_scan_exit[args[0], args[1], args[2], args[3], args[4], args[5]] = count();
}

sdt:::iwm_gld_wl_scan_exit
{
	@gld_wl_scan_exit[args[0], args[1], args[2]] = count();
}

sdt:::iwm_gld_ess_list_snapshot
{
	@gld_ess_snapshot[args[0], args[1], args[2], args[3]] = count();
}

END
{
	printf("\n=== RX HEADER: group, opcode, raw qid, normalized qid ===\n");
	printa(@rx_by_header);
	printf("\n=== RX packet lengths ===\n");
	printa(@rx_by_length);
	printf("\n=== RX validation: valid, unsolicited ===\n");
	printa(@rx_validation);
	printf("\n=== RX rejected: raw qid, idx, errno ===\n");
	printa(@rx_rejected);
	printf("\n=== RX normalized code, qid, idx, unsolicited ===\n");
	printa(@rx_normalized);
	printf("\n=== Unknown RX code, length, qid, idx ===\n");
	printa(@rx_unknown);
	printf("\n=== Firmware IWX_SCAN_COMPLETE_UMAC notifications: uid, status, last_iter, schedule ===\n");
	printa(@scan_complete);
	printf("\n=== Firmware IWX_SCAN_ITERATION_COMPLETE_UMAC notifications ===\n");
	printa(@scan_iteration);
	printf("\n=== Malformed scan-complete lengths ===\n");
	printa(@scan_complete_malformed);
	printf("\n=== iwx_endscan enter / ignored ===\n");
	printa(@scan_end_enter);
	printa(@scan_end_ignored);
	printf("\n=== Scan-state publication ===\n");
	printa(@scan_state_publish);
	printf("\n=== init_task scheduling: reason, flags, gen, if_flags, state, task_add_rc, scan_gen ===\n");
	printa(@init_task_schedule);
	printf("\n=== init_task enter/exit ===\n");
	printa(@init_task_enter);
	printa(@init_task_exit);
	printf("\n=== RFKill raw CSR, asserted, sc_flags ===\n");
	printa(@rfkill);
	printf("\n=== Valid RX MPDU lengths ===\n");
	printa(@rx_mpdu_valid);
	printf("\n=== Beacon/probe response subtype, channel, RSSI ===\n");
	printa(@rx_beacon_probe);
	printf("\n=== net80211 BSS new/update, node_count, scan_count, ESS len, RSSI ===\n");
	printa(@bss_node_update);
	printf("\n=== BSS tree walked, ESS results, net80211 state ===\n");
	printa(@bss_tree);
	printf("\n=== iwm_il_scan enter/exit ===\n");
	printa(@il_scan_enter);
	printa(@il_scan_exit);
	printf("\n=== GLDv3 WL_SCAN result ===\n");
	printa(@gld_wl_scan_exit);
	printf("\n=== GLDv3 ESS-list snapshot ===\n");
	printa(@gld_ess_snapshot);
}
