#!/usr/bin/env bash
set -Eeuo pipefail

REPO_ROOT=\${1:?repository root required}
RUNNER_INPUTS=\${2:?verified R5 runner inputs directory required}
ARTIFACTS=\${3:?artifact output directory required}
SOURCE_COMMIT=\${4:?source commit required}
RUN_ID=\${5:?GitHub run id required}
RUNNER_KERNEL=\${6:?runner host kernel required}

BUILDER_COMMIT=25a7144f849eec4c6c31303b41dfd2593ef59cfd
ISO_URL=https://dlc.openindiana.org/isos/hipster/20261030/OI-hipster-text-20261030.iso
ISO_SHA256=c5cd9572219f49975c333ac8339b18989e5477ab2e81844b0b84c3ae776a6e1a
ORIGINAL_POSTBUILD_BLOB=557c8e0217e5dc69612022d5b1e6c57cb13ca6ec

mkdir -p "$ARTIFACTS"
printf '%s\n' "$RUNNER_KERNEL" > "$ARTIFACTS/runner-host-kernel.txt"

BUILDER="$REPO_ROOT/anyvm-openindiana-builder"
rm -rf "$BUILDER"
git clone --filter=blob:none https://github.com/anyvm-org/openindiana-builder.git "$BUILDER"
git -C "$BUILDER" checkout --detach "$BUILDER_COMMIT"
test "$(git -C "$BUILDER" rev-parse HEAD)" = "$BUILDER_COMMIT"

POSTBUILD="$BUILDER/hooks/vm_postBuild.sh"
actual_postbuild_blob=$(git -C "$BUILDER" hash-object "$POSTBUILD")
test "$actual_postbuild_blob" = "$ORIGINAL_POSTBUILD_BLOB"
python3 - "$POSTBUILD" <<'PY'
from pathlib import Path
import sys
p = Path(sys.argv[1])
s = p.read_text()
old = "\npkg update\n"
if s.count(old) != 1:
    raise SystemExit(f"expected exactly one pkg update statement, found {s.count(old)}")
s = s.replace(old, "\necho 'RUN106: pkg update intentionally skipped to preserve the verified October 30 kernel'\n", 1)
p.write_text(s)
PY
if grep -Eq '^[[:space:]]*pkg[[:space:]]+update([[:space:]]|$)' "$POSTBUILD"; then
    echo "ERROR: postBuild still contains pkg update" >&2
    exit 1
fi
MODIFIED_POSTBUILD_SHA=$(sha256sum "$POSTBUILD" | awk '{print $1}')

cat > "$BUILDER/hooks/host_beforeBuild.sh" <<EOF
#!/usr/bin/env bash
set -Eeuo pipefail
ISO_URL="$ISO_URL"
EXPECTED_SHA256="$ISO_SHA256"
mkdir -p build
curl -fL --retry 4 --retry-delay 3 "\$ISO_URL" -o build/openindiana.iso.download
printf '%s  %s\\n' "\$EXPECTED_SHA256" build/openindiana.iso.download | sha256sum -c -
mv -f build/openindiana.iso.download build/openindiana.iso
{
  echo "OPENINDIANA_ISO_URL=\$ISO_URL"
  echo "OPENINDIANA_ISO_SHA256=\$EXPECTED_SHA256"
  echo "OPENINDIANA_ISO_VERIFICATION=PASS"
  echo "BUILDER_COMMIT=$BUILDER_COMMIT"
  echo "POSTBUILD_ORIGINAL_GIT_BLOB=$ORIGINAL_POSTBUILD_BLOB"
  echo "POSTBUILD_MODIFIED_SHA256=$MODIFIED_POSTBUILD_SHA"
  echo "PKG_UPDATE=SKIPPED"
} > build/run106-iso-provenance.txt
EOF
chmod 0755 "$BUILDER/hooks/host_beforeBuild.sh"

