/*
  The sketch is displaying a nice Internet Weather Station with 3 4 days forecast on an
  ESP32-Cheap Yellow Display (CYD) with 2.8-inch TFT display and 320x240 pixels resolution.

  The weather data is retrieved from OpenWeatherMap.org service and you need to get your own
  (free) API key before running the sketch.

  Please add the key, your Wi-Fi credentials, the location coordinates for the weather information and
  additional data about your Timezone in the settings file: 'All_Settings.h'.

  The code is not written by me, but from Daniel Eichhorn (https://blog.squix.ch) and was
  adapted by Bodmer as an example for his OpenWeather library (https://github.com/Bodmer/OpenWeather/).

  I changed lines of code to crrect some issues, for more information see: https://github.com/Bodmer/OpenWeather/issues/26
  1):
  Search for: String date = "Updated: " + strDate(local_time);
  Change to:  String date = "Updated: " + strDate(now());
  2)
  Search for: if ( today && id/100 == 8 && (forecast->dt[0] < forecast->sunrise || forecast->dt[0] > forecast->sunset)) id += 1000;
  Change to:  if ( today && id/100 == 8 && (now() < forecast->sunrise || now() > forecast->sunset)) id += 1000;

  The code is optimized to run on the Cheap Yellow Display ('CYD') with a screen resolution of 320 x 240 pixels in Landscape
  orientation, that is the 'ESP32-2432S028R' variant.

  The weather icons and fonts are stored in ESP32's LittleFS filesystem, so you need to upload the files in the 'data' subfolder
  first before uploading the code and starting the device.
  See 'Upload_To_LittleFS.md' for more details about this.

              >>>       IMPORTANT TO PREVENT CRASHES      <<<
  >>>>>>  Set LittleFS to at least 1.5Mbytes before uploading files  <<<<<<

  The sketch is using the TFT_eSPI library by Bodmer (https://github.com/Bodmer/TFT_eSPI), so please select the correct
  'User_Setups' file in the library folder. I prepare two files (see my GitHub Repository) that should work:
  For older device with one USB connector (chip driver ILI9341) use:
    Setup801_ESP32_CYD_ILI9341_240x320.h
  New devices with two USB connectors (chip driver ST7789) require:
    Setup805_ESP32_CYD_ST7789_240x320.h

  Original by Daniel Eichhorn, see license at end of file.

*/

/*
  This sketch uses font files created from the Noto family of fonts as bitmaps
  generated from these fonts may be freely distributed:
  https://www.google.com/get/noto/

  A processing sketch to create new fonts can be found in the Tools folder of TFT_eSPI
  https://github.com/Bodmer/TFT_eSPI/tree/master/Tools/Create_Smooth_Font/Create_font
  New fonts can be generated to include language specific characters. The Noto family
  of fonts has an extensive character set coverage.

  Json streaming parser (do not use IDE library manager version) to use is here:
  https://github.com/Bodmer/JSON_Decoder
*/

#define SERIAL_MESSAGES  // For serial output weather reports
//#define SCREEN_SERVER   // For dumping screen shots from TFT
//#define RANDOM_LOCATION // Test only, selects random weather location every refresh
//#define FORMAT_LittleFS   // Wipe LittleFS and all files!

const char* PROGRAM_VERSION = "ESP32 CYD OpenWeatherMap LittleFS V02";

#include <FS.h>
#include <LittleFS.h>

#define AA_FONT_SMALL "fonts/NSBold15"  // 15 point Noto sans serif bold
#define AA_FONT_LARGE "fonts/NSBold36"  // 36 point Noto sans serif bold
/***************************************************************************************
**                          Load the libraries and settings
***************************************************************************************/
#include <Arduino.h>

#include <SPI.h>
#include <TFT_eSPI.h>  // https://github.com/Bodmer/TFT_eSPI

// Additional functions
#include "GfxUi.h"  // Attached to this sketch
#include "MoonPhase.h"  // Attached to this sketch

