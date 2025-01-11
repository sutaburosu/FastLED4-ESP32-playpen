# FastLED4-ESP32-playpen

### Overview

##### FastLED 4 adds many new features. Using them is my goal here.
  * `XYMap` for 2D arrays of LEDs.
    * Supports user functions for unusual mappings. Perfect for my multi-panel
    mapper.
      * TODO: can this be cached in a lookup table like the other mappings?
  * `Fx` engine to encapsulate animation effects and chain them together.
    * `Fx.next()` cross-fades between effects.
    * Built-in `Fx` effects can be easily be used in your own sketches.
    * Add your own effects to the `Fx` engine.
      * My old sketch Water has seen drastic aesthetic improvement compared to
      the version in FastLED's examples folder. TODO: actually move the new
      code into this repo.
  * `fled` compiles some FastLED sketches into wasm, runs them in a browser and
  shows the result on simulated LED strips/matrices.
    * I got it running, along with the local compiler.
      * You get feedback a little faster than pushing a .bin via OTA.
      * Way faster build time & execution speed than [Wokwi](https://wokwi.com/),
      *if* your sketch works with it. 
  * FastLED 3.9.10 glitches the RMT output when using Serial/WiFi.
    * Tried queueing and streaming Serial data at just a few bytes per loop().
    Still glitchy.
    * Moved all Serial output to UDP.
      * Coalesced data to reduce packet rate.
      * Tried limiting packet rate and/or data rate. Still glitchy.
    * Even tiny, infrequent WiFi/Serial TX glitch the RMT output sooner or later.
      * I haven't done enough Serial/WiFi RX to know if that's a problem too.
      Or ESPNow. Presumably other interrupt sources can causes glitches too.

##### ESP32 is new to me, so I'm playing around with interesting looking stuff.
  * NVS storage for configuration data. Keep your secrets secret by putting
  them in the encrypted storage of the MCU rather than in plaintext somewhere
  in your sketch.
  * OTA updates, faster than flashing over 2Mbaud Serial.
  * LD2450 radar sensor for presence detection, displayed on the LEDs. I now
  have an ugly visual indicator of the speed I'm moving around the room.
    * TODO: try EspNOW so I can put the radar sensor somewhere useful and send
    the data to the device running the LEDs. Maybe offer UDP as an alternative
    transport so I can compare mixed EspNow/WiFi to pure WiFi.
  * Does it have a built-in temperature sensor? I know the numbers won't be
  accurate, but it's a fun thing to have.

##### PlatformIO, network code, html/css/js, and C++ standard library are all new to me.
  * [Teleplot](https://github.com/nesnes/teleplot) is a massive improvement over
  reading logs in a terminal, and just as easy to use in your code. Try it!
    * Graphs of FPS, µs spent in show(), free memory, whatever you want.
    * TODO: play with [Telecmd remote function calls](https://github.com/nesnes/teleplot?tab=readme-ov-file#remote-function-calls),
    although I think WebSockets is likely to be a better idea for remote control.
  * I resolved a hostname, sent a UDP packet, and served my first useful webpage
  with buttons that do things.
  * I used std::deque, std::map, lambdas, iterators, simple templates, and raw
  literal strings. I'm not sure I used them well, but I used them.
  * I wrote something that [others might find useful](src/telemetry.hpp). It
  should probably be a class; that struct grew legs.
  * I still can't keep a project tidy.

### Installation

Edit the `setPreferences()` function in `main.cpp` to set your WiFi details,
etc.  These will be stored in NVS, so you can remove them from the sketch as
soon as you've set them.

With no stored preferences, the sketch will pause at startup for you to enter
WiFi config via Serial.

Set the entire matrix and per-panel dimensions at the top of `main.cpp`.
Edit `setup()` to set the data pins and leds[] data slices.