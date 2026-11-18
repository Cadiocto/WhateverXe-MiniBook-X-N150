#!/bin/sh
# ROTFB-only clean-build and linker evidence collector for illumos nightly.
set -u

usage() {
    echo "usage: $0 prepare|watch|finalize WORKSPACE LOGDIR" >&2
    exit 2
}

[ "$#" -eq 3 ] || usage
MODE=$1
WORKSPACE=$2
LOGDIR=$3

mkdir -p "$LOGDIR" || exit 1
READY="$LOGDIR/ld-wrapper-ready.txt"
FAIL="$LOGDIR/ld-wrapper-install-failure.txt"
STOP="$LOGDIR/ld-wrapper-stop"
EVENTS="$LOGDIR/ld-wrapper-events.log"
LD_PATH="$WORKSPACE/usr/src/tools/proto/root_i386-nd/opt/onbld/bin/amd64/ld"
LD_BACKUP="$LD_PATH.rotfb-real"

record_file_state() {
    label=$1
    path=$2
    if [ -f "$path" ]; then
        values=$(perl -e 'my @s = stat($ARGV[0]); die "stat failed\n" unless @s; printf "%d %d", $s[9], $s[7]' "$path" 2>/dev/null) || values="UNKNOWN UNKNOWN"
        set -- $values
        printf '%s\tPRESENT\t%s\t%s\t%s\n' "$label" "$path" "${1:-UNKNOWN}" "${2:-UNKNOWN}"
    else
        printf '%s\tABSENT\t%s\t-\t-\n' "$label" "$path"
    fi
}

case "$MODE" in
prepare)
    : > "$LOGDIR/target-preclean-state.tsv"
    : > "$LOGDIR/linker-status.tsv"
    : > "$LOGDIR/linker-commands.txt"
    : > "$LOGDIR/rotfb-boot-link-commands.txt"
    : > "$LOGDIR/rotfb-runtime-link-commands.txt"
    : > "$EVENTS"
    rm -f "$READY" "$FAIL" "$STOP" "$LD_BACKUP" "$LD_BACKUP.tmp" 2>/dev/null || exit 1
    mkdir -p "$LOGDIR/linker-output" || exit 1
    rm -f "$LOGDIR/linker-output/"* 2>/dev/null || true

    printf 'target_cleanup_started_utc=%s\n' "$(date -u +%FT%TZ)" > "$LOGDIR/rotfb-target-cleanup.txt"
    for pair in i86pc:dboot i86pc:unix.o i86pc:gfx_private i86xpv:dboot i86xpv:unix.o i86xpv:gfx_private; do
        arch=${pair%%:*}
        target=${pair#*:}
        case "$arch:$target" in
            i86pc:dboot) path="$WORKSPACE/usr/src/uts/i86pc/unix/dboot/obj64/dboot" ;;
            i86pc:unix.o) path="$WORKSPACE/usr/src/uts/i86pc/unix/obj64/unix.o" ;;
            i86pc:gfx_private) path="$WORKSPACE/usr/src/uts/i86pc/gfx_private/obj64/gfx_private" ;;
            i86xpv:dboot) path="$WORKSPACE/usr/src/uts/i86xpv/unix/dboot/obj64/dboot" ;;
            i86xpv:unix.o) path="$WORKSPACE/usr/src/uts/i86xpv/unix/obj64/unix.o" ;;
            i86xpv:gfx_private) path="$WORKSPACE/usr/src/uts/i86xpv/gfx_private/obj64/gfx_private" ;;
        esac
        record_file_state "$arch.$target" "$path" >> "$LOGDIR/target-preclean-state.tsv"
        if [ -e "$path" ] || [ -L "$path" ]; then
            rm -f "$path" || exit 1
        fi
        if [ -e "$path" ] || [ -L "$path" ]; then
            echo "CLEANUP_FAILED $path" >> "$LOGDIR/rotfb-target-cleanup.txt"
            exit 1
        fi
        echo "REMOVED_OR_ABSENT $path" >> "$LOGDIR/rotfb-target-cleanup.txt"
    done
    echo "target_cleanup_status=PASS" >> "$LOGDIR/rotfb-target-cleanup.txt"

    # Start the freshness clock only after every ROTFB final target is absent.
    START_EPOCH=$(date -u +%s) || exit 1
    START_UTC=$(date -u +%FT%TZ) || exit 1
    printf '%s\n' "$START_EPOCH" > "$LOGDIR/build-start-epoch.txt"
    printf '%s\n' "$START_UTC" > "$LOGDIR/build-start-utc.txt"
    printf 'build_started_utc=%s\nbuild_started_epoch=%s\n' "$START_UTC" "$START_EPOCH" >> "$LOGDIR/rotfb-target-cleanup.txt"
    ;;

