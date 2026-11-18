#!/bin/sh
set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
HELPER="$SCRIPT_DIR/../rotfb_link_evidence.sh"
TMP=$(mktemp -d "${TMPDIR:-/tmp}/rotfb-link-evidence.XXXXXX")
trap 'rm -rf "$TMP"' EXIT HUP INT TERM

make_fake_linker() {
    path=$1
    mkdir -p "$(dirname "$path")"
    cat > "$path" <<'LINKER'
#!/bin/sh
prev=
out=
for arg do
    if [ "$prev" = "-o" ]; then out=$arg; fi
    prev=$arg
done
[ -n "$out" ] || exit 64
if [ -n "${FAKE_LD_FAIL_OUTPUT:-}" ] && [ "$out" = "$FAKE_LD_FAIL_OUTPUT" ]; then
    echo "ld: fatal: injected linker failure for $out" >&2
    exit 7
fi
mkdir -p "$(dirname "$out")"
printf '\177ELF\001\002\001\000' > "$out"
exit 0
LINKER
    chmod 0755 "$path"
}

write_compile_evidence() {
    logdir=$1
    printf '%s\n' \
        'cc -Di86pc -O2 -c -o obj64/rotfb_boot.o rotfb_boot.c' \
        'cc -D__i86xpv -O2 -c -o obj64/rotfb_boot.o rotfb_boot.c' \
        > "$logdir/rotfb-boot-compile-commands.txt"
    printf '%s\n' \
        'cc -Di86pc -O2 -c -o obj64/rotfb_runtime.o rotfb_runtime.c' \
        'cc -D__i86xpv -O2 -c -o obj64/rotfb_runtime.o rotfb_runtime.c' \
        > "$logdir/rotfb-runtime-compile-commands.txt"
    printf '%s\n' \
        'cc -Di86pc -O2 -c -o obj64/gfxp_bitmap.o gfxp_bitmap.c' \
        'cc -D__i86xpv -O2 -c -o obj64/gfxp_bitmap.o gfxp_bitmap.c' \
        > "$logdir/gfxp-bitmap-compile-commands.txt"
    printf '%s\n' \
        'cc -Di86pc -O2 -c -o obj64/gfxp_fb.o gfxp_fb.c' \
        'cc -D__i86xpv -O2 -c -o obj64/gfxp_fb.o gfxp_fb.c' \
        > "$logdir/gfxp-fb-compile-commands.txt"
}

write_patch_evidence() {
    logdir=$1
    cat > "$logdir/patch-apply.log" <<'PATCHES'
APPLIED 0001-framebuffer-metadata.patch
APPLIED 0002-boot-fb-rotation.patch
APPLIED 0003-gfxp-bitmap-tem-rotation.patch
APPLIED 0004-Makefile-files.patch
APPLIED 0005-rotfb-lifecycle-hardening.patch
APPLIED 0006-rotfb-console-contract-and-copy-safety.patch
APPLIED 0007-gfxp-bitmap-bounds-hardening.patch
APPLIED 0008-gfxp-detach-failure-safety.patch
APPLIED 0009-gfxp-bitmap-wide-row-offsets.patch
APPLIED 0010-gfxp-bitmap-vis-safety.patch
APPLIED 0011-gfxp-bitmap-cursor-state.patch
PATCHES
    printf '0\n' > "$logdir/nightly-exit-code.txt"
}

wait_for_file() {
    path=$1
    i=0
    while [ ! -f "$path" ]; do
        i=$((i + 1))
        [ "$i" -lt 15 ] || { echo "timed out waiting for $path" >&2; return 1; }
        sleep 1
    done
}