// Choose library to load
#ifdef ESP8266
#include <ESP8266WiFi.h>
#elif defined(ARDUINO_ARCH_MBED) || defined(ARDUINO_ARCH_RP2040)
#if defined(ARDUINO_RASPBERRY_PI_PICO_W)
#include <WiFi.h>
#else
#include <WiFiNINA.h>
#endif
#else  // ESP32
#include <WiFi.h>
#endif


// check All_Settings.h for adapting to your needs
#include "All_Settings.h"

#include <JSON_Decoder.h>  // https://github.com/Bodmer/JSON_Decoder

#include <OpenWeather.h>  // Latest here: https://github.com/Bodmer/OpenWeather

#include "NTP_Time.h"  // Attached to this sketch, see that tab for library needs
// Time zone correction library: https://github.com/JChristensen/Timezone

/***************************************************************************************
**                          Define the globals and class instances
***************************************************************************************/

TFT_eSPI tft = TFT_eSPI();  // Invoke custom library

OW_Weather ow;  // Weather forecast library instance

OW_forecast* forecast;

boolean booted = true;

GfxUi ui = GfxUi(&tft);  // Jpeg and bmpDraw functions

long lastDownloadUpdate = millis();

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

/***************************************************************************************
**                          Declare prototypes
***************************************************************************************/
void updateData();
void drawProgress(uint8_t percentage, String text);
void drawTime();
void drawCurrentWeather();
void drawForecast();
void drawForecastDetail(uint16_t cx, uint16_t dayY, uint16_t tempY, uint16_t iconY, uint8_t dayIndex);
const char* getMeteoconIcon(uint16_t id, bool today);
void drawAstronomy();
void drawSeparator(uint16_t y);
void drawArrow(int x, int yCentre, bool up, uint16_t colour);
void fillSegment(int x, int y, int start_angle, int sub_angle, int r, unsigned int colour);
String strDate(time_t unixTime);
String strTime(time_t unixTime);
void printWeather(void);
int leftOffset(String text, String sub);
int rightOffset(String text, String sub);
int splitIndex(String text);
int getNextDayIndex(void);

bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
  // Stop further decoding as image is running off bottom of screen
  if (y >= tft.height()) return 0;

  // This function will clip the image block rendering automatically at the TFT boundaries
  tft.pushImage(x, y, w, h, bitmap);

  // Return 1 to decode next block
  return 1;
}

/***************************************************************************************
**                          Setup
***************************************************************************************/
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println(PROGRAM_VERSION);

  tft.begin();
  tft.setRotation(1);  // Landscape 320x240 (use 3 to flip 180 deg)
  tft.fillScreen(TFT_BLACK);

  if (!LittleFS.begin()) {
    Serial.println("Flash FS initialisation failed!");
    while (1) yield();  // Stay here twiddling thumbs waiting
  }
  Serial.println("\nFlash FS available!");

// Enable if you want to erase LittleFS, this takes some time!
// then disable and reload sketch to avoid reformatting on every boot!
#ifdef FORMAT_LittleFS
  tft.setTextDatum(BC_DATUM);  // Bottom Centre datum
  tft.drawString("Formatting LittleFS, so wait!", 120, 195);
  LittleFS.format();
#endif

  TJpgDec.setJpgScale(1);
  TJpgDec.setCallback(tft_output);
  TJpgDec.setSwapBytes(true);  // May need to swap the jpg colour bytes (endianess)

  // Draw splash screen
  if (LittleFS.exists("/splash/OpenWeather.jpg") == true) {
    TJpgDec.drawFsJpg(0, 40, "/splash/OpenWeather.jpg", LittleFS);
  }

  delay(2000);

  // Clear bottom section of screen
  tft.fillRect(0, 206, 240, 320 - 206, TFT_BLACK);

  tft.loadFont(AA_FONT_SMALL, LittleFS);
  tft.setTextDatum(BC_DATUM);  // Bottom Centre datum
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);

  tft.drawString("Original by: blog.squix.org", 120, 260);
  tft.drawString("Adapted by: Bodmer", 120, 280);

  tft.setTextColor(TFT_YELLOW, TFT_BLACK);

  delay(2000);

  tft.fillRect(0, 206, 240, 320 - 206, TFT_BLACK);

  tft.drawString("Connecting to WiFi", 120, 240);
  tft.setTextPadding(240);  // Pad next drawString() text to full width to over-write old text