watch)
    install_wrapper() {
        [ -x "$LD_PATH" ] || return 1
        if grep -q '^# ROTFB_LD_WRAPPER_V1$' "$LD_PATH" 2>/dev/null; then
            return 0
        fi

        backup_tmp="$LD_BACKUP.tmp.$$"
        if ! cp -p "$LD_PATH" "$backup_tmp"; then
            echo "failed to preserve original linker: $LD_PATH" > "$FAIL"
            return 1
        fi
        if ! mv -f "$backup_tmp" "$LD_BACKUP"; then
            echo "failed to publish original linker backup: $LD_BACKUP" > "$FAIL"
            return 1
        fi

        wrapper_tmp="$LD_PATH.rotfb-wrapper.$$"
        rendered_tmp="$LD_PATH.rotfb-rendered.$$"
        cat > "$wrapper_tmp" <<'WRAPPER'
#!/bin/sh
# ROTFB_LD_WRAPPER_V1
ROTFB_LD_REAL='@ROTFB_LD_REAL@'
ROTFB_LINK_OUTPUT_DIR='@ROTFB_LINK_OUTPUT_DIR@'
prev=
out=
target=
expected=
expected_alt=
arch=
for arg do
    if [ "$prev" = "-o" ]; then
        out=$arg
    fi
    prev=$arg
done
case "$PWD" in
*/usr/src/uts/i86pc/*) arch=i86pc ;;
*/usr/src/uts/i86xpv/*) arch=i86xpv ;;
esac
case "$arch:$out" in
    i86pc:dboot/obj64/dboot|i86xpv:dboot/obj64/dboot)
        target=dboot.$arch
        expected=dboot/obj64/rotfb_boot.o
        expected_alt=obj64/rotfb_boot.o
        ;;
    i86pc:obj64/dboot|i86xpv:obj64/dboot)
        target=dboot.$arch
        expected=obj64/rotfb_boot.o
        expected_alt=dboot/obj64/rotfb_boot.o
        ;;
    i86pc:obj64/unix.o|i86xpv:obj64/unix.o)
        target=unix.o.$arch
        expected=obj64/rotfb_boot.o
        ;;
    i86pc:obj64/gfx_private|i86xpv:obj64/gfx_private)
        case "$PWD" in
        */usr/src/uts/i86pc/gfx_private|*/usr/src/uts/i86xpv/gfx_private)
            target=gfx_private.$arch
            expected=obj64/rotfb_runtime.o
            ;;
        esac
        ;;
esac
if [ -z "$target" ]; then
    exec "$ROTFB_LD_REAL" "$@"
fi
has_object=NO
expected_found=$expected
for arg do
    case "$arg" in
        "$expected") has_object=YES; expected_found=$arg ;;
        "$expected_alt") has_object=YES; expected_found=$arg ;;
    esac
