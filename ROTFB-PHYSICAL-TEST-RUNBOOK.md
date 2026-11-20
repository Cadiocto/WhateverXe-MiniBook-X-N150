# ROTFB Physical Device Test Runbook

**Status: PREPARATION ONLY — DO NOT EXECUTE UNTIL THE RELEASE STATE IS `READY FOR PHYSICAL DEVICE TEST`.**

Target: OpenIndiana 2026.10 x86_64 on Chuwi MiniBook X N150  
GPU: Intel N150, PCI ID `8086:46D4`  
ROTFB source: `8e2a6e8f246c43c151793046934bcb848371768d`  
Pinned illumos upstream: `098e95820891f8a83fae679daece7fcf58b7b3a4`

This procedure is for the device owner. CI agents must not run it against the physical laptop.

## 1. Safety rules

1. Keep the existing working boot environment (BE) intact. Do not install ROTFB into it.
2. Use only the kit's `INSTALL.sh` after the kit has passed all release gates, including the target-equivalent offline IPS dependency-closure gate.
3. The installer must verify `manifests/SHA256SUMS` and the local IPS repository, run solver dry-runs against the current image and the mounted test BE, create a separate BE with `beadm create`, and use `pkg -R` to install into that inactive BE. It must not call `beadm activate`.
4. The new test BE may appear in the boot menu, but it must not become the default next-boot BE. Do not reboot if the original BE is no longer the default.
5. Do not change BIOS/UEFI settings, firmware, EFI variables, NVRAM variables, bootloader configuration files, or the active BE manually.
6. Do not run `bootadm update-archive` until the test BE has actually been booted. Run it only while booted into that test BE.
7. If the target release, architecture, pool health, repository verification, checksum verification, or solver dry-run does not match expectations, stop and preserve the logs.

## 2. Capture the untouched target baseline

Boot the existing working OpenIndiana BE. Connect AC power and ensure the machine is stable. Before extracting or installing the kit, capture the current state:

```sh
umask 077
EVIDENCE_DIR="/var/tmp/rotfb-pretest-$(date +%Y%m%d-%H%M%S)"
mkdir -p "$EVIDENCE_DIR"

cat /etc/release > "$EVIDENCE_DIR/release.txt"
uname -a > "$EVIDENCE_DIR/uname.txt"
beadm list > "$EVIDENCE_DIR/beadm-list-before.txt"
zpool status > "$EVIDENCE_DIR/zpool-status-before.txt"
zfs list > "$EVIDENCE_DIR/zfs-list-before.txt"
isainfo -kv > "$EVIDENCE_DIR/isainfo-before.txt"
prtconf | head -80 > "$EVIDENCE_DIR/prtconf-head-before.txt"
pkg list > "$EVIDENCE_DIR/installed-fmris-before.txt"
pkg publisher > "$EVIDENCE_DIR/publishers-before.txt"
pkg variant > "$EVIDENCE_DIR/variants-before.txt"
pkg facet > "$EVIDENCE_DIR/facets-before.txt"
pkg list | grep -i incorporation > "$EVIDENCE_DIR/incorporations-before.txt" || true
dmesg > "$EVIDENCE_DIR/dmesg-before.txt"
modinfo > "$EVIDENCE_DIR/modinfo-before.txt"

echo "$EVIDENCE_DIR"
```

Confirm all of the following before proceeding:

- `/etc/release` identifies OpenIndiana 2026.10.
- `uname -a` and `isainfo -kv` identify the expected x86_64/AMD64 environment.
- `beadm list` clearly identifies the current working BE.
- `zpool status` reports healthy pools; do not proceed with a degraded or faulted pool.
- There is enough free space for a cloned BE and the package transaction.
- The captured FMRI inventory, publishers, incorporations, variants, and facets are retained. They are also the target-side inputs needed for the offline dependency-closure evidence.

Do not assume that a generic OpenIndiana VM is an equivalent baseline for this laptop. The release, architecture, installed FMRIs, publishers, incorporations, variants, and facets must match the target.

## 3. Verify the release kit before installation

Proceed only when the release record explicitly says:

- `INSTALL KIT READY`; and
- `READY FOR PHYSICAL DEVICE TEST`.

Extract the ZIP into a new directory under `/var/tmp`. Do not extract over an existing kit or into system directories. Keep the complete directory structure intact.

Check that the kit contains at least:

- `packages/nightly-nd/repo.redist/` with `catalog/`, `pkg/`, and `file/`;
- `kernel/` with the validated i86pc kernel payload;
- `boot/`;
- `manifests/SHA256SUMS`;
- `manifests/package-inventory.txt`;
- `manifests/package-dependencies.txt`;
- `manifests/source-revision.txt`;
- `manifests/build-evidence.txt`;
- `manifests/rotfb-package-fmris.txt`;
- `INSTALL.sh` and `ROLLBACK-README.txt`.

Read `INSTALL-README.txt`, `ROLLBACK-README.txt`, and `manifests/build-evidence.txt`. The evidence must say `OFFLINE_DEPENDENCY_CLOSURE=PASS`; `DIRECT_BUILD_REPO_DEPENDENCIES=RESOLVED` is not a substitute.

For a genuine offline test, disconnect the laptop from wired and wireless networks before running the installer. Do not disable or rewrite configured publishers; the installer uses `--no-refresh` and the local repository. If the dry-run cannot resolve using the installed baseline and local kit, stop. Do not reconnect to a publisher to make the transaction pass.

## 4. Run the installer into a separate inactive BE

From the extracted kit directory, run as root:

```sh
./INSTALL.sh
```

The installer is expected to:

1. verify the checksum manifest and each listed file;
2. verify the local IPS repository and its package inventory;
3. confirm the ROTFB package FMRI list is present in the repository;
4. run `pkg install -n` against the current image, then run a second dry-run against the mounted test BE;
5. create a uniquely named BE with `beadm create`, mount it under `/mnt`, and use `pkg -R` with `--no-refresh` and the local repository to install into that inactive BE;
6. unmount the test BE, leave it inactive, and confirm it appears in `beadm list`;
7. if installation fails after BE creation, leave the inactive BE for manual inspection rather than activating or automatically destroying it.

The installer must not run `beadm activate`. It must not install into the current BE. It must not alter BIOS/UEFI firmware or NVRAM.

If the installer exits nonzero after creating the test BE, do not boot that BE. Preserve its name and installer output for manual inspection; the installer intentionally leaves an incomplete inactive BE rather than destroying it automatically.

After it finishes successfully, capture:

```sh
beadm list > "$EVIDENCE_DIR/beadm-list-after-install.txt"
zpool status > "$EVIDENCE_DIR/zpool-status-after-install.txt"
zfs list > "$EVIDENCE_DIR/zfs-list-after-install.txt"
pkg history > "$EVIDENCE_DIR/pkg-history-after-install.txt"
```

Identify the newly created `rotfb-test-*` BE. Confirm that the original working BE is still present and remains the default next-boot selection. The test BE must not be marked active for the next boot. If the default selection changed, do not reboot; record the evidence and restore the original selection only using the documented rollback procedure.

## 5. Boot and validate the test BE

Only after confirming that the original BE remains the default, use the machine's normal boot menu to select the named test BE for this boot. Do not edit EFI variables, NVRAM, firmware settings, or bootloader configuration files to make it appear.

Once the test BE is running, first capture:

```sh
umask 077
TEST_EVIDENCE="/var/tmp/rotfb-device-test-$(date +%Y%m%d-%H%M%S)"
mkdir -p "$TEST_EVIDENCE"

cat /etc/release > "$TEST_EVIDENCE/release.txt"
uname -a > "$TEST_EVIDENCE/uname.txt"
beadm list > "$TEST_EVIDENCE/beadm-list.txt"
zpool status > "$TEST_EVIDENCE/zpool-status.txt"
zfs list > "$TEST_EVIDENCE/zfs-list.txt"
isainfo -kv > "$TEST_EVIDENCE/isainfo.txt"
prtconf | head -80 > "$TEST_EVIDENCE/prtconf-head.txt"
dmesg > "$TEST_EVIDENCE/dmesg.txt"
modinfo > "$TEST_EVIDENCE/modinfo.txt"
pkg list > "$TEST_EVIDENCE/installed-fmris.txt"
svcs -xv > "$TEST_EVIDENCE/svcs-xv.txt" 2>&1 || true
```

Now, while booted into the test BE, update that BE's own boot archive:

```sh
bootadm update-archive
```

Capture the command output and exit status. Do not run this command from the original BE against the test BE's filesystems.

### Display acceptance checks

The expected ROTFB mapping is:

| Property | Expected value |
|---|---:|
| Logical desktop geometry | 1920 × 1200 |
| Physical panel geometry | 1200 × 1920 |
| Physical framebuffer format | BGRA32 |
| Physical pitch | 4800 bytes |
| Logical pitch | 7680 bytes |
| Rotation | 90° clockwise |

Verify and record:

- The desktop is upright and correctly oriented on the physically portrait panel.
- The complete image is visible: no clipping, stretching, mirroring, or unexpected scaling.
- Text, cursor, window borders, and screen edges are consistently oriented.
- The display remains stable after login, ordinary window movement, repainting, and a period of idle use.
- No new framebuffer, kernel, or service errors appear in `dmesg` or `svcs -xv`.
- The reported dimensions, pixel format, and pitch are supported by actual target-side evidence. Do not infer them from CI logs alone.

Save photographs or screenshots that clearly show the whole panel and its orientation. Record the test BE name and the exact time of each observation.

### Reboot stability

Perform at least two owner-controlled reboot checks:

1. Reboot and manually select the test BE again from the normal boot menu.
2. Confirm the same display geometry and orientation after the second boot.
3. Confirm the original BE remains available and remains the default next-boot selection unless the owner deliberately chooses otherwise.

If the test BE fails to boot, the display becomes unusable, or the system reports kernel or pool errors, stop testing and roll back.

## 6. Rollback

The original BE must remain intact throughout the test.

- If the test BE does not work, reboot and select the original working BE from the normal boot menu. Because the installer uses `--no-be-activate`, the original BE should remain the default.
- Once booted into the original BE, capture `beadm list`, `zpool status`, `zfs list`, `uname -a`, and `dmesg` again.
- If the default next-boot selection was unexpectedly changed, use `beadm activate <ORIGINAL_BE_NAME>` from the working BE to restore the original default, then verify with `beadm list`. This is a deliberate owner-performed rollback action, not part of the installer.
- Do not destroy the test BE until all logs and evidence have been copied off the machine and the owner has decided it is no longer needed. Never destroy the original BE.

## 7. Evidence package and acceptance decision

Preserve the pre-test and test evidence directories, the installer output, the IPS dry-run plan, the package history, screenshots/photos, and the rollback result. Copy them off the laptop before deleting any test data.

The owner should report:

- exact test BE name;
- whether the original BE remained the default;
- installer, checksum, repository, and solver results;
- boot archive update result;
- observed panel orientation and dimensions;
- evidence for BGRA32 and pitch;
- first-boot and second-boot results;
- any kernel, framebuffer, or service errors;
- rollback result.

Only owner-provided target-device logs and observations can advance the project to `DEVICE VERIFIED`. A successful CI build, repository check, ZIP upload, or VM test cannot establish physical display correctness.
