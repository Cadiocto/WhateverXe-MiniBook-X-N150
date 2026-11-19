#!/bin/sh
set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
ROTFB_DIR=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)
CC=${CC:-cc}
CFLAGS=${ROTFB_TEST_CFLAGS:--O2}
TMP=$(mktemp -d "${TMPDIR:-/tmp}/rotfb-runtime-test.XXXXXX")
trap 'rm -rf "$TMP"' EXIT HUP INT TERM
mkdir -p "$TMP/include/sys"

cat > "$TMP/include/sys/types.h" <<EOF_TYPES
#ifndef ROTFB_TEST_SYS_TYPES_H
#define ROTFB_TEST_SYS_TYPES_H
#include_next <sys/types.h>
#include "$SCRIPT_DIR/rotfb_runtime_test_compat.h"
#endif
EOF_TYPES
for h in param atomic kmem ddi sunddi framebuffer gfx_private; do
    cat > "$TMP/include/sys/$h.h" <<EOF_HEADER
#ifndef ROTFB_TEST_SYS_${h}_H
#define ROTFB_TEST_SYS_${h}_H
#include "$SCRIPT_DIR/rotfb_runtime_test_compat.h"
#endif
EOF_HEADER
done
cat > "$TMP/include/sys/systm.h" <<EOF_SYSTM
#ifndef ROTFB_TEST_SYS_SYSTM_H
#define ROTFB_TEST_SYS_SYSTM_H
#include "$SCRIPT_DIR/rotfb_runtime_test_compat.h"
#define bcopy(src, dst, n) memmove((dst), (src), (n))
#endif
EOF_SYSTM
cat > "$TMP/include/sys/rotfb.h" <<EOF_ROTFB
#ifndef ROTFB_TEST_SYS_ROTFB_H
#define ROTFB_TEST_SYS_ROTFB_H
#include "$ROTFB_DIR/rotfb.h"
#endif
EOF_ROTFB

"$CC" -std=c11 $CFLAGS -Wall -Wextra -Werror -pthread \
    -I"$TMP/include" \
    -o "$TMP/rotfb-runtime-test" \
    "$ROTFB_DIR/rotfb_runtime.c" "$SCRIPT_DIR/rotfb_runtime_test.c"
"$TMP/rotfb-runtime-test"