done
pid=$$
ts=$(date -u +%FT%TZ)
logfile="$ROTFB_LINK_OUTPUT_DIR/$target.$pid.log"
cmdfile="$ROTFB_LINK_OUTPUT_DIR/$target.$pid.cmd"
statusfile="$ROTFB_LINK_OUTPUT_DIR/$target.$pid.status"
printf 'timestamp=%s\npid=%s\ntarget=%s\ncwd=%s\noutput=%s\nexpected_object=%s\nobject_argument_present=%s\ncommand=%s\n' \
    "$ts" "$pid" "$target" "$PWD" "$out" "$expected_found" "$has_object" "$*" \
    > "$cmdfile"
"$ROTFB_LD_REAL" "$@" > "$logfile" 2>&1
rc=$?
cat "$logfile"
printf '%s\t%s\t%s\t%s\t%s\t%s\t%s\n' \
    "$ts" "$target" "$rc" "$out" "$has_object" "$PWD" "$pid" \
    > "$statusfile"
exit "$rc"
WRAPPER
        if ! sed \
            -e "s|@ROTFB_LD_REAL@|$LD_BACKUP|g" \
            -e "s|@ROTFB_LINK_OUTPUT_DIR@|$LOGDIR/linker-output|g" \
            "$wrapper_tmp" > "$rendered_tmp"; then
            echo "failed to render linker wrapper at $LD_PATH" > "$FAIL"
            rm -f "$wrapper_tmp" "$rendered_tmp"
            return 1
        fi
        rm -f "$wrapper_tmp"
        if ! chmod 0755 "$rendered_tmp" || ! mv -f "$rendered_tmp" "$LD_PATH"; then
            echo "failed to install linker wrapper at $LD_PATH" > "$FAIL"
            rm -f "$rendered_tmp"
            return 1
        fi

        count=$(grep -c '^event=' "$EVENTS" 2>/dev/null || true)
        if [ "$count" -eq 0 ]; then
            event=installed
        else
            event=reinstalled
        fi
        printf 'event=%s timestamp=%s path=%s backup=%s\n' \
            "$event" "$(date -u +%FT%TZ)" "$LD_PATH" "$LD_BACKUP" >> "$EVENTS"
        count=$((count + 1))
        {
            printf 'first_or_last_install_utc=%s\n' "$(date -u +%FT%TZ)"
            printf 'wrapper_path=%s\nreal_linker=%s\nwrapper_install_count=%s\n' \
                "$LD_PATH" "$LD_BACKUP" "$count"
        } > "$READY.tmp"
        mv -f "$READY.tmp" "$READY"
        return 0
    }

    tries=0
    while [ ! -f "$STOP" ]; do
        if [ -x "$LD_PATH" ] && ! grep -q '^# ROTFB_LD_WRAPPER_V1$' "$LD_PATH" 2>/dev/null; then
            if ! install_wrapper; then
                exit 1
            fi
        fi
        if [ ! -f "$READY" ]; then
            tries=$((tries + 1))
            if [ "$tries" -ge 1800 ]; then
                echo "timed out waiting for $LD_PATH" > "$FAIL"
                exit 1
            fi
        fi
        sleep 1
    done

    if [ ! -f "$READY" ]; then
        echo "linker wrapper was never installed at $LD_PATH" > "$FAIL"
        exit 1
    fi
    if grep -q '^# ROTFB_LD_WRAPPER_V1$' "$LD_PATH" 2>/dev/null; then
        restore_tmp="$LD_PATH.rotfb-restore.$$"
        if [ ! -f "$LD_BACKUP" ] || ! cp -p "$LD_BACKUP" "$restore_tmp" ||
            ! mv -f "$restore_tmp" "$LD_PATH"; then
            echo "failed to restore original linker at $LD_PATH" > "$FAIL"
            rm -f "$restore_tmp"
            exit 1
        fi
        echo "event=restored timestamp=$(date -u +%FT%TZ) path=$LD_PATH" >> "$EVENTS"
    fi
    rm -f "$LD_BACKUP"
    echo "watcher_stopped_utc=$(date -u +%FT%TZ)" >> "$EVENTS"
    [ ! -f "$FAIL" ]
    ;;

