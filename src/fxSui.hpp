/*

This is a 2D FastLED FX engine effect which draws a water ripple effect.

It is derived from the FastLED example "FxWater" so I had to pick a name
that was not already taken. I chose 水 (sui) which means water.

The water ripple effect is based on the algorithm described here:
https://web.archive.org/web/20160418004149/http://freespace.virgin.net/hugo.elias/graphics/x_water.htm

*/

#pragma once
#include <FastLED.h>
// #include "fl/dbg.h"
#include "fl/namespace.h"
#include "fl/ptr.h"
#include "fl/scoped_ptr.h"
#include "fl/xymap.h"
#include "fx/fx2d.h"

// TODO add pause and resume methods and free memory when paused?
namespace fl {
FASTLED_SMART_PTR(FxSui);

// A 2D FastLED FX engine effect which draws a water ripple effect.
class FxSui : public Fx2d {
  private:
    // Private variables and methods can only be used within this class
    uint16_t width;                   // width of the XYMap
    uint16_t height;                  // height of the XYMap
    uint32_t wwidth;                  // width of the water buffer
    uint32_t wheight;                 // height of the water buffer
    uint32_t wsize;                   // size of a single water buffer
    fl::scoped_array<uint8_t> waterA; // temporary buffer for water simulation
    fl::scoped_array<uint8_t> waterB; // temporary buffer for water simulation
    uint8_t edgeDamping;              // affects reflections at the edges
    bool buffer = false;              // used to swap buffers on each frame
    uint16_t phase[3];                // phase offsets for the moving stimulus
    uint8_t *buffptr[2];              // pointer to the water buffer
    uint16_t tankPhase = 0;           // phase offset for the wave tank

    // These methods are defined below this Class declaration
    void setPerimeter();
    uint8_t const wuWeight(uint8_t const a, uint8_t const b);
    void advanceWater();
    void swapBuffers();

  public:
    // Public variables and methods (these are accessible from the main sketch)

    // Constructor (called when the effect is created by FxEngine)
    FxSui(XYMap xyMap, uint8_t edgeDamping = 250)
        : Fx2d(xyMap), edgeDamping(edgeDamping) {
        mXyMap.convertToLookUpTable();
        width = mXyMap.getWidth();
        height = mXyMap.getHeight();
        wwidth = width + 2;
        wheight = height + 2;
        wsize = wwidth * wheight;
        waterA.reset(new uint8_t[wsize]);
        waterB.reset(new uint8_t[wsize]);
        // flags.movingStimulus = true;
        flags.randomDrops = true;
        swapBuffers();
    }

    // Destructor (called when the effect is destroyed)
    ~FxSui() {}

    // What is the name of the effect?
    fl::Str fxName() const override { return "水 (sui) — water"; }

    // Flags to enable various plotting options
    struct Flags {
        bool movingStimulus : 1; // add a moving stimulus
        bool randomDrops : 1;    // add random drops
        bool waveTop : 1;        // wave generator at the top
        bool waveRight : 1;      // wave generator at the right
        bool waveBottom : 1;     // wave generator at the bottom
        bool waveLeft : 1;       // wave generator at the left
    };
    Flags flags;