// Call once for ESP32 and ESP8266
#if !defined(ARDUINO_ARCH_MBED)
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
#endif

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
#if defined(ARDUINO_ARCH_MBED) || defined(ARDUINO_ARCH_RP2040)
    if (WiFi.status() != WL_CONNECTED) WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
#endif
    delay(500);
  }
  Serial.println();

  tft.setTextDatum(BC_DATUM);
  tft.setTextPadding(240);        // Pad next drawString() text to full width to over-write old text
  tft.drawString(" ", 120, 220);  // Clear line above using set padding width
  tft.drawString("Fetching weather data...", 120, 240);

  // Fetch the time
  udp.begin(localPort);
  syncTime();

  tft.unloadFont();
}

/***************************************************************************************
**                          Loop
***************************************************************************************/
void loop() {

  // Check if we should update weather information
  if (booted || (millis() - lastDownloadUpdate > 1000UL * UPDATE_INTERVAL_SECS)) {
    updateData();
    lastDownloadUpdate = millis();
  }

  // If minute has changed then request new time from NTP server
  if (booted || minute() != lastMinute) {
    // Update displayed time first as we may have to wait for a response
    drawTime();
    lastMinute = minute();

    // Request and synchronise the local clock
    syncTime();

#ifdef SCREEN_SERVER
    screenServer();
#endif
  }

  booted = false;
}

/***************************************************************************************
**                          Fetch the weather data  and update screen
***************************************************************************************/
// Update the Internet based information and update screen
void updateData() {
  // booted = true;  // Test only
  // booted = false; // Test only

  if (booted) drawProgress(20, "Updating time...");
  else fillSegment(22, 22, 0, (int)(20 * 3.6), 16, TFT_NAVY);

  if (booted) drawProgress(50, "Updating conditions...");
  else fillSegment(22, 22, 0, (int)(50 * 3.6), 16, TFT_NAVY);

  // Create the structure that holds the retrieved weather
  forecast = new OW_forecast;

#ifdef RANDOM_LOCATION  // Randomly choose a place on Earth to test icons etc
  String latitude = "";
  latitude = (random(180) - 90);
  String longitude = "";
  longitude = (random(360) - 180);
  Serial.print("Lat = ");
  Serial.print(latitude);
  Serial.print(", Lon = ");
  Serial.println(longitude);
#endif

#define SHOW_HEADER 1

  Serial.println("Getting weather data...");
  Serial.print("https://api.openweathermap.org/data/2.5/forecast?lat=" + latitude + "&lon=" + longitude + "&units=" + units + "&lang=" + language + "&appid=" + api_key);

  bool parsed = ow.getForecast(forecast, api_key, latitude, longitude, units, language);

  if (parsed) Serial.println("Data points received");
  else Serial.println("Failed to get data points");

  //Serial.print("Free heap = "); Serial.println(ESP.getFreeHeap(), DEC);

  printWeather();  // For debug, turn on output with #define SERIAL_MESSAGES

  if (booted) {
    drawProgress(100, "Done...");
    delay(2000);
    tft.fillScreen(TFT_BLACK);
  } else {
    fillSegment(22, 22, 0, 360, 16, TFT_NAVY);
    fillSegment(22, 22, 0, 360, 22, TFT_BLACK);
  }

  if (parsed) {
    tft.loadFont(AA_FONT_SMALL, LittleFS);
    drawCurrentWeather();
    drawForecast();
    drawAstronomy();
    tft.unloadFont();

    // Update the temperature here so we don't need to keep
    // loading and unloading font which takes time
    tft.loadFont(AA_FONT_LARGE, LittleFS);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);

    // Font ASCII code 0xB0 is a degree symbol, but o used instead in small font
    tft.setTextPadding(tft.textWidth("-88"));  // Max width of values

    String weatherText = "";
    weatherText = String(forecast->temp[0], 0);  // Make it integer temperature
    tft.drawString(weatherText, TEMP_X, TEMP_Y); //  + "°" symbol is big... use o in small font
    tft.unloadFont();
  } else {
    Serial.println("Failed to get weather");
  }

  // Delete to free up space
  delete forecast;
}

