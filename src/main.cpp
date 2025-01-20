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

CRGB leds[NUM_LEDS + 1]; // + 1 for the onboard LED on pin 48
UITitle title("sutaburosu's FastLED 4 ESP32 Playpen");
UIDescription
    description("Making a mess with FastLED 4, ESP32, and some other stuff");
UISlider brightness("Brightness", BRIGHTNESS, 0, 255);
UINumberField fxIndex("Animartrix index", FIRST_ANIMATION, 0,
                      NUM_ANIMATIONS - 1);
UISlider timeSpeed("Time Speed", 2, -10, 10, .1);
UICheckbox switchFx("Switch Fx", true);

Animartrix animartrix(xyMap, FIRST_ANIMATION);
NoisePalette noisePalette1(xyMap);
NoisePalette noisePalette2(xyMap);
FxSui fxSui(xyMap);
FxEngine fxEngine(NUM_LEDS);

LD2450 ld2450;

void setup() {
    preferencesBegin();
    telemetry.begin();
    Serial.begin(preferences.getUInt("baudrate", 115200));
    Serial2.begin(LD2450_SERIAL_SPEED, SERIAL_8N1, 16, 17);
    ld2450.begin(Serial2, true);
    setupWiFi();
    setupWebServer();

    // 4 x 256 LEDs in 16x16 serpentine with LED0 in bottom left and LED1 above
    // it
    FastLED.addLeds<WS2812, 14, GRB>(leds, NUM_LEDS / 4);
    FastLED.addLeds<WS2812, 13, GRB>(leds, NUM_LEDS / 4, NUM_LEDS / 4);
    FastLED.addLeds<WS2812, 12, GRB>(leds, NUM_LEDS / 2, NUM_LEDS / 4);
    FastLED.addLeds<WS2812, 11, GRB>(leds, NUM_LEDS * 3 / 4, NUM_LEDS / 4);
    // FastLED.addLeds<WS2812, 48, GRB>(leds, NUM_LEDS, 1);

    fxEngine.addFx(animartrix);
    fxEngine.addFx(noisePalette1);
    fxEngine.addFx(noisePalette2);
    fxEngine.addFx(fxSui);
    fxSui.setEdgeDamping(255);
    // fxSui.setMovingStimulus(false);
    // fxSui.setRandomDrops(false);
    // fxSui.setRandomDropsRate(0);
    // fxSui.setWaveTank(false);
    // fxSui.setWaveTankRate(0);
    // fxSui.setWaveTankSpeed(0);

    noisePalette1.setPalettePreset(1);
    noisePalette1.setSpeed(3);
    noisePalette1.setScale(10);
    noisePalette2.setPalettePreset(4);

    // benchmarkXYmaps();

    // Confirm if radar reports are being received
    if (ld2450.read() < 4)
        Serial.printf("LD2450 radar active");

    // Set custom parameters for some telemetry data points
    telemetry.add("draw", {.minMs = 100, .unit = "ms", .teleplot = ""});
    telemetry.add("show", {.minMs = 100, .unit = "ms", .teleplot = ""});
    telemetry.add("nonFastLED", {.minMs = 100, .unit = "ms", .teleplot = ""});
    telemetry.add("fps", {.minMs = 100, .unit = "Hz", .teleplot = ""});
}

void draw() {
    // Apply any changed settings from the UI
    FastLED.setBrightness(brightness);
    fxEngine.setSpeed(timeSpeed);

    // Crossfade to a different effect
    EVERY_N_SECONDS(8) {
        if (switchFx) {
            fxEngine.nextFx(2000);
            const auto fxId = fxEngine.getCurrentFxId();
            telemetry.add("FxId", String(fxId));
            if (2 == fxId) {
                animartrix.fxNext();
                telemetry.add("AnimartrixFx", String(animartrix.fxGet()));
            }
        }
    }

    // Draw the current effect
    fxEngine.draw(millis(), leds);

    // onboard LED
    leds[NUM_LEDS] = CHSV(millis() / 16, 255, 128);

    // show the speed of any detected radar targets
    radar(leds, xyMap, ld2450);
}

void loop() {
    static uint64_t µsStart = 0;   // start of the first sample
    static uint64_t µsDraw = 0;    // total time spent drawing effects
    static uint64_t µsShow = 0;    // total time spent showing the leds
    static uint32_t µsSamples = 0; // number of samples taken
    if (!µsStart)
        µsStart = micros();

    // Draw the effects
    µsDraw -= micros();
    draw();
    µsDraw += micros();

    // Send the framebuffer to the LEDs
    µsShow -= micros();
    FastLED.show();
    µsShow += micros();

    // Handle OTA updates
    ElegantOTA.loop();

    // And our own ability to reboot (to test preferences and stuff)
    if (flags.restartPending) {
        preferences.end();
        ESP.restart();
    }

    if (flags.doConnectActions) {
        flags.doConnectActions = false;
        // It might be useful for effects to know when WiFi connects...
    }

    // Gather loop() timing data at most 5 times per second
    µsSamples++;
    uint64_t µsElapsed = micros() - µsStart;
    if (µsElapsed >= 200000) {
        float divisor = 1000.f * µsSamples;
        uint64_t µsNonFastLED = µsElapsed - µsDraw - µsShow;
        telemetry.add("draw", String(µsDraw / divisor));
        telemetry.add("show", String(µsShow / divisor));
        telemetry.add("nonFastLED", String(µsNonFastLED / divisor));
        telemetry.add("fps", String(µsSamples * 1000000.f / µsElapsed));
        µsSamples = µsShow = µsDraw = µsStart = 0;

        // Gather RAM usage, uptime, and WiFi signal data
        telemetry.sysStats();
    }
    // Send telemetry that has changed
    telemetry.send();
}
