# FastLED4-ESP32-playpen

## More and better hardware drivers in FastLED 4
There are many great new features in FastLED 4 which each bring major
improvements compared to FastLED 3.8.

 * There are several new output drivers, making FastLED 4 the best choice of
   library for driving vast numbers of LEDs on many modern MCUs. Old MCUs continue to be well supported so far as their capabilities allow.
 * Some of FastLED's existing drivers are now asynchronous, so your sketch
   can do other things whilst the LED data is sent in the background.
 * On some platforms, FastLED can now overclock your LEDs, so you can get more
   frames per second out of them. There are reports of 40-60% more FPS with the
   same LEDs. My LEDs can go 42% faster with FastLED using the simple #define,
   but 65%
   [with my own code](https://gist.github.com/sutaburosu/89920c71a9635fec595e243fa6e7360e).
   **TODO** I don't understand what I should set the T1, T2, T3 defines to for
   FastLED to achieve the same 250/500 500/250ns timings. This was discussed in
   an issue; find it again.

This is all very shiny, new, and exciting. But what excites me *most* are the new
features for developing animations and effects. I'm going to concentrate on
those in this document and the code in this repo.

## `XYMap`
  * `XYMap` works for string(s) of LEDs forming *any* arrangement in 2D.
    * Take a photo of your custom layout and click on the LEDs. That's how easy
      it is to create an `XYMap` with FastLED's
      [great new web tool](https://ledmapper.com).
    * For a LED matrix there is `XYMap::constructRectangularGrid` which
      constructs a map for horizontal raster or serpentine layouts. It works
      when LED0 is in the bottom-left corner, and LED1 is to the right of it.
    * For other matrix layouts, there is `XYMap::constructWithUserFunction`
      which takes a function very similar to `XY` used in FastLED 3.
      * **TODO** FastLED appears to have the origin at the bottom-left corner,
        with increasing X to the right and increasing Y upwards. Wherever I say
        raster layout, assume I mean inverted raster.
      * [This file](src/XY.hpp) contains `XYpanel` and `XYpanels`, more flexible
        single- and multi-panel mappers I created for `XYMap`.
      * If a programmatic mapper is used, consider caching the results in a
        lookup-table for speed. Just call the `convertToLookUpTable()` method on
        the XYMap, then continue using the XYMap as normal.
    * Mappings can be applied in two places:
      * `NoisePalette noisePalette1(xyMap);`
    * It's really nice to finally have a standard way to handle 2D layouts.

## FX engine
* `Fx` engine encapsulates animation effects and allows you chain them
  together.
  * `Fx.next()` cross-fades between effects.
  * Built-in `Fx` effects can be easily be used in your own sketches.
  * Add your own effects to the `Fx` engine.
    * I ported my old sketch
      [Water](https://github.com/FastLED/FastLED/blob/master/examples/FxWater/FxWater.ino)
      to `Fx`. It's now called [水 (sui)](src/fxSui.hpp) and I think it
      [looks much better](https://st4vs.net/sui/) than before.
* **TODO** I want to use one effect as an input for another, e.g. distort
  the output of any other effect. I don't think there's a way to chain
  effects like that yet.
  * I haven't yet tried to do this, but calling draw() on another Fx into a
    temp CRGB buffer should work I think.
* **TODO** custom transitions between effects.
  * It looks like only cross-fades/dissolves are supported currently.
  * I want to try a few different transitions, like wipes/slides/zooms.
    * Do I just sub-class Transition? How do I get Fx.next() to use it?
    * Easing functions add flavour and variety to transitions. Are FastLED's
      existing easing functions useful here? Enough resolution?
* **TODO** frame interpolation. I *need* to play with this.
  * frame generation to smooth low-FPS CPU-intensive effects.
    * is progressive rendering possible? e.g. render ¼ of the frame per
      loop(), submit a complete frame every 4th loop(), and have the
      interpolation engine fill in.
  * time-stretching and bouncing/looping/glitching for artistic effect.


## WASM  **todo** what was the new name again?
  * `fastled` is a command line tool for
    compiling FastLED sketches. It automatically compiles your sketch each time
    you save your changes. A brief moment later, the updated sketch runs in your
    web browser and shows your sketch running on simulated LEDs.
    * You can add `UI` buttons, sliders, etc with just a few lines of code in
      your sketch. They will appear in your browser, and change the value of
      variables in your sketch when pressed, slid, etc.
    * You get feedback *much* faster than uploading over USB or pushing a .bin
      via OTA. On my machine, it's less than 3s from saving my changes to seeing
      the results.
    * Many sketches work unchanged. Some need minor changes. A few won't work at
      all yet. The details are complicated. Sketches that use only FastLED and
      SDcard should work. Sketches using other libraries may not.

### UI and `ScreenMap`
**TODO**
  * `addLeds<>().setScreenMap(xyMap);`


## ESP32 is new to me, so I'm playing around with interesting looking stuff.
  * There is a good amount code in this repo that is not directly related to
    FastLED. I've tried to keep it separated so as not to confuse the
    reader.
  * NVS storage for configuration data. Keep your secrets secret by putting them
    in the encrypted storage of the MCU rather than in plaintext somewhere in
    your sketch.
  * OTA updates, faster than flashing over 2Mbaud Serial.
  * LD2450 radar sensor for presence detection, displayed on the LEDs. I now
    have an ugly visual indicator of the speed I'm moving around the room.
    * TODO: try EspNOW so I can put the radar sensor somewhere useful and send
      the data to the device running the LEDs. Maybe offer UDP as an alternative
      transport so I can compare mixed EspNow/WiFi to pure WiFi.
  * Does it have a built-in temperature sensor? I know the numbers won't be
    accurate, but it's a fun thing to have.

### PlatformIO, network code, html/css/js, and C++ standard library are also all new to me.
  * It is impossible to overstate how **massively better** the
    [Teleplot extension for VSCode](https://github.com/nesnes/teleplot) is
    compared to reading logs in a terminal. Try it!
    * I am so enamoured of Teleport, I wrote [Telemetry](src/telemetry.hpp). It
      sends to Teleplot via Serial and/or WiFi, queues startup messages to
      deliver when WiFi connects, and more. It is a little more capable than the
      official library in some ways, but doesn't carry the whole 2D/3D shapes
      baggage. This may become a separate project if there's sufficient
      interest.
        * If you just want graphs of RAM/PSRAM usage can be as simple as:
          ```cpp
          #include "preferences.hpp"
          #include "telemetry.hpp"

          void setup() {
              preferences.begin();
              telemetry.begin();
          }

          void loop() {
            telemetry.send();
          }
          ```
        * Graphs of FPS, µs spent in show(), free memory, whatever you want, and
          a separate stream for logging diagnostic messages so they don't scroll
          unheeded into the void.
        * **TODO**: play with
          [Teleplot/Telecmd remote function calls](https://github.com/nesnes/teleplot?tab=readme-ov-file#remote-function-calls),
          although I think WebSockets is likely to be a better idea for remote
          control.
  * I resolved a hostname, sent a UDP packet, and served my first useful webpage
    with buttons that do things.
  * I used `std::deque`, `std::map`, lambdas, iterators, simple templates, and
    raw literal strings. I'm not sure I used them well, but I used them.

## Installation

Edit the `storePreferences()` function in
[preferences.hpp](src/preferences.hpp) to set your WiFi details, etc. Undo your
edits after you've uploaded it. Your preferences are stored in NVS, so *should*
remain on the device after uploading different sketches if the partition table
remains the same.

If no stored preferences are found, the sketch will pause at startup for you
to enter WiFi config via Serial.

Set the entire matrix and per-panel dimensions at the top of `main.cpp`.
Edit `setup()` to set the data pins and leds[] data slices.