assert_target_status() {
    label=$1
    expected=$2
    file=$3
    awk -v wanted="[$label]" -v expected="status=$expected" '
        $0 == wanted { inside = 1; next }
        /^\[/ { if (inside) exit }
        inside && $0 == expected { found = 1 }
        END { exit (found ? 0 : 1) }
    ' "$file"
}

run_target_case() {
    mode=$1
    workspace="$TMP/$mode-workspace"
    logdir="$TMP/$mode-logs"
    ldpath="$workspace/usr/src/tools/proto/root_i386-nd/opt/onbld/bin/amd64/ld"
    mkdir -p "$workspace/usr/src/uts/i86pc/unix/dboot/obj64" \
        "$workspace/usr/src/uts/i86pc/unix/obj64" \
        "$workspace/usr/src/uts/i86pc/gfx_private/obj64" \
        "$workspace/usr/src/uts/i86xpv/unix/dboot/obj64" \
        "$workspace/usr/src/uts/i86xpv/unix/obj64" \
        "$workspace/usr/src/uts/i86xpv/gfx_private/obj64"
    make_fake_linker "$ldpath"
    sh "$HELPER" prepare "$workspace" "$logdir"
    sh "$HELPER" watch "$workspace" "$logdir" > "$logdir/watch.log" 2>&1 &
    watch_pid=$!
    wait_for_file "$logdir/ld-wrapper-ready.txt"

    # Simulate nightly's build_tools replacing the linker after the first install.
    make_fake_linker "$TMP/rebuilt-ld"
    cp -p "$TMP/rebuilt-ld" "$ldpath"
    i=0
    while ! grep -q 'event=reinstalled' "$logdir/ld-wrapper-events.log" 2>/dev/null; do
        i=$((i + 1))
        [ "$i" -lt 15 ] || { echo "watcher did not reinstall after linker replacement" >&2; kill "$watch_pid" 2>/dev/null || true; return 1; }
        sleep 1
    done

    for arch in i86pc i86xpv; do
        (
            cd "$workspace/usr/src/uts/$arch/unix"
            "$ldpath" -o dboot/obj64/dboot dboot/obj64/rotfb_boot.o
            "$ldpath" -o obj64/unix.o obj64/rotfb_boot.o
        )
        if [ "$mode" = wrongcwd ] && [ "$arch" = i86xpv ]; then
            mkdir -p "$workspace/usr/src/uts/$arch/io/gfx_private/obj64"
            (
                cd "$workspace/usr/src/uts/$arch/io/gfx_private"
                "$ldpath" -o obj64/gfx_private obj64/rotfb_runtime.o
            )
        else
            (
                cd "$workspace/usr/src/uts/$arch/gfx_private"
                "$ldpath" -o obj64/gfx_private obj64/rotfb_runtime.o
            )
        fi
    done

    case "$mode" in
        missing)
            rm -f "$workspace/usr/src/uts/i86xpv/gfx_private/obj64/gfx_private"
            ;;
        stale)
            touch -t 200001010000 "$workspace/usr/src/uts/i86pc/gfx_private/obj64/gfx_private"
            ;;
    esac

    : > "$logdir/ld-wrapper-stop"
    wait "$watch_pid"
    write_compile_evidence "$logdir"
    write_patch_evidence "$logdir"

    if [ "$mode" = success ]; then
        if ! sh "$HELPER" finalize "$workspace" "$logdir"; then
            cat "$logdir/rotfb-build-evidence.txt" >&2
            return 1
        fi
        grep -q '^overall_status=PASS$' "$logdir/rotfb-build-evidence.txt"
        grep -q '^linker_wrapper_install_count=2$' "$logdir/rotfb-build-evidence.txt"
        grep -q '^patch_order_match=YES$' "$logdir/rotfb-build-evidence.txt"
        [ "$(grep -c '^status=PASS$' "$logdir/rotfb-build-evidence.txt")" -eq 6 ]
        [ "$(wc -l < "$logdir/linker-status.tsv" | tr -d ' ')" -eq 6 ]
        grep -F -q "cwd=$workspace/usr/src/uts/i86pc/gfx_private" "$logdir/linker-commands.txt"
        grep -F -q "cwd=$workspace/usr/src/uts/i86xpv/gfx_private" "$logdir/linker-commands.txt"
        grep -F -q 'output=obj64/gfx_private' "$logdir/linker-commands.txt"
    else
        if sh "$HELPER" finalize "$workspace" "$logdir"; then
            echo "evidence incorrectly accepted $mode target state" >&2
            return 1
        fi
        grep -q '^overall_status=INCOMPLETE$' "$logdir/rotfb-build-evidence.txt"
        case "$mode" in
            missing) assert_target_status gfx_private.i86xpv INCOMPLETE "$logdir/rotfb-build-evidence.txt" ;;
            stale) assert_target_status gfx_private.i86pc INCOMPLETE "$logdir/rotfb-build-evidence.txt" ;;
            wrongcwd) assert_target_status gfx_private.i86xpv INCOMPLETE "$logdir/rotfb-build-evidence.txt" ;;
        esac
    fi
}

run_failure_case() {
    workspace="$TMP/failure-workspace"
    logdir="$TMP/failure-logs"
    ldpath="$workspace/usr/src/tools/proto/root_i386-nd/opt/onbld/bin/amd64/ld"
    mkdir -p "$workspace/usr/src/uts/i86xpv/gfx_private/obj64"
    make_fake_linker "$ldpath"
    sh "$HELPER" prepare "$workspace" "$logdir"
    sh "$HELPER" watch "$workspace" "$logdir" > "$logdir/watch.log" 2>&1 &
    watch_pid=$!
    wait_for_file "$logdir/ld-wrapper-ready.txt"

    if (cd "$workspace/usr/src/uts/i86xpv/gfx_private" && \
        FAKE_LD_FAIL_OUTPUT=obj64/gfx_private "$ldpath" -o obj64/gfx_private obj64/rotfb_runtime.o); then
        echo "expected fake linker failure was not propagated" >&2
        : > "$logdir/ld-wrapper-stop"
        wait "$watch_pid" || true
        return 1
    else
        rc=$?
        [ "$rc" -eq 7 ] || { echo "wrong propagated linker status: $rc" >&2; return 1; }
    fi

    : > "$logdir/ld-wrapper-stop"
    wait "$watch_pid"
    write_patch_evidence "$logdir"
    if sh "$HELPER" finalize "$workspace" "$logdir"; then
        echo "evidence incorrectly accepted a failed linker" >&2
        return 1
    fi
    grep -q '^linker_exit_status=NONZERO$' "$logdir/rotfb-build-evidence.txt"
    grep -q '^status=FAIL$' "$logdir/rotfb-build-evidence.txt"
    grep -q '^overall_status=FAIL$' "$logdir/rotfb-build-evidence.txt"
}

run_target_case success
run_target_case missing
run_target_case stale
run_target_case wrongcwd
run_failure_case
printf '%s\n' 'ROTFB linker evidence tests: PASS'
