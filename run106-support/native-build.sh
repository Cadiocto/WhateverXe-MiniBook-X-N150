#!/bin/sh
set -eu
mkdir -p /tmp/run106-inputs
curl -fL --retry 3 --retry-delay 2 http://192.168.122.1:8000/run106-inputs.tar.gz -o /tmp/run106-inputs.tar.gz
curl -fL --retry 3 --retry-delay 2 http://192.168.122.1:8000/run106-inputs.tar.gz.sha256 -o /tmp/run106-inputs.tar.gz.sha256
expected=$(cat /tmp/run106-inputs.tar.gz.sha256)
actual=$(digest -a sha256 /tmp/run106-inputs.tar.gz | awk '{print $1}')
test "$actual" = "$expected"
tar -xzf /tmp/run106-inputs.tar.gz -C /tmp/run106-inputs
test -s /tmp/run106-inputs/BUILD-IDENTITY.txt
echo "RUN106_INPUTS_SHA256=$actual"
/usr/bin/bash -s <<'RUN106_BASH'
set -Eeuo pipefail
ART=/tmp/ax101-run106-artifacts
INPUTS=/tmp/run106-inputs/r5-runner-inputs
WIFI=/tmp/run106-inputs/wifi-ax101
SRC=/tmp/solnix-wifi
PIN=1cee7021091f3df92b7816ece2902ff59b37f1be
ILLUMOS_PIN=098e95820891f8a83fae679daece7fcf58b7b3a4
mkdir -p "$ART"
. "$WIFI/ci/native-diagnostics.sh"
ax101_diag_init "$ART"
finish() {
  local finish_rc=$?
  trap - EXIT ERR
  set +e
  if [ -s "$ART/iwx" ]; then
    digest -a sha256 "$ART/iwx" > "$ART/iwx.sha256"
    file "$ART/iwx" > "$ART/iwx.file.txt"
    elfdump -e "$ART/iwx" > "$ART/iwx.elf-header.txt" 2>&1 || true
    elfdump -d "$ART/iwx" > "$ART/iwx.dynamic.txt" 2>&1 || true
    elfdump -r "$ART/iwx" > "$ART/iwx.relocations.txt" 2>&1 || true
    nm -p "$ART/iwx" > "$ART/iwx.symbols.txt" 2>&1 || true
  fi
  ax101_diag_finish "$finish_rc"
  if [ -d "$ART" ]; then
    (cd /tmp && tar -czf /tmp/ax101-run106-artifacts.tar.gz ax101-run106-artifacts)
    if [ -s /tmp/ax101-run106-artifacts.tar.gz ] && curl -f --retry 3 --retry-delay 2 -X POST --data-binary @/tmp/ax101-run106-artifacts.tar.gz http://192.168.122.1:18081/guest-artifacts; then
      echo "RUN106_ARTIFACT_UPLOAD=PASS"
    else
      echo "RUN106_ARTIFACT_UPLOAD=FAIL" >&2
      [ "$finish_rc" -ne 0 ] || finish_rc=98
    fi
  fi
  exit "$finish_rc"
}
trap finish EXIT
ax101_diag_stage VM_ENVIRONMENT_INITIALIZATION

GUEST_KERNEL=$(uname -v)
{
  echo "OPENINDIANA_GUEST_KERNEL=$GUEST_KERNEL"
  echo "TARGET_LAPTOP_KERNEL=illumos-c893fd03ad"
  if [ "$GUEST_KERNEL" = "illumos-c893fd03ad" ]; then
    echo "KERNEL_MATCH=PASS"
  else
    echo "KERNEL_MATCH=FAIL"
  fi
  uname -a
  cat /etc/release
  pkg list -Hv entire 2>&1 || true
  pkg list -Hv consolidation/osnet/osnet-incorporation 2>&1 || true
} | tee "$ART/target-kernel-identity.txt"
if [ "$GUEST_KERNEL" != "illumos-c893fd03ad" ]; then
  echo "ERROR: refusing native build on non-target kernel" >&2
  exit 86
fi
test -s /tmp/run106-inputs/BUILD-IDENTITY.txt
cp /tmp/run106-inputs/BUILD-IDENTITY.txt "$ART/BUILD-IDENTITY.txt"
test -s "$INPUTS/R5-TRANSFER-MANIFEST.tsv"

echo "RUN106_SOURCE_BRANCH=ax101-r5-integration-validation"
echo "RUN106_CANDIDATE_BASE_COMMIT=f530b111e03b966b863d40bd8cb11adf07f60650"
echo "RUN106_TARGET_KERNEL=illumos-c893fd03ad"
test -s "$INPUTS/R5-TRANSFER-MANIFEST.tsv"
test -s "$INPUTS/wifi-ax101/patches/0027-run80-r5-active-iwx-integration.patch"
test -s "$INPUTS/wifi-ax101/INTEGRATION-EXPECTED-TREE.tsv"
uname -a
cat /etc/release
test "$(uname -s)" = SunOS
grep -q OpenIndiana /etc/release

pkg refresh
for fmri in developer/gcc-14 developer/build/gnu-make developer/versioning/git; do
  if ! pkg list -H "$fmri" >/dev/null 2>&1; then pkg install --accept "$fmri"; fi
  pkg list -H "$fmri"
done
export PATH="/usr/gcc/14/bin:/usr/bin:/usr/sbin"
export CC=/usr/gcc/14/bin/gcc
export LD=/usr/bin/ld
"$CC" --version | head -1
gmake --version | head -1
test -d /usr/include/sys
{
  echo "uname=$(uname -a)"
  echo "release=$(head -1 /etc/release)"
  echo "pwd=$PWD"
  echo "umask=$(umask)"
  echo "PATH=$PATH"
  echo "CC=$CC"
  echo "LD=$LD"
  echo "MAKE=$(gmake --version | head -1)"
  echo "GCC=$("$CC" --version | head -1)"
} > "$ART/runner-environment.txt"
ax101_diag_pass VM_ENVIRONMENT_INITIALIZATION
ax101_diag_stage PINNED_UPSTREAM_CHECKOUT

# Never consume a Solnix checkout or generated tree from the repository branch.
# Recreate the exact pinned upstream in a fresh directory, then apply R5+0027.
rm -rf "$SRC"
git clone --no-checkout https://github.com/atiq-cs/solnix.git "$SRC"
git -C "$SRC" checkout --detach "$PIN"
test "$(git -C "$SRC" rev-parse HEAD)" = "$PIN"
test "$(git -C "$SRC" status --porcelain | wc -l | tr -d ' ')" = "0"
ax101_diag_pass PINNED_UPSTREAM_CHECKOUT
ax101_diag_stage PATCH_STACK_APPLICATION
ln -sfn "$WIFI/solnix-$PIN.zip" "$INPUTS/wifi-ax101/solnix-$PIN.zip"
test -s "$INPUTS/wifi-ax101/solnix-$PIN.zip"
printf 'solnix_revision=%s\nillumos_gate_revision=%s\n' "$PIN" "$ILLUMOS_PIN" > "$ART/source-revision.txt"
echo "local_candidate_archive_sha256=f251541a7c60cc46cdaecc0719e42c576f6f5bd4699dea712a7db5ab9f54a1d0" >> "$ART/source-revision.txt"
echo "runner_transfer_bundle_sha256=a716503cc1a4fcf42004f459584492144739ea7aa12563e6fa1af5d2bc4dab0a" >> "$ART/source-revision.txt"
echo "checked_out_solnix_head=$(git -C "$SRC" rev-parse HEAD)" >> "$ART/source-revision.txt"

set +e
ax101_diag_command integration-replay bash "$INPUTS/wifi-ax101/apply-run80-r5-integration-stack.sh" "$SRC" "$ART/r5-integration-replay" 2>&1 | tee "$ART/r5-integration-replay.log"
replay_rc=${PIPESTATUS[0]}
set -e
echo "integration_replay_exit=$replay_rc" > "$ART/integration-replay-status.txt"
test "$replay_rc" -eq 0
test -s "$ART/r5-integration-replay/final-result.txt"
grep -Fq 'RUN80_R5_INTEGRATION_REPLAY=PASS' "$ART/r5-integration-replay/final-result.txt"

# RUN87: apply the exact-count post-generation adapter after replay.
ax101_diag_stage RUN87_ADAPTATION
python3 "$WIFI/ci/apply-run87-illumos-adaptation.py" --source-root "$SRC"
ax101_diag_pass RUN87_ADAPTATION

