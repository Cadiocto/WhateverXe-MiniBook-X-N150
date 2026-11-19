#!/bin/sh
# Apply the complete ROTFB patch series to a disposable worktree at the pinned
# illumos revision. Reproduce historical 0011 and both 0012 context failures, then apply 0013
# before accepting the corrected patches.
set -eu

if [ "$#" -ne 3 ]; then
    echo "usage: $0 UPSTREAM_GIT_DIR PATCH_DIR TMP_PARENT" >&2
    exit 2
fi

UPSTREAM=$1
PATCH_DIR=$2
TMP_PARENT=$3
PIN=098e95820891f8a83fae679daece7fcf58b7b3a4

ACTUAL_PIN=$(git -C "$UPSTREAM" rev-parse HEAD)
if [ "$ACTUAL_PIN" != "$PIN" ]; then
    echo "wrong upstream HEAD: $ACTUAL_PIN (expected $PIN)" >&2
    exit 1
fi
if [ -n "$(git -C "$UPSTREAM" status --porcelain)" ]; then
    echo "upstream worktree must be clean before patch-series regression" >&2
    exit 1
fi

mkdir -p "$TMP_PARENT"
TEST_TREE="$TMP_PARENT/rotfb-patch-series-$$"
SCRIPT_DIR=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
OLD_PATCH="$SCRIPT_DIR/fixtures/0011-before-blank-context-fix.patch"
OLD_LOG="$TMP_PARENT/0011-before-blank-context-fix.log"
OLD_COPY_PATCH="$SCRIPT_DIR/fixtures/0012-before-errno-context.patch"
OLD_COPY_LOG="$TMP_PARENT/0012-before-errno-context.log"
OLD_COPY_FLUSH_PATCH="$SCRIPT_DIR/fixtures/0012-before-flush-context-fix.patch"
OLD_COPY_FLUSH_LOG="$TMP_PARENT/0012-before-flush-context-fix.log"
cleanup() {
    git -C "$UPSTREAM" worktree remove --force "$TEST_TREE" >/dev/null 2>&1 || true
    rm -f "$OLD_LOG" "$OLD_COPY_LOG" "$OLD_COPY_FLUSH_LOG"
}
trap cleanup EXIT HUP INT TERM

git -C "$UPSTREAM" worktree add --detach "$TEST_TREE" "$PIN"

set -- "$PATCH_DIR"/[0-9][0-9][0-9][0-9]-*.patch
[ "$#" -eq 13 ] || {
    echo "expected exactly 13 ROTFB patches, found $#" >&2
    exit 1
}
EXPECTED_PATCH_ORDER=$(cat <<'EXPECTED_PATCHES'
0001-framebuffer-metadata.patch
0002-boot-fb-rotation.patch
0003-gfxp-bitmap-tem-rotation.patch
0004-Makefile-files.patch
0005-rotfb-lifecycle-hardening.patch
0006-rotfb-console-contract-and-copy-safety.patch
0007-gfxp-bitmap-bounds-hardening.patch
0008-gfxp-detach-failure-safety.patch
0009-gfxp-bitmap-wide-row-offsets.patch
0010-gfxp-bitmap-vis-safety.patch
0011-gfxp-bitmap-cursor-state.patch
0012-gfxp-bitmap-copy-helper.patch
0013-vgatext-detach-failure-propagation.patch
EXPECTED_PATCHES
)
ACTUAL_PATCH_ORDER=$(for p in "$PATCH_DIR"/[0-9][0-9][0-9][0-9]-*.patch; do basename "$p"; done)
if [ "$ACTUAL_PATCH_ORDER" != "$EXPECTED_PATCH_ORDER" ]; then
    echo "ROTFB patch order mismatch" >&2
    printf '%s\n' "$ACTUAL_PATCH_ORDER" >&2
    exit 1
fi

count=0
for p in "$PATCH_DIR"/[0-9][0-9][0-9][0-9]-*.patch; do
    [ -f "$p" ] || continue
    name=$(basename "$p")
    [ "$name" = "0011-gfxp-bitmap-cursor-state.patch" ] && break
    echo "CHECK $name"
    git -C "$TEST_TREE" apply --check "$p"
    git -C "$TEST_TREE" apply "$p"
    echo "APPLIED $name"
    count=$((count + 1))
done
[ "$count" -eq 10 ] || {
    echo "expected exactly 10 preceding patches, applied $count" >&2
    exit 1
}

FIXED_PATCH="$PATCH_DIR/0011-gfxp-bitmap-cursor-state.patch"
[ -f "$FIXED_PATCH" ] || {
    echo "missing fixed 0011 patch" >&2
    exit 1
}

# Reproduce Build #79's actual pre-fix patch-context failure. The fixture
# is the exact 0011 patch from the parent commit, before the missing blank
# line after cursor.origin.y was restored.
if git -C "$TEST_TREE" apply --check "$OLD_PATCH" > "$OLD_LOG" 2>&1; then
    echo "REGRESSION FAILED: pre-fix 0011 unexpectedly applies" >&2
    exit 1
fi
if ! grep -F 'patch failed: usr/src/uts/i86pc/io/gfx_private/gfxp_bitmap.c:730' "$OLD_LOG" >/dev/null ||
    ! grep -F 'patch does not apply' "$OLD_LOG" >/dev/null; then
    echo "REGRESSION FAILED: pre-fix 0011 did not reproduce the Build #79 context failure" >&2
    cat "$OLD_LOG" >&2
    exit 1
fi
echo "EXPECTED_FAILURE pre-fix 0011 reproduces missing blank line at gfxp_bitmap.c:730"

git -C "$TEST_TREE" apply --check "$FIXED_PATCH"
git -C "$TEST_TREE" apply "$FIXED_PATCH"
echo "APPLIED 0011-gfxp-bitmap-cursor-state.patch"