finalize)
    START_EPOCH=$(cat "$LOGDIR/build-start-epoch.txt" 2>/dev/null || echo 0)
    [ -n "$START_EPOCH" ] || START_EPOCH=0
    : > "$LOGDIR/linker-status.tsv"
    : > "$LOGDIR/linker-commands.txt"
    : > "$LOGDIR/rotfb-boot-link-commands.txt"
    : > "$LOGDIR/rotfb-runtime-link-commands.txt"
    for f in "$LOGDIR/linker-output/"*.status; do
        [ -f "$f" ] || continue
        cat "$f" >> "$LOGDIR/linker-status.tsv"
    done
    for f in "$LOGDIR/linker-output/"*.cmd; do
        [ -f "$f" ] || continue
        cat "$f" >> "$LOGDIR/linker-commands.txt"
        case "$f" in
            */dboot.*.cmd|*/unix.o.*.cmd) cat "$f" >> "$LOGDIR/rotfb-boot-link-commands.txt" ;;
            */gfx_private.*.cmd) cat "$f" >> "$LOGDIR/rotfb-runtime-link-commands.txt" ;;
        esac
    done

    EVIDENCE="$LOGDIR/rotfb-build-evidence.txt"
    : > "$EVIDENCE"
    OVERALL=PASS
    if [ -f "$READY" ]; then
        echo "linker_wrapper_status=INSTALLED" >> "$EVIDENCE"
        install_count=$(sed -n 's/^wrapper_install_count=//p' "$READY" | tail -n 1)
        printf 'linker_wrapper_install_count=%s\n' "${install_count:-UNKNOWN}" >> "$EVIDENCE"
        [ "${install_count:-0}" -ge 1 ] 2>/dev/null || OVERALL=INCOMPLETE
    else
        echo "linker_wrapper_status=INCOMPLETE" >> "$EVIDENCE"
        OVERALL=INCOMPLETE
    fi
    if [ -f "$FAIL" ]; then
        echo "linker_wrapper_failure=$(cat "$FAIL")" >> "$EVIDENCE"
        OVERALL=FAIL
    fi
    printf 'build_started_utc=%s\nbuild_started_epoch=%s\n' \
        "$(cat "$LOGDIR/build-start-utc.txt" 2>/dev/null || echo UNKNOWN)" "$START_EPOCH" >> "$EVIDENCE"
    echo >> "$EVIDENCE"

    check_target() {
        label=$1
        path=$2
        expected=$3
        expected_cwd=$4
        expected_output=$5
        command_count=0
        status_count=0
        log_count=0
        bad_rc_count=0
        object_bad_count=0
        pair_bad_count=0
        for f in "$LOGDIR/linker-output/$label."*.cmd; do
            [ -f "$f" ] || continue
            command_count=$((command_count + 1))
            base=$(printf '%s' "$f" | sed 's/\.cmd$//')
            [ -f "$base.status" ] && [ -f "$base.log" ] || pair_bad_count=$((pair_bad_count + 1))
            cmd_target=$(sed -n 's/^target=//p' "$f" | tail -n 1)
            cmd_cwd=$(sed -n 's/^cwd=//p' "$f" | tail -n 1)
            cmd_output=$(sed -n 's/^output=//p' "$f" | tail -n 1)
            cmd_expected=$(sed -n 's/^expected_object=//p' "$f" | tail -n 1)
            cmd_has_object=$(sed -n 's/^object_argument_present=//p' "$f" | tail -n 1)
            if [ "$cmd_target" != "$label" ] || [ "$cmd_cwd" != "$expected_cwd" ] ||
                [ "$cmd_output" != "$expected_output" ] || [ "$cmd_expected" != "$expected" ]; then
                pair_bad_count=$((pair_bad_count + 1))
            fi
            if [ "$cmd_has_object" != YES ]; then
                object_bad_count=$((object_bad_count + 1))
            fi
        done
        for f in "$LOGDIR/linker-output/$label."*.status; do
            [ -f "$f" ] || continue
            status_count=$((status_count + 1))
            base=$(printf '%s' "$f" | sed 's/\.status$//')
            [ -f "$base.cmd" ] && [ -f "$base.log" ] || pair_bad_count=$((pair_bad_count + 1))
            rc=$(awk -F '\t' '{print $3}' "$f")
            [ "$rc" = 0 ] || bad_rc_count=$((bad_rc_count + 1))
            if ! awk -F '\t' -v label="$label" -v out="$expected_output" -v cwd="$expected_cwd" '
                $2 != label || $4 != out || $5 != "YES" || $6 != cwd { bad = 1 }
                END { exit (bad ? 1 : 0) }
            ' "$f"; then
                pair_bad_count=$((pair_bad_count + 1))
            fi
        done
        for f in "$LOGDIR/linker-output/$label."*.log; do
            [ -f "$f" ] || continue
            log_count=$((log_count + 1))
        done

        if [ -f "$path" ]; then
            stats=$(perl -e 'my @s = stat($ARGV[0]); die "stat failed\n" unless @s; printf "%d\t%d", $s[9], $s[7]' "$path" 2>/dev/null) || stats="0\t0"
            mtime=$(printf '%s' "$stats" | awk -F '\t' '{print $1}')
            size=$(printf '%s' "$stats" | awk -F '\t' '{print $2}')
            mtime_utc=$(perl -MPOSIX=strftime -e 'my @s=stat($ARGV[0]); die unless @s; print strftime("%Y-%m-%dT%H:%M:%SZ", gmtime($s[9]))' "$path" 2>/dev/null || echo UNKNOWN)
        else
            mtime=0; size=0; mtime_utc=ABSENT
        fi
        fresh=NO
        if [ -f "$path" ] && [ "$size" -gt 0 ] 2>/dev/null && [ "$mtime" -ge "$START_EPOCH" ] 2>/dev/null; then
            fresh=YES
        fi
        elf=NO
        if [ -f "$path" ] && perl -e 'open(my $f, "<", $ARGV[0]) or exit 2; binmode($f); my $magic; my $n = read($f, $magic, 4); exit($n == 4 && $magic eq "\x7fELF" ? 0 : 1)' "$path"; then
            elf=YES
        fi
        diagnostics=NONE
        if [ "$log_count" -gt 0 ]; then
            if grep -E -i -q 'ld:[[:space:]]+fatal:|undefined symbol|ld:[[:space:]]+error:|relocation error|fatal error:|cannot open.*(file|input)|file not found' "$LOGDIR/linker-output/$label."*.log 2>/dev/null; then
                diagnostics=FOUND
            fi
        else
            diagnostics=INCOMPLETE
        fi
        object_present=NO
        if [ "$command_count" -eq 1 ] && [ "$object_bad_count" -eq 0 ]; then
            object_present=YES
        fi
        linker_status=INCOMPLETE
        if [ "$bad_rc_count" -gt 0 ]; then
            linker_status=NONZERO
        elif [ "$command_count" -eq 1 ] && [ "$status_count" -eq 1 ] &&
            [ "$log_count" -eq 1 ] && [ "$pair_bad_count" -eq 0 ]; then
            if [ "$object_present" = YES ]; then
                linker_status=0
            else
                linker_status=OBJECT_MISSING
            fi
        fi
        status=PASS
        if [ "$linker_status" = NONZERO ] || [ "$diagnostics" = FOUND ] ||
            [ "$linker_status" = OBJECT_MISSING ]; then
            status=FAIL
            OVERALL=FAIL
        elif [ "$fresh" != YES ] || [ "$elf" != YES ] ||
            [ "$object_present" != YES ] || [ "$linker_status" != 0 ] ||
            [ "$diagnostics" != NONE ] || [ "$pair_bad_count" -ne 0 ]; then
            status=INCOMPLETE
            [ "$OVERALL" = FAIL ] || OVERALL=INCOMPLETE
        fi
        {
            printf '[%s]\n' "$label"
            printf 'target_path=%s\n' "$path"
            printf 'expected_working_directory=%s\n' "$expected_cwd"
            printf 'expected_output=%s\n' "$expected_output"
            printf 'expected_object_argument=%s\n' "$expected"
            printf 'linker_command_detected_count=%s\n' "$command_count"
            printf 'linker_status_record_count=%s\n' "$status_count"
            printf 'linker_output_log_count=%s\n' "$log_count"
            printf 'linker_pair_mismatches=%s\n' "$pair_bad_count"
            printf 'target_fresh_after_build_start=%s\n' "$fresh"
            printf 'mtime_epoch=%s\nmtime_utc=%s\nsize_bytes=%s\n' "$mtime" "$mtime_utc" "$size"
            printf 'elf_magic_valid=%s\n' "$elf"
            printf 'object_argument_present=%s\n' "$object_present"
            printf 'linker_exit_status=%s\n' "$linker_status"
            printf 'linker_nonzero_invocations=%s\n' "$bad_rc_count"
            printf 'linker_diagnostics=%s\n' "$diagnostics"
            printf 'status=%s\n\n' "$status"
        } >> "$EVIDENCE"
    }

    check_target dboot.i86pc "$WORKSPACE/usr/src/uts/i86pc/unix/dboot/obj64/dboot" dboot/obj64/rotfb_boot.o "$WORKSPACE/usr/src/uts/i86pc/unix" dboot/obj64/dboot
    check_target unix.o.i86pc "$WORKSPACE/usr/src/uts/i86pc/unix/obj64/unix.o" obj64/rotfb_boot.o "$WORKSPACE/usr/src/uts/i86pc/unix" obj64/unix.o
    check_target gfx_private.i86pc "$WORKSPACE/usr/src/uts/i86pc/gfx_private/obj64/gfx_private" obj64/rotfb_runtime.o "$WORKSPACE/usr/src/uts/i86pc/gfx_private" obj64/gfx_private
    check_target dboot.i86xpv "$WORKSPACE/usr/src/uts/i86xpv/unix/dboot/obj64/dboot" dboot/obj64/rotfb_boot.o "$WORKSPACE/usr/src/uts/i86xpv/unix" dboot/obj64/dboot
    check_target unix.o.i86xpv "$WORKSPACE/usr/src/uts/i86xpv/unix/obj64/unix.o" obj64/rotfb_boot.o "$WORKSPACE/usr/src/uts/i86xpv/unix" obj64/unix.o
    check_target gfx_private.i86xpv "$WORKSPACE/usr/src/uts/i86xpv/gfx_private/obj64/gfx_private" obj64/rotfb_runtime.o "$WORKSPACE/usr/src/uts/i86xpv/gfx_private" obj64/gfx_private

    if [ -s "$LOGDIR/linker-status.tsv" ]; then
        echo "linker_status_records=$(wc -l < "$LOGDIR/linker-status.tsv" | tr -d ' ')" >> "$EVIDENCE"
    else
        echo "linker_status_records=0" >> "$EVIDENCE"
        [ "$OVERALL" = FAIL ] || OVERALL=INCOMPLETE
    fi

    compile_check() {
        key=$1
        file=$2
        pattern=$3
        count=0
        if [ -f "$file" ]; then
            count=$(grep -E -c "$pattern" "$file" 2>/dev/null || true)
        fi
        printf '%s_compile_commands=%s\n' "$key" "$count" >> "$EVIDENCE"
        if [ "$count" -lt 1 ]; then
            [ "$OVERALL" = FAIL ] || OVERALL=INCOMPLETE
        fi
    }
    compile_check rotfb_boot_i86pc "$LOGDIR/rotfb-boot-compile-commands.txt" ' -Di86pc .* -c -o obj64/rotfb_boot\.o .*rotfb_boot\.c'
    compile_check rotfb_boot_i86xpv "$LOGDIR/rotfb-boot-compile-commands.txt" ' -D__i86xpv .* -c -o obj64/rotfb_boot\.o .*rotfb_boot\.c'
    compile_check rotfb_runtime_i86pc "$LOGDIR/rotfb-runtime-compile-commands.txt" ' -Di86pc .* -c -o obj64/rotfb_runtime\.o .*rotfb_runtime\.c'
    compile_check rotfb_runtime_i86xpv "$LOGDIR/rotfb-runtime-compile-commands.txt" ' -D__i86xpv .* -c -o obj64/rotfb_runtime\.o .*rotfb_runtime\.c'
    compile_check gfxp_bitmap_i86pc "$LOGDIR/gfxp-bitmap-compile-commands.txt" ' -Di86pc .* -c -o obj64/gfxp_bitmap\.o .*gfxp_bitmap\.c'
    compile_check gfxp_bitmap_i86xpv "$LOGDIR/gfxp-bitmap-compile-commands.txt" ' -D__i86xpv .* -c -o obj64/gfxp_bitmap\.o .*gfxp_bitmap\.c'
    compile_check gfxp_fb_i86pc "$LOGDIR/gfxp-fb-compile-commands.txt" ' -Di86pc .* -c -o obj64/gfxp_fb\.o .*gfxp_fb\.c'
    compile_check gfxp_fb_i86xpv "$LOGDIR/gfxp-fb-compile-commands.txt" ' -D__i86xpv .* -c -o obj64/gfxp_fb\.o .*gfxp_fb\.c'

    if [ -f "$LOGDIR/patch-apply.log" ]; then
        APPLIED=$(grep -c '^APPLIED ' "$LOGDIR/patch-apply.log" || true)
        PATCH_ORDER=$(awk '/^APPLIED / { if (n++) printf ","; printf "%s", $2 }' "$LOGDIR/patch-apply.log")
        EXPECTED_ORDER='0001-framebuffer-metadata.patch,0002-boot-fb-rotation.patch,0003-gfxp-bitmap-tem-rotation.patch,0004-Makefile-files.patch,0005-rotfb-lifecycle-hardening.patch,0006-rotfb-console-contract-and-copy-safety.patch,0007-gfxp-bitmap-bounds-hardening.patch,0008-gfxp-detach-failure-safety.patch,0009-gfxp-bitmap-wide-row-offsets.patch,0010-gfxp-bitmap-vis-safety.patch,0011-gfxp-bitmap-cursor-state.patch'
        echo "patches_applied=$APPLIED" >> "$EVIDENCE"
        echo "patch_order=$PATCH_ORDER" >> "$EVIDENCE"
        if [ "$APPLIED" -ne 11 ] || [ "$PATCH_ORDER" != "$EXPECTED_ORDER" ]; then
            OVERALL=FAIL
            echo "patch_order_match=NO" >> "$EVIDENCE"
        else
            echo "patch_order_match=YES" >> "$EVIDENCE"
        fi
    else
        echo "patches_applied=UNKNOWN" >> "$EVIDENCE"
        echo "patch_order=UNKNOWN" >> "$EVIDENCE"
        OVERALL=INCOMPLETE
    fi

    NIGHTLY_RC=$(cat "$LOGDIR/nightly-exit-code.txt" 2>/dev/null || echo UNKNOWN)
    echo "nightly_exit_code=$NIGHTLY_RC" >> "$EVIDENCE"
    case "$NIGHTLY_RC" in
        0) ;;
        UNKNOWN|'') [ "$OVERALL" = FAIL ] || OVERALL=INCOMPLETE ;;
        *) OVERALL=FAIL ;;
    esac
    printf 'overall_status=%s\n' "$OVERALL" >> "$EVIDENCE"
    [ "$OVERALL" = PASS ]
    ;;
*)
    usage
    ;;
esac