ax101_diag_stage GENERATOR_REPLAY
cd "$SRC/src/ports/wifi/iwx"
bash ./gen-illumos-src.sh --update
digest -a sha256 patches/if_iwx.illumos.patch > "$ART/run87-canonical-iwx-patch.sha256"
bash ./gen-illumos-src.sh
digest -a sha256 build/if_iwx_illumos.c > "$ART/run87-generated-iwx-source-first.sha256"
bash ./gen-illumos-src.sh
digest -a sha256 build/if_iwx_illumos.c > "$ART/run87-generated-iwx-source-second.sha256"
cmp -s "$ART/run87-generated-iwx-source-first.sha256" "$ART/run87-generated-iwx-source-second.sha256"
cp patches/if_iwx.illumos.patch "$ART/run87-if_iwx.illumos.patch"
cp build/if_iwx_illumos.c "$ART/run87-if_iwx_illumos.c"
digest -a sha256 "$ART/run87-if_iwx.illumos.patch" > "$ART/run87-if_iwx.illumos.patch.sha256"
digest -a sha256 "$ART/run87-if_iwx_illumos.c" > "$ART/run87-if_iwx_illumos.c.sha256"
cd "$SRC"
ax101_diag_pass GENERATOR_REPLAY

# RUN107 scan-discovery fix: apply only after the exact RUN106/RUN87 generated
# sources exist. The tarball contains the reviewed additive 0028 patch, tests,
# DTrace program and one-shot physical validation script.
SCANFIX_ARCHIVE="$WIFI/scanfix-implementation.tar.gz"
SCANFIX_PARTS="$WIFI/scanfix-archive-parts"
test -d "$SCANFIX_PARTS"
python3 - "$SCANFIX_PARTS" "$SCANFIX_ARCHIVE" <<'PY'
import base64, hashlib, pathlib, sys
parts_dir, output = pathlib.Path(sys.argv[1]), pathlib.Path(sys.argv[2])
parts = sorted(parts_dir.glob("*.b64"))
expected_names = [f"{i:02d}.b64" for i in range(8)]
if [p.name for p in parts] != expected_names:
    raise SystemExit("scanfix archive parts incomplete or out of order")
encoded = "".join(p.read_text().strip() for p in parts)
payload = base64.b64decode(encoded, validate=True)
actual = hashlib.sha256(payload).hexdigest()
expected = "f108bbabc11e37457a4fc4b2a49308ef69f1aba384d933c916582b6267f342c2"
if actual != expected:
    raise SystemExit(f"scanfix source archive SHA-256 mismatch: {actual}")
output.write_bytes(payload)
print(f"SCANFIX_SOURCE_ARCHIVE_SHA256={actual}")
PY
SCANFIX_ROOT="$WIFI/scanfix-implementation"
rm -rf "$SCANFIX_ROOT"
mkdir -p "$SCANFIX_ROOT"
tar -xzf "$SCANFIX_ARCHIVE" -C "$SCANFIX_ROOT"
SCANFIX_ROOT="$SCANFIX_ROOT/scanfix-implementation"
SCANFIX_PATCH="$SCANFIX_ROOT/patches/0028-ax101-scan-notification-rx-fix.patch"
test -s "$SCANFIX_PATCH"
digest -a sha256 "$SCANFIX_PATCH" > "$ART/scanfix-patch.sha256"
grep -Fq '9d469589bc5397efa89199a5c2fa86654a62956ab6ff4e0c33807e52b8e91f80' "$ART/scanfix-patch.sha256"

ax101_diag_stage SCANFIX_PATCH_APPLICATION
# The patch also changes the generated net80211 input file. Materialise the
# pinned baseline first; never patch a stale/generated file from another run.
cd "$SRC/src/ports/wifi/net80211"
bash ./gen-illumos-src.sh
cd "$SRC"
patch --batch --dry-run --fuzz=0 -p1 < "$SCANFIX_PATCH"
patch --batch --forward --fuzz=0 -p1 < "$SCANFIX_PATCH"

# RUN128: 0028's recovery reason enum was emitted after iwx_send_cmd(), so
# CMD_TIMEOUT was not in scope at its first use. Keep 0028 byte-identical and
# apply this additive, exact relocation before refreshing the canonical
# illumos generator patch. The --update pass bakes the corrected file-scope
# declaration order into patches/if_iwx.illumos.patch.
INIT_REASON_PATCH="$WIFI/patches/0029-iwx-init-task-reason-enum-scope.patch"
test -s "$INIT_REASON_PATCH"
digest -a sha256 "$INIT_REASON_PATCH" > "$ART/run128-init-reason-patch.sha256"
grep -Fq '265e25c861f17823113889e340a12321968a26e7d6023455c7aa28b473ce17aa' "$ART/run128-init-reason-patch.sha256"
cd "$SRC/src/ports/wifi/iwx"
patch --batch --dry-run --fuzz=0 -p1 < "$INIT_REASON_PATCH"
patch --batch --forward --fuzz=0 -p1 < "$INIT_REASON_PATCH"
cd "$SRC"
ax101_diag_pass SCANFIX_PATCH_APPLICATION

ax101_diag_stage SCANFIX_GENERATOR_REPLAY
cd "$SRC/src/ports/wifi/iwx"
bash ./gen-illumos-src.sh --update
bash ./gen-illumos-src.sh
digest -a sha256 build/if_iwx_illumos.c > "$ART/run107-generated-iwx-source-first.sha256"
bash ./gen-illumos-src.sh
digest -a sha256 build/if_iwx_illumos.c > "$ART/run107-generated-iwx-source-second.sha256"
cmp -s "$ART/run107-generated-iwx-source-first.sha256" "$ART/run107-generated-iwx-source-second.sha256"
python3 "$WIFI/tests/check-iwx-init-reason-scope.py" --self-test build/if_iwx_illumos.c 2>&1 | tee "$ART/run128-init-reason-scope.log"
grep -Fq 'INIT_REASON_SCOPE_NEGATIVE_CONTROLS=3/3 PASS' "$ART/run128-init-reason-scope.log"
grep -Fq 'INIT_REASON_SCOPE=PASS' "$ART/run128-init-reason-scope.log"
cp build/if_iwx_illumos.c "$ART/run107-if_iwx_illumos.c"
cp build/if_iwx_illumos.c "$ART/run128-if_iwx_illumos.c"
digest -a sha256 build/if_iwx_illumos.c > "$ART/run128-generated-iwx-source.sha256"
cp patches/if_iwx.illumos.patch "$ART/run107-if_iwx.illumos.patch"
cp patches/if_iwx.illumos.patch "$ART/run128-if_iwx.illumos.patch"
ax101_diag_stage RUN128_INIT_REASON_VALIDATION
grep -q 'IWX_INIT_REASON_CMD_TIMEOUT' build/if_iwx_illumos.c
grep -q '^enum iwx_init_task_reason {' build/if_iwx_illumos.c
ax101_diag_pass RUN128_INIT_REASON_VALIDATION
cd "$SRC/src/ports/wifi/net80211"
bash ./gen-illumos-src.sh --update ieee80211_input
bash ./gen-illumos-src.sh
digest -a sha256 build/ieee80211_input.c > "$ART/run107-generated-net80211-source-first.sha256"
bash ./gen-illumos-src.sh
digest -a sha256 build/ieee80211_input.c > "$ART/run107-generated-net80211-source-second.sha256"
cmp -s "$ART/run107-generated-net80211-source-first.sha256" "$ART/run107-generated-net80211-source-second.sha256"
cp build/ieee80211_input.c "$ART/run107-ieee80211_input.c"
cp patches/ieee80211_input.illumos.patch "$ART/run107-ieee80211_input.illumos.patch"
cd "$SRC"
ax101_diag_pass SCANFIX_GENERATOR_REPLAY

