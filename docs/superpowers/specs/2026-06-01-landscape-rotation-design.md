# Landscape Rotation Reflow — Design

**Date:** 2026-06-01
**Branch:** `rotation`
**Status:** Approved

## Goal

Convert the CYD weather display from its current **portrait** layout (240×320)
to a **landscape** layout (320×240), reorganised to match the supplied mockup
(`weather-mockup.html` / `after.png`). This is a reflow, not a rotation: the
information is rearranged, not just turned 90°.

Reuse all existing bitmaps, fonts, and colours. Do not recreate or resize icons
— only change where they are drawn.

## Orientation findings (current code)

- Rotation is set once: `tft.setRotation(0)` at `src/main.cpp:162` → portrait
  240×320. The trailing `// For 320x480 screen` comment is stale.
- **Bitmap anchor is top-left:** `ui.drawBmp(path, x, y)` (`src/GfxUi.cpp:43`).
- **Text uses per-call datums** via `tft.setTextDatum()` — `BC` (bottom-centre),
  `BR`, `TR`, `TC`, `TL`. A text Y is therefore sometimes the *bottom* of the
  text, not the top. These datums stay hard-coded inline (see Decisions).
- `drawSeparator()` draws `tft.drawFastHLine(10, y, 240 - 2*10, …)` — the `240`
  is a hard-coded width that must become `320`.
- Layout values are inline magic numbers today; no `#define`s or layout struct.
- The big current temperature is drawn separately in `updateData()`
  (`src/main.cpp:323-332`) with `AA_FONT_LARGE`, not in `drawCurrentWeather()`.

### Verified bitmap sizes (fixed — do NOT resize)

| Folder | Use | Size |
|---|---|---|
| `/icon/` | current condition | 100×100 |
| `/icon50/` | forecast day | 50×50 |
| `/wind/` | wind compass | 50×50 |
| `/moon/` | moon phase | 60×60 |

These real sizes are larger than the mockup's idealised SVGs (86×62, 46×32,
34×34). Every band is therefore tighter than the mockup — see Tight Spots.

### Font reality

Only two smooth `.vlw` fonts exist: `AA_FONT_SMALL` (NSBold15, ~15px) and
`AA_FONT_LARGE` (NSBold36, ~36px). The mockup implies ~7 sizes; we map
everything to these two — `LARGE` for the clock and current temperature (as
today), `SMALL` for everything else. No new font asset will be added, so the
visual hierarchy is flatter than the mockup. In particular the `Updated:` line
will render larger than the mockup's 9px and the header will be tighter.

## Decisions

- **Location:** add a configurable `const String location` to `All_Settings.h`
  (and `.Example.h`), e.g. `"Fernie, BC"`. The API's `forecast->city_name`
  returns only the bare city ("Fernie") with no province, so a setting gives
  exact control and matches the mockup.
- **Coordinate style:** introduce a block of named layout constants for
  **positions only** (x/y, band heights, divider Ys, column centres). Text
  datums and bitmap anchors stay hard-coded inline in each draw call — alignment
  is fixed once the layout is right; only positions get tweaked.
- **Boot screens:** the splash, "Connecting to WiFi", and progress bar are
  re-centred for 320×240 as part of this work so nothing clips at startup.
- **Build target while iterating:** compile only the `ST7789` env. The two envs
  differ only in the TFT_eSPI setup shim, so this halves build time. A final
  `ILI9341` build check is done by the user at the end.

## Approach

In-place reflow plus a layout-constants header. Keep the existing function
structure (`drawTime`, `drawCurrentWeather`, `drawForecast` /
`drawForecastDetail`, `drawAstronomy`, `drawProgress`, the `setup()` splash).
Change rotation, replace portrait coordinates with named landscape position
constants, add the location and sun-arrow drawing, and widen the separators.
No unrelated refactoring.

## Global changes

- `tft.setRotation(0)` → `tft.setRotation(1)` (landscape 320×240). Rotation `1`
  vs `3` only flips which edge the USB sits on — a trivial later change if it
  mounts upside-down. Default `1`.
