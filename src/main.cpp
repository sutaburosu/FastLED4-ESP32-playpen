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
#define XY_CONFIG (xySerpentine | xyColumnMajor | xySerpentineTiling)

#define FIRST_ANIMATION RGB_BLOBS5

#include "main.hpp"

using namespace fl;

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

LD2450 ld2450;
CRGB leds[NUM_LEDS + 1]; // + 1 for the onboard LED on pin 48

void setup()
{
  // 4 x 256 LEDs in 16x16 serpentine with LED0 in bottom left and LED1 above it
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

  setupPreferences();

  Serial.begin(preferences.getUInt("baudrate", 115200));
  Serial2.begin(LD2450_SERIAL_SPEED, SERIAL_8N1, 16, 17);
  ld2450.begin(Serial2, true);

  benchmarkXYmaps();

  setupWiFi();
  setupWebServer();

  // Confirm if radar reports are being received
  if (ld2450.read() < 4)
    Serial.printf("LD2450 radar active");
}

void loop()
{
  static uint64_t µsStart = 0;   // start of the first sample
  static uint64_t µsDraw = 0;    // total time spent drawing effects
  static uint64_t µsShow = 0;    // total time spent showing the leds
  static uint32_t µsSamples = 0; // number of samples taken
  if (!µsStart)
    µsStart = micros();

  // Apply any changed settings from the UI
  FastLED.setBrightness(brightness);
  fxEngine.setSpeed(timeSpeed);

  // Crossfade to a different effect
  EVERY_N_SECONDS(8)
  {
    if (switchFx)
    {
      fxEngine.nextFx(2000);
      telemetry.log("FxId", {.minMs = 2000,
                             .value = String(fxEngine.getCurrentFxId()),
                             .teleplot = "np"});
      if (2 == fxEngine.getCurrentFxId())
      {
        animartrix.fxNext();
        telemetry.log("AnimartrixFx", {.minMs = 2000,
                                       .value = String(animartrix.fxGet()),
                                       .teleplot = "np"});
      }
    }
  }

  // Draw the current effect
  µsDraw -= micros();
  fxEngine.draw(millis(), leds);
  // onboard LED
  leds[NUM_LEDS] = CHSV(millis() / 16, 255, 128);
  // show the speed of any detected radar targets
  radar(leds, xyMap, ld2450);
  µsDraw += micros();

  // Send the framebuffer to the LEDs
  µsShow -= micros();
  FastLED.show();
  µsShow += micros();

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

  // Gather loop() timing data at most 5 times per second
  µsSamples++;
  if (micros() - µsStart >= 200000)
  {
    uint64_t other = (micros() - µsStart) - µsDraw - µsShow;
    telemetry.log("draw",
                  {.minMs = 250,
                   .value = String(µsDraw / 1000.f / µsSamples),
                   .unit = "ms"});
    telemetry.log("show",
                  {.minMs = 250,
                   .value = String(µsShow / 1000.f / µsSamples),
                   .unit = "ms"});
    telemetry.log("nonFastLED",
                  {.minMs = 250,
                   .value = String(other / 1000.f / µsSamples),
                   .unit = "ms"});
    telemetry.log("fps",
                  {.minMs = 250,
                   .value = String(µsSamples * 1000000.f / (micros() - µsStart)),
                   .unit = "Hz"});
    µsSamples = µsShow = µsDraw = µsStart = 0;
  }

  // Gather RAM, uptime and WiFi signal data and send all telemetry
  sysStats();
  telemetry.send();
}
