# Landscape Rotation Reflow Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Reflow the CYD weather display from portrait (240×320) to landscape (320×240) matching the approved mockup, reusing all existing bitmaps/fonts/colours.

**Architecture:** In-place reflow of `src/main.cpp`. A new block of named position constants drives every coordinate; text datums and bitmap anchors stay hard-coded inline. Two new pieces of drawing logic are added: a configurable location string (top-right) and a line-primitive sun-arrow helper. Boot/splash/progress screens are re-centred for 320×240.

**Tech Stack:** ESP32 Arduino, TFT_eSPI (smooth `.vlw` fonts), PlatformIO. No unit-test harness exists — the per-task gate is a successful `ST7789` build; pixel-level visual verification is done by the user on hardware.

**Testing note:** Every task ends by building the `ST7789` env only (the two envs differ solely in the TFT setup shim). The `ILI9341` build is checked once by the user at the very end (Task 9). The build command is:

```
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -e ESP32_2432S028R_ST7789
```

Expected on success: `========================= [SUCCESS] =========================`

---

## File structure

- **Modify** `src/main.cpp` — all rotation + layout changes, the new constants block, `drawArrow` helper, and reflowed draw functions.
- **Modify** `src/All_Settings.Example.h` — add the `location` setting (committed template).
- **Modify** `src/All_Settings.h` — add the `location` setting (gitignored local copy; needed so the build compiles and the device shows a location).

No new files. No new assets.

---

### Task 1: Landscape orientation, full-width separators, and layout-constants block

**Files:**
- Modify: `src/main.cpp` (rotation `162`, `drawSeparator` `625-627`, new constants block after `119`)

- [ ] **Step 1: Add the layout-constants block**

Insert this block immediately **after** line `long lastDownloadUpdate = millis();` (currently `src/main.cpp:119`) and before the `Declare prototypes` section:

```cpp
/***************************************************************************************
**                          Landscape layout constants (320x240)
**  Positions only. Text datums and bitmap anchors are hard-coded inline at each
**  draw call. Y values are starting points and may be nudged when viewed on hardware.
***************************************************************************************/
#define SCREEN_W       320
#define SCREEN_H       240

// Header band
#define HEADER_DIV_Y    36
#define CLOCK_X          6   // TL datum
#define CLOCK_Y          0
#define LOCATION_X     313   // TR datum
#define LOCATION_Y       2
#define UPDATED_X      313   // TR datum
#define UPDATED_Y       21

// Left column: current conditions
#define COL_DIV_X      160   // vertical divider
#define COL_DIV_TOP     40
#define COL_DIV_BOT    188
#define CUR_ICON_X       4   // 100x100 bitmap, TL anchor
#define CUR_ICON_Y      36
#define COND_LABEL_X   110   // TL datum
#define COND_LABEL_Y    42
#define TEMP_X         110   // TL datum, LARGE font (drawn in updateData)
#define TEMP_Y          78
#define TEMP_UNIT_X    150   // TL datum
#define TEMP_UNIT_Y     80
#define WIND_ICON_X     18   // 50x50 compass, TL anchor
#define WIND_ICON_Y    138
#define WIND_TXT_X      78   // TL datum
#define WIND_SPEED_Y   150
#define WIND_PRESS_Y   170

// Bottom divider
#define BOTTOM_DIV_Y   190

// Forecast 2x2 grid (column centres)
#define FC_COL_L_X     200
#define FC_COL_R_X     276
#define FC_R1_DAY_Y     50
#define FC_R1_TEMP_Y    66
#define FC_R1_ICON_Y    72   // 50x50 icon, TL anchor at (centre-25, this)
#define FC_R2_DAY_Y    122
#define FC_R2_TEMP_Y   138
#define FC_R2_ICON_Y   144

// Bottom band (sun / moon / cloud / humidity)
#define SUN_X           44   // column centre
#define SUN_HDR_Y      200
#define SUN_RISE_Y     216
#define SUN_SET_Y      231
#define MOON_CX        126   // disc centre x; 60x60 bitmap anchored at (CX-30, ICON_Y)
#define MOON_ICON_Y    164
#define MOON_LABEL_Y   238
#define CLOUD_X        205   // column centre
#define HUM_X          281   // column centre
#define BB_LABEL_Y     202
#define BB_VALUE_Y     220
```

