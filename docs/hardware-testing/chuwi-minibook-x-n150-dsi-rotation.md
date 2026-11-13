# CHUWI MiniBook X N150 — MIPI DSI rotation: hardware test passed

**Result: the internal display orientation is working after a macOS reboot.**

On 2026-11-13, the CHUWI MiniBook X N150 was tested on real hardware with macOS Monterey 12.7.6 (21H1320), using the `DSIRotate` helper and a per-user LaunchAgent.

## Verified hardware configuration

- Internal panel: MIPI DSI, 1920×1200, 50 Hz
- Display service: `AppleIntelFramebuffer@0`
- CoreGraphics display ID observed during diagnostics: `0x2e354400`
- Requested transform: `80` (`kIOScaleRotate270`)
- IOKit probe options observed: `1025` (`0x401`), including `kIOFBSetTransform`

## Test sequence and results

| Test | Observed result |
| --- | --- |
| Identify the built-in display and its IOKit service path | Found `IGPU@2/AppleIntelFramebuffer@0` |
| Inspect display transform before applying the fix | Transform was initially `0` |
| Apply 90° transform (`48`) | Image was upside down; rejected |
| Apply 270° transform (`80`) | Correct physical orientation relative to the keyboard; display artifacts disappeared |
| Re-run the helper while transform is already correct | `DSI already has desired transform: 80`; no unnecessary change |
| Install the per-user LaunchAgent | `launchctl bootstrap` succeeded; job was registered |
| Reboot macOS and inspect the display | Display came up in the correct orientation |
| Verify transform after reboot | `IOFBTransform = 80` |
| Verify helper log after reboot | `DSI already has desired transform: 80` |
| Verify LaunchAgent exit status | `last exit code = 0`; `runs = 1` |
| Correlate log timestamp with login | Login: 2026-11-13 15:23:04; log write: 2026-11-13 15:23:11 |

The LaunchAgent showing `state = not running` after the check is expected: this is a one-shot helper, not a resident background process. The successful exit code and log confirm that it ran and exited normally.

## Installed components

- Helper executable: `/usr/local/libexec/wex-dsi-rotate`
- LaunchAgent: `~/Library/LaunchAgents/org.whateverxe.chuwi-dsi-rotation.plist`
- Log: `/tmp/wex-dsi-rotation.log`
- Source: [DSIRotate.c](../../tools/dsi-rotation/DSIRotate.c)
- LaunchAgent template: [org.whateverxe.chuwi-dsi-rotation.plist](../../tools/dsi-rotation/org.whateverxe.chuwi-dsi-rotation.plist)

The helper is local and does not require an internet connection at runtime. Internet was used only to download the plist during setup.

## Reproduction / verification commands

```sh
cat /tmp/wex-dsi-rotation.log

ioreg -lw0 -p IOService -r -n AppleIntelFramebuffer@0 | grep IOFBTransform

launchctl print "gui/$(id -u)/org.whateverxe.chuwi-dsi-rotation" \
  | grep -E 'state =|runs =|last exit code'

stat -f "Last log write: %Sm" /tmp/wex-dsi-rotation.log
```

Expected key values after login:

- `IOFBTransform = 80`
- Log: `DSI already has desired transform: 80`
- `last exit code = 0`

## Conclusion

**The CHUWI MiniBook X N150 internal MIPI DSI panel orientation issue is resolved for the tested macOS installation.** The 270° IOKit transform gives the correct orientation, and the per-user LaunchAgent reapplies/checks it automatically at login. The result was confirmed after a real reboot on the target laptop.

This report documents the tested machine and OS configuration; it does not claim that every CHUWI N150 unit or every macOS version has been tested.
