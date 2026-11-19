#!/bin/sh
set -eu

if [ "$#" -ne 3 ]; then
	echo "usage: $0 UPSTREAM_GIT_DIR PATCH_DIR LOG_DIR" >&2
	exit 2
fi

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
UPSTREAM=$1
PATCH_DIR=$(CDPATH= cd -- "$2" && pwd)
LOG_DIR=$3
PIN=098e95820891f8a83fae679daece7fcf58b7b3a4
TMP=$(mktemp -d "/tmp/rotfb-ddi-integration.XXXXXX")
WORKTREE="$TMP/illumos-gate"

mkdir -p "$LOG_DIR"
cleanup()
{
	git -C "$UPSTREAM" worktree remove --force "$WORKTREE" \
	    >/dev/null 2>&1 || true
	rm -rf "$TMP"
}
trap cleanup EXIT
trap 'exit 1' HUP INT TERM

test "$(git -C "$UPSTREAM" rev-parse HEAD)" = "$PIN"
git -C "$UPSTREAM" worktree add --detach "$WORKTREE" "$PIN" \
    >"$LOG_DIR/worktree.stdout.log" 2>"$LOG_DIR/worktree.stderr.log"

PATCH_COUNT=0
for patch in "$PATCH_DIR"/*.patch; do
	name=$(basename "$patch")
	git -C "$WORKTREE" apply --check "$patch" \
	    >"$LOG_DIR/$name.check.stdout" 2>"$LOG_DIR/$name.check.stderr"
	git -C "$WORKTREE" apply "$patch" \
	    >"$LOG_DIR/$name.apply.stdout" 2>"$LOG_DIR/$name.apply.stderr"
	PATCH_COUNT=$((PATCH_COUNT + 1))
done
test "$PATCH_COUNT" -eq 12
echo "UPSTREAM_PIN=$PIN" | tee "$LOG_DIR/metadata.txt"
echo "PATCH_COUNT=$PATCH_COUNT" | tee -a "$LOG_DIR/metadata.txt"

python3 "$SCRIPT_DIR/rotfb_extract_ddi_entrypoints.py" \
    "$WORKTREE/usr/src/uts/i86pc/io/gfx_private/gfxp_bitmap.c" \
    "$WORKTREE/usr/src/uts/i86pc/io/gfx_private/gfxp_fb.c" \
    "$WORKTREE/usr/src/uts/intel/io/vgatext/vgatext.c" \
    "$TMP/production_ddi_entrypoints.inc" \
    2>&1 | tee "$LOG_DIR/extraction.log"

cc -std=c11 -Wall -Wextra -Werror -Wno-unused-parameter \
    -I"$SCRIPT_DIR" -I"$TMP" \
    -o "$TMP/rotfb-ddi-integration-test" \
    "$SCRIPT_DIR/rotfb_ddi_integration_test.c" \
    >"$LOG_DIR/compiler.stdout.log" 2>"$LOG_DIR/compiler.stderr.log"
echo "COMPILER_EXIT_CODE=0" | tee "$LOG_DIR/compiler.exit-code"
cat "$LOG_DIR/compiler.stdout.log"
cat "$LOG_DIR/compiler.stderr.log"

"$TMP/rotfb-ddi-integration-test" \
    >"$LOG_DIR/runtime.stdout.log" 2>"$LOG_DIR/runtime.stderr.log"
echo "RUNTIME_EXIT_CODE=0" | tee "$LOG_DIR/runtime.exit-code"
cat "$LOG_DIR/runtime.stdout.log"
cat "$LOG_DIR/runtime.stderr.log"