- [ ] **Step 2: Switch to landscape rotation**

In `setup()` (currently `src/main.cpp:162`), change:

```cpp
  tft.setRotation(0);  // For 320x480 screen
```
to:
```cpp
  tft.setRotation(1);  // Landscape 320x240 (use 3 to flip 180 deg)
```

- [ ] **Step 3: Make separators span the full landscape width**

In `drawSeparator()` (currently `src/main.cpp:625-627`), change:

```cpp
void drawSeparator(uint16_t y) {
  tft.drawFastHLine(10, y, 240 - 2 * 10, 0x4228);
}
```
to:
```cpp
void drawSeparator(uint16_t y) {
  tft.drawFastHLine(10, y, SCREEN_W - 2 * 10, 0x4228);
}
```

- [ ] **Step 4: Build the ST7789 env**

Run: `& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -e ESP32_2432S028R_ST7789`
Expected: `[SUCCESS]`

- [ ] **Step 5: Commit**

```bash
git add src/main.cpp
git commit -m "Landscape rotation + layout constants scaffold"
```

---

### Task 2: Add the configurable `location` setting

**Files:**
- Modify: `src/All_Settings.Example.h`
- Modify: `src/All_Settings.h`

- [ ] **Step 1: Add `location` to the example template**

In `src/All_Settings.Example.h`, after the `language` line (currently line 47), add:

```cpp
// Location label shown top-right of the display (the API has no province/state,
// so set the full label you want here, e.g. "Fernie, BC").
const String location = "Fernie, BC";
```

- [ ] **Step 2: Add `location` to the live settings file**

In `src/All_Settings.h`, add the same line in the same place (after the `language` definition):

```cpp
const String location = "Fernie, BC";
```

- [ ] **Step 3: Build the ST7789 env**