ax101_diag_stage SCANFIX_HOST_REGRESSION
python3 -m py_compile "$SCANFIX_ROOT/tests/scan-regression.py"
AX101_SOURCE_ROOT="$SRC/src/ports/wifi" python3 "$SCANFIX_ROOT/tests/scan-regression.py" 2>&1 | tee "$ART/scan-regression-final.log"
grep -Fq 'Ran 35 tests' "$ART/scan-regression-final.log"
grep -Fq 'OK' "$ART/scan-regression-final.log"
scan_nc_count=$(grep -Ec '^test_nc[0-9]{2}_.*\.\.\. ok

# Re-run existing ownership/DMA suites against the scanfix-mutated tree.
ax101_diag_command run107-mbuf-regression env CC="$CC" bash "$SRC/src/ports/wifi/iwx/run-mbuf-tests.sh" 2>&1 | tee "$ART/run107-mbuf-regression.log"
ax101_diag_command run107-dma-regression env CC="$CC" bash "$SRC/src/ports/wifi/iwm/run-dma-tests.sh" 2>&1 | tee "$ART/run107-dma-regression.log"
ax101_diag_pass SCANFIX_EXISTING_REGRESSION_REPLAY

# Retain the complete reviewed source-fix materials in the guest evidence.
mkdir -p "$ART/scanfix-source"
cp "$SCANFIX_PATCH" "$ART/scanfix-source/"
cp "$SCANFIX_ROOT/tests/scan-regression.py" "$ART/scanfix-source/"
cp "$SCANFIX_ROOT/tests/run-ax101-scan-validation.sh" "$ART/scanfix-source/"
cp "$SCANFIX_ROOT/diagnostics/ax101-scan.d" "$ART/scanfix-source/"
cp "$SCANFIX_ROOT/engineering/SCAN-DISCOVERY-ROOT-CAUSE.md" "$ART/scanfix-source/"
cp "$SCANFIX_ROOT/evidence/source-candidate-verification.txt" "$ART/scanfix-source/"
{
  echo "SCANFIX_PATCH_SHA256=$(awk '{print $1}' "$ART/scanfix-patch.sha256")"
  digest -a sha256 "$ART/run107-if_iwx_illumos.c"
  echo "TARGET_KERNEL=$GUEST_KERNEL"
  echo "SOLNIX_PIN=$PIN"
  echo "ILLUMOS_GATE_PIN=$ILLUMOS_PIN"
  echo "SCANFIX_REGRESSION=35/35"
  echo "SCANFIX_NEGATIVE_CONTROLS=10/10"
} > "$ART/scanfix-source-identity.txt"
{
  echo "INIT_REASON_ENUM_SCOPE=PASS"
  echo "INIT_REASON_ENUMERATOR_COUNT=14"
  echo "INIT_REASON_NEGATIVE_CONTROLS=3/3"
  echo "INIT_REASON_PATCH_0029_SHA256=$(awk '{print $1}' "$ART/run128-init-reason-patch.sha256")"
  digest -a sha256 "$ART/run128-if_iwx_illumos.c"
} > "$ART/run128-init-reason-identity.txt"
cat "$ART/run128-init-reason-identity.txt" >> "$ART/scanfix-source-identity.txt"
ax101_diag_stage RUN87_SOURCE_STATIC
python3 "$WIFI/ci/check-run87-illumos-adaptation.py" --source-root "$SRC" --self-test
test -s /usr/include/sys/callo.h
grep -Eq '^[[:space:]]*#define[[:space:]]+CALLOUT_FLAG_ABSOLUTE[[:space:]]+0x2' /usr/include/sys/callo.h
grep -q 'cv_timedwait_hires' /usr/include/sys/condvar.h
ax101_diag_stage RUN87_HOST_REGRESSION
cd "$SRC/src/ports/wifi/iwx"
set +e
ax101_diag_command run87-mbuf-regression env CC="$CC" bash ./run-mbuf-tests.sh 2>&1 | tee "$ART/run87-mbuf-regression.log"
run87_mbuf_rc=${PIPESTATUS[0]}
set -e
test "$run87_mbuf_rc" -eq 0
set +e
ax101_diag_command run87-dma-regression env CC="$CC" bash ../iwm/run-dma-tests.sh 2>&1 | tee "$ART/run87-dma-regression.log"
run87_dma_rc=${PIPESTATUS[0]}
set -e
test "$run87_dma_rc" -eq 0
cd "$SRC"
ax101_diag_pass RUN87_HOST_REGRESSION

echo "CI_SOURCE_IDENTITY_VERIFIED=PASS"
echo "PRODUCTION_SOURCE_ROOT=$SRC/src/ports/wifi/iwx"
echo "PRODUCTION_SOURCE_ROOT=$SRC/src/ports/wifi/iwx" > "$ART/production-source-root.txt"
ax101_diag_pass PATCH_STACK_APPLICATION
ax101_diag_pass INTEGRATION_REPLAY_VALIDATION
ax101_diag_pass SOURCE_STATIC_TESTS
ax101_diag_pass HOST_MODEL_TESTS
ax101_diag_stage NATIVE_BUILD_PREPARATION

# Exact pinned illumos-gate compatibility headers; use native OI headers first.
: > "$ART/net80211-header-closure.txt"
for rel in sys/net80211_proto.h sys/net80211.h sys/net80211_crypto.h sys/net80211_ht.h sys/net80211_amrr.h net/wpa.h; do
  case "$rel" in
    sys/net80211_proto.h) expected_blob=4fe7d40604615e3ffd1adb80d6e061fd4859c546 ;;
    sys/net80211.h) expected_blob=be6b65fe905fabd6ebfd31cf74d7048357510e12 ;;
    sys/net80211_crypto.h) expected_blob=72ef3ebec3c9ccc599ba29823d3561198d953252 ;;
    sys/net80211_ht.h) expected_blob=449726aa0feae79d6bcea0f73e0625a7078a150e ;;
    sys/net80211_amrr.h) expected_blob=36bd2fcd0eacd88c7b2c307c32a4db32bff5ffa0 ;;
    net/wpa.h) expected_blob=82c4fd468a5f623411a98a814c40ba3d9ac4c87b ;;
    *) echo "ERROR: unlisted header $rel" >&2; exit 1 ;;
  esac
  src="$INPUTS/wifi-ax101/compat/$rel"
  test -s "$src"
  raw_blob=$(git hash-object "$src")
  actual_blob=$(tr -d '\015' < "$src" | git hash-object --stdin)
  printf '%s\\t%s\\t%s\\n' "$rel" "$raw_blob" "$actual_blob" >> "$ART/compat-header-blob-normalization.txt"
  test "$actual_blob" = "$expected_blob"
  native="/usr/include/$rel"
  dst="$SRC/src/ports/wifi/iwm/compat-illumos/$rel"
  safe=$(printf '%s' "$rel" | tr '/' '-')
  if [ -s "$native" ]; then
    echo "NATIVE_HEADER_USED=$rel"
    digest -a sha256 "$native" > "$ART/native-header-$safe.sha256"
    printf '%s\t%s\tnative\n' "$rel" "$actual_blob" >> "$ART/net80211-header-closure.txt"
  else
    mkdir -p "$(dirname "$dst")"
    tr -d '\015' < "$src" > "$dst"
    chmod 0644 "$dst"
    tr -d '\015' < "$src" | cmp -s - "$dst"
    digest -a sha256 "$dst" > "$ART/header-$safe.sha256"
    printf '%s\t%s\tstaged-pinned-illumos-gate\n' "$rel" "$actual_blob" >> "$ART/net80211-header-closure.txt"
  fi
done
for rel in sys/mac.h sys/mac_provider.h sys/ethernet.h sys/atomic.h inet/wifi_ioctl.h; do
  test -s "/usr/include/$rel" || { echo "ERROR: missing native OpenIndiana header /usr/include/$rel" >&2; exit 1; }
  printf '%s\tnative-dependency\n' "$rel" >> "$ART/net80211-header-closure.txt"
done
DLS_SRC="$INPUTS/wifi-ax101/compat/sys/dls.h"
DLS_DST="$SRC/src/ports/wifi/iwm/compat-illumos/sys/dls.h"
test -s "$DLS_SRC"
mkdir -p "$(dirname "$DLS_DST")"
cp "$DLS_SRC" "$DLS_DST"
chmod 0644 "$DLS_DST"
cmp -s "$DLS_SRC" "$DLS_DST"
digest -a sha256 "$DLS_DST" > "$ART/dls-shim.sha256"

python3 "$INPUTS/wifi-ax101/tests/test-connect-wifi.py" 2>&1 | tee "$ART/connect-wifi-tests.txt"
python3 --version | tee "$ART/target-python-version.txt"
echo "RUN77_MODEL_PYTHON39_COMPAT=PRECHECKED_BY_TRANSFER_STEP"
echo "NATIVE_SOURCE_ROOT_CONFIRMED=$SRC/src/ports/wifi/iwx"
cd "$SRC/src/ports/wifi/iwx"
ax101_diag_command gmake-clean gmake -f Makefile.illumos clean
ax101_diag_pass NATIVE_BUILD_PREPARATION
ax101_diag_stage EXACT_NATIVE_GMAKE
ax101_diag_stage NATIVE_COMPILATION
set +e
ax101_diag_command native-gmake gmake -k -f Makefile.illumos -j4 UTSINC=/usr/include LD=/usr/bin/ld 2>&1 | tee "$ART/compiler-output.log"
compile_rc=${PIPESTATUS[0]}
set -e
echo "gmake_exit=$compile_rc" | tee "$ART/native-compile-exit.txt"
if [ "$compile_rc" -ne 0 ]; then
  grep -E '(^|: )(fatal error:|error:)|undefined reference|multiple definition' "$ART/compiler-output.log" > "$ART/compiler-errors.txt" || true
  grep -E 'warning:' "$ART/compiler-output.log" > "$ART/compiler-warnings.txt" || true
  exit "$compile_rc"
