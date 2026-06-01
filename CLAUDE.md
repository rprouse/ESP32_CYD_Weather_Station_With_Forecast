# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project overview

ESP32 firmware for the "Cheap Yellow Display" (CYD, board variant `ESP32-2432S028R`) — a 320x240 TFT with an ESP32 WROOM. The sketch fetches data from OpenWeatherMap and renders current conditions plus a 4-day forecast. Originally written by Daniel Eichhorn / adapted by Bodmer for the Arduino IDE, then ported to PlatformIO in this repo.

Two hardware variants exist and are exposed as separate PlatformIO environments — the older ILI9341 panel (single USB connector) and the newer ST7789 panel (dual USB connectors). They differ only in the TFT_eSPI `Setup_*.h` shim force-included via `build_flags`.

## Build / flash / monitor

PlatformIO Core CLI is **not on PATH**. Either open *PlatformIO: New Terminal* in VSCode (activates the bundled venv), or call directly:

```
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" <args>
```

Common tasks (substitute env as needed; default is `ESP32_2432S028R_ILI9341` per `platformio.ini`):

```
pio run -e ESP32_2432S028R_ILI9341                  # build
pio run -e ESP32_2432S028R_ILI9341 -t upload        # flash firmware
pio run -e ESP32_2432S028R_ILI9341 -t uploadfs      # flash LittleFS image from ./data
pio run -e ESP32_2432S028R_ILI9341 -t erase         # erase flash (use after Arduino-IDE → PIO migration to clear stale SPIFFS layout)
pio device monitor -b 115200
```

In the VSCode UI, filesystem upload lives under *PlatformIO → Project Tasks → \<env\> → Platform → Upload Filesystem Image*. There is no global "Upload Filesystem Image" because there are multiple envs — you must pick one.

No test suite exists; this is firmware that has to be exercised on real hardware.

## Required first-time setup

1. Copy `src/All_Settings.Example.h` → `src/All_Settings.h` and fill in WiFi credentials, OpenWeatherMap API key, lat/lon, and `TIMEZONE` (one of the `Timezone` objects defined in `src/NTP_Time.h` — `UK`, `euCET`, `usET`, `usCT`, `usMT`, `usPT`, etc.). `src/All_Settings.h` is gitignored so the API key never ships.
2. Run **Upload Filesystem Image** before the first firmware upload — the weather icons and `.vlw` fonts under `./data/` are loaded at runtime from LittleFS. Without them the sketch will fail at `LittleFS.begin()` or render blank icons.

## Architecture notes

### Project layout quirk

The LittleFS source folder is `./data/` at the **project root** (PlatformIO convention) — *not* `src/data/` (Arduino IDE convention). Don't move it back. If you ever want to keep it elsewhere, set `data_dir =` under `[platformio]` in `platformio.ini`.

### Display variant selection

`platformio.ini` uses `-include src/Setup_ESP32_2432S028R_<panel>.h` in `build_flags` to force-include a TFT_eSPI configuration header *before* every translation unit. This replaces the Arduino-IDE workflow of editing `TFT_eSPI/User_Setup_Select.h`. The two `Setup_*.h` files in `src/` are the pin/driver/rotation config for each panel — don't rename them without updating `platformio.ini`. `src/User_Setup.h` is a relic and not actually included.

### Time handling is split across three layers

This bites every non-trivial change to the forecast logic:

- **NTP** (`src/NTP_Time.h`) sets the device's UTC epoch via `setSyncProvider`. `now()` returns UTC seconds.
- **Timezone library** (JChristensen/Timezone) converts UTC → local via `TIMEZONE.toLocal(epoch, &tz1_Code)`. The `TIMEZONE` macro and the `tz1_Code` pointer are set up by `All_Settings.h` + `NTP_Time.h`.
- **OpenWeather payload** (`OW_forecast` in `src/OpenWeather.h`) exposes both `dt[i]` (Unix epoch, timezone-neutral, **use this for all calendar math**) and `dt_txt[i]` (UTC-formatted string, **display-only**). Mixing the two is the source of off-by-one-day bugs — see `getNextDayIndex` in `src/main.cpp` for the canonical pattern (convert epoch to local, *then* extract `day()`).

### Forecast slot indexing

OpenWeatherMap's 5-day/3-hour endpoint returns 40 three-hour slots. The four forecast columns work by:
1. `getNextDayIndex()` returns the index of the first slot belonging to *tomorrow* in local time.
2. `drawForecastDetail` is called with that index, then `+8`, `+16`, `+24` (one full day = 8 slots).
3. Within each day, it samples slot `dayIndex + 4` (~midday) for the icon and label, and aggregates min/max temperature across all 8 slots of the day.

If the forecast is off by a day, suspect timezone math first.

### LittleFS contents

Everything under `./data/` is mounted as the root of LittleFS at runtime:
- `data/fonts/NSBold15.vlw`, `NSBold36.vlw` — TFT_eSPI smooth fonts (referenced by `AA_FONT_SMALL` / `AA_FONT_LARGE` macros in `main.cpp`).
- `data/icon/*.jpg` — Meteocon weather icons keyed by OpenWeatherMap condition ID via `getMeteoconIcon()` in `main.cpp`. The `id += 1000` trick swaps day/night variants for the "clouds" family (id/100 == 8) based on whether `now()` is between sunrise and sunset.

The README warns to **set the LittleFS partition to ≥ 1.5 MB**. PlatformIO uses `board_build.filesystem = littlefs` with the board's default partition table; if uploadfs fails with "image too large," choose a custom partition scheme in `platformio.ini` (e.g. `board_build.partitions = huge_app.csv` or a custom CSV).

### Library lock-ins

`lib_deps` in `platformio.ini` pins TFT_eSPI to `^2.5.43` and pulls the others straight from Bodmer's GitHub master. The OpenWeather and JSON_Decoder libraries don't have tagged versions — `pio pkg update` will pull whatever is on master. Be aware that breaking changes in OpenWeather's `OW_forecast` struct have happened historically.

## Tooling gotchas

- IntelliSense (`clang` via the C/C++ extension) may flag `FS.h not found` and similar — these resolve correctly during the actual PlatformIO build because the ESP32 Arduino core's include paths are injected then, not at editor parse time. Running `PlatformIO: Rebuild IntelliSense Index` after `pio run` once usually clears the squigglies.
- The `Corrupted dir pair at {0x1, 0x0}` LittleFS error at boot almost always means stale flash content from a previous SPIFFS layout (e.g. Arduino-IDE era). Fix with `pio run -t erase` then re-upload firmware + filesystem.
