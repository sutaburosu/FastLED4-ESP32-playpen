# FastLED4-ESP32-playpen

### Overview

##### FastLED 4 adds many new features. Learning to use them is my main goal here.
  * `XYMap` works for string(s) of LEDs forming *any* arrangement in 2D.
    * Take a photo of your custom layout and click on the LEDs. That's how easy
      it is to create an `XYMap` with FastLED's
      [great new web tool](https://ledmapper.com).
    * For a LED matrix there is `XYMap::constructRectangularGrid` which
      constructs a map for horizontal raster or serpentine layouts. It works
      when LED0 is in the top-left corner, and LED1 is to the right of it.
    * For other matrix layouts, there is `XYMap::constructWithUserFunction`
      which takes a function very similar to `XY` used in FastLED 3.
      * [This file](src/XY.hpp) contains `XYpanel` and `XYpanels`, more flexible
        single- and multi-panel mappers I created for `XYMap`.
      * If your programmatic mapper is complex and slow, you can cache the
        results with `XYMap::convertToLookUpTable`. **TODO**: do this and show
        benchmarks
    * Mappings can be applied in two places:
      * `addLeds<>().setScreenMap(xyMap);`
      * `NoisePalette noisePalette1(xyMap);`
  * `Fx` engine encapsulates animation effects and allows you chain them
    together.
    * `Fx.next()` cross-fades between effects.
    * Built-in `Fx` effects can be easily be used in your own sketches.
    * Add your own effects to the `Fx` engine.
      * My old sketch Water has seen drastic aesthetic improvement compared to
        the version in FastLED's examples folder. TODO: actually port the new
        code to `Fx`.
  * **TODO** what was the new name again?`fastled` is a command line tool for
    compiling FastLED sketches. It requires no configuration. It automatically
    compiles your sketch each time you save your changes. A brief moment later,
    the updated sketch runs in your web browser and shows simulated LEDs.
    * You can add `UI` buttons, sliders, etc with just a few lines of code in
      your sketch. They will appear in your browser, and change the value of
      variables in your sketch when pressed, slid, etc.
    * You get feedback *much* faster than uploading over USB, pushing a .bin via
      OTA, or building in Wokwi. On my machine, it's less than 3s from saving my
      changes to seeing the result in a browser.
    * Way faster build time & execution speed than [Wokwi](https://wokwi.com/),
      *if* your sketch works with it.
      * Many sketches work unchanged. Some need minor changes. A few won't work
        at all yet. The details are complicated. Sketches that use only FastLED
        and SDcard should work. Using other libraries reduces your chances of
        success.
  * TODO: custom transitions between effects.
    * It looks like only cross-fades/dissolves are supported currently.
    * I want to try a few different transitions, like wipes/slides/zooms.
      * Do I just sub-class Transition? How do I get Fx.next() to use it?
      * Easing functions add flavour and variety to transitions. Are FastLED's
        existing easing functions useful here? Enough resolution?
  * TODO: frame interpolation. I *need* to play with this.
    * time-stretching to smooth low-FPS CPU-intensive effects.
      * is progressive rendering possible? e.g. render ¼ of the frame per
        loop(), submit a complete frame every 4th loop(), and have the
        interpolation engine fill in.
    * time-stretching and bouncing/looping/glitching for artistic effect.
  * FastLED 3.9.10 glitches the RMT output when using Serial/WiFi.
    * Tried queueing and streaming Serial data at just a few bytes per loop().
      Still glitchy.
    * Moved all Serial output to UDP.
      * Coalesced data to reduce packet rate.
      * Tried limiting packet rate and/or data rate. Still glitchy.
    * Even tiny, infrequent WiFi/Serial TX glitch the RMT output sooner or
      later.
      * I haven't done enough Serial/WiFi RX to know if that's a problem too. Or
        ESPNow. Presumably other interrupt sources can causes glitches too.

##### ESP32 is new to me, so I'm playing around with interesting looking stuff.
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

##### PlatformIO, network code, html/css/js, and C++ standard library are all new to me.
  * The [Teleplot extension for VSCode](https://github.com/nesnes/teleplot) is a
    **massive** improvement over reading logs in a terminal, and just as easy to
    use as Serial.println(). Try it!
    * I wrote something that [others might find useful](src/telemetry.hpp). Your
      sketch can report via Serial and/or WiFi, queue startup messages to deliver
      when WiFi connects, and more. This may become a separate project if
      there's sufficient interest.
        * Usage can be as simple as:
          ```cpp
          #include "telemetry.hpp"

          void setup() {
              telemetry.begin();
          }

          void loop() {
            telemetry.add("WiFi RSSI", String(WiFi.RSSI()));`
            telemetry.send();
          }
          ```
      * Graphs of FPS, µs spent in show(), free memory, whatever you want, and a
      separate stream for logging diagnostic messages so they don't scroll
      unheeded into the void.
      * TODO: play with
      [Telecmd remote function calls](https://github.com/nesnes/teleplot?tab=readme-ov-file#remote-function-calls),
      although I think WebSockets is likely to be a better idea for remote
      control.
  * I resolved a hostname, sent a UDP packet, and served my first useful webpage
    with buttons that do things.
  * I used std::deque, std::map, lambdas, iterators, simple templates, and raw
    literal strings. I'm not sure I used them well, but I used them.

### Installation

Edit the `storePreferences()` function in
[preferences.hpp](src/preferences.hpp) to set your WiFi details, etc. Undo your
edits after you've uploaded it. Your preferences are stored in NVS, so *should*
remain on the device after uploading different sketches if the partition table
remains the same.

If no stored preferences are found, the sketch will pause at startup for you
to enter WiFi config via Serial.

Set the entire matrix and per-panel dimensions at the top of `main.cpp`.
Edit `setup()` to set the data pins and leds[] data slices.