fi
ax101_diag_pass EXACT_NATIVE_GMAKE
ax101_diag_pass NATIVE_COMPILATION
ax101_diag_stage NATIVE_LINKING
grep -Fq 'warning: taking address of packed member' "$ART/compiler-output.log" && { echo "ERROR: packed member warning" >&2; exit 1; } || true
grep -Fq 'warning: "m_free" redefined' "$ART/compiler-output.log" && { echo "ERROR: m_free macro warning" >&2; exit 1; } || true
grep -Eq 'warning: "SIOC[GS]80211[^"]*" redefined' "$ART/compiler-output.log" && { echo "ERROR: ioctl macro warning" >&2; exit 1; } || true
grep -Eq 'warning: "(ETHERMTU|ETHERTYPE_IP|ETHERTYPE_ARP|ETHERTYPE_IPV6|roundup)" redefined' "$ART/compiler-output.log" && { echo "ERROR: native Ethernet/roundup macro warning" >&2; exit 1; } || true
test -s iwx
cp iwx "$ART/iwx"
ax101_diag_pass NATIVE_LINKING
ax101_diag_stage ELF_MODULE_INSPECTION

python3 "$INPUTS/wifi-ax101/tests/test-check-elf-exports.py"
nm -u "$ART/iwx" > "$ART/iwx.undefined.txt"
KERNEL_IMAGE=
for candidate in /platform/i86pc/kernel/amd64/unix /kernel/amd64/unix /kernel/unix; do
  if [ -s "$candidate" ]; then KERNEL_IMAGE="$candidate"; break; fi
done
test -n "$KERNEL_IMAGE"
echo "TARGET_KERNEL_IMAGE=$KERNEL_IMAGE" > "$ART/target-kernel-image.txt"
nm -g -p "$KERNEL_IMAGE" > "$ART/kernel-unix-symbols.txt"
MAC_MODULE=
for candidate in /kernel/misc/mac /kernel/misc/amd64/mac /usr/kernel/misc/mac /usr/kernel/misc/amd64/mac /platform/i86pc/kernel/misc/amd64/mac; do
  if [ -s "$candidate" ]; then MAC_MODULE="$candidate"; break; fi
done
test -n "$MAC_MODULE"
echo "TARGET_MAC_MODULE=$MAC_MODULE" > "$ART/target-mac-module.txt"
nm -g -p "$MAC_MODULE" > "$ART/mac-module-symbols.txt"
python3 "$INPUTS/wifi-ax101/tests/check-elf-exports.py" "$ART/iwx.undefined.txt" "$ART/kernel-unix-symbols.txt" "$ART/mac-module-symbols.txt" | tee "$ART/elf-symbol-reconciliation.txt"
elfdump -d "$ART/iwx" > "$ART/iwx.dynamic-acceptance.txt"
elfdump -r "$ART/iwx" > "$ART/iwx.relocations.txt"
grep -Eq 'NEEDED.*misc/mac' "$ART/iwx.dynamic-acceptance.txt"
echo "ELF_NEEDED_MISC_MAC=PASS"
if grep -q 'TEXTREL' "$ART/iwx.dynamic-acceptance.txt"; then echo "ELF_TEXTREL_PRESENT=YES" | tee "$ART/elf-relocation-policy.txt"; else echo "ELF_TEXTREL_PRESENT=NO" | tee "$ART/elf-relocation-policy.txt"; fi

if command -v modinfo >/dev/null 2>&1; then modinfo -c > "$ART/modinfo-before-modload.txt" 2>&1 || true; fi
if command -v dmesg >/dev/null 2>&1; then dmesg > "$ART/dmesg-before-modload.txt" 2>&1 || true; fi
mkdir -p /kernel/drv/amd64
cp "$ART/iwx" /kernel/drv/amd64/iwx
chmod 0644 /kernel/drv/amd64/iwx
if grep -Eq '^iwx[[:space:]]' /etc/name_to_major; then echo "ERROR: unexpected pre-existing iwx major" >&2; exit 1; fi
add_drv -v -i '\"pci8086,54f0\"' iwx > "$ART/add-drv.txt" 2>&1
cat "$ART/add-drv.txt"
grep -Eq '^iwx[[:space:]]+[0-9]+' /etc/name_to_major
grep -E '^iwx[[:space:]]' /etc/name_to_major > "$ART/iwx-name-to-major.txt"
grep -E '^iwx[[:space:]]' /etc/driver_aliases > "$ART/iwx-driver-aliases.txt" || true
echo "IWX_DRIVER_REGISTRATION=PASS"
set +e
modload /kernel/drv/amd64/iwx > "$ART/iwx-krtld-modload.txt" 2>&1
modload_rc=$?
set -e
echo "modload_exit=$modload_rc" > "$ART/iwx-krtld-modload-status.txt"
if [ "$modload_rc" -ne 0 ]; then cat "$ART/iwx-krtld-modload.txt"; dmesg > "$ART/dmesg-after-modload.txt" 2>&1 || true; tail -n 250 "$ART/dmesg-after-modload.txt" | grep -Ei 'iwx|krtld|modload|module|symbol|reloc|error|warn|pci' > "$ART/iwx-krtld-dmesg.txt" || true; exit "$modload_rc"; fi
echo "KRTLD_NATIVE_MODLOAD=PASS"
modinfo -c > "$ART/modinfo-after-modload.txt"
iwx_modid=$(awk '$3 == "iwx" && $2 ~ /^[0-9]+$/ && $2 > 0 && $4 == "LOADED/INSTALLED" { print $1; exit }' "$ART/modinfo-after-modload.txt")
case "$iwx_modid" in ''|*[!0-9]*) cat "$ART/modinfo-after-modload.txt"; exit 1;; esac
echo "$iwx_modid" > "$ART/iwx-module-id.txt"
dmesg > "$ART/dmesg-before-modunload.txt"
set +e
modunload -i "$iwx_modid" > "$ART/iwx-modunload.txt" 2>&1
modunload_rc=$?
set -e
echo "modunload_exit=$modunload_rc" > "$ART/iwx-modunload-status.txt"
if [ "$modunload_rc" -ne 0 ]; then cat "$ART/iwx-modunload.txt"; exit "$modunload_rc"; fi
dmesg > "$ART/dmesg-after-modunload.txt"
diff -u "$ART/dmesg-before-modunload.txt" "$ART/dmesg-after-modunload.txt" > "$ART/iwx-modunload-dmesg-delta.txt" || true
if grep -Eiq '(iwx.*(error|fail|warn|unable|cannot)|(error|fail|warn|unable|cannot).*iwx)' "$ART/iwx-modunload-dmesg-delta.txt"; then cat "$ART/iwx-modunload-dmesg-delta.txt"; exit 1; fi
modinfo -c > "$ART/modinfo-after-modunload.txt"
awk '$3 == "iwx" && $4 == "UNLOADED/UNINSTALLED" { unloaded=1 } END { exit !unloaded }' "$ART/modinfo-after-modunload.txt"
rem_drv iwx > "$ART/rem-drv.txt" 2>&1
rm -f /kernel/drv/amd64/iwx
echo "KRTLD_CONTROLLED_UNLOAD=PASS"
ax101_diag_pass ELF_MODULE_INSPECTION
ax101_diag_stage ARTIFACT_PACKAGING

FWBASE="https://git.kernel.org/pub/scm/linux/kernel/git/firmware/linux-firmware.git/plain"
curl -fL "$FWBASE/iwlwifi-so-a0-hr-b0-77.ucode?id=20241017" -o "$ART/iwx-so-a0-hr-b0-77"
curl -fL "$FWBASE/LICENCE.iwlwifi_firmware?id=20241017" -o "$ART/iwx-license"
test -s "$ART/iwx-so-a0-hr-b0-77"
grep -qi redistribution "$ART/iwx-license"
digest -a sha256 "$ART/iwx-so-a0-hr-b0-77" > "$ART/ax101-firmware.sha256"
            firmware_sha=$(awk '{print $1}' "$ART/ax101-firmware.sha256")
            test "$firmware_sha" = "bed4958236366c3c451b3ec994a56b2ad83c08d70e0a1eae42fb00f2eaabbcc8"
echo "NATIVE_OPENINDIANA_BUILD=PASS" > "$ART/STATUS.txt"
echo "BUILD_OK: exact R5 integration source replayed; native module built, ELF checked, registered, loaded and unloaded; firmware API-77 image staged" >> "$ART/STATUS.txt"
ax101_diag_pass ARTIFACT_PACKAGING

RUN106_BASH
 "$ART/scan-regression-final.log" || true)