/***************************************************************************************
**                          Update progress bar
***************************************************************************************/
void drawProgress(uint8_t percentage, String text) {
  tft.loadFont(AA_FONT_SMALL, LittleFS);
  tft.setTextDatum(BC_DATUM);
  tft.setTextColor(TFT_ORANGE, TFT_BLACK);
  tft.setTextPadding(240);
  tft.drawString(text, 120, 260);

  ui.drawProgressBar(10, 269, 240 - 20, 15, percentage, TFT_WHITE, TFT_BLUE);

  tft.setTextPadding(0);
  tft.unloadFont();
}

/***************************************************************************************
**                          Draw the clock digits
***************************************************************************************/
void drawTime() {
  tft.loadFont(AA_FONT_LARGE, LittleFS);

  // Convert UTC to local time, returns zone code in tz1_Code, e.g "GMT"
  time_t local_time = TIMEZONE.toLocal(now(), &tz1_Code);

  String timeNow = "";

  if (hour(local_time) < 10) timeNow += "0";
  timeNow += hour(local_time);
  timeNow += ":";
  if (minute(local_time) < 10) timeNow += "0";
  timeNow += minute(local_time);

  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextPadding(tft.textWidth("44:44"));  // String width
  tft.drawString(timeNow, CLOCK_X, CLOCK_Y);

  drawSeparator(HEADER_DIV_Y);

  tft.setTextPadding(0);

  tft.unloadFont();
}

/***************************************************************************************
**                          Draw the current weather
***************************************************************************************/
void drawCurrentWeather() {
  time_t local_time = TIMEZONE.toLocal(now(), &tz1_Code);
  //String date = "Updated: " + strDate(local_time);
  String date = "Updated: " + strDate(now());  // see isue https://github.com/Bodmer/OpenWeather/issues/26
  String weatherText = "None";

  // Location label (top-right)
  tft.setTextDatum(TR_DATUM);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setTextPadding(tft.textWidth(" Wwwwwwwwwwwwww "));  // wide enough to erase old label
  tft.drawString(location, LOCATION_X, LOCATION_Y);

  // Updated timestamp (top-right, below the location)
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextPadding(tft.textWidth(" Updated: Mmm 44 44:44 "));  // String width + margin
  tft.drawString(date, UPDATED_X, UPDATED_Y);

  String weatherIcon = "";

  String currentSummary = forecast->main[0];
  currentSummary.toLowerCase();

  weatherIcon = getMeteoconIcon(forecast->id[0], true);

  ui.drawBmp("/icon/" + weatherIcon + ".bmp", CUR_ICON_X, CUR_ICON_Y);

  // Weather Text
  if (language == "en")
    weatherText = forecast->main[0];
  else
    weatherText = forecast->description[0];

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

  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextDatum(TL_DATUM);
  tft.setTextPadding(0);
  if (units == "metric") tft.drawString("oC", TEMP_UNIT_X, TEMP_UNIT_Y);
  else tft.drawString("oF", TEMP_UNIT_X, TEMP_UNIT_Y);

  //Temperature large digits added in updateData() to save swapping font here

  tft.setTextColor(TFT_ORANGE, TFT_BLACK);
  weatherText = String(forecast->wind_speed[0], 0);

  if (units == "metric") weatherText += " m/s";
  else weatherText += " mph";

  tft.setTextDatum(TL_DATUM);
  tft.setTextPadding(tft.textWidth("888 m/s"));  // Max string length?
  tft.drawString(weatherText, WIND_TXT_X, WIND_SPEED_Y);

  if (units == "imperial") {
    weatherText = forecast->pressure[0];
    weatherText += " in";
  } else {
    weatherText = String(forecast->pressure[0], 0);
    weatherText += " hPa";
  }

  tft.setTextDatum(TL_DATUM);
  tft.setTextPadding(tft.textWidth("8888 hPa"));  // Max string length?
  tft.drawString(weatherText, WIND_TXT_X, WIND_PRESS_Y);

  int windAngle = (forecast->wind_deg[0] + 22.5) / 45;
  if (windAngle > 7) windAngle = 0;
  String wind[] = { "N", "NE", "E", "SE", "S", "SW", "W", "NW" };
  ui.drawBmp("/wind/" + wind[windAngle] + ".bmp", WIND_ICON_X, WIND_ICON_Y);

  tft.drawFastVLine(COL_DIV_X, COL_DIV_TOP, COL_DIV_BOT - COL_DIV_TOP, 0x4228);

  tft.setTextDatum(TL_DATUM);  // Reset datum to normal
  tft.setTextPadding(0);       // Reset padding width to none
}

