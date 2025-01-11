/*
    sutaburosu's FastLED 4 ESP32-S3 playpen
*/

#define FASTLED_OVERCLOCK_SUPPRESS_WARNING
#define FASTLED_OVERCLOCK 1.42
// #define FASTLED_WS2812_T1 250
// #define FASTLED_WS2812_T2 500
// #define FASTLED_WS2812_T3 250

#define BRIGHTNESS 48
#define MATRIX_WIDTH 32
#define MATRIX_HEIGHT 32
#define NUM_LEDS (MATRIX_WIDTH * MATRIX_HEIGHT)

#define PANEL_WIDTH 16
#define PANEL_HEIGHT 16
#define XY_CONFIG (XY_Serpentine | XY_ColumnMajor | XY_SerpentineTiling)

#define FIRST_ANIMATION RGB_BLOBS5

#include "preferences.hpp"
#include "main.hpp"
#include "XY.hpp"
#include "LD2450.h"
#include "wifi.hpp"
#include "web_pages.hpp"
#include "fxWater.hpp"

using namespace fl;


// select the multi-panel mapper only when it is required
#if defined(PANEL_WIDTH) && defined(PANEL_HEIGHT) && ((PANEL_WIDTH != MATRIX_WIDTH) || (PANEL_HEIGHT != MATRIX_HEIGHT))
XYMap xyMap = XYMap::constructWithUserFunction(
    MATRIX_WIDTH, MATRIX_HEIGHT,
    XY_panels<XY_CONFIG, MATRIX_WIDTH, MATRIX_HEIGHT, PANEL_WIDTH, PANEL_HEIGHT>);
#else
XYMap xyMap = XYMap::constructWithUserFunction(
    MATRIX_WIDTH, MATRIX_HEIGHT,
    XY_panel<XY_CONFIG, MATRIX_WIDTH, MATRIX_HEIGHT>);
#endif

UITitle title("sutaburosu's FastLED 4 ESP32 Playpen");
UIDescription description("Making a mess with FastLED 4, ESP32, and some other stuff");
UISlider brightness("Brightness", BRIGHTNESS, 0, 255);
UINumberField fxIndex("Animartrix - index", FIRST_ANIMATION, 0, NUM_ANIMATIONS - 1);
UISlider timeSpeed("Time Speed", 2, -10, 10, .1);
UICheckbox switchFx("Switch Fx", true);

Animartrix animartrix(xyMap, FIRST_ANIMATION);
NoisePalette noisePalette1(xyMap);
NoisePalette noisePalette2(xyMap);
FxEngine fxEngine(NUM_LEDS);

AsyncWebServer server(80);
LD2450 ld2450;
CRGB leds[NUM_LEDS + 1]; // + 1 for the onboard LED on pin 48

void setup()
{
  setupPreferences();

  Serial.begin(preferences.getUInt("baudrate", 115200));
  Serial.printf("Total PSRAM: %lu\n", ESP.getPsramSize());
  Serial.printf("Free PSRAM: %lu\n", ESP.getFreePsram());
  Serial.printf("Total heap: %lu\n", ESP.getHeapSize());
  Serial.printf("Free heap: %lu\n", ESP.getFreeHeap());
  benchmark_xymaps();

  flags.serialTelemetry = preferences.getBool("serialTelemetry", true);
  flags.UDPTelemetry = preferences.getBool("UDPTelemetry", false);

  Serial2.begin(LD2450_SERIAL_SPEED, SERIAL_8N1, 16, 17);
  ld2450.begin(Serial2, true);

  wifi_setup();

  FastLED.addLeds<WS2812, 14, GRB>(leds, NUM_LEDS / 4);
  FastLED.addLeds<WS2812, 13, GRB>(leds, NUM_LEDS / 4, NUM_LEDS / 4);
  FastLED.addLeds<WS2812, 12, GRB>(leds, NUM_LEDS / 2, NUM_LEDS / 4);
  FastLED.addLeds<WS2812, 11, GRB>(leds, NUM_LEDS * 3 / 4, NUM_LEDS / 4);
  // FastLED.addLeds<WS2812, 48, GRB>(leds, NUM_LEDS, 1);

  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear();

  fxEngine.addFx(animartrix);
  fxEngine.addFx(noisePalette1);
  fxEngine.addFx(noisePalette2);
  noisePalette1.setPalettePreset(1);
  noisePalette1.setSpeed(3);
  noisePalette1.setScale(10);
  noisePalette2.setPalettePreset(4);

  // Confirm that radar reports are being received
  if (ld2450.read() < 4)
    Serial.printf("LD2450 radar active");

  // Set up web server and OTA updates
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/html", indexContent); });
  server.on("/restart", HTTP_GET, [](AsyncWebServerRequest *request)
            { flags.restartPending = true;
              request->send(200, "text/plain", "Restarting..."); });
  server.on("/telemetryon", HTTP_GET, [](AsyncWebServerRequest *request)
            { flags.serialTelemetry = true;
              preferences.putBool("serialTelemetry", true);
              request->send(200, "text/plain", "Serial telemetry on"); });
  server.on("/telemetryoff", HTTP_GET, [](AsyncWebServerRequest *request)
            { flags.serialTelemetry = false;
              preferences.putBool("serialTelemetry", false);
              request->send(200, "text/plain", "Serial telemetry off"); });
  server.on("/UDPtelemetryon", HTTP_GET, [](AsyncWebServerRequest *request)
            { flags.UDPTelemetry = true;
              preferences.putBool("UDPTelemetry", true);
              request->send(200, "text/plain", "UDP telemetry on"); });
  server.on("/UDPtelemetryoff", HTTP_GET, [](AsyncWebServerRequest *request)
            { flags.UDPTelemetry = false;
              preferences.putBool("UDPTelemetry", false);
              request->send(200, "text/plain", "UDP telemetry off"); });

  ElegantOTA.begin(&server);
  ElegantOTA.onStart(onOTAStart);
  ElegantOTA.onProgress(onOTAProgress);
  ElegantOTA.onEnd(onOTAEnd);
  server.begin();
}