COPY_PATCH="$PATCH_DIR/0012-gfxp-bitmap-copy-helper.patch"
[ -f "$COPY_PATCH" ] || {
    echo "missing 0012 bitmap-copy helper patch" >&2
    exit 1
}
[ -f "$OLD_COPY_PATCH" ] || {
    echo "missing pre-fix 0012 context fixture" >&2
    exit 1
}
if git -C "$TEST_TREE" apply --check "$OLD_COPY_PATCH" > "$OLD_COPY_LOG" 2>&1; then
    echo "REGRESSION FAILED: pre-fix 0012 unexpectedly applies" >&2
    exit 1
fi
if ! grep -F 'patch failed: usr/src/uts/i86pc/io/gfx_private/gfxp_bitmap.c:29' "$OLD_COPY_LOG" >/dev/null ||
    ! grep -F 'patch does not apply' "$OLD_COPY_LOG" >/dev/null; then
    echo "REGRESSION FAILED: pre-fix 0012 did not reproduce the Build #81 include-context failure" >&2
    cat "$OLD_COPY_LOG" >&2
    exit 1
fi
echo "EXPECTED_FAILURE pre-fix 0012 reproduces missing sys/errno.h context at gfxp_bitmap.c:29"

# Reproduce Build #82's second 0012 failure: the include context is already
# corrected, but the old hunk collapses the two-line flush call introduced by
# patch 0002 into one line, so the copy-loop hunk cannot match after 0001-0011.
[ -f "$OLD_COPY_FLUSH_PATCH" ] || {
    echo "missing pre-fix 0012 flush-context fixture" >&2
    exit 1
}
if git -C "$TEST_TREE" apply --check "$OLD_COPY_FLUSH_PATCH" > "$OLD_COPY_FLUSH_LOG" 2>&1; then
    echo "REGRESSION FAILED: pre-fix 0012 flush-context patch unexpectedly applies" >&2
    exit 1
fi
if ! grep -F 'patch failed: usr/src/uts/i86pc/io/gfx_private/gfxp_bitmap.c:379' "$OLD_COPY_FLUSH_LOG" >/dev/null ||
    ! grep -F 'patch does not apply' "$OLD_COPY_FLUSH_LOG" >/dev/null; then
    echo "REGRESSION FAILED: pre-fix 0012 did not reproduce Build #82 line-379 context failure" >&2
    cat "$OLD_COPY_FLUSH_LOG" >&2
    exit 1
fi
echo "EXPECTED_FAILURE pre-fix 0012 reproduces split flush-call context mismatch at gfxp_bitmap.c:379"

git -C "$TEST_TREE" apply --check "$COPY_PATCH"
git -C "$TEST_TREE" apply "$COPY_PATCH"
echo "APPLIED 0012-gfxp-bitmap-copy-helper.patch"

DETACH_PATCH="$PATCH_DIR/0013-vgatext-detach-failure-propagation.patch"
[ -f "$DETACH_PATCH" ] || { echo "missing 0013 vgatext detach failure propagation patch" >&2; exit 1; }
git -C "$TEST_TREE" apply --check "$DETACH_PATCH"
git -C "$TEST_TREE" apply "$DETACH_PATCH"
echo "APPLIED 0013-vgatext-detach-failure-propagation.patch"
PATCHED_VGATEXT="$TEST_TREE/usr/src/uts/intel/io/vgatext/vgatext.c"
grep -F "error = gfxp_fb_detach" "$PATCHED_VGATEXT" >/dev/null
grep -F "if (error != DDI_SUCCESS)" "$PATCHED_VGATEXT" >/dev/null
grep -F "return (error);" "$PATCHED_VGATEXT" >/dev/null

PATCHED_BITMAP="$TEST_TREE/usr/src/uts/i86pc/io/gfx_private/gfxp_bitmap.c"
grep -F '#include <sys/rotfb_copy.h>' "$PATCHED_BITMAP" >/dev/null
grep -F 'rotfb_copy_rows(' "$PATCHED_BITMAP" >/dev/null

git -C "$TEST_TREE" diff --check

git -C "$TEST_TREE" diff --name-only | LC_ALL=C sort > "$TMP_PARENT/rotfb-patch-series-actual-$$.txt"
cat > "$TMP_PARENT/rotfb-patch-series-expected-$$.txt" <<'EXPECTED'
usr/src/uts/i86pc/Makefile.files
usr/src/uts/i86pc/boot/boot_fb.c
usr/src/uts/i86pc/io/gfx_private/gfxp_bitmap.c
usr/src/uts/i86pc/io/gfx_private/gfxp_fb.c
usr/src/uts/intel/io/vgatext/vgatext.c
usr/src/uts/i86pc/sys/framebuffer.h
usr/src/uts/i86xpv/Makefile.files
EXPECTED
if ! cmp -s "$TMP_PARENT/rotfb-patch-series-expected-$$.txt" \
    "$TMP_PARENT/rotfb-patch-series-actual-$$.txt"; then
    echo "patch series changed paths outside the ROTFB allowlist" >&2
    diff -u "$TMP_PARENT/rotfb-patch-series-expected-$$.txt" \
        "$TMP_PARENT/rotfb-patch-series-actual-$$.txt" >&2 || true
    exit 1
fi
rm -f "$TMP_PARENT/rotfb-patch-series-expected-$$.txt" \
    "$TMP_PARENT/rotfb-patch-series-actual-$$.txt"

echo "PATCH_COUNT=13"
echo "UPSTREAM_PIN=$PIN"
echo "PATCH_PATH_ALLOWLIST=PASS"
echo "PATCH_SERIES_STATUS=PASS"
