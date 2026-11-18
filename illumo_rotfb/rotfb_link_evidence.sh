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
    START_EPOCH=$(date -u +%s) || exit 1
    START_UTC=$(date -u +%FT%TZ) || exit 1
    printf '%s\n' "$START_EPOCH" > "$LOGDIR/build-start-epoch.txt"
    printf '%s\n' "$START_UTC" > "$LOGDIR/build-start-utc.txt"
    : > "$LOGDIR/target-preclean-state.tsv"
    : > "$LOGDIR/linker-status.tsv"
    : > "$LOGDIR/rotfb-boot-link-commands.txt"
    : > "$LOGDIR/rotfb-runtime-link-commands.txt"
    mkdir -p "$LOGDIR/linker-output"

    printf 'build_started_utc=%s\nbuild_started_epoch=%s\n' "$START_UTC" "$START_EPOCH" > "$LOGDIR/rotfb-target-cleanup.txt"
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
    ;;

watch)
    LD_PATH="$WORKSPACE/usr/src/tools/proto/root_i386-nd/opt/onbld/bin/amd64/ld"
    LD_BACKUP="$LD_PATH.rotfb-real"
    tries=0
    while [ ! -x "$LD_PATH" ]; do
        tries=$((tries + 1))
        if [ "$tries" -ge 1800 ]; then
            echo "timed out waiting for $LD_PATH" > "$FAIL"
            exit 1
        fi
        sleep 1
    done
    if [ -e "$LD_BACKUP" ]; then
        echo "refusing to overwrite existing linker backup: $LD_BACKUP" > "$FAIL"
        exit 1
    fi
    if ! cp -p "$LD_PATH" "$LD_BACKUP"; then
        echo "failed to preserve original linker: $LD_PATH" > "$FAIL"
        exit 1
    fi
    if ! rm -f "$LD_PATH"; then
        echo "failed to replace linker path: $LD_PATH" > "$FAIL"
        exit 1
    fi
    cat > "$LD_PATH.in" <<'WRAPPER'
#!/bin/sh
# Temporary wrapper: record exact ROTFB target linker invocations and status.
ROTFB_LD_REAL='@ROTFB_LD_REAL@'
ROTFB_LINK_OUTPUT_DIR='@ROTFB_LINK_OUTPUT_DIR@'
prev=
out=
target=
expected=
for arg do
    if [ "$prev" = "-o" ]; then
        out=$arg
    fi
    prev=$arg
done
case "$PWD" in
*/usr/src/uts/i86pc/unix)
    case "$out" in
        dboot/obj64/dboot) target=dboot.i86pc; expected=dboot/obj64/rotfb_boot.o ;;
        obj64/unix.o) target=unix.o.i86pc; expected=obj64/rotfb_boot.o ;;
    esac
    ;;
*/usr/src/uts/i86xpv/unix)
    case "$out" in
        dboot/obj64/dboot) target=dboot.i86xpv; expected=dboot/obj64/rotfb_boot.o ;;
        obj64/unix.o) target=unix.o.i86xpv; expected=obj64/rotfb_boot.o ;;
    esac
    ;;
*/usr/src/uts/i86pc/gfx_private)
    [ "$out" = "obj64/gfx_private" ] && target=gfx_private.i86pc && expected=obj64/rotfb_runtime.o
    ;;
*/usr/src/uts/i86xpv/gfx_private)
    [ "$out" = "obj64/gfx_private" ] && target=gfx_private.i86xpv && expected=obj64/rotfb_runtime.o
    ;;
esac
if [ -z "$target" ]; then
    exec "$ROTFB_LD_REAL" "$@"
fi
has_object=NO
for arg do
    [ "$arg" = "$expected" ] && has_object=YES
done
ts=$(date -u +%FT%TZ)
logfile="$ROTFB_LINK_OUTPUT_DIR/$target.$$.log"
cmdfile="$ROTFB_LINK_OUTPUT_DIR/$target.$$.cmd"
statusfile="$ROTFB_LINK_OUTPUT_DIR/$target.$$.status"
printf 'timestamp=%s\ntarget=%s\ncwd=%s\noutput=%s\nexpected_object=%s\nobject_argument_present=%s\ncommand=%s\n' \
    "$ts" "$target" "$PWD" "$out" "$expected" "$has_object" "$*" \
    > "$cmdfile"
"$ROTFB_LD_REAL" "$@" > "$logfile" 2>&1
rc=$?
cat "$logfile"
printf '%s\t%s\t%s\t%s\t%s\t%s\n' \
    "$ts" "$target" "$rc" "$out" "$has_object" "$PWD" \
    > "$statusfile"
exit "$rc"
WRAPPER
    if ! sed \
        -e "s|@ROTFB_LD_REAL@|$LD_BACKUP|g" \
        -e "s|@ROTFB_LINK_OUTPUT_DIR@|$LOGDIR/linker-output|g" \
        "$LD_PATH.in" > "$LD_PATH"; then
        echo "failed to render linker wrapper at $LD_PATH" > "$FAIL"
        exit 1
    fi
    rm -f "$LD_PATH.in"
    if ! chmod 0755 "$LD_PATH"; then
        echo "failed to install linker wrapper at $LD_PATH" > "$FAIL"
        exit 1
    fi
    cat > "$READY" <<EOF_READY
