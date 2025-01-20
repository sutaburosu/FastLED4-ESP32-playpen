# Getting started with FastLED-wasm

I'm on Ubuntu 24.10 and I almost gave up at the first hurdle.

FastLED-wasm really wants the `uv` command but `apt` says it's not in the repos,
try `pip` instead. `pip` says `venv` might work, or I can break your system
packages? I flipped a coin and went with `venv`. After some reading and typing,
it worked. Don't do this.

It turns out `uv` does the same thing as `venv`, but with less reading and
typing. They can co-exist happily on your system. Using
`--break-system-packages` to install anything is usually *a **very** bad idea*.
It's fine for `uv` because it doesn't pull in any other packages which may
collide with those provided by Ubuntu.

## Ubuntu

```bash
pip install uv --break-system-packages
git clone https://github.com/FastLED/FastLED
cd FastLED
./install

# You should never need to update fastled, but if you do:
uv pip install fastled -U
```

## Windows

Install the following:

 * [Docker Desktop](https://www.docker.com/products/docker-desktop/)
   * Accept the default settings during installation.

 * [VS Code](https://code.visualstudio.com/Download)
   * Accept the default settings during installation. You may want to select add Desktop Icon if you want one.

 * [Git](https://git-scm.com/downloads/win)
   * You may want to enable "Check daily for updates".

 * [Astral 'uv'](https://github.com/astral-sh/uv)
   * To install it, open a Powershell from your Start menu and paste this:

     `powershell -ExecutionPolicy ByPass -c "irm https://astral.sh/uv/install.ps1 | iex"`

### Video
Perhaps useful for screenshots.
 * [46MiB H.264](https://st4vs.net/Win11_FastLED4_install.mp4), browser compatible.
 * [187MiB "lossless" AV1](https://st4vs.net/Win11_FastLED4_install.mkv), 4:2:0 sub-sampled. I can't get OBS to record 4:4:4 using *any* codec.

Video: show how to find documentation.

Video: show how to find examples.

Video: approach Scott Marley

## Both Windows and Linux

pio ext
** TODO **
clone FastLED

```bash
cd FastLED
./install
```
 * `uv venv`
 * `uv run fastled` (and Ctrl-C to exit it)




<br><br><br><br><br><br>
<hr>

# TODOs

FxSui is now giving this. Can I get more of those addresses resolved?
I fixed my out-of-bounds ptr access. But, I'd still like a line
number in future.

```log
assert failed: block_locate_free tlsf.c:566 (block_size(block) >= size)


Backtrace: 0x40377216:0x3fcae310 0x4037e601:0x3fcae330 0x40385071:0x3fcae350 0x403842e6:0x3fcae480 0x40383c7d:0x3fcae4a0 0x40383de0:0x3fcae4c0 0x40377533:0x3fcae4e0 0x40377555:0x3fcae510 0x403776c1:0x3fcae530 0x420037d3:0x3fcae550 0x420037fd:0x3fcae570 0x4200475b:0x3fcae590 0x42005f89:0x3fcae5b0 0x42006259:0x3fcae5d0 0x4200657e:0x3fcae620 0x420065be:0x3fcae640 0x42005106:0x3fcae670 0x42004a1d:0x3fcae6b0 0x42004bfd:0x3fcae6f0 0x42004fcd:0x3fcae730 0x42012f0a:0x3fcae770 0x42013c8a:0x3fcae7c0 0x42020ae0:0x3fcae860 0x4037f316:0x3fcae880
  #0  0x42012f0a in draw() at src/main.cpp:103 (discriminator 1)
  #1  0x42013c8a in loop() at src/main.cpp:123
  #2  0x42020ae0 in loopTask(void*) at /home/steve/.platformio/packages/framework-arduinoespressif32/cores/esp32/main.cpp:74
  #3  0x4037f316 in vPortTaskWrapper at /home/runner/work/esp32-arduino-lib-builder/esp32-arduino-lib-builder/esp-idf/components/freertos/FreeRTOS-Kernel/portable/xtensa/port.c:139
```


## FastLED debug macros

`FASTLED_DBG("Failed to write frame data, wrote " << bytes_written << " bytes");`


#### Detect FastLED for WebAssembly (WASM)
```cpp
#if defined(__EMSCRIPTEN__)
// compiling inside WebAssembly (WASM)
#endif
```


#### Zack's String replacement.
 Not direct replacement, but portable and the best in class for embedded.

```cpp
#include <fl/str.h>
#typedef fl::Str String;
```

#### Ongoing tasks
/r/FastLED subscribers over time.
[Graphs, which appear to be accurate](https://reddstats.com/subreddit/FastLED)
even after the Reddit's 2024 API clampdown. Refresh
[archive.org](https://web.archive.org/web/20250117020022/https://reddstats.com/subreddit/FastLED)
too.

| Date       | Subscribers  | Change in 2024 |
|------------|--------------|----------------|
| 2024-01-01 | 16,086       |                |
| 2025-01-01 | 18,307       | +2,221         |
| 2025-01-17 | 18,441       |                |