    // More methods are defined below this Class declaration
    void setEdgeDamping(uint8_t value);
    void waveTank();
    void draw(DrawContext context) override;
    void wuPixel(uint16_t x, uint16_t y, uint8_t bright);
    CRGB ColorBlend(const TProgmemRGBPalette16 pal, uint16_t index,
                    uint8_t brightness, TBlendType blendType);
};

// Swap the src/dest buffers on each frame
void FxSui::swapBuffers() {
    uint8_t *const bufA = waterA.get();
    uint8_t *const bufB = waterB.get();
    if (buffer) {
        buffptr[0] = bufB;
        buffptr[1] = bufA;
    } else {
        buffptr[0] = bufA;
        buffptr[1] = bufB;
    }
    buffer = !buffer;
}

// Called by the FX engine when it needs us to draw a frame
void FxSui::draw(DrawContext context) {
    CRGB *leds = context.leds;
    if (nullptr == leds) {
        return;
    }

    waveTank();

    // Add a moving stimulus
    if (flags.movingStimulus) {
        phase[0] += beatsin16(9, 500, 1800);
        phase[1] += beatsin16(7, 500, 1500);
        phase[2] += beatsin16(2, 500, 5000);
        uint16_t x =
            256 + ((uint32_t(width - 2) * (sin16(phase[0]) + 32768)) >> 8);
        uint16_t y =
            256 + ((uint32_t(height - 2) * (sin16(phase[1]) + 32768)) >> 8);
        uint8_t z = 127 + ((sin16(phase[2]) + 32768) >> 9);
        wuPixel(x, y, z);
    }

    // Add random drops
    if (flags.randomDrops) {
        static uint32_t nextMs = 0; // time of last drop
        static uint16_t lastX = 0, lastY = 0;
        if (millis() > nextMs) {
            int x, y, dx, dy, dist, tries = 4;
            do {
                x = 256 + random16(width * 256);
                y = 256 + random16(height * 256);
                dx = abs(lastX - x), dy = abs(lastY - y);
                dist = sqrt(dx * dx + dy * dy);
            } while (dist < width * 100 && --tries);
            if (tries) {
                wuPixel(x, y, 255);
                lastX = x, lastY = y;
                nextMs = millis() + random8();
            }
        }
    }

    // Advance the water simulation forwards a single step
    advanceWater();

    // Swapping here allows painting into the next frame's water buffer with
    // sui.wuPixel() before calling fxEngine.draw().
    swapBuffers();

    // Map the water buffer to the LED array
    static uint16_t pal_offset = 0; // TODO - make this a member variable?
    uint8_t *input = buffptr[0] + wwidth - 1;
    pal_offset += 96;
    for (uint8_t y = 0; y < height; y++) {
        input += 2;
        for (uint8_t x = 0; x < width; x++) {
            uint16_t xy = xyMap(x, y);
            leds[xy] = ColorFromPaletteExtended(
                (const CRGBPalette16 &)RainbowColors_p,
                uint16_t(pal_offset + (*input << 5)), *input, LINEARBLEND);
            input++;
        }
    }
}

// Set damping values for the perimeter of the water buffer.
// This affects how waves reflect off the edges.
void FxSui::setEdgeDamping(uint8_t value) { edgeDamping = value; }

// Wave tank simulation? We'll find out soon enough… Yes! That works.
// Haha, even beam-forming works. This algorithm is awesome. Thanks, Hugo et al.
void FxSui::waveTank() {
    tankPhase += 800;
    uint16_t theta = 327.675f * (1.0f + sin(millis() / 300.f));

    // Calculate the length of the perimeter
    uint16_t perimeterLength = 2 * (width + height - 2);
    // Stretch a single cycle of cos8() around the perimeter
    uint16_t dtheta = 8 * (65536 / perimeterLength) / 256;

    // top
    uint16_t value;
    for (int i = wwidth - 1; i >= 0; i--) {
        if (flags.waveTop)
            value = cos8(theta) / 3, theta += dtheta;
        else
            value = 255 - edgeDamping;
        int j = i + wwidth * (wheight - 1);
        buffptr[0][j] = value;
        // buffptr[1][j] = value;
    }
    // right
    for (int i = 1; i < wheight - 1; i++) {
        if (flags.waveRight)
            value = cos8(theta) / 3, theta += dtheta;
        else
            value = 255 - edgeDamping;
        int j = wwidth - 1 + i * wwidth;
        buffptr[0][j] = value;
        // buffptr[1][j] = value;
    }
    // bottom
    for (int i = 0; i < wwidth; i++) {
        if (flags.waveBottom)
            value = cos8(theta) / 3, theta += dtheta;
        else
            value = 255 - edgeDamping;
        buffptr[0][i] = value;
        // buffptr[1][i] = value;
    }
    // left
    for (int i = wheight - 2; i >= 1; i--) {
        if (flags.waveLeft)
            value = cos8(theta) / 3, theta += dtheta;
        else
            value = 255 - edgeDamping;
        int j = i * wwidth;
        buffptr[0][j] = value;
        // buffptr[1][j] = value;
    }
}

// Calculate the Wu weight for a pair of values.
uint8_t const FxSui::wuWeight(uint8_t const a, uint8_t const b) {
    return (uint8_t)((a * b + a + b) >> 8);
}
// Draw a blob of 4 pixels with their relative brightnesses conveying subpixel
// information. This is the Wu antialiased pixel plotting algorithm.
void FxSui::wuPixel(uint16_t x, uint16_t y, uint8_t bright) {
    // Nothing to plot within the perimeter?
    if (x >= (wwidth - 1) << 8 || y >= (wheight - 1) << 8)
        return;
    // extract the fractional parts and derive their inverses
    uint8_t xx = x & 0xff, yy = y & 0xff, ix = 255 - xx, iy = 255 - yy;
    // calculate the intensities for each affected pixel
    uint8_t wu[4]{wuWeight(ix, iy), wuWeight(xx, iy),  // top left, top right
                  wuWeight(ix, yy), wuWeight(xx, yy)}; // btm left, btm right
    for (uint8_t i = 0; i < 4; i++) {
        uint8_t local_x = (x >> 8) + (i & 1);
        if (!local_x || local_x >= wwidth - 1) // clip left and right
            continue;
        uint8_t local_y = (y >> 8) + ((i >> 1) & 1);
        if (!local_y || local_y >= wheight - 1) // clip top and bottom
            continue;
        uint16_t xy = wwidth * local_y + local_x;
        // scale by the Wu weight, and saturating-add to the buffer
        uint16_t scaled = bright * wu[i];
        buffptr[0][xy] = qadd8(buffptr[0][xy], scaled >> 8);
    }
}

// Advance the water simulation by one frame
void FxSui::advanceWater() {
    uint8_t *src = buffptr[0];
    uint8_t *dst = buffptr[1];

    src += wwidth - 1;
    dst += wwidth - 1;
    for (uint8_t y = 1; y < wheight - 1; y++) {
        src += 2;
        dst += 2;
        for (uint8_t x = 1; x < wwidth - 1; x++) {
            // This is slightly different to the Elias algorithm.
            // Rather than negative values clamping at 0, this reflects off 0.
            // It preserves a tiny bit more information in the water buffers.
            uint16_t t = 64 * (src[-1] + src[1] + src[-wwidth] + src[wwidth]);
            uint16_t bigdst = *dst * 128;
            if (t <= bigdst)
                *dst = (bigdst - t) >> 8;
            else {
                t -= bigdst;
                if (t < 32768)
                    *dst = t >> 7;
                else
                    *dst = 255;
            }
            src++;
            dst++;
        }
    }
}

} // namespace fl