test "$scan_nc_count" -eq 10
{
  echo "SCAN_NEGATIVE_CONTROLS=$scan_nc_count/10"
  grep -E '^test_nc[0-9]{2}_.*\.\.\. ok

# Re-run existing ownership/DMA suites against the scanfix-mutated tree.
ax101_diag_command run107-mbuf-regression env CC="$CC" bash "$SRC/src/ports/wifi/iwx/run-mbuf-tests.sh" 2>&1 | tee "$ART/run107-mbuf-regression.log"
ax101_diag_command run107-dma-regression env CC="$CC" bash "$SRC/src/ports/wifi/iwm/run-dma-tests.sh" 2>&1 | tee "$ART/run107-dma-regression.log"
ax101_diag_pass SCANFIX_EXISTING_REGRESSION_REPLAY

# Retain the complete reviewed source-fix materials in the guest evidence.
mkdir -p "$ART/scanfix-source"
cp "$SCANFIX_PATCH" "$ART/scanfix-source/"
cp "$SCANFIX_ROOT/tests/scan-regression.py" "$ART/scanfix-source/"
cp "$SCANFIX_ROOT/tests/run-ax101-scan-validation.sh" "$ART/scanfix-source/"
cp "$SCANFIX_ROOT/diagnostics/ax101-scan.d" "$ART/scanfix-source/"
cp "$SCANFIX_ROOT/engineering/SCAN-DISCOVERY-ROOT-CAUSE.md" "$ART/scanfix-source/"
cp "$SCANFIX_ROOT/evidence/source-candidate-verification.txt" "$ART/scanfix-source/"
{
  echo "SCANFIX_PATCH_SHA256=$(awk '{print $1}' "$ART/scanfix-patch.sha256")"
  digest -a sha256 "$ART/run107-if_iwx_illumos.c"
  echo "TARGET_KERNEL=$GUEST_KERNEL"
  echo "SOLNIX_PIN=$PIN"
  echo "ILLUMOS_GATE_PIN=$ILLUMOS_PIN"
  echo "SCANFIX_REGRESSION=35/35"
  echo "SCANFIX_NEGATIVE_CONTROLS=10/10"
} > "$ART/scanfix-source-identity.txt"
{
  echo "INIT_REASON_ENUM_SCOPE=PASS"
  echo "INIT_REASON_ENUMERATOR_COUNT=14"
  echo "INIT_REASON_NEGATIVE_CONTROLS=3/3"
  echo "INIT_REASON_PATCH_0029_SHA256=$(awk '{print $1}' "$ART/run128-init-reason-patch.sha256")"
  digest -a sha256 "$ART/run128-if_iwx_illumos.c"
} > "$ART/run128-init-reason-identity.txt"
cat "$ART/run128-init-reason-identity.txt" >> "$ART/scanfix-source-identity.txt"
ax101_diag_stage RUN87_SOURCE_STATIC
python3 "$WIFI/ci/check-run87-illumos-adaptation.py" --source-root "$SRC" --self-test
test -s /usr/include/sys/callo.h
grep -Eq '^[[:space:]]*#define[[:space:]]+CALLOUT_FLAG_ABSOLUTE[[:space:]]+0x2' /usr/include/sys/callo.h
grep -q 'cv_timedwait_hires' /usr/include/sys/condvar.h
ax101_diag_stage RUN87_HOST_REGRESSION
cd "$SRC/src/ports/wifi/iwx"
set +e
ax101_diag_command run87-mbuf-regression env CC="$CC" bash ./run-mbuf-tests.sh 2>&1 | tee "$ART/run87-mbuf-regression.log"
run87_mbuf_rc=${PIPESTATUS[0]}
set -e
test "$run87_mbuf_rc" -eq 0
set +e
ax101_diag_command run87-dma-regression env CC="$CC" bash ../iwm/run-dma-tests.sh 2>&1 | tee "$ART/run87-dma-regression.log"
run87_dma_rc=${PIPESTATUS[0]}
set -e
test "$run87_dma_rc" -eq 0
cd "$SRC"
ax101_diag_pass RUN87_HOST_REGRESSION

echo "CI_SOURCE_IDENTITY_VERIFIED=PASS"
echo "PRODUCTION_SOURCE_ROOT=$SRC/src/ports/wifi/iwx"
echo "PRODUCTION_SOURCE_ROOT=$SRC/src/ports/wifi/iwx" > "$ART/production-source-root.txt"
ax101_diag_pass PATCH_STACK_APPLICATION
ax101_diag_pass INTEGRATION_REPLAY_VALIDATION
ax101_diag_pass SOURCE_STATIC_TESTS
ax101_diag_pass HOST_MODEL_TESTS
ax101_diag_stage NATIVE_BUILD_PREPARATION

# Exact pinned illumos-gate compatibility headers; use native OI headers first.
: > "$ART/net80211-header-closure.txt"
for rel in sys/net80211_proto.h sys/net80211.h sys/net80211_crypto.h sys/net80211_ht.h sys/net80211_amrr.h net/wpa.h; do
  case "$rel" in
    sys/net80211_proto.h) expected_blob=4fe7d40604615e3ffd1adb80d6e061fd4859c546 ;;
    sys/net80211.h) expected_blob=be6b65fe905fabd6ebfd31cf74d7048357510e12 ;;
    sys/net80211_crypto.h) expected_blob=72ef3ebec3c9ccc599ba29823d3561198d953252 ;;
    sys/net80211_ht.h) expected_blob=449726aa0feae79d6bcea0f73e0625a7078a150e ;;
    sys/net80211_amrr.h) expected_blob=36bd2fcd0eacd88c7b2c307c32a4db32bff5ffa0 ;;
    net/wpa.h) expected_blob=82c4fd468a5f623411a98a814c40ba3d9ac4c87b ;;
    *) echo "ERROR: unlisted header $rel" >&2; exit 1 ;;
  esac
  src="$INPUTS/wifi-ax101/compat/$rel"
  test -s "$src"
  raw_blob=$(git hash-object "$src")
  actual_blob=$(tr -d '\015' < "$src" | git hash-object --stdin)
  printf '%s\\t%s\\t%s\\n' "$rel" "$raw_blob" "$actual_blob" >> "$ART/compat-header-blob-normalization.txt"
  test "$actual_blob" = "$expected_blob"
  native="/usr/include/$rel"
  dst="$SRC/src/ports/wifi/iwm/compat-illumos/$rel"
  safe=$(printf '%s' "$rel" | tr '/' '-')
  if [ -s "$native" ]; then
    echo "NATIVE_HEADER_USED=$rel"
    digest -a sha256 "$native" > "$ART/native-header-$safe.sha256"
    printf '%s\t%s\tnative\n' "$rel" "$actual_blob" >> "$ART/net80211-header-closure.txt"
  else
    mkdir -p "$(dirname "$dst")"
    tr -d '\015' < "$src" > "$dst"
    chmod 0644 "$dst"
    tr -d '\015' < "$src" | cmp -s - "$dst"
    digest -a sha256 "$dst" > "$ART/header-$safe.sha256"
    printf '%s\t%s\tstaged-pinned-illumos-gate\n' "$rel" "$actual_blob" >> "$ART/net80211-header-closure.txt"
  fi
done
for rel in sys/mac.h sys/mac_provider.h sys/ethernet.h sys/atomic.h inet/wifi_ioctl.h; do
  test -s "/usr/include/$rel" || { echo "ERROR: missing native OpenIndiana header /usr/include/$rel" >&2; exit 1; }
  printf '%s\tnative-dependency\n' "$rel" >> "$ART/net80211-header-closure.txt"
done
DLS_SRC="$INPUTS/wifi-ax101/compat/sys/dls.h"
DLS_DST="$SRC/src/ports/wifi/iwm/compat-illumos/sys/dls.h"
test -s "$DLS_SRC"
mkdir -p "$(dirname "$DLS_DST")"
cp "$DLS_SRC" "$DLS_DST"
chmod 0644 "$DLS_DST"
cmp -s "$DLS_SRC" "$DLS_DST"
digest -a sha256 "$DLS_DST" > "$ART/dls-shim.sha256"

python3 "$INPUTS/wifi-ax101/tests/test-connect-wifi.py" 2>&1 | tee "$ART/connect-wifi-tests.txt"
python3 --version | tee "$ART/target-python-version.txt"
echo "RUN77_MODEL_PYTHON39_COMPAT=PRECHECKED_BY_TRANSFER_STEP"
echo "NATIVE_SOURCE_ROOT_CONFIRMED=$SRC/src/ports/wifi/iwx"
cd "$SRC/src/ports/wifi/iwx"
ax101_diag_command gmake-clean gmake -f Makefile.illumos clean
ax101_diag_pass NATIVE_BUILD_PREPARATION
ax101_diag_stage EXACT_NATIVE_GMAKE
ax101_diag_stage NATIVE_COMPILATION
set +e
ax101_diag_command native-gmake gmake -k -f Makefile.illumos -j4 UTSINC=/usr/include LD=/usr/bin/ld 2>&1 | tee "$ART/compiler-output.log"
compile_rc=${PIPESTATUS[0]}
set -e
echo "gmake_exit=$compile_rc" | tee "$ART/native-compile-exit.txt"
if [ "$compile_rc" -ne 0 ]; then
  grep -E '(^|: )(fatal error:|error:)|undefined reference|multiple definition' "$ART/compiler-output.log" > "$ART/compiler-errors.txt" || true
  grep -E 'warning:' "$ART/compiler-output.log" > "$ART/compiler-warnings.txt" || true
  exit "$compile_rc"
fi
ax101_diag_pass EXACT_NATIVE_GMAKE
ax101_diag_pass NATIVE_COMPILATION
ax101_diag_stage NATIVE_LINKING
grep -Fq 'warning: taking address of packed member' "$ART/compiler-output.log" && { echo "ERROR: packed member warning" >&2; exit 1; } || true
grep -Fq 'warning: "m_free" redefined' "$ART/compiler-output.log" && { echo "ERROR: m_free macro warning" >&2; exit 1; } || true
grep -Eq 'warning: "SIOC[GS]80211[^"]*" redefined' "$ART/compiler-output.log" && { echo "ERROR: ioctl macro warning" >&2; exit 1; } || true
grep -Eq 'warning: "(ETHERMTU|ETHERTYPE_IP|ETHERTYPE_ARP|ETHERTYPE_IPV6|roundup)" redefined' "$ART/compiler-output.log" && { echo "ERROR: native Ethernet/roundup macro warning" >&2; exit 1; } || true
test -s iwx
cp iwx "$ART/iwx"
ax101_diag_pass NATIVE_LINKING
ax101_diag_stage ELF_MODULE_INSPECTION

python3 "$INPUTS/wifi-ax101/tests/test-check-elf-exports.py"
nm -u "$ART/iwx" > "$ART/iwx.undefined.txt"
KERNEL_IMAGE=
for candidate in /platform/i86pc/kernel/amd64/unix /kernel/amd64/unix /kernel/unix; do
  if [ -s "$candidate" ]; then KERNEL_IMAGE="$candidate"; break; fi
done
test -n "$KERNEL_IMAGE"
echo "TARGET_KERNEL_IMAGE=$KERNEL_IMAGE" > "$ART/target-kernel-image.txt"
nm -g -p "$KERNEL_IMAGE" > "$ART/kernel-unix-symbols.txt"
MAC_MODULE=
for candidate in /kernel/misc/mac /kernel/misc/amd64/mac /usr/kernel/misc/mac /usr/kernel/misc/amd64/mac /platform/i86pc/kernel/misc/amd64/mac; do
  if [ -s "$candidate" ]; then MAC_MODULE="$candidate"; break; fi
done
test -n "$MAC_MODULE"
echo "TARGET_MAC_MODULE=$MAC_MODULE" > "$ART/target-mac-module.txt"
nm -g -p "$MAC_MODULE" > "$ART/mac-module-symbols.txt"
python3 "$INPUTS/wifi-ax101/tests/check-elf-exports.py" "$ART/iwx.undefined.txt" "$ART/kernel-unix-symbols.txt" "$ART/mac-module-symbols.txt" | tee "$ART/elf-symbol-reconciliation.txt"
elfdump -d "$ART/iwx" > "$ART/iwx.dynamic-acceptance.txt"
elfdump -r "$ART/iwx" > "$ART/iwx.relocations.txt"
grep -Eq 'NEEDED.*misc/mac' "$ART/iwx.dynamic-acceptance.txt"
echo "ELF_NEEDED_MISC_MAC=PASS"
if grep -q 'TEXTREL' "$ART/iwx.dynamic-acceptance.txt"; then echo "ELF_TEXTREL_PRESENT=YES" | tee "$ART/elf-relocation-policy.txt"; else echo "ELF_TEXTREL_PRESENT=NO" | tee "$ART/elf-relocation-policy.txt"; fi

if command -v modinfo >/dev/null 2>&1; then modinfo -c > "$ART/modinfo-before-modload.txt" 2>&1 || true; fi
if command -v dmesg >/dev/null 2>&1; then dmesg > "$ART/dmesg-before-modload.txt" 2>&1 || true; fi
mkdir -p /kernel/drv/amd64
cp "$ART/iwx" /kernel/drv/amd64/iwx
chmod 0644 /kernel/drv/amd64/iwx
if grep -Eq '^iwx[[:space:]]' /etc/name_to_major; then echo "ERROR: unexpected pre-existing iwx major" >&2; exit 1; fi
add_drv -v -i '\"pci8086,54f0\"' iwx > "$ART/add-drv.txt" 2>&1
cat "$ART/add-drv.txt"
grep -Eq '^iwx[[:space:]]+[0-9]+' /etc/name_to_major
grep -E '^iwx[[:space:]]' /etc/name_to_major > "$ART/iwx-name-to-major.txt"
grep -E '^iwx[[:space:]]' /etc/driver_aliases > "$ART/iwx-driver-aliases.txt" || true
echo "IWX_DRIVER_REGISTRATION=PASS"
set +e
modload /kernel/drv/amd64/iwx > "$ART/iwx-krtld-modload.txt" 2>&1
modload_rc=$?
set -e
echo "modload_exit=$modload_rc" > "$ART/iwx-krtld-modload-status.txt"
if [ "$modload_rc" -ne 0 ]; then cat "$ART/iwx-krtld-modload.txt"; dmesg > "$ART/dmesg-after-modload.txt" 2>&1 || true; tail -n 250 "$ART/dmesg-after-modload.txt" | grep -Ei 'iwx|krtld|modload|module|symbol|reloc|error|warn|pci' > "$ART/iwx-krtld-dmesg.txt" || true; exit "$modload_rc"; fi
echo "KRTLD_NATIVE_MODLOAD=PASS"
modinfo -c > "$ART/modinfo-after-modload.txt"
iwx_modid=$(awk '$3 == "iwx" && $2 ~ /^[0-9]+$/ && $2 > 0 && $4 == "LOADED/INSTALLED" { print $1; exit }' "$ART/modinfo-after-modload.txt")
case "$iwx_modid" in ''|*[!0-9]*) cat "$ART/modinfo-after-modload.txt"; exit 1;; esac
echo "$iwx_modid" > "$ART/iwx-module-id.txt"
dmesg > "$ART/dmesg-before-modunload.txt"
set +e
modunload -i "$iwx_modid" > "$ART/iwx-modunload.txt" 2>&1
modunload_rc=$?
set -e
echo "modunload_exit=$modunload_rc" > "$ART/iwx-modunload-status.txt"
if [ "$modunload_rc" -ne 0 ]; then cat "$ART/iwx-modunload.txt"; exit "$modunload_rc"; fi
dmesg > "$ART/dmesg-after-modunload.txt"
diff -u "$ART/dmesg-before-modunload.txt" "$ART/dmesg-after-modunload.txt" > "$ART/iwx-modunload-dmesg-delta.txt" || true
if grep -Eiq '(iwx.*(error|fail|warn|unable|cannot)|(error|fail|warn|unable|cannot).*iwx)' "$ART/iwx-modunload-dmesg-delta.txt"; then cat "$ART/iwx-modunload-dmesg-delta.txt"; exit 1; fi
modinfo -c > "$ART/modinfo-after-modunload.txt"
awk '$3 == "iwx" && $4 == "UNLOADED/UNINSTALLED" { unloaded=1 } END { exit !unloaded }' "$ART/modinfo-after-modunload.txt"
rem_drv iwx > "$ART/rem-drv.txt" 2>&1
rm -f /kernel/drv/amd64/iwx
echo "KRTLD_CONTROLLED_UNLOAD=PASS"
ax101_diag_pass ELF_MODULE_INSPECTION
ax101_diag_stage ARTIFACT_PACKAGING

FWBASE="https://git.kernel.org/pub/scm/linux/kernel/git/firmware/linux-firmware.git/plain"
curl -fL "$FWBASE/iwlwifi-so-a0-hr-b0-77.ucode?id=20241017" -o "$ART/iwx-so-a0-hr-b0-77"
curl -fL "$FWBASE/LICENCE.iwlwifi_firmware?id=20241017" -o "$ART/iwx-license"
test -s "$ART/iwx-so-a0-hr-b0-77"
grep -qi redistribution "$ART/iwx-license"
digest -a sha256 "$ART/iwx-so-a0-hr-b0-77" > "$ART/ax101-firmware.sha256"
            firmware_sha=$(awk '{print $1}' "$ART/ax101-firmware.sha256")
            test "$firmware_sha" = "bed4958236366c3c451b3ec994a56b2ad83c08d70e0a1eae42fb00f2eaabbcc8"
echo "NATIVE_OPENINDIANA_BUILD=PASS" > "$ART/STATUS.txt"
echo "BUILD_OK: exact R5 integration source replayed; native module built, ELF checked, registered, loaded and unloaded; firmware API-77 image staged" >> "$ART/STATUS.txt"
ax101_diag_pass ARTIFACT_PACKAGING

RUN106_BASH
 "$ART/scan-regression-final.log"
  echo "EXIT_CODE=0"
} | tee "$ART/scanfix-negative-controls.log"
ax101_diag_pass SCANFIX_HOST_REGRESSION

