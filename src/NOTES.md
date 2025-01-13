# Getting started with FastLED-wasm

I'm on Ubuntu 24.10 and I almost gave up at the first hurdle.

FastLED-wasm really wants the `uv` command but `apt` says it's not in the repos,
try `pip` instead. `pip` says `venv` might work, or I can break your system
packages? I flipped a coin and went with `venv`. After some reading and typing,
it worked. Don't do this.

It turns out `uv` does the same thing as `venv`, but with less reading and
typing. They can co-exist happily on your system. Using
`--break-system-packages` to install anything is usually *very bad idea*. It's
fine for `uv` because it doesn't pull in any other packages which may collide
with those provided by Ubuntu.

```bash
pip install uv --break-system-packages
git clone https://github.com/FastLED/FastLED
cd FastLED
uv pip install fastled
uv run fastled

# You should never need to update FastLED, but if you ever do:
uv pip install -U fastled
```

/// TODO search FastLEd debug macros
FASTLED_DBG("Failed to write frame data, wrote " << bytes_written << " bytes");


// Detect FastLED for WebAssembly (WASM)
#if defined(__EMSCRIPTEN__)


// Zack's String replacement.
// Not direct replacement, but portable and the best in class for embedded.
#include <fl/str.h> 
#typedef fl::Str String;

