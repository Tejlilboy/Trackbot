# Trackbot (Pip)

> ⚠️ **FIRST DRAFT, WORK IN PROGRESS.** This is the first draft of the firmware and it has **never run on real hardware**. Expect bugs, and expect **a LOT of changes**: pins, tuning values, behaviors and even the file layout will move around as Pip gets built and tested.

Arduino sketch in `Trackbot/`. Open `Trackbot/Trackbot.ino` in Arduino IDE.

## Board settings
- Board: **XIAO_ESP32S3** (esp32 by Espressif, core 3.x)
- USB CDC On Boot: **Enabled** (Serial over USB; D6/D7 are used for motors)
- Partition scheme: one with OTA (default 8MB works)

## Libraries (Library Manager)
Adafruit SSD1306, Adafruit GFX, Adafruit NeoPixel, ESP32Servo,
SparkFun VL53L5CX Arduino Library, ESP Async WebServer, Async TCP

## First steps
1. Copy `Trackbot/secrets.example.h` to `Trackbot/secrets.h` and set your WiFi name/password (`secrets.h` is git-ignored). Set `USE_*` switches to 0 for parts not wired yet.
2. Upload, open Serial Monitor at 115200. Type `p` (pet), `s` (shake), `h` (hand), `w` (wander), `c` (sneeze), `o` (zoomies), `1` (1-min focus timer), `m` (print moods).
3. Phone: open `http://pip.local` (or the IP printed on Serial). DEBUG tab fires events too.
4. Tune values marked TUNE in `config.h` once hardware is assembled.

## Status
Compiles against stubbed headers and runs in a desktop simulation; **not yet run on real hardware**.
More features and fixes are coming; see the TODO notes in the code.

## License
[MIT](LICENSE): free to use, copy, modify and share, for anything. R2-D2 sound samples are not included and not covered.