/***************************************************************************************
**                          Draw the 4 forecast columns
***************************************************************************************/
// draws the 2x2 grid of 4 forecast days
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

/***************************************************************************************
**                          Draw 1 forecast column at x, y
***************************************************************************************/
// helper for the forecast columns
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

/***************************************************************************************
**                          Draw Sun rise/set, Moon, cloud cover and humidity
***************************************************************************************/
void drawAstronomy() {

  tft.setTextDatum(BC_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextPadding(tft.textWidth(" Last qtr "));

  time_t local_time = TIMEZONE.toLocal(forecast->dt[0], &tz1_Code);
  uint16_t y = year(local_time);
  uint8_t m = month(local_time);
  uint8_t d = day(local_time);
  uint8_t h = hour(local_time);
  int ip;
  uint8_t icon = moon_phase(y, m, d, h, &ip);

  tft.drawString(moonPhase[ip], 120, 319);
  ui.drawBmp("/moon/moonphase_L" + String(icon) + ".bmp", 120 - 30, 318 - 16 - 60);

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

  tft.setTextPadding(0);  // Reset padding width to none
}

/***************************************************************************************
**                          Get the icon file name from the index number
***************************************************************************************/
const char* getMeteoconIcon(uint16_t id, bool today) {
  // if ( today && id/100 == 8 && (forecast->dt[0] < forecast->sunrise || forecast->dt[0] > forecast->sunset)) id += 1000;
  if (today && id / 100 == 8 && (now() < forecast->sunrise || now() > forecast->sunset)) id += 1000;
  // see issue https://github.com/Bodmer/OpenWeather/issues/26
  if (id / 100 == 2) return "thunderstorm";
  if (id / 100 == 3) return "drizzle";
  if (id / 100 == 4) return "unknown";
  if (id == 500) return "lightRain";
  else if (id == 511) return "sleet";
  else if (id / 100 == 5) return "rain";
  if (id >= 611 && id <= 616) return "sleet";
  else if (id / 100 == 6) return "snow";
  if (id / 100 == 7) return "fog";
  if (id == 800) return "clear-day";
  if (id == 801) return "partly-cloudy-day";
  if (id == 802) return "cloudy";
  if (id == 803) return "cloudy";
  if (id == 804) return "cloudy";
  if (id == 1800) return "clear-night";
  if (id == 1801) return "partly-cloudy-night";
  if (id == 1802) return "cloudy";
  if (id == 1803) return "cloudy";
  if (id == 1804) return "cloudy";

  return "unknown";
}

/***************************************************************************************
**                          Draw screen section separator line
***************************************************************************************/
// if you don't want separators, comment out the tft-line
void drawSeparator(uint16_t y) {
  tft.drawFastHLine(10, y, SCREEN_W - 2 * 10, 0x4228);
}

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

/***************************************************************************************
**                          Determine place to split a line line
***************************************************************************************/
// determine the "space" split point in a long string
int splitIndex(String text) {
  uint16_t index = 0;
  while ((text.indexOf(' ', index) >= 0) && (index <= text.length() / 2)) {
    index = text.indexOf(' ', index) + 1;
  }
  if (index) index--;
  return index;
}

/***************************************************************************************
**                          Right side offset to a character
***************************************************************************************/
// Calculate coord delta from end of text String to start of sub String contained within that text
// Can be used to vertically right align text so for example a colon ":" in the time value is always
// plotted at same point on the screen irrespective of different proportional character widths,
// could also be used to align decimal points for neat formatting
int rightOffset(String text, String sub) {
  int index = text.indexOf(sub);
  return tft.textWidth(text.substring(index));
}

/***************************************************************************************
**                          Left side offset to a character
***************************************************************************************/
// Calculate coord delta from start of text String to start of sub String contained within that text
// Can be used to vertically left align text so for example a colon ":" in the time value is always
// plotted at same point on the screen irrespective of different proportional character widths,
// could also be used to align decimal points for neat formatting
int leftOffset(String text, String sub) {
  int index = text.indexOf(sub);
  return tft.textWidth(text.substring(0, index));
}

/***************************************************************************************
**                          Draw circle segment
***************************************************************************************/
// Draw a segment of a circle, centred on x,y with defined start_angle and subtended sub_angle
// Angles are defined in a clockwise direction with 0 at top
// Segment has radius r and it is plotted in defined colour
// Can be used for pie charts etc, in this sketch it is used for wind direction
#define DEG2RAD 0.0174532925  // Degrees to Radians conversion factor
#define INC 2                 // Minimum segment subtended angle and plotting angle increment (in degrees)
void fillSegment(int x, int y, int start_angle, int sub_angle, int r, unsigned int colour) {
  // Calculate first pair of coordinates for segment start
  float sx = cos((start_angle - 90) * DEG2RAD);
  float sy = sin((start_angle - 90) * DEG2RAD);
  uint16_t x1 = sx * r + x;
  uint16_t y1 = sy * r + y;

  // Draw colour blocks every INC degrees
  for (int i = start_angle; i < start_angle + sub_angle; i += INC) {

    // Calculate pair of coordinates for segment end
    int x2 = cos((i + 1 - 90) * DEG2RAD) * r + x;
    int y2 = sin((i + 1 - 90) * DEG2RAD) * r + y;

    tft.fillTriangle(x1, y1, x2, y2, x, y, colour);

    // Copy segment end to segment start for next segment
    x1 = x2;
    y1 = y2;
  }
}

/***************************************************************************************
**                          Get 3 hourly index at start of next day
***************************************************************************************/
int getNextDayIndex(void) {
  int todayLocal = day(TIMEZONE.toLocal(now(), &tz1_Code));
  for (int index = 0; index < 9; index++) {
    if (day(TIMEZONE.toLocal(forecast->dt[index], &tz1_Code)) != todayLocal) return index;
  }
  return 0;
}

/***************************************************************************************
**                          Print the weather info to the Serial Monitor
***************************************************************************************/
void printWeather(void) {
#ifdef SERIAL_MESSAGES
  Serial.println("Weather from OpenWeather\n");

  Serial.print("city_name           : ");
  Serial.println(forecast->city_name);
  Serial.print("sunrise             : ");
  Serial.println(strTime(forecast->sunrise));
  Serial.print("sunset              : ");
  Serial.println(strTime(forecast->sunset));
  Serial.print("Latitude            : ");
  Serial.println(ow.lat);
  Serial.print("Longitude           : ");
  Serial.println(ow.lon);
  // We can use the timezone to set the offset eventually...
  Serial.print("Timezone            : ");
  Serial.println(forecast->timezone);
  Serial.println();

  if (forecast) {
    Serial.println("###############  Forecast weather  ###############\n");
    for (int i = 0; i < (MAX_DAYS * 8); i++) {
      Serial.print("3 hourly forecast   ");
      if (i < 10) Serial.print(" ");
      Serial.print(i);
      Serial.println();
      Serial.print("dt (time)        : ");
      Serial.println(strTime(forecast->dt[i]));

      Serial.print("temp             : ");
      Serial.println(forecast->temp[i]);
      Serial.print("temp.min         : ");
      Serial.println(forecast->temp_min[i]);
      Serial.print("temp.max         : ");
      Serial.println(forecast->temp_max[i]);

      Serial.print("pressure         : ");
      Serial.println(forecast->pressure[i]);
      Serial.print("sea_level        : ");
      Serial.println(forecast->sea_level[i]);
      Serial.print("grnd_level       : ");
      Serial.println(forecast->grnd_level[i]);
      Serial.print("humidity         : ");
      Serial.println(forecast->humidity[i]);

      Serial.print("clouds           : ");
      Serial.println(forecast->clouds_all[i]);
      Serial.print("wind_speed       : ");
      Serial.println(forecast->wind_speed[i]);
      Serial.print("wind_deg         : ");
      Serial.println(forecast->wind_deg[i]);
      Serial.print("wind_gust        : ");
      Serial.println(forecast->wind_gust[i]);

      Serial.print("visibility       : ");
      Serial.println(forecast->visibility[i]);
      Serial.print("pop              : ");
      Serial.println(forecast->pop[i]);
      Serial.println();

      Serial.print("dt_txt           : ");
      Serial.println(forecast->dt_txt[i]);
      Serial.print("id               : ");
      Serial.println(forecast->id[i]);
      Serial.print("main             : ");
      Serial.println(forecast->main[i]);
      Serial.print("description      : ");
      Serial.println(forecast->description[i]);
      Serial.print("icon             : ");
      Serial.println(forecast->icon[i]);

      Serial.println();
    }
  }
#endif
}
/***************************************************************************************
**             Convert Unix time to a "local time" time string "12:34"
***************************************************************************************/
String strTime(time_t unixTime) {
  time_t local_time = TIMEZONE.toLocal(unixTime, &tz1_Code);

  String localTime = "";

  if (hour(local_time) < 10) localTime += "0";
  localTime += hour(local_time);
  localTime += ":";
  if (minute(local_time) < 10) localTime += "0";
  localTime += minute(local_time);

  return localTime;
}

/***************************************************************************************
**  Convert Unix time to a local date + time string "Oct 16 17:18", ends with newline
***************************************************************************************/
String strDate(time_t unixTime) {
  time_t local_time = TIMEZONE.toLocal(unixTime, &tz1_Code);

  String localDate = "";

  localDate += monthShortStr(month(local_time));
  localDate += " ";
  localDate += day(local_time);
  localDate += " " + strTime(unixTime);

  return localDate;
}

/**The MIT License (MIT)
  Copyright (c) 2015 by Daniel Eichhorn
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYBR_DATUM HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
  See more at http://blog.squix.ch
*/

//  Changes made by Bodmer:

//  Minor changes to text placement and auto-blanking out old text with background colour padding
//  Moon phase text added (not provided by OpenWeather)
//  Forecast text lines are automatically split onto two lines at a central space (some are long!)
//  Time is printed with colons aligned to tidy display
//  Min and max forecast temperatures spaced out
//  New smart splash startup screen and updated progress messages
//  Display does not need to be blanked between updates
//  Icons nudged about slightly to add wind direction + speed
//  Barometric pressure added

//  Adapted to use the OpenWeather library: https://github.com/Bodmer/OpenWeather
//  Moon phase/rise/set (not provided by OpenWeather) replace with  and cloud cover humidity
//  Created and added new 100x100 and 50x50 pixel weather icons, these are in the
//  sketch data folder, press Ctrl+K to view
//  Add moon icons, eliminate all downloads of icons (may lose server!)
//  Adapted to use anti-aliased fonts, tweaked coords
//  Added forecast for 4th day
//  Added cloud cover and humidity in lieu of Moon rise/set
//  Adapted to be compatible with ESP32
