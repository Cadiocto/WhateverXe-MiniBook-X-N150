#!/bin/sh
# RUN106 guest package bootstrap. The first kernel gate runs BEFORE pkg refresh
# or package installation, so no toolchain operation can silently change the target.
set -eu
TARGET_KERNEL="illumos-c893fd03ad"
ART="/tmp/ax101-run106-artifacts"
mkdir -p "$ART"
PREFLIGHT="$ART/target-kernel-preflight-before-tools.txt"
{
    echo "OPENINDIANA_GUEST_KERNEL=$(uname -v)"
    echo "TARGET_LAPTOP_KERNEL=$TARGET_KERNEL"
    uname -a
    uname -v
    cat /etc/release
    pkg list -Hv entire 2>&1 || true
    pkg list -Hv consolidation/osnet/osnet-incorporation 2>&1 || true
} > "$PREFLIGHT"
GUEST_KERNEL=$(uname -v)
if [ "$GUEST_KERNEL" != "$TARGET_KERNEL" ]; then
    echo "KERNEL_MATCH=FAIL" >> "$PREFLIGHT"
    cat "$PREFLIGHT"
    if command -v curl >/dev/null 2>&1; then
        curl -f --max-time 10 -X POST --data-binary @"$PREFLIGHT" http://192.168.122.1:18081/preflight || true
    fi
    exit 86
fi
echo "KERNEL_MATCH=PASS" >> "$PREFLIGHT"
cat "$PREFLIGHT"
if [ -z "${ANYVM_PKGS:-}" ]; then
    echo "ERROR: ANYVM_PKGS was not supplied by the pinned builder config" >&2
    exit 87
fi
case " $ANYVM_PKGS " in
    *" developer/gcc-14 "*) ;;
    *) echo "ERROR: gcc-14 missing from ANYVM_PKGS" >&2; exit 87;;
esac
case " $ANYVM_PKGS " in
    *" developer/build/gnu-make "*) ;;
    *) echo "ERROR: gnu-make missing from ANYVM_PKGS" >&2; exit 87;;
esac
case " $ANYVM_PKGS " in
    *" developer/versioning/git "*) ;;
    *) echo "ERROR: git missing from ANYVM_PKGS" >&2; exit 87;;
esac
# Refresh only the catalog. Deliberately do NOT run 'pkg update'.
pkg refresh
pkg install --accept $ANYVM_PKGS
GUEST_KERNEL_AFTER_TOOLS=$(uname -v)
echo "OPENINDIANA_GUEST_KERNEL_AFTER_TOOLS=$GUEST_KERNEL_AFTER_TOOLS"
if [ "$GUEST_KERNEL_AFTER_TOOLS" != "$TARGET_KERNEL" ]; then
    echo "KERNEL_MATCH_AFTER_TOOLS=FAIL"
    exit 88
fi
export PATH="/usr/gcc/14/bin:/usr/bin:/usr/sbin"
export CC="/usr/gcc/14/bin/gcc"
export LD="/usr/bin/ld"
test -x "$CC"
test -x "$LD"
command -v gmake
{
    echo "KERNEL_MATCH=PASS"
    echo "KERNEL_MATCH_AFTER_TOOLS=PASS"
    echo "OPENINDIANA_GUEST_KERNEL=$GUEST_KERNEL_AFTER_TOOLS"
    echo "CC=$CC"
    echo "GCC=$("$CC" --version | head -1)"
    echo "LD=$LD"
    echo "MAKE=$(gmake --version | head -1)"
    echo "PKG_LIST_BEGIN"
    pkg list -H developer/gcc-14 developer/build/gnu-make developer/versioning/git 2>&1 || true
    echo "PKG_LIST_END"
} > "$ART/toolchain-preflight.txt"
cat "$ART/toolchain-preflight.txt"
if command -v curl >/dev/null 2>&1; then
    curl -f --max-time 10 -X POST --data-binary @"$PREFLIGHT" http://192.168.122.1:18081/preflight || true
fi