# Re-run existing ownership/DMA suites against the scanfix-mutated tree.
ax101_diag_command run107-mbuf-regression env CC="$CC" bash "$SRC/src/ports/wifi/iwx/run-mbuf-tests.sh" 2>&1 | tee "$ART/run107-mbuf-regression.log"
ax101_diag_command run107-dma-regression env CC="$CC" bash "$SRC/src/ports/wifi/iwm/run-dma-tests.sh" 2>&1 | tee "$ART/run107-dma-regression.log"
ax101_diag_pass SCANFIX_EXISTING_REGRESSION_REPLAY

# Retain the complete reviewed source-fix materials in the guest evidence.
mkdir -p "$ART/scanfix-source"
cp "$SCANFIX_PATCH" "$ART/scanfix-source/"
cp "$SCANFIX_ROOT/tests/scan-regression.py" "$ART/scanfix-source/"
cp "$SCANFIX_ROOT/tests/run-ax101-scan-validation.sh" "$ART/scanfix-source/"
cp "$SCANFIX_ROOT/diagnostics/ax101-scan.d" "$ART/scanfix-source/"
cp "$SCANFIX_ROOT/engineering/SCAN-DISCOVERY-ROOT-CAUSE.md" "$ART/scanfix-source/"
cp "$SCANFIX_ROOT/evidence/source-candidate-verification.txt" "$ART/scanfix-source/"
{
  echo "SCANFIX_PATCH_SHA256=$(awk '{print $1}' "$ART/scanfix-patch.sha256")"
  digest -a sha256 "$ART/run107-if_iwx_illumos.c"
  echo "TARGET_KERNEL=$GUEST_KERNEL"
  echo "SOLNIX_PIN=$PIN"
  echo "ILLUMOS_GATE_PIN=$ILLUMOS_PIN"
  echo "SCANFIX_REGRESSION=35/35"
  echo "SCANFIX_NEGATIVE_CONTROLS=10/10"
} > "$ART/scanfix-source-identity.txt"
{
  echo "INIT_REASON_ENUM_SCOPE=PASS"
  echo "INIT_REASON_ENUMERATOR_COUNT=14"
  echo "INIT_REASON_NEGATIVE_CONTROLS=3/3"
  echo "INIT_REASON_PATCH_0029_SHA256=$(awk '{print $1}' "$ART/run128-init-reason-patch.sha256")"
  digest -a sha256 "$ART/run128-if_iwx_illumos.c"
} > "$ART/run128-init-reason-identity.txt"
cat "$ART/run128-init-reason-identity.txt" >> "$ART/scanfix-source-identity.txt"
ax101_diag_stage RUN87_SOURCE_STATIC
python3 "$WIFI/ci/check-run87-illumos-adaptation.py" --source-root "$SRC" --self-test
test -s /usr/include/sys/callo.h
grep -Eq '^[[:space:]]*#define[[:space:]]+CALLOUT_FLAG_ABSOLUTE[[:space:]]+0x2' /usr/include/sys/callo.h
grep -q 'cv_timedwait_hires' /usr/include/sys/condvar.h
ax101_diag_stage RUN87_HOST_REGRESSION
cd "$SRC/src/ports/wifi/iwx"
set +e
ax101_diag_command run87-mbuf-regression env CC="$CC" bash ./run-mbuf-tests.sh 2>&1 | tee "$ART/run87-mbuf-regression.log"
run87_mbuf_rc=${PIPESTATUS[0]}
set -e
test "$run87_mbuf_rc" -eq 0
set +e
ax101_diag_command run87-dma-regression env CC="$CC" bash ../iwm/run-dma-tests.sh 2>&1 | tee "$ART/run87-dma-regression.log"
run87_dma_rc=${PIPESTATUS[0]}
set -e
test "$run87_dma_rc" -eq 0
cd "$SRC"
ax101_diag_pass RUN87_HOST_REGRESSION

echo "CI_SOURCE_IDENTITY_VERIFIED=PASS"
echo "PRODUCTION_SOURCE_ROOT=$SRC/src/ports/wifi/iwx"
echo "PRODUCTION_SOURCE_ROOT=$SRC/src/ports/wifi/iwx" > "$ART/production-source-root.txt"
ax101_diag_pass PATCH_STACK_APPLICATION
ax101_diag_pass INTEGRATION_REPLAY_VALIDATION
ax101_diag_pass SOURCE_STATIC_TESTS
ax101_diag_pass HOST_MODEL_TESTS
ax101_diag_stage NATIVE_BUILD_PREPARATION

# Exact pinned illumos-gate compatibility headers; use native OI headers first.
: > "$ART/net80211-header-closure.txt"
for rel in sys/net80211_proto.h sys/net80211.h sys/net80211_crypto.h sys/net80211_ht.h sys/net80211_amrr.h net/wpa.h; do
  case "$rel" in
    sys/net80211_proto.h) expected_blob=4fe7d40604615e3ffd1adb80d6e061fd4859c546 ;;
    sys/net80211.h) expected_blob=be6b65fe905fabd6ebfd31cf74d7048357510e12 ;;
    sys/net80211_crypto.h) expected_blob=72ef3ebec3c9ccc599ba29823d3561198d953252 ;;
    sys/net80211_ht.h) expected_blob=449726aa0feae79d6bcea0f73e0625a7078a150e ;;
    sys/net80211_amrr.h) expected_blob=36bd2fcd0eacd88c7b2c307c32a4db32bff5ffa0 ;;
    net/wpa.h) expected_blob=82c4fd468a5f623411a98a814c40ba3d9ac4c87b ;;
    *) echo "ERROR: unlisted header $rel" >&2; exit 1 ;;
  esac
  src="$INPUTS/wifi-ax101/compat/$rel"
  test -s "$src"
  raw_blob=$(git hash-object "$src")
  actual_blob=$(tr -d '\015' < "$src" | git hash-object --stdin)
  printf '%s\\t%s\\t%s\\n' "$rel" "$raw_blob" "$actual_blob" >> "$ART/compat-header-blob-normalization.txt"
  test "$actual_blob" = "$expected_blob"
  native="/usr/include/$rel"
  dst="$SRC/src/ports/wifi/iwm/compat-illumos/$rel"
  safe=$(printf '%s' "$rel" | tr '/' '-')
  if [ -s "$native" ]; then
    echo "NATIVE_HEADER_USED=$rel"
    digest -a sha256 "$native" > "$ART/native-header-$safe.sha256"
    printf '%s\t%s\tnative\n' "$rel" "$actual_blob" >> "$ART/net80211-header-closure.txt"
  else
    mkdir -p "$(dirname "$dst")"
    tr -d '\015' < "$src" > "$dst"
    chmod 0644 "$dst"
    tr -d '\015' < "$src" | cmp -s - "$dst"
    digest -a sha256 "$dst" > "$ART/header-$safe.sha256"
    printf '%s\t%s\tstaged-pinned-illumos-gate\n' "$rel" "$actual_blob" >> "$ART/net80211-header-closure.txt"
  fi
done
for rel in sys/mac.h sys/mac_provider.h sys/ethernet.h sys/atomic.h inet/wifi_ioctl.h; do
  test -s "/usr/include/$rel" || { echo "ERROR: missing native OpenIndiana header /usr/include/$rel" >&2; exit 1; }
  printf '%s\tnative-dependency\n' "$rel" >> "$ART/net80211-header-closure.txt"
done
DLS_SRC="$INPUTS/wifi-ax101/compat/sys/dls.h"
DLS_DST="$SRC/src/ports/wifi/iwm/compat-illumos/sys/dls.h"
test -s "$DLS_SRC"
mkdir -p "$(dirname "$DLS_DST")"
cp "$DLS_SRC" "$DLS_DST"
chmod 0644 "$DLS_DST"
cmp -s "$DLS_SRC" "$DLS_DST"
digest -a sha256 "$DLS_DST" > "$ART/dls-shim.sha256"

python3 "$INPUTS/wifi-ax101/tests/test-connect-wifi.py" 2>&1 | tee "$ART/connect-wifi-tests.txt"
python3 --version | tee "$ART/target-python-version.txt"
echo "RUN77_MODEL_PYTHON39_COMPAT=PRECHECKED_BY_TRANSFER_STEP"
echo "NATIVE_SOURCE_ROOT_CONFIRMED=$SRC/src/ports/wifi/iwx"
cd "$SRC/src/ports/wifi/iwx"
ax101_diag_command gmake-clean gmake -f Makefile.illumos clean
ax101_diag_pass NATIVE_BUILD_PREPARATION
ax101_diag_stage EXACT_NATIVE_GMAKE
ax101_diag_stage NATIVE_COMPILATION
set +e
ax101_diag_command native-gmake gmake -k -f Makefile.illumos -j4 UTSINC=/usr/include LD=/usr/bin/ld 2>&1 | tee "$ART/compiler-output.log"
compile_rc=${PIPESTATUS[0]}
set -e
echo "gmake_exit=$compile_rc" | tee "$ART/native-compile-exit.txt"
if [ "$compile_rc" -ne 0 ]; then
  grep -E '(^|: )(fatal error:|error:)|undefined reference|multiple definition' "$ART/compiler-output.log" > "$ART/compiler-errors.txt" || true
  grep -E 'warning:' "$ART/compiler-output.log" > "$ART/compiler-warnings.txt" || true
  exit "$compile_rc"
