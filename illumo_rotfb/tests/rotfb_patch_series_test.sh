#!/bin/sh
# Apply the complete ROTFB patch series to a disposable worktree at the pinned
# illumos revision. Reproduce the pre-fix 0011 failure before accepting 0011.
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
OLD_PATCH="$TMP_PARENT/0011-original-offsets-$$.patch"
OLD_LOG="$TMP_PARENT/0011-original-offsets-$$.log"
cleanup() {
    git -C "$UPSTREAM" worktree remove --force "$TEST_TREE" >/dev/null 2>&1 || true
    rm -f "$OLD_PATCH" "$OLD_LOG"
}
trap cleanup EXIT HUP INT TERM

git -C "$UPSTREAM" worktree add --detach "$TEST_TREE" "$PIN"

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

# Recreate the exact hunk coordinates from Build #78.
sed \
    -e 's/^@@ -618,3 +618,48 @@$/@@ -603,3 +603,48 @@/' \
    -e 's/^@@ -648,13 +693,4 @@$/@@ -632,13 +677,4 @@/' \
    -e 's/^@@ -730,33 +766,14 @@$/@@ -714,33 +750,14 @@/' \
    "$FIXED_PATCH" > "$OLD_PATCH"

if git -C "$TEST_TREE" apply --check "$OLD_PATCH" > "$OLD_LOG" 2>&1; then
    echo "REGRESSION FAILED: original 0011 offsets unexpectedly apply" >&2
    exit 1
fi
if ! grep -F 'gfxp_bitmap.c:714' "$OLD_LOG" >/dev/null; then
    echo "REGRESSION FAILED: old 0011 did not reproduce the Build #78 failure point" >&2
    cat "$OLD_LOG" >&2
    exit 1
fi
echo "EXPECTED_FAILURE original 0011 offsets reproduce gfxp_bitmap.c:714"

git -C "$TEST_TREE" apply --check "$FIXED_PATCH"
git -C "$TEST_TREE" apply "$FIXED_PATCH"
echo "APPLIED 0011-gfxp-bitmap-cursor-state.patch"

git -C "$TEST_TREE" diff --check

git -C "$TEST_TREE" diff --name-only | LC_ALL=C sort > "$TMP_PARENT/rotfb-patch-series-actual-$$.txt"
cat > "$TMP_PARENT/rotfb-patch-series-expected-$$.txt" <<'EXPECTED'
usr/src/uts/i86pc/Makefile.files
usr/src/uts/i86pc/boot/boot_fb.c
usr/src/uts/i86pc/io/gfx_private/gfxp_bitmap.c
usr/src/uts/i86pc/io/gfx_private/gfxp_fb.c
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

echo "PATCH_COUNT=11"
echo "UPSTREAM_PIN=$PIN"
echo "PATCH_PATH_ALLOWLIST=PASS"
echo "PATCH_SERIES_STATUS=PASS"