cp "$REPO_ROOT/run106-support/install-tools.sh" "$BUILDER/run106-install-tools.sh"
cp "$REPO_ROOT/run106-support/native-build.sh" "$BUILDER/run106-native-build.sh"
chmod 0755 "$BUILDER/run106-install-tools.sh" "$BUILDER/run106-native-build.sh"

STAGING="$BUILDER/run106-inputs-staging"
rm -rf "$STAGING"
mkdir -p "$STAGING"
cp -a "$RUNNER_INPUTS" "$STAGING/r5-runner-inputs"
cp -a "$REPO_ROOT/wifi-ax101" "$STAGING/wifi-ax101"
cat > "$STAGING/BUILD-IDENTITY.txt" <<EOF
source_repository=Cadiocto/WhateverXe-MiniBook-X-N150
source_branch=ax101-r5-integration-validation
source_commit=$SOURCE_COMMIT
run_id=$RUN_ID
runner_host_kernel=$RUNNER_KERNEL
openindiana_builder_commit=$BUILDER_COMMIT
openindiana_iso_url=$ISO_URL
openindiana_iso_sha256=$ISO_SHA256
target_kernel=illumos-c893fd03ad
solnix_commit=1cee7021091f3df92b7816ece2902ff59b37f1be
illumos_gate_commit=098e95820891f8a83fae679daece7fcf58b7b3a4
pci_vendor_device=8086:54F0
pci_subsystem=8086:0244
firmware_api=77
firmware_filename=iwx-so-a0-hr-b0-77
EOF
tar -czf "$BUILDER/run106-inputs.tar.gz" -C "$STAGING" .
sha256sum "$BUILDER/run106-inputs.tar.gz" | awk '{print $1}' > "$BUILDER/run106-inputs.tar.gz.sha256"
rm -rf "$STAGING"

cat > "$BUILDER/conf/run106.conf" <<EOF
VM_ISO_LINK="$ISO_URL"
VM_OS_NAME="openindiana"
VM_SSH_PORT=2222
VM_RELEASE=202610-build
VM_LOGIN_TAG="console login:"
VM_SHUTDOWN_CMD="shutdown -y -i5 -g0"
VM_INSTALL_CMD="pkg install"
VM_DISK="sata"
VM_ROOT_PASSWORD=anyvm.org
VM_RSYNC_PKG=""
VM_SSHFS_PKG="sshfs"
VM_PRE_INSTALL_PKGS="tree sshfs developer/gcc-14 developer/build/gnu-make developer/versioning/git"
VM_INSTALL_SCRIPT="$BUILDER/run106-install-tools.sh"
VM_EXTRA_SCRIPT="$BUILDER/run106-native-build.sh"
VM_OPTS="conf/openindiana-202610.opts.txt"
VM_SYNC_METHODS="rsync,scp,sshfs,nfs,tar"
EOF

{
    echo "BUILDER_COMMIT=$BUILDER_COMMIT"
    echo "POSTBUILD_ORIGINAL_GIT_BLOB=$ORIGINAL_POSTBUILD_BLOB"
    echo "POSTBUILD_MODIFIED_SHA256=$MODIFIED_POSTBUILD_SHA"
    echo "ISO_URL=$ISO_URL"
    echo "ISO_SHA256=$ISO_SHA256"
    echo "SOURCE_COMMIT=$SOURCE_COMMIT"
    echo "RUN_ID=$RUN_ID"
    echo "RUNNER_HOST_KERNEL=$RUNNER_KERNEL"
    echo "TARGET_KERNEL=illumos-c893fd03ad"
} > "$ARTIFACTS/builder-preparation.txt"

test -s "$BUILDER/conf/run106.conf"
test -s "$BUILDER/run106-inputs.tar.gz"
test -s "$BUILDER/run106-inputs.tar.gz.sha256"
test -x "$BUILDER/hooks/host_beforeBuild.sh"
echo "RUN106_BUILDER_PREPARATION=PASS"
echo "RUN106_ISO_SHA256=$ISO_SHA256"
echo "RUN106_BUILDER_COMMIT=$BUILDER_COMMIT"