void loop()
{
  static uint64_t us_total_draw = 0; // total time spent drawing effects
  static uint64_t us_total_show = 0; // total time spent showing the leds
  static uint64_t us_start = 0;      // start of the first sample
  static uint32_t us_samples = 0;    // number of samples taken
  if (!us_start)
    us_start = micros();

  // Handle OTA updates
  ElegantOTA.loop();

  // And our own ability to reboot (to test preferences and stuff)
  if (flags.restartPending)
  {
    preferences.end();
    ESP.restart();
  }

  if (flags.doConnectActions)
  {
    flags.doConnectActions = false;
    // It might be useful for effects to know when WiFi connects...
  }

  // Apply any changed settings from the UI
  FastLED.setBrightness(brightness);
  fxEngine.setSpeed(timeSpeed);

  // Crossfade to a different effect
  EVERY_N_SECONDS(8)
  {
    if (switchFx)
    {
      fxEngine.nextFx(2000);
      telemetry.update("FxId", String(fxEngine.getCurrentFxId()),
                       "", "np", 1000, 60000, false);
      if (2 == fxEngine.getCurrentFxId())
      {
        animartrix.fxNext();
        telemetry.update("AnimartrixFx", String(animartrix.fxGet()),
                         "", "np", 1000, 60000, false);
      }
    }
  }

  us_total_draw -= micros();
  if (false)
  {
    static int ledno = 0;
    leds[ledno] = CRGB::Black;
    ledno = (ledno + 1) % NUM_LEDS;
    leds[ledno] = CRGB::White;
  }
  else
  {
    fxEngine.draw(millis(), leds);
  }
  // onboard LED
  leds[NUM_LEDS] = CHSV(millis() / 16, 255, 128);
  radar();
  us_total_draw += micros();

  // Send the framebuffer to the LEDs
  us_total_show -= micros();
  FastLED.show();
  us_total_show += micros();

  // Send frame timing telemetry at most 5 times per second
  us_samples++;
  if (micros() - us_start >= 200000)
  {
    telemetry.update("RSSI", String(WiFi.RSSI()), "dBm", "", 250);
    telemetry.update("draw", String(us_total_draw / us_samples), "µs", "", 250);
    telemetry.update("show", String(us_total_show / us_samples), "µs", "", 250);
    telemetry.update("fps", String(us_samples * 1000000 / (micros() - us_start)), "Hz", "", 250);
    us_samples = us_total_show = us_total_draw = us_start = 0;
  }

  low_freq_stats();
  telemetry.send();
}

void radar()
{
  const int sensor_got_valid_targets = ld2450.read();
  if (sensor_got_valid_targets <= 0)
  {
    // Serial.print('.');
    return;
  }

  uint8_t printcnt = 0;
  static uint16_t pos[3]{0, 0, 0};
  static uint32_t last_millis[3]{0, 0, 0};
  for (int i = 0; i < ld2450.getSensorSupportedTargetCount(); i++)
  {
    const LD2450::RadarTarget result_target = ld2450.getTarget(i);
    // if (0 == i)
    //   result_target.valid = true, result_target.speed = sinf(millis() / 1000.f) * 100;

    if (result_target.valid && abs(result_target.speed) > 0)
    {
      printcnt++;
      // Serial.printf("id:%u xy:%5d,%5d res:%4u spd:%4d dst:%5u (%d %d, %d %d)  ",
      //               result_target.id, result_target.x, result_target.y, result_target.resolution, result_target.speed, result_target.distance,
      //               result_target.xmin, result_target.xmax, result_target.ymin, result_target.ymax);
      if (last_millis[i] - millis() >= 50)
      {
        if (result_target.speed > 0)
          pos[i] += result_target.speed;
        else
          pos[i] -= -result_target.speed;
        last_millis[i] = millis();
      }
    }
    // Reset if target is not moving
    if (last_millis[i] && (millis() - last_millis[i] > 5000))
    {
      last_millis[i] = 0;
    }
    if (last_millis[i])
    {
      // plot a scroller indicating the speed
      uint16_t ipos = pos[i];
      for (int x = 0; x < MATRIX_WIDTH; x++)
      {
        leds[xyMap.mapToIndex(x, i)] = CHSV(millis() / 32 + i * 86, 255, ((ipos >> 10) % 8) * 32);
        ipos += 256 * 4; // * (result_target.speed >= 0 ? 1 : -1);
      }
    }
  }
  if (printcnt > 0)
  {
    // Serial.println();
  }
}
