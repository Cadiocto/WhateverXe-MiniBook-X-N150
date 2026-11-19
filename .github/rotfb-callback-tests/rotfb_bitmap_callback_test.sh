#!/bin/sh
set -eu
if [ "$#" -ne 2 ]; then
	echo "usage: $0 UPSTREAM_GIT_DIR PATCH_DIR" >&2
	exit 2
fi
SCRIPT_DIR=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
ROTFB_DIR=$(CDPATH= cd -- "$SCRIPT_DIR/../../illumo_rotfb" && pwd)
UPSTREAM_GIT_DIR=$1
PATCH_DIR=$2
UPSTREAM_PIN=098e95820891f8a83fae679daece7fcf58b7b3a4
CC=cc
CFLAGS=-O1
TMP=$(mktemp -d "/tmp/rotfb-bitmap-callback.XXXXXX")
LOG_DIR=${ROTFB_TEST_LOG_DIR:-"$TMP/logs"}
mkdir -p "$LOG_DIR"
PATCH_LOG="$LOG_DIR/patch-application.log"
: > "$PATCH_LOG"
WORKTREE="$TMP/illumos-gate"
cleanup()
{
	git -C "$UPSTREAM_GIT_DIR" worktree remove --force "$WORKTREE" \
	    >/dev/null 2>&1 || true
	rm -rf "$TMP"
}
trap cleanup EXIT
trap 'exit 1' HUP INT TERM
test "$(git -C "$UPSTREAM_GIT_DIR" rev-parse HEAD)" = "$UPSTREAM_PIN"
git -C "$UPSTREAM_GIT_DIR" worktree add --detach "$WORKTREE" "$UPSTREAM_PIN" >/dev/null
# The ordered production patch series currently contains patches 0001 through 0013.
# The manifest and patch-series gates run before this production callback harness.
# This CI job does not invoke the full OpenIndiana build workflow.
# Patch-series path allowlist is sorted lexicographically in the regression fixture.
PATCH_COUNT=0
for patch in "$PATCH_DIR"/*.patch; do
	[ -f "$patch" ] || continue
	name=$(basename "$patch")
	echo "PATCH_CHECK_BEGIN=$name" | tee -a "$PATCH_LOG"
	if git -C "$WORKTREE" apply --check "$patch" \
	    >"$LOG_DIR/$name.check.stdout" 2>"$LOG_DIR/$name.check.stderr"; then
		rc=0
	else
		rc=$?
	fi
	printf '%s\n' "$rc" > "$LOG_DIR/$name.check.exit-code"
	cat "$LOG_DIR/$name.check.stdout" "$LOG_DIR/$name.check.stderr" >> "$PATCH_LOG"
	test "$rc" -eq 0
	if git -C "$WORKTREE" apply "$patch" \
	    >"$LOG_DIR/$name.apply.stdout" 2>"$LOG_DIR/$name.apply.stderr"; then
		rc=0
	else
		rc=$?
	fi
	printf '%s\n' "$rc" > "$LOG_DIR/$name.apply.exit-code"
	cat "$LOG_DIR/$name.apply.stdout" "$LOG_DIR/$name.apply.stderr" >> "$PATCH_LOG"
	test "$rc" -eq 0
	echo "PATCH_APPLIED=$name" | tee -a "$PATCH_LOG"
	PATCH_COUNT=$((PATCH_COUNT + 1))
done
test "$PATCH_COUNT" -eq 13
echo "UPSTREAM_PIN=$UPSTREAM_PIN" | tee "$LOG_DIR/harness-metadata.log"
echo "PATCH_COUNT=$PATCH_COUNT" | tee -a "$LOG_DIR/harness-metadata.log"
git -C "$WORKTREE" diff --check
BITMAP_SOURCE="$WORKTREE/usr/src/uts/i86pc/io/gfx_private/gfxp_bitmap.c"
python3 "$SCRIPT_DIR/rotfb_extract_bitmap_callbacks.py" "$BITMAP_SOURCE" "$TMP/production_callbacks.inc"
mkdir -p "$TMP/include/sys"
cat > "$TMP/include/sys/types.h" <<EOF_TYPES
#ifndef ROTFB_CALLBACK_TEST_SYS_TYPES_H
#define ROTFB_CALLBACK_TEST_SYS_TYPES_H
#include_next <sys/types.h>
#include "$SCRIPT_DIR/rotfb_bitmap_callback_test_compat.h"
#endif
EOF_TYPES
for header in param atomic kmem ddi sunddi framebuffer gfx_private; do
	cat > "$TMP/include/sys/$header.h" <<EOF_HEADER
#ifndef ROTFB_CALLBACK_TEST_SYS_HEADER_H
#define ROTFB_CALLBACK_TEST_SYS_HEADER_H
#include "$SCRIPT_DIR/rotfb_bitmap_callback_test_compat.h"
#endif
EOF_HEADER
done
cat > "$TMP/include/sys/systm.h" <<EOF_SYSTM
#ifndef ROTFB_CALLBACK_TEST_SYS_SYSTM_H
#define ROTFB_CALLBACK_TEST_SYS_SYSTM_H
#include "$SCRIPT_DIR/rotfb_bitmap_callback_test_compat.h"
#define bcopy(src, dst, n) memmove((dst), (src), (n))
#endif
EOF_SYSTM
cat > "$TMP/include/sys/rotfb.h" <<EOF_ROTFB
#ifndef ROTFB_CALLBACK_TEST_SYS_ROTFB_H
#define ROTFB_CALLBACK_TEST_SYS_ROTFB_H
#include "$ROTFB_DIR/rotfb.h"
#endif
EOF_ROTFB
# Preserve compiler and runtime output/exit codes outside TMP cleanup.
COMPILE_STDOUT="$LOG_DIR/compiler.stdout.log"
COMPILE_STDERR="$LOG_DIR/compiler.stderr.log"
RUNTIME_STDOUT="$LOG_DIR/runtime.stdout.log"
RUNTIME_STDERR="$LOG_DIR/runtime.stderr.log"
if "$CC" -std=c11 $CFLAGS -Wall -Wextra -Werror -Wno-error=sign-compare -Wsign-compare -pthread -fno-builtin-memmove -fno-strict-aliasing -Wl,--wrap=memmove \
    -I"$TMP/include" -I"$ROTFB_DIR" -I"$SCRIPT_DIR" -I"$TMP" \
    -o "$TMP/rotfb-bitmap-callback-test" \
    "$ROTFB_DIR/rotfb_runtime.c" "$SCRIPT_DIR/rotfb_bitmap_callback_test.c" \
    >"$COMPILE_STDOUT" 2>"$COMPILE_STDERR"; then
	COMPILE_RC=0
else
	COMPILE_RC=$?
fi
printf '%s\n' "$COMPILE_RC" > "$LOG_DIR/compiler.exit-code"
echo "COMPILER_EXIT_CODE=$COMPILE_RC"
cat "$COMPILE_STDOUT"
cat "$COMPILE_STDERR" >&2
if [ "$COMPILE_RC" -ne 0 ]; then
	exit "$COMPILE_RC"
fi
if timeout 90 "$TMP/rotfb-bitmap-callback-test" \
    >"$RUNTIME_STDOUT" 2>"$RUNTIME_STDERR"; then
	RUNTIME_RC=0
else
	RUNTIME_RC=$?
fi
printf '%s\n' "$RUNTIME_RC" > "$LOG_DIR/runtime.exit-code"
echo "RUNTIME_EXIT_CODE=$RUNTIME_RC"
cat "$RUNTIME_STDOUT"
cat "$RUNTIME_STDERR" >&2
for scenario in A B C D; do
	result_file="$LOG_DIR/scenario-$scenario.result.txt"
	if grep "^SCENARIO_${scenario}_RESULT=" "$RUNTIME_STDOUT" >"$result_file"; then
		:
	else
		printf 'SCENARIO_%s_RESULT=NOT_REPORTED\n' "$scenario" >"$result_file"
	fi
done
if grep '^EXPLORATORY_DISPLAY_A_TO_B_RESULT=' "$RUNTIME_STDOUT" \
    >"$LOG_DIR/exploratory-display-a-to-b.result.txt"; then
	:
else
	printf '%s\n' 'EXPLORATORY_DISPLAY_A_TO_B_RESULT=NOT_REPORTED' \
	    >"$LOG_DIR/exploratory-display-a-to-b.result.txt"
fi
if [ "$RUNTIME_RC" -ne 0 ]; then
	exit "$RUNTIME_RC"
fi
echo "PRODUCTION_CALLBACKS_EXECUTED=bitmap_polled_cursor,bitmap_cons_cursor,bitmap_display_cursor,bitmap_cons_copy"
echo "PRODUCTION_RUNTIME_FUNCTIONS_EXECUTED=rotfb_runtime_flush_rect,rotfb_copy_rows"
echo "ROTFB_PATCHED_PRODUCTION_CALLBACK_TEST=PASS"
