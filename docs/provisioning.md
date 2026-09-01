# Provisioning register

Firmware: this fork at `afcd243` (master), full chip erase before each flash.

| Board | MAC / USB serial | Image | LED config | Status |
|---|---|---|---|---|
| ESP32-S3-USB-Bridge #1 | `F412FA5965FC` | `rel_s3usb` | -1/-1/-1, RGB 42 | done |
| ESP32-S3-USB-Bridge #2 | `F412FA5980B4` | `rel_s3usb` | -1/-1/-1, RGB 42 | done |
| ESP32-S3-USB-Bridge #3 | `F412FA5980C4` | `rel_s3usb` | -1/-1/-1, RGB 42 | done |
| ESP-Prog-2 #A | `94A990398BA8` | `rel_prog2` | 48/48/34 | done |
| ESP-Prog-2 #B | `94A990397E80` | `rel_prog2` | 48/48/34 | done (was esp-tether) |
| ESP-Prog-2 #C | `94A990397D4C` | `rel_prog2` | 48/48/34 | done |
| ESP-Prog-2 #D | `94A990398780` | `rel_prog2` | 48/48/34 | done |

ESProg v6.2b x3 run esp-tether, not this firmware - classic ESP32, no native USB.

## Notes

- Stock devkit firmware varies: bridges #1 and #3 needed a BOOT-hold, #2 accepted the
  magic baud. Try the magic baud first.
- A Prog-2 running esp-tether is reset into download mode by esptool on its own, over
  the native USB-Serial-JTAG console - no button, no magic baud.
- Erase times: ~24 s on the S3-USB-Bridge (4 MB), ~14 s on the ESP-Prog-2.
- After flashing from ROM download mode the S3 may stay in the loader; a plain
  unplug/replug starts the app.
- Each USB identity (`303A:1002` app, `303A:1001` ROM) needs its own VirtualHere Auto-Use.
- If `SERIALCOMM` is empty and no COM port appears, clear the greyed-out entries under
  Device Manager > Ports and replug. They accumulate with every board swap.