fi
ax101_diag_pass EXACT_NATIVE_GMAKE
ax101_diag_pass NATIVE_COMPILATION
ax101_diag_stage NATIVE_LINKING
grep -Fq 'warning: taking address of packed member' "$ART/compiler-output.log" && { echo "ERROR: packed member warning" >&2; exit 1; } || true
grep -Fq 'warning: "m_free" redefined' "$ART/compiler-output.log" && { echo "ERROR: m_free macro warning" >&2; exit 1; } || true
grep -Eq 'warning: "SIOC[GS]80211[^"]*" redefined' "$ART/compiler-output.log" && { echo "ERROR: ioctl macro warning" >&2; exit 1; } || true
grep -Eq 'warning: "(ETHERMTU|ETHERTYPE_IP|ETHERTYPE_ARP|ETHERTYPE_IPV6|roundup)" redefined' "$ART/compiler-output.log" && { echo "ERROR: native Ethernet/roundup macro warning" >&2; exit 1; } || true
test -s iwx
cp iwx "$ART/iwx"
ax101_diag_pass NATIVE_LINKING
ax101_diag_stage ELF_MODULE_INSPECTION

python3 "$INPUTS/wifi-ax101/tests/test-check-elf-exports.py"
nm -u "$ART/iwx" > "$ART/iwx.undefined.txt"
KERNEL_IMAGE=
for candidate in /platform/i86pc/kernel/amd64/unix /kernel/amd64/unix /kernel/unix; do
  if [ -s "$candidate" ]; then KERNEL_IMAGE="$candidate"; break; fi
done
test -n "$KERNEL_IMAGE"
echo "TARGET_KERNEL_IMAGE=$KERNEL_IMAGE" > "$ART/target-kernel-image.txt"
nm -g -p "$KERNEL_IMAGE" > "$ART/kernel-unix-symbols.txt"
MAC_MODULE=
for candidate in /kernel/misc/mac /kernel/misc/amd64/mac /usr/kernel/misc/mac /usr/kernel/misc/amd64/mac /platform/i86pc/kernel/misc/amd64/mac; do
  if [ -s "$candidate" ]; then MAC_MODULE="$candidate"; break; fi
done
test -n "$MAC_MODULE"
echo "TARGET_MAC_MODULE=$MAC_MODULE" > "$ART/target-mac-module.txt"
nm -g -p "$MAC_MODULE" > "$ART/mac-module-symbols.txt"
python3 "$INPUTS/wifi-ax101/tests/check-elf-exports.py" "$ART/iwx.undefined.txt" "$ART/kernel-unix-symbols.txt" "$ART/mac-module-symbols.txt" | tee "$ART/elf-symbol-reconciliation.txt"
elfdump -d "$ART/iwx" > "$ART/iwx.dynamic-acceptance.txt"
elfdump -r "$ART/iwx" > "$ART/iwx.relocations.txt"
grep -Eq 'NEEDED.*misc/mac' "$ART/iwx.dynamic-acceptance.txt"
echo "ELF_NEEDED_MISC_MAC=PASS"
if grep -q 'TEXTREL' "$ART/iwx.dynamic-acceptance.txt"; then echo "ELF_TEXTREL_PRESENT=YES" | tee "$ART/elf-relocation-policy.txt"; else echo "ELF_TEXTREL_PRESENT=NO" | tee "$ART/elf-relocation-policy.txt"; fi

if command -v modinfo >/dev/null 2>&1; then modinfo -c > "$ART/modinfo-before-modload.txt" 2>&1 || true; fi
if command -v dmesg >/dev/null 2>&1; then dmesg > "$ART/dmesg-before-modload.txt" 2>&1 || true; fi
mkdir -p /kernel/drv/amd64
cp "$ART/iwx" /kernel/drv/amd64/iwx
chmod 0644 /kernel/drv/amd64/iwx
if grep -Eq '^iwx[[:space:]]' /etc/name_to_major; then echo "ERROR: unexpected pre-existing iwx major" >&2; exit 1; fi
add_drv -v -i '\"pci8086,54f0\"' iwx > "$ART/add-drv.txt" 2>&1
cat "$ART/add-drv.txt"
grep -Eq '^iwx[[:space:]]+[0-9]+' /etc/name_to_major
grep -E '^iwx[[:space:]]' /etc/name_to_major > "$ART/iwx-name-to-major.txt"
grep -E '^iwx[[:space:]]' /etc/driver_aliases > "$ART/iwx-driver-aliases.txt" || true
echo "IWX_DRIVER_REGISTRATION=PASS"
set +e
modload /kernel/drv/amd64/iwx > "$ART/iwx-krtld-modload.txt" 2>&1
modload_rc=$?
set -e
echo "modload_exit=$modload_rc" > "$ART/iwx-krtld-modload-status.txt"
if [ "$modload_rc" -ne 0 ]; then cat "$ART/iwx-krtld-modload.txt"; dmesg > "$ART/dmesg-after-modload.txt" 2>&1 || true; tail -n 250 "$ART/dmesg-after-modload.txt" | grep -Ei 'iwx|krtld|modload|module|symbol|reloc|error|warn|pci' > "$ART/iwx-krtld-dmesg.txt" || true; exit "$modload_rc"; fi
echo "KRTLD_NATIVE_MODLOAD=PASS"
modinfo -c > "$ART/modinfo-after-modload.txt"
iwx_modid=$(awk '$3 == "iwx" && $2 ~ /^[0-9]+$/ && $2 > 0 && $4 == "LOADED/INSTALLED" { print $1; exit }' "$ART/modinfo-after-modload.txt")
case "$iwx_modid" in ''|*[!0-9]*) cat "$ART/modinfo-after-modload.txt"; exit 1;; esac
echo "$iwx_modid" > "$ART/iwx-module-id.txt"
dmesg > "$ART/dmesg-before-modunload.txt"
set +e
modunload -i "$iwx_modid" > "$ART/iwx-modunload.txt" 2>&1
modunload_rc=$?
set -e
echo "modunload_exit=$modunload_rc" > "$ART/iwx-modunload-status.txt"
if [ "$modunload_rc" -ne 0 ]; then cat "$ART/iwx-modunload.txt"; exit "$modunload_rc"; fi
dmesg > "$ART/dmesg-after-modunload.txt"
diff -u "$ART/dmesg-before-modunload.txt" "$ART/dmesg-after-modunload.txt" > "$ART/iwx-modunload-dmesg-delta.txt" || true
if grep -Eiq '(iwx.*(error|fail|warn|unable|cannot)|(error|fail|warn|unable|cannot).*iwx)' "$ART/iwx-modunload-dmesg-delta.txt"; then cat "$ART/iwx-modunload-dmesg-delta.txt"; exit 1; fi
modinfo -c > "$ART/modinfo-after-modunload.txt"
awk '$3 == "iwx" && $4 == "UNLOADED/UNINSTALLED" { unloaded=1 } END { exit !unloaded }' "$ART/modinfo-after-modunload.txt"
rem_drv iwx > "$ART/rem-drv.txt" 2>&1
rm -f /kernel/drv/amd64/iwx
echo "KRTLD_CONTROLLED_UNLOAD=PASS"
ax101_diag_pass ELF_MODULE_INSPECTION
ax101_diag_stage ARTIFACT_PACKAGING

FWBASE="https://git.kernel.org/pub/scm/linux/kernel/git/firmware/linux-firmware.git/plain"
curl -fL "$FWBASE/iwlwifi-so-a0-hr-b0-77.ucode?id=20241017" -o "$ART/iwx-so-a0-hr-b0-77"
curl -fL "$FWBASE/LICENCE.iwlwifi_firmware?id=20241017" -o "$ART/iwx-license"
test -s "$ART/iwx-so-a0-hr-b0-77"
grep -qi redistribution "$ART/iwx-license"
digest -a sha256 "$ART/iwx-so-a0-hr-b0-77" > "$ART/ax101-firmware.sha256"
            firmware_sha=$(awk '{print $1}' "$ART/ax101-firmware.sha256")
            test "$firmware_sha" = "bed4958236366c3c451b3ec994a56b2ad83c08d70e0a1eae42fb00f2eaabbcc8"
echo "NATIVE_OPENINDIANA_BUILD=PASS" > "$ART/STATUS.txt"
echo "BUILD_OK: exact R5 integration source replayed; native module built, ELF checked, registered, loaded and unloaded; firmware API-77 image staged" >> "$ART/STATUS.txt"
ax101_diag_pass ARTIFACT_PACKAGING

RUN106_BASH