Run: `& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -e ESP32_2432S028R_ST7789`
Expected: `[SUCCESS]` (the `location` symbol now resolves even though it isn't drawn yet)

- [ ] **Step 4: Commit**

```bash
git add src/All_Settings.Example.h
git commit -m "Add configurable location setting"
```
(Note: `src/All_Settings.h` is gitignored and will not be staged — that is expected.)

---

### Task 3: Header band — clock, location, updated, divider

**Files:**
- Modify: `src/main.cpp` — `drawTime()` (`361-385`), `drawCurrentWeather()` header lines (`396-399`)

- [ ] **Step 1: Reposition the clock to top-left and move the divider**

In `drawTime()`, change:

```cpp
  tft.setTextDatum(BC_DATUM);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextPadding(tft.textWidth(" 44:44 "));  // String width + margin
  tft.drawString(timeNow, 120, 53);

  drawSeparator(51);
```
to:
```cpp
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextPadding(tft.textWidth("44:44"));  // String width
  tft.drawString(timeNow, CLOCK_X, CLOCK_Y);

  drawSeparator(HEADER_DIV_Y);
```

- [ ] **Step 2: Draw the location (new) and reposition the "Updated:" line, both top-right**

In `drawCurrentWeather()`, change the opening header draw (currently `src/main.cpp:396-399`):

```cpp
  tft.setTextDatum(BC_DATUM);
  tft.setTextColor(TFT_ORANGE, TFT_BLACK);
  tft.setTextPadding(tft.textWidth(" Updated: Mmm 44 44:44 "));  // String width + margin
  tft.drawString(date, 120, 16);
```
to:
```cpp
  // Location label (top-right)
  tft.setTextDatum(TR_DATUM);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setTextPadding(tft.textWidth(" Wwwwwwwwwwwwww "));  // wide enough to erase old label
  tft.drawString(location, LOCATION_X, LOCATION_Y);

  // Updated timestamp (top-right, below the location)
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextPadding(tft.textWidth(" Updated: Mmm 44 44:44 "));  // String width + margin
  tft.drawString(date, UPDATED_X, UPDATED_Y);
```

- [ ] **Step 3: Build the ST7789 env**

Run: `& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -e ESP32_2432S028R_ST7789`
Expected: `[SUCCESS]`

- [ ] **Step 4: Commit**

```bash
git add src/main.cpp
git commit -m "Reflow header band (clock, location, updated, divider)"
```

---

### Task 4: Left column — current condition icon, label, temperature, units, wind, compass, column divider

**Files:**
- Modify: `src/main.cpp` — `drawCurrentWeather()` (`406-463`), `updateData()` temp draw (`323-333`)

- [ ] **Step 1: Reposition the current-condition icon**

In `drawCurrentWeather()`, change (currently `src/main.cpp:408`):

```cpp
  ui.drawBmp("/icon/" + weatherIcon + ".bmp", 0, 53);
```
to:
```cpp
  ui.drawBmp("/icon/" + weatherIcon + ".bmp", CUR_ICON_X, CUR_ICON_Y);
```

- [ ] **Step 2: Reposition the condition label (left-aligned, two-line split kept)**

Change the condition-text block (currently `src/main.cpp:416-426`):

```cpp
  tft.setTextDatum(BR_DATUM);
  tft.setTextColor(TFT_ORANGE, TFT_BLACK);

  int splitPoint = 0;
  int xpos = 235;
  splitPoint = splitIndex(weatherText);

  tft.setTextPadding(xpos - 100);  // xpos - icon width
  if (splitPoint) tft.drawString(weatherText.substring(0, splitPoint), xpos, 69);
  else tft.drawString(" ", xpos, 69);
  tft.drawString(weatherText.substring(splitPoint), xpos, 86);
```
to:
```cpp
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(TFT_ORANGE, TFT_BLACK);

  int splitPoint = splitIndex(weatherText);

  tft.setTextPadding(tft.textWidth(" Wwwwwwww "));  // erase old label width
  if (splitPoint) {
    tft.drawString(weatherText.substring(0, splitPoint), COND_LABEL_X, COND_LABEL_Y);
    tft.drawString(weatherText.substring(splitPoint),    COND_LABEL_X, COND_LABEL_Y + 16);
  } else {
    tft.drawString(weatherText, COND_LABEL_X, COND_LABEL_Y);
    tft.drawString(" ",         COND_LABEL_X, COND_LABEL_Y + 16);
  }
```

- [ ] **Step 3: Reposition the temperature unit (`oC`/`oF`)**

Change (currently `src/main.cpp:428-432`):

```cpp
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextDatum(TR_DATUM);
  tft.setTextPadding(0);
  if (units == "metric") tft.drawString("oC", 237, 95);
  else tft.drawString("oF", 237, 95);
```
to:
```cpp
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextDatum(TL_DATUM);
  tft.setTextPadding(0);
  if (units == "metric") tft.drawString("oC", TEMP_UNIT_X, TEMP_UNIT_Y);
  else tft.drawString("oF", TEMP_UNIT_X, TEMP_UNIT_Y);
```

- [ ] **Step 4: Reposition wind speed and pressure (stacked, right of compass)**

Change the wind-speed draw (currently `src/main.cpp:442-444`):

```cpp
  tft.setTextDatum(TC_DATUM);
  tft.setTextPadding(tft.textWidth("888 m/s"));  // Max string length?
  tft.drawString(weatherText, 124, 136);
```
to:
```cpp
  tft.setTextDatum(TL_DATUM);
  tft.setTextPadding(tft.textWidth("888 m/s"));  // Max string length?
  tft.drawString(weatherText, WIND_TXT_X, WIND_SPEED_Y);
```

Then change the pressure draw (currently `src/main.cpp:454-456`):

```cpp
  tft.setTextDatum(TR_DATUM);
  tft.setTextPadding(tft.textWidth(" 8888hPa"));  // Max string length?
  tft.drawString(weatherText, 230, 136);
```
to:
```cpp
  tft.setTextDatum(TL_DATUM);
  tft.setTextPadding(tft.textWidth("8888 hPa"));  // Max string length?
  tft.drawString(weatherText, WIND_TXT_X, WIND_PRESS_Y);
```

- [ ] **Step 5: Reposition the wind compass and replace the horizontal separator with the vertical column divider**

Change (currently `src/main.cpp:461-463`):

```cpp
  ui.drawBmp("/wind/" + wind[windAngle] + ".bmp", 101, 86);

  drawSeparator(153);
```
to:
```cpp
  ui.drawBmp("/wind/" + wind[windAngle] + ".bmp", WIND_ICON_X, WIND_ICON_Y);

  tft.drawFastVLine(COL_DIV_X, COL_DIV_TOP, COL_DIV_BOT - COL_DIV_TOP, 0x4228);
```

- [ ] **Step 6: Reposition the large temperature digits in `updateData()`**

Change (currently `src/main.cpp:323-333`):

```cpp
    tft.loadFont(AA_FONT_LARGE, LittleFS);
    tft.setTextDatum(TR_DATUM);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);

    // Font ASCII code 0xB0 is a degree symbol, but o used instead in small font
    tft.setTextPadding(tft.textWidth(" -88"));  // Max width of values

    String weatherText = "";
    weatherText = String(forecast->temp[0], 0);  // Make it integer temperature
    tft.drawString(weatherText, 215, 95);        //  + "°" symbol is big... use o in small font
    tft.unloadFont();
```
to:
```cpp
    tft.loadFont(AA_FONT_LARGE, LittleFS);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);

    // Font ASCII code 0xB0 is a degree symbol, but o used instead in small font
    tft.setTextPadding(tft.textWidth("-88"));  // Max width of values

    String weatherText = "";
    weatherText = String(forecast->temp[0], 0);  // Make it integer temperature
    tft.drawString(weatherText, TEMP_X, TEMP_Y); //  + "°" symbol is big... use o in small font
    tft.unloadFont();
```

- [ ] **Step 7: Build the ST7789 env**

Run: `& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -e ESP32_2432S028R_ST7789`
Expected: `[SUCCESS]`

- [ ] **Step 8: Commit**

```bash
git add src/main.cpp
git commit -m "Reflow left column (condition, temp, wind, compass, divider)"
```

---

### Task 5: Forecast 2×2 grid

**Files:**
- Modify: `src/main.cpp` — prototype (`129`), `drawForecast()` (`473-484`), `drawForecastDetail()` (`490-523`)

- [ ] **Step 1: Update the `drawForecastDetail` prototype**

Change (currently `src/main.cpp:129`):

```cpp
void drawForecastDetail(uint16_t x, uint16_t y, uint8_t dayIndex);
```
to:
```cpp
void drawForecastDetail(uint16_t cx, uint16_t dayY, uint16_t tempY, uint16_t iconY, uint8_t dayIndex);
```

- [ ] **Step 2: Rewrite the `drawForecast()` call sites for a 2×2 grid**

Replace the body of `drawForecast()` (currently `src/main.cpp:473-484`):

```cpp
void drawForecast() {
  int8_t dayIndex = getNextDayIndex();

  drawForecastDetail(8, 171, dayIndex);
  dayIndex += 8;
  drawForecastDetail(66, 171, dayIndex);  // was 95
  dayIndex += 8;
  drawForecastDetail(124, 171, dayIndex);  // was 180
  dayIndex += 8;
  drawForecastDetail(182, 171, dayIndex);  // was 180
  drawSeparator(171 + 69);
}
```
with:
```cpp
void drawForecast() {
  int8_t dayIndex = getNextDayIndex();

  drawForecastDetail(FC_COL_L_X, FC_R1_DAY_Y, FC_R1_TEMP_Y, FC_R1_ICON_Y, dayIndex);  // TUE
  dayIndex += 8;
  drawForecastDetail(FC_COL_R_X, FC_R1_DAY_Y, FC_R1_TEMP_Y, FC_R1_ICON_Y, dayIndex);  // WED
  dayIndex += 8;
  drawForecastDetail(FC_COL_L_X, FC_R2_DAY_Y, FC_R2_TEMP_Y, FC_R2_ICON_Y, dayIndex);  // THU
  dayIndex += 8;
  drawForecastDetail(FC_COL_R_X, FC_R2_DAY_Y, FC_R2_TEMP_Y, FC_R2_ICON_Y, dayIndex);  // FRI

  drawSeparator(BOTTOM_DIV_Y);
}
```

- [ ] **Step 3: Rewrite `drawForecastDetail()` to centre each cell on its column with two-colour temps**

Replace the whole function (currently `src/main.cpp:490-523`):

```cpp
void drawForecastDetail(uint16_t x, uint16_t y, uint8_t dayIndex) {

  if (dayIndex >= MAX_DAYS * 8) return;

  String day = shortDOW[weekday(TIMEZONE.toLocal(forecast->dt[dayIndex + 4], &tz1_Code))];
  day.toUpperCase();

  tft.setTextDatum(BC_DATUM);

  tft.setTextColor(TFT_ORANGE, TFT_BLACK);
  tft.setTextPadding(tft.textWidth("WWW"));
  tft.drawString(day, x + 25, y);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextPadding(tft.textWidth("-88   -88"));

  // Find the temperature min and max during the day
  float tmax = -9999;
  float tmin = 9999;
  for (int i = 0; i < 8; i++)
    if (forecast->temp_max[dayIndex + i] > tmax) tmax = forecast->temp_max[dayIndex + i];
  for (int i = 0; i < 8; i++)
    if (forecast->temp_min[dayIndex + i] < tmin) tmin = forecast->temp_min[dayIndex + i];

  String highTemp = String(tmax, 0);
  String lowTemp = String(tmin, 0);
  tft.drawString(highTemp + " " + lowTemp, x + 25, y + 17);

  String weatherIcon = getMeteoconIcon(forecast->id[dayIndex + 4], false);

  ui.drawBmp("/icon50/" + weatherIcon + ".bmp", x, y + 18);

  tft.setTextPadding(0);  // Reset padding width to none
}
```
with:
```cpp
void drawForecastDetail(uint16_t cx, uint16_t dayY, uint16_t tempY, uint16_t iconY, uint8_t dayIndex) {

  if (dayIndex >= MAX_DAYS * 8) return;

  String day = shortDOW[weekday(TIMEZONE.toLocal(forecast->dt[dayIndex + 4], &tz1_Code))];
  day.toUpperCase();

  // Day label (gold, centred on the column)
  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(TFT_ORANGE, TFT_BLACK);
  tft.setTextPadding(tft.textWidth("WWW"));
  tft.drawString(day, cx, dayY);

  // Find the temperature min and max during the day
  float tmax = -9999;
  float tmin = 9999;
  for (int i = 0; i < 8; i++)
    if (forecast->temp_max[dayIndex + i] > tmax) tmax = forecast->temp_max[dayIndex + i];
  for (int i = 0; i < 8; i++)
    if (forecast->temp_min[dayIndex + i] < tmin) tmin = forecast->temp_min[dayIndex + i];

  String highTemp = String(tmax, 0);
  String lowTemp = String(tmin, 0);

  // High (white) ends just left of centre; low (cyan) starts just right of centre
  tft.setTextPadding(tft.textWidth("-88"));
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextDatum(TR_DATUM);
  tft.drawString(highTemp, cx - 3, tempY);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setTextDatum(TL_DATUM);
  tft.drawString(lowTemp, cx + 3, tempY);

  // Condition icon (50x50) centred on the column
  String weatherIcon = getMeteoconIcon(forecast->id[dayIndex + 4], false);
  ui.drawBmp("/icon50/" + weatherIcon + ".bmp", cx - 25, iconY);

  tft.setTextPadding(0);  // Reset padding width to none
}
```

- [ ] **Step 4: Build the ST7789 env**

Run: `& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -e ESP32_2432S028R_ST7789`
Expected: `[SUCCESS]`

- [ ] **Step 5: Commit**

```bash
git add src/main.cpp
git commit -m "Reflow forecast into 2x2 grid with two-colour temps"
```

---

### Task 6: Bottom band — sun block and the line-primitive arrow helper

**Files:**
- Modify: `src/main.cpp` — new prototype (after `132`), new `drawArrow()` function (after `drawSeparator`), `drawAstronomy()` sun lines (`545-560`)

- [ ] **Step 1: Add the `drawArrow` prototype**

After the `drawSeparator` prototype (currently `src/main.cpp:132`), add:

```cpp
void drawArrow(int x, int yCentre, bool up, uint16_t colour);
```

- [ ] **Step 2: Add the `drawArrow` helper**

Immediately after the `drawSeparator()` function definition (currently ends `src/main.cpp:627`), add:

```cpp
/***************************************************************************************
**                          Draw a small up/down arrow from line primitives
***************************************************************************************/
// ~8px tall, vertically centred on yCentre, horizontally centred on x.
void drawArrow(int x, int yCentre, bool up, uint16_t colour) {
  int half = 4;  // half-height -> ~8px tall
  int top = yCentre - half;
  int bot = yCentre + half;
  tft.drawLine(x, top, x, bot, colour);  // vertical shaft
  if (up) {
    tft.drawLine(x, top, x - 3, top + 4, colour);  // left head
    tft.drawLine(x, top, x + 3, top + 4, colour);  // right head
  } else {
    tft.drawLine(x, bot, x - 3, bot - 4, colour);  // left head
    tft.drawLine(x, bot, x + 3, bot - 4, colour);  // right head
  }
}
```

- [ ] **Step 3: Reflow the sun header and times (arrow + time centred per line)**

In `drawAstronomy()`, replace the sun block (currently `src/main.cpp:545-560`):

```cpp
  tft.setTextDatum(BC_DATUM);
  tft.setTextColor(TFT_ORANGE, TFT_BLACK);
  tft.setTextPadding(0);  // Reset padding width to none
  tft.drawString(sunStr, 40, 270);

  tft.setTextDatum(BR_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextPadding(tft.textWidth(" 88:88 "));

  String rising = strTime(forecast->sunrise) + " ";
  int dt = rightOffset(rising, ":");  // Draw relative to colon to them aligned
  tft.drawString(rising, 40 + dt, 290);

  String setting = strTime(forecast->sunset) + " ";
  dt = rightOffset(setting, ":");
  tft.drawString(setting, 40 + dt, 305);
```
with:
```cpp
  // Sun header (centred)
  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(TFT_ORANGE, TFT_BLACK);
  tft.setTextPadding(0);  // Reset padding width to none
  tft.drawString(sunStr, SUN_X, SUN_HDR_Y);

  // Sunrise / sunset times, each preceded by a line-drawn arrow, centred as a unit.
  // ML_DATUM => y is the vertical centre of the text, matching the arrow centre.
  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setTextPadding(tft.textWidth("88:88"));

  String rising = strTime(forecast->sunrise);
  int textX = SUN_X - (8 + tft.textWidth(rising)) / 2 + 8;  // centre the arrow+time pair
  drawArrow(textX - 6, SUN_RISE_Y, true, TFT_CYAN);
  tft.drawString(rising, textX, SUN_RISE_Y);

  String setting = strTime(forecast->sunset);
  textX = SUN_X - (8 + tft.textWidth(setting)) / 2 + 8;
  drawArrow(textX - 6, SUN_SET_Y, false, TFT_CYAN);
  tft.drawString(setting, textX, SUN_SET_Y);
```

- [ ] **Step 4: Build the ST7789 env**

Run: `& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -e ESP32_2432S028R_ST7789`
Expected: `[SUCCESS]`

- [ ] **Step 5: Commit**

```bash
git add src/main.cpp
git commit -m "Reflow sun block with line-primitive arrows"
```

---

### Task 7: Bottom band — moon, cloud, humidity

**Files:**
- Modify: `src/main.cpp` — `drawAstronomy()` moon lines (`542-543`), cloud/humidity lines (`562-586`)

- [ ] **Step 1: Reposition the moon disc and label**

In `drawAstronomy()`, change the moon draw (currently `src/main.cpp:542-543`):

```cpp
  tft.drawString(moonPhase[ip], 120, 319);
  ui.drawBmp("/moon/moonphase_L" + String(icon) + ".bmp", 120 - 30, 318 - 16 - 60);
```
to:
```cpp
  ui.drawBmp("/moon/moonphase_L" + String(icon) + ".bmp", MOON_CX - 30, MOON_ICON_Y);
  tft.drawString(moonPhase[ip], MOON_CX, MOON_LABEL_Y);
```
(The `tft.setTextDatum(BC_DATUM)` set just above at `src/main.cpp:530` still applies — the moon label stays bottom-centred.)

- [ ] **Step 2: Reflow cloud and humidity into the two right-hand bottom columns**

Replace the cloud/humidity block (currently `src/main.cpp:562-586`):

```cpp
  tft.setTextDatum(BC_DATUM);
  tft.setTextColor(TFT_ORANGE, TFT_BLACK);
  tft.drawString(cloudStr, 195, 260);  // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< ?

  String cloudCover = "";
  cloudCover += forecast->clouds_all[0];
  cloudCover += "%";

  tft.setTextDatum(BR_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextPadding(tft.textWidth(" 100%"));
  tft.drawString(cloudCover, 210, 277);

  tft.setTextDatum(BC_DATUM);
  tft.setTextColor(TFT_ORANGE, TFT_BLACK);
  tft.drawString(humidityStr, 195, 300 - 2);  // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< ?

  String humidity = "";
  humidity += forecast->humidity[0];
  humidity += "%";

  tft.setTextDatum(BR_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextPadding(tft.textWidth("100%"));
  tft.drawString(humidity, 210, 315);
```
with:
```cpp
  // Cloud (centred column)
  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(TFT_ORANGE, TFT_BLACK);
  tft.setTextPadding(0);
  tft.drawString(cloudStr, CLOUD_X, BB_LABEL_Y);

  String cloudCover = "";
  cloudCover += forecast->clouds_all[0];
  cloudCover += "%";

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextPadding(tft.textWidth(" 100% "));
  tft.drawString(cloudCover, CLOUD_X, BB_VALUE_Y);

  // Humidity (centred column)
  tft.setTextColor(TFT_ORANGE, TFT_BLACK);
  tft.setTextPadding(0);
  tft.drawString(humidityStr, HUM_X, BB_LABEL_Y);

  String humidity = "";
  humidity += forecast->humidity[0];
  humidity += "%";

  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setTextPadding(tft.textWidth(" 100% "));
  tft.drawString(humidity, HUM_X, BB_VALUE_Y);
```

- [ ] **Step 3: Build the ST7789 env**

Run: `& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -e ESP32_2432S028R_ST7789`
Expected: `[SUCCESS]`

- [ ] **Step 4: Commit**

```bash
git add src/main.cpp
git commit -m "Reflow moon, cloud, and humidity into bottom band"
```

---

### Task 8: Boot / splash / progress reflow

**Files:**
- Modify: `src/main.cpp` — `setup()` splash + boot text (`175`, `185`, `191`, `197-198`, `204-207`, `224-226`), `drawProgress()` (`349-352`)

- [ ] **Step 1: Re-centre the FORMAT_LittleFS message**

Change (currently `src/main.cpp:175`):

```cpp
  tft.drawString("Formatting LittleFS, so wait!", 120, 195);
```
to:
```cpp
  tft.drawString("Formatting LittleFS, so wait!", 160, 120);
```

- [ ] **Step 2: Re-centre the splash image (240×124 → centred in 320 wide)**

Change (currently `src/main.cpp:185`):

```cpp
    TJpgDec.drawFsJpg(0, 40, "/splash/OpenWeather.jpg", LittleFS);
```
to:
```cpp
    TJpgDec.drawFsJpg(40, 30, "/splash/OpenWeather.jpg", LittleFS);
```

- [ ] **Step 3: Re-centre the credit lines and their clearing rects**

Change the first credits block (currently `src/main.cpp:191-198`):

```cpp
  // Clear bottom section of screen
  tft.fillRect(0, 206, 240, 320 - 206, TFT_BLACK);

  tft.loadFont(AA_FONT_SMALL, LittleFS);
  tft.setTextDatum(BC_DATUM);  // Bottom Centre datum
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);

  tft.drawString("Original by: blog.squix.org", 120, 260);
  tft.drawString("Adapted by: Bodmer", 120, 280);
```
to:
```cpp
  // Clear bottom section of screen
  tft.fillRect(0, 160, SCREEN_W, SCREEN_H - 160, TFT_BLACK);

  tft.loadFont(AA_FONT_SMALL, LittleFS);
  tft.setTextDatum(BC_DATUM);  // Bottom Centre datum
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);

  tft.drawString("Original by: blog.squix.org", 160, 180);
  tft.drawString("Adapted by: Bodmer", 160, 200);
```

- [ ] **Step 4: Re-centre the "Connecting to WiFi" block**

Change (currently `src/main.cpp:204-207`):

```cpp
  tft.fillRect(0, 206, 240, 320 - 206, TFT_BLACK);

  tft.drawString("Connecting to WiFi", 120, 240);
  tft.setTextPadding(240);  // Pad next drawString() text to full width to over-write old text
```
to:
```cpp
  tft.fillRect(0, 160, SCREEN_W, SCREEN_H - 160, TFT_BLACK);

  tft.drawString("Connecting to WiFi", 160, 150);
  tft.setTextPadding(SCREEN_W);  // Pad next drawString() text to full width to over-write old text
```

- [ ] **Step 5: Re-centre the "Fetching weather data..." block**

Change (currently `src/main.cpp:223-226`):

```cpp
  tft.setTextDatum(BC_DATUM);
  tft.setTextPadding(240);        // Pad next drawString() text to full width to over-write old text
  tft.drawString(" ", 120, 220);  // Clear line above using set padding width
  tft.drawString("Fetching weather data...", 120, 240);
```
to:
```cpp
  tft.setTextDatum(BC_DATUM);
  tft.setTextPadding(SCREEN_W);   // Pad next drawString() text to full width to over-write old text
  tft.drawString(" ", 160, 140);  // Clear line above using set padding width
  tft.drawString("Fetching weather data...", 160, 160);
```

- [ ] **Step 6: Re-centre the progress text and bar in `drawProgress()`**

Change (currently `src/main.cpp:349-352`):

```cpp
  tft.setTextPadding(240);
  tft.drawString(text, 120, 260);

  ui.drawProgressBar(10, 269, 240 - 20, 15, percentage, TFT_WHITE, TFT_BLUE);
```
to:
```cpp
  tft.setTextPadding(SCREEN_W);
  tft.drawString(text, 160, 150);

  ui.drawProgressBar(10, 175, SCREEN_W - 20, 15, percentage, TFT_WHITE, TFT_BLUE);
```

- [ ] **Step 7: Build the ST7789 env**

Run: `& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -e ESP32_2432S028R_ST7789`
Expected: `[SUCCESS]`

- [ ] **Step 8: Commit**

```bash
git add src/main.cpp
git commit -m "Re-centre boot, splash, and progress screens for landscape"
```

---

### Task 9: Final verification

**Files:** none (verification only)

- [ ] **Step 1: Clean ST7789 build**

Run: `& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -e ESP32_2432S028R_ST7789`
Expected: `[SUCCESS]`

- [ ] **Step 2: Update the stale header comment**

The file header already claims landscape (`src/main.cpp:22`), which is now true — no change needed. Confirm no remaining `setRotation(0)` or hard-coded `240`/`320`/`120` portrait coordinates remain in the reflowed draw functions by grepping:

Run: `git grep -n "120, \|, 320 -\|240 -\|setRotation(0)" src/main.cpp`
Expected: no matches inside `drawTime`, `drawCurrentWeather`, `drawForecast*`, `drawAstronomy`, `drawProgress`, or `setup()`'s screen draws. (Matches inside unrelated logic, e.g. `MAX_*` math, are fine.)

- [ ] **Step 3: Hand off to the user for hardware verification**

The following CANNOT be verified without a real CYD and must be checked by the user by flashing firmware + filesystem (`-t upload`, `-t uploadfs`) and looking at the screen:
- Overall alignment vs the mockup.
- The four known tight spots: left column gap (100px icon + 50px compass), condition-label width for long words, forecast row-2 icon clearing the `y=190` divider, and the moon disc/`Full` label fit near the bottom edge.
- Rotation direction (`1` vs `3`) — flip if mounted upside-down.
- A final `ILI9341` build: `& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -e ESP32_2432S028R_ILI9341`

---

## Notes on known tight spots (tune these Y constants on hardware)

These are consequences of the real bitmaps being larger than the mockup's SVGs; all are isolated to the constants block:
1. **Left column full** — `CUR_ICON_Y` (36) + 100px icon ends at 136; `WIND_ICON_Y` (138) + 50px ends at 188. ~2px gap. Nudge if needed.
2. **Condition label width** — only ~50px right of the icon; long single words (e.g. "Thunderstorm") will clip. Two-line split helps multi-word descriptions only.
3. **Forecast row-2 icon** — `FC_R2_ICON_Y` (144) + 50px = 194, ~4px past the `BOTTOM_DIV_Y` (190) divider. Raise `FC_R2_*` constants to compress the grid if the overlap looks wrong.
4. **Moon** — 60px disc at `MOON_ICON_Y` (164) ends at 224; `MOON_LABEL_Y` (238, bottom-centred) sits just below. Disc top (164) intrudes above the divider into the (empty) lower-right of the current-conditions column. Nudge `MOON_ICON_Y` up/down to balance.
