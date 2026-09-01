# Cabling

Pin-by-pin wiring between the bridge host boards and the IRMACOS target boards.
Open the HTML files in a browser.

- `host-target-cabling.html` — all three hosts (ESP32-S3-USB-Bridge, MuseLab
  ESPLink v1.2, ESP-Prog-2) against all three targets (AC01, RS01, RS02).
  Signal-per-row tables: pick a host column and a target column.
- `esp32-s3-usb-bridge-to-ac01.html` — worked example for that pair, with the
  power budget and pre-flight meter checks.

Every header was read from the board's rendered schematic sheet. Header pitch
and gender are not documented on the schematics; meter the power pins before
connecting.