installed_utc=$(date -u +%FT%TZ)
wrapper_path=$LD_PATH
real_linker=$LD_BACKUP
EOF_READY
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
    printf 'evidence_version=2\nbuild_started_utc=%s\nbuild_started_epoch=%s\n' \
        "$(cat "$LOGDIR/build-start-utc.txt" 2>/dev/null || echo UNKNOWN)" "$START_EPOCH" >> "$EVIDENCE"
    if [ -f "$READY" ]; then
        echo "linker_wrapper_status=INSTALLED" >> "$EVIDENCE"
    else
        echo "linker_wrapper_status=INCOMPLETE" >> "$EVIDENCE"
        OVERALL=INCOMPLETE
    fi
    if [ -f "$FAIL" ]; then
        echo "linker_wrapper_failure=$(cat "$FAIL")" >> "$EVIDENCE"
        OVERALL=INCOMPLETE
    fi
    echo >> "$EVIDENCE"

    check_target() {
        label=$1
        path=$2
        expected=$3
        link_count=$(awk -F '\t' -v t="$label" '$2 == t { n++ } END { print n + 0 }' "$LOGDIR/linker-status.tsv" 2>/dev/null)
        bad_rc_count=$(awk -F '\t' -v t="$label" '$2 == t && $3 != "0" { n++ } END { print n + 0 }' "$LOGDIR/linker-status.tsv" 2>/dev/null)
        object_bad_count=$(awk -F '\t' -v t="$label" '$2 == t && $5 != "YES" { n++ } END { print n + 0 }' "$LOGDIR/linker-status.tsv" 2>/dev/null)
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
        diagnostics=NONE
        if ls "$LOGDIR/linker-output/$label."*.log >/dev/null 2>&1; then
            if grep -E -i -q 'ld:[[:space:]]+fatal:|undefined symbol|ld:[[:space:]]+error:|relocation error|fatal error:|cannot open.*(file|input)|file not found' "$LOGDIR/linker-output/$label."*.log 2>/dev/null; then
                diagnostics=FOUND
            fi
        else
            diagnostics=INCOMPLETE
        fi
        object_present=NO
        if [ "$link_count" -gt 0 ] 2>/dev/null && [ "$object_bad_count" -eq 0 ] 2>/dev/null; then
            object_present=YES
        fi
        linker_status=INCOMPLETE
        if [ "$link_count" -gt 0 ] 2>/dev/null; then
            if [ "$bad_rc_count" -eq 0 ] 2>/dev/null; then linker_status=0; else linker_status=NONZERO; fi
        fi
        status=PASS
        if [ "$fresh" != YES ] || [ "$object_present" != YES ] ||
            [ "$linker_status" != 0 ] || [ "$diagnostics" != NONE ]; then
            status=INCOMPLETE
            OVERALL=INCOMPLETE
        fi
        {
            printf '[%s]\n' "$label"
            printf 'target_path=%s\n' "$path"
            printf 'expected_object_argument=%s\n' "$expected"
            printf 'linker_command_detected_count=%s\n' "$link_count"
            printf 'target_fresh_after_build_start=%s\n' "$fresh"
            printf 'mtime_epoch=%s\nmtime_utc=%s\nsize_bytes=%s\n' "$mtime" "$mtime_utc" "$size"
            printf 'object_argument_present=%s\n' "$object_present"
            printf 'linker_exit_status=%s\n' "$linker_status"
            printf 'linker_nonzero_invocations=%s\n' "$bad_rc_count"
            printf 'linker_diagnostics=%s\n' "$diagnostics"
            printf 'status=%s\n\n' "$status"
        } >> "$EVIDENCE"
    }

    check_target dboot.i86pc "$WORKSPACE/usr/src/uts/i86pc/unix/dboot/obj64/dboot" dboot/obj64/rotfb_boot.o
    check_target unix.o.i86pc "$WORKSPACE/usr/src/uts/i86pc/unix/obj64/unix.o" obj64/rotfb_boot.o
    check_target gfx_private.i86pc "$WORKSPACE/usr/src/uts/i86pc/gfx_private/obj64/gfx_private" obj64/rotfb_runtime.o
    check_target dboot.i86xpv "$WORKSPACE/usr/src/uts/i86xpv/unix/dboot/obj64/dboot" dboot/obj64/rotfb_boot.o
    check_target unix.o.i86xpv "$WORKSPACE/usr/src/uts/i86xpv/unix/obj64/unix.o" obj64/rotfb_boot.o
    check_target gfx_private.i86xpv "$WORKSPACE/usr/src/uts/i86xpv/gfx_private/obj64/gfx_private" obj64/rotfb_runtime.o

    if [ -s "$LOGDIR/linker-status.tsv" ]; then
        echo "linker_status_records=$(wc -l < "$LOGDIR/linker-status.tsv" | tr -d ' ')" >> "$EVIDENCE"
    else
        echo "linker_status_records=0" >> "$EVIDENCE"
        OVERALL=INCOMPLETE
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
        if [ "$count" -lt 1 ]; then OVERALL=INCOMPLETE; fi
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
        echo "patches_applied=$APPLIED" >> "$EVIDENCE"
        echo "patch_order=$PATCH_ORDER" >> "$EVIDENCE"
        [ "$APPLIED" -eq 10 ] || OVERALL=INCOMPLETE
    else
        echo "patches_applied=UNKNOWN" >> "$EVIDENCE"
        echo "patch_order=UNKNOWN" >> "$EVIDENCE"
        OVERALL=INCOMPLETE
    fi
    NIGHTLY_RC=$(cat "$LOGDIR/nightly-exit-code.txt" 2>/dev/null || echo UNKNOWN)
    echo "nightly_exit_code=$NIGHTLY_RC" >> "$EVIDENCE"
    [ "$NIGHTLY_RC" = 0 ] || OVERALL=INCOMPLETE
    printf 'overall_status=%s\n' "$OVERALL" >> "$EVIDENCE"
    [ "$OVERALL" = PASS ]
    ;;
*)
    usage
    ;;
esac