- `drawSeparator()` width `240` → `320` (full-width dividers).
- Remove/replace the stale `// For 320x480 screen` comment.

## Layout (320×240, top-left origin)

Position constants only; datums/anchors noted for reference but stay inline.
Y values are starting points to be tuned on hardware.

| Element | Datum/anchor (inline) | x, y | Font |
|---|---|---|---|
| Clock `12:50` | TL | 6, 0 | LARGE |
| Location (`location`) | TR | 313, 2 | SMALL |
| `Updated: …` | TR | 313, 22 | SMALL |
| Header divider | — | y=36, x 7→313 | — |
| Current icon (100×100) | TL bitmap | 4, 38 | — |
| Condition label (`Rain`) | TL | 112, 44 | SMALL |
| Big temp + `oC` unit | TL | 110, 58 | LARGE |
| Column divider | — | x=160, y 40→188 | — |
| Wind compass (50×50) | TL bitmap | 18, 138 | — |
| `1 m/s` / `1023 hPa` | stacked | ~78, centre ~163 | SMALL |
| Forecast grid column centres | — | x=200 (L), x=276 (R) | — |
| Row 1 TUE / WED | day BC | day 50 / temps 66 / icon-top 72 | SMALL |
| Row 2 THU / FRI | day BC | day 122 / temps 138 / icon-top 144 | SMALL |
| Bottom divider | — | y=190 | — |
| Sun block | centre x≈44 | hdr 200 / rise 216 / set 231 | SMALL |
| Moon (60×60) | centre x≈126 | disc-top ~178 / `Full` ~236 | SMALL |
| Cloud | centre x≈205 | label 202 / `100%` 220 | SMALL |
| Humidity | centre x≈281 | label 202 / `95%` 220 | SMALL |

Forecast reading order is left-to-right then top-to-bottom (TUE, WED, THU, FRI).
Per cell: day label, then `high low` (high white, low cyan), then condition
icon, vertically stacked and centred on the column.

## New content

- **Location text:** new `location` setting drawn top-right (TR datum). Add to
  both `All_Settings.h` and `All_Settings.Example.h`.
- **Sun arrows:** a small helper `drawArrow(x, yCentre, bool up, colour)` using
  `tft.drawLine` primitives — a short vertical stroke plus two angled head
  strokes, ~7–8px tall, vertically centred on the time text, same colour as the
  time. Up arrow before sunrise, down arrow before sunset. No glyph or bitmap.

## Known tight spots (real bitmaps larger than mockup SVGs)

Each is flagged in the diff and tuned on hardware. Strategy: match the mockup's
*arrangement and alignment*; accept that the real bitmaps are more prominent;
prioritise no off-screen clipping, allow minor overlap into gaps, nudge Ys.

1. **Left column vertically full** — 100×100 icon + 50×50 compass nearly fill
   the 40→190 column (150px) with no gap. Nudge icon up / compass down.
2. **Condition label width** — only ~50px sits right of the 100px icon. `Rain`
   fits; long names (`Thunderstorm`) clip. Keep the existing 2-line `splitIndex`
   stacking to mitigate.
3. **Forecast row-2 icon** (50px) bottoms out ~y194, just past the 190 divider.
   Compress row pitch / nudge up to clear it.
4. **Moon (60px)** is bigger than its ~50px band. Nudge the disc up and keep the
   `Full` label from clipping at the bottom edge.

## Boot / splash reflow

Re-centre `setup()`'s splash JPG, the "Connecting to WiFi" / "Fetching weather
data…" text, and `drawProgress()`'s text + progress bar for 320×240 (x-centre
160, Ys within 240) so nothing is clipped at startup.

## Testing

- Compile the `ST7789` env via PlatformIO to confirm it builds while iterating.
- Final `ILI9341` build check performed by the user.
- **No visual verification without hardware.** All pixel-level alignment,
  clipping, and the four tight spots above require a real CYD. The
  implementation notes will list exactly what could not be verified.

## Out of scope

- No new bitmaps, fonts, or colours.
- No unrelated refactoring (e.g. no layout-struct extraction beyond the position
  constants block).
- Touch input, partition tables, and timezone logic are untouched.
