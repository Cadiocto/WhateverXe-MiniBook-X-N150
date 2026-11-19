#!/bin/sh
set -eu
if [ "$#" -ne 2 ]; then
	echo "usage: $0 UPSTREAM_GIT_DIR PATCH_DIR" >&2
	exit 2
fi
SCRIPT_DIR=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
ROTFB_DIR=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)
UPSTREAM_GIT_DIR=$1
PATCH_DIR=$2
UPSTREAM_PIN=098e95820891f8a83fae679daece7fcf58b7b3a4
CC=cc
CFLAGS=-O1
TMP=$(mktemp -d "/tmp/rotfb-bitmap-callback.XXXXXX")
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
PATCH_COUNT=0
for patch in "$PATCH_DIR"/*.patch; do
	[ -f "$patch" ] || continue
	git -C "$WORKTREE" apply --check "$patch"
	git -C "$WORKTREE" apply "$patch"
	PATCH_COUNT=$((PATCH_COUNT + 1))
done
test "$PATCH_COUNT" -eq 12
echo "UPSTREAM_PIN=$UPSTREAM_PIN"
echo "PATCH_COUNT=$PATCH_COUNT"
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
# Preserve the upstream VIS signed types; keep every warning fatal except legacy sign comparisons.
"$CC" -std=c11 $CFLAGS -Wall -Wextra -Werror -Wno-error=sign-compare -Wsign-compare -pthread -fno-builtin-memmove -fno-strict-aliasing -Wl,--wrap=memmove \
    -I"$TMP/include" -I"$ROTFB_DIR" -I"$SCRIPT_DIR" -I"$TMP" \
    -o "$TMP/rotfb-bitmap-callback-test" \
    "$ROTFB_DIR/rotfb_runtime.c" "$SCRIPT_DIR/rotfb_bitmap_callback_test.c"
timeout 90 "$TMP/rotfb-bitmap-callback-test"
echo "PRODUCTION_CALLBACKS_EXECUTED=bitmap_polled_cursor,bitmap_cons_cursor,bitmap_display_cursor,bitmap_cons_copy"
echo "PRODUCTION_RUNTIME_FUNCTIONS_EXECUTED=rotfb_runtime_flush_rect,rotfb_copy_rows"
echo "ROTFB_PATCHED_PRODUCTION_CALLBACK_TEST=PASS"
