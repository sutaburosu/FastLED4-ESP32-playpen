#pragma once
#include <fl/xymap.h>

using namespace fl;

/**
 * @enum XY_config_enum
 * @brief Configuration options for XY mapping.
 *
 * @var xySerpentine
 * Serpentine not raster: odd rows/columns of LEDs are reversed
 *
 * @var xyColumnMajor
 * Column-major layout: LEDs are arranged in columns not rows
 *
 * @var xyFlipMajor
 * Flip major axis: reflect each panel along the major axis
 *
 * @var xyFlipMinor
 * Flip minor axis: reflect each panel along the minor axis
 *
 * @var xySerpentineTiling
 * Serpentine tiling: panels are arranged in serpentine not raster order
 *
 * @var xyVerticalTiling
 * Vertical tiling: panels are arranged in columns not rows
 */

enum XY_config_enum
{
  xySerpentine = 1,
  xyColumnMajor = 2,
  xyFlipMajor = 4,
  xyFlipMinor = 8,
  xySerpentineTiling = 16,
  xyVerticalTiling = 32,
};

/**
 * @brief Maps (x, y) coordinates to a linear index for a single panel.
 *
 * @tparam config Configuration options from XY_config_enum.
 * @tparam width Width of the panel.
 * @tparam height Height of the panel.
 * @param x X-coordinate.
 * @param y Y-coordinate.
 * @param w Width (unused).
 * @param h Height (unused).
 * @return uint16_t Linear index corresponding to the (x, y) coordinates.
 */

template <int config, uint16_t width, uint16_t height>
uint16_t XY_panel(uint16_t x, uint16_t y, uint16_t w = 0, uint16_t h = 0)
{
  (void)w;
  (void)h;
  uint8_t major, minor, sz_major, sz_minor;
  if (config & xyColumnMajor)
    major = y, minor = x, sz_major = height, sz_minor = width;
  else
    major = x, minor = y, sz_major = width, sz_minor = height;
  if (config & xyFlipMinor)
    minor = sz_minor - 1 - minor;
  if ((config & xyFlipMajor) ^ ((minor & 1) && (config & xySerpentine)))
    major = sz_major - 1 - major;
  return (uint16_t)(minor * sz_major + major);
}
template <const int config, const uint16_t width, const uint16_t height>
uint16_t XY_panel_const(const uint16_t x, const uint16_t y,
                  const uint16_t w = 0, const uint16_t h = 0)
{
  (void)w;
  (void)h;
  uint8_t major, minor, sz_major, sz_minor;
  if (config & xyColumnMajor)
    major = y, minor = x, sz_major = height, sz_minor = width;
  else
    major = x, minor = y, sz_major = width, sz_minor = height;
  if (config & xyFlipMinor)
    minor = sz_minor - 1 - minor;
  if ((config & xyFlipMajor) ^ ((minor & 1) && (config & xySerpentine)))
    major = sz_major - 1 - major;
  return (uint16_t)(minor * sz_major + major);
}



/**
 * @brief Maps (x, y) coordinates to a linear index for multiple panels.
 *
 * @tparam config Configuration options from XY_config_enum.
 * @tparam width Total width of the display.
 * @tparam height Total height of the display.
 * @tparam panel_width Width of a single panel.
 * @tparam panel_height Height of a single panel.
 * @param x X-coordinate.
 * @param y Y-coordinate.
 * @param w Width (unused).
 * @param h Height (unused).
 * @return uint16_t Linear index corresponding to the (x, y) coordinates.
 */

template <int config, uint16_t width, uint16_t height, uint16_t panel_width, uint16_t panel_height>
uint16_t XY_panels(uint16_t x, uint16_t y, uint16_t w = 0, uint16_t h = 0)
{
  (void)w, (void)h;

  // if (x >= width || y >= height)
  //   return -1; // the caller must detect results >= NUM_LEDS and not plot them

  // determine which panel this pixel falls on
  const uint8_t x_panels = width / panel_width;
  const uint8_t y_panels = height / panel_height;
  uint8_t x_panel = x / panel_width;
  uint8_t y_panel = y / panel_height;

  // for serpentine layouts of panels
  if (config & xySerpentineTiling)
    if (config & xyVerticalTiling)
    {
      if (x_panel & 1) // odd columns of panels are reversed
        y_panel = y_panels - 1 - y_panel;
    }
    else if (y_panel & 1) // odd rows of panels are reversed
      x_panel = x_panels - 1 - x_panel;

  // find the start of this panel
  uint16_t panel_offset = panel_width * panel_height;
  if (config & xyVerticalTiling)
    panel_offset *= y_panels * x_panel + y_panel;
  else
    panel_offset *= x_panels * y_panel + x_panel;

  // constrain coordinates to the panel size
  x %= panel_width;
  y %= panel_height;

  if (false) // slower like this, so less DRY it must be
    return (panel_offset + XY_panel<config, panel_width, panel_height>(x, y));

  uint16_t major, minor, sz_major, sz_minor;
  if (config & xyColumnMajor)
      major = y, minor = x, sz_major = panel_height, sz_minor = panel_width;
  else
      major = x, minor = y, sz_major = panel_width, sz_minor = panel_height;

  if (config & xyFlipMinor)
      minor = sz_minor - 1 - minor;

  if ((config & xyFlipMajor) ^ ((minor & 1) && (config & xySerpentine)))
      major = sz_major - 1 - major;

  return (panel_offset + major + minor * sz_major);
}


//////////////////////////////////////////////////////////////////////////////
// These less specialised versions are measurably slower, but maintain the
// dynamic aspects of the FastLED API rather than being fixed width/height.

template <int config>
uint16_t XY_panel(uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
  uint8_t major, minor, sz_major, sz_minor;
  if (config & xyColumnMajor)
    major = y, minor = x, sz_major = height, sz_minor = width;
  else
    major = x, minor = y, sz_major = width,  sz_minor = height;
  if (config & xyFlipMinor)
    minor = sz_minor - 1 - minor;
  if ((config & xyFlipMajor) ^ ((minor & 1) && (config & xySerpentine)))
    major = sz_major - 1 - major;
  return (uint16_t) (minor * sz_major + major);
}

template <int config, uint16_t panel_width, uint16_t panel_height>
uint16_t XY_panels(uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
  // if (x >= width || y >= height)
  //   return -1; // the caller must detect results >= NUM_LEDS and not plot them

  // determine which panel this pixel falls on
  const uint8_t x_panels = width / panel_width;
  const uint8_t y_panels = height / panel_height;
  uint8_t x_panel = x / panel_width;
  uint8_t y_panel = y / panel_height;

  // for serpentine layouts of panels
  if (config & xySerpentineTiling) {
    if (config & xyVerticalTiling) {
      if (x_panel & 1) // odd columns of panels are reversed
        y_panel = y_panels - 1 - y_panel;
    } else {
      if (y_panel & 1) // odd rows of panels are reversed
        x_panel = x_panels - 1 - x_panel;
    }
  }

  // find the start of this panel
  uint16_t panel_offset = panel_width * panel_height;
  if (config & xyVerticalTiling)
    panel_offset *= y_panels * x_panel + y_panel;
  else
    panel_offset *= x_panels * y_panel + x_panel;

  // constrain coordinates to the panel size
  x %= panel_width;
  y %= panel_height;

  uint16_t major, minor, sz_major, sz_minor;
  if (config & xyColumnMajor)
    major = y, minor = x, sz_major = panel_height, sz_minor = panel_width;
  else
    major = x, minor = y, sz_major = panel_width,  sz_minor = panel_height;

  if (config & xyFlipMinor)
    minor = sz_minor - 1 - minor;

  if ((config & xyFlipMajor) ^ ((minor & 1) && (config & xySerpentine)))
    major = sz_major - 1 - major;

  return (panel_offset + major + minor * sz_major);
}

////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////


#if true
/**
 * @brief Benchmark different XY mapping functions.
 * 
 * This function runs a series of benchmarks on different XY mapping functions
 * and prints the time taken and the sum of the indices to the Serial output.
 */

void benchmarkXYmaps() {
  XYMap xyMapRectGrid = XYMap::constructRectangularGrid(64, 64);
  XYMap xyMapPanel = XYMap::constructWithUserFunction(64, 64, XY_panel<XY_CONFIG>);
  XYMap xyMapPanel_wh = XYMap::constructWithUserFunction(64, 64, XY_panel<XY_CONFIG, 64, 64>);
  XYMap xyMapPanel_const = XYMap::constructWithUserFunction(64, 64, XY_panel_const<XY_CONFIG, 64, 64>);
  XYMap xyMapPanels = XYMap::constructWithUserFunction(64, 64, XY_panels<XY_CONFIG, 32, 32>);
  XYMap xyMapPanels_wh = XYMap::constructWithUserFunction(64, 64, XY_panels<XY_CONFIG, 64, 64, 32, 32>);

  const int iterations = 100;
  uint32_t us;
  uint32_t sum;

  us = micros(), sum = 0;
  for (uint32_t i = 0; i < iterations; i++)
    for (uint16_t y = 0; y < 64; y++)
      for (uint16_t x = 0; x < 64; x++)
        sum += xyMapRectGrid(x, y);
  Serial.printf("MapRectGrid\t%luus \tsum: %lu\r\n", micros() - us, sum);

  us = micros(), sum = 0;
  for (uint32_t i = 0; i < iterations; i++)
    for (uint16_t y = 0; y < 64; y++)
      for (uint16_t x = 0; x < 64; x++)
        sum += xyMapPanel(x, y);
  Serial.printf("MapPanel\t%luus \tsum: %lu\r\n", micros() - us, sum);

  us = micros(), sum = 0;
  for (uint32_t i = 0; i < iterations; i++)
    for (uint16_t y = 0; y < 64; y++)
      for (uint16_t x = 0; x < 64; x++)
        sum += xyMapPanel_wh(x, y);
  Serial.printf("MapPanel_wh\t%luus \tsum: %lu\r\n", micros() - us, sum);

  us = micros(), sum = 0;
  for (uint32_t i = 0; i < iterations; i++)
    for (uint16_t y = 0; y < 64; y++)
      for (uint16_t x = 0; x < 64; x++)
        sum += xyMapPanel_const(x, y);
  Serial.printf("MapPanel_const\t%luus \tsum: %lu\r\n", micros() - us, sum);

  us = micros(), sum = 0;
  for (uint32_t i = 0; i < iterations; i++)
    for (uint16_t y = 0; y < 64; y++)
      for (uint16_t x = 0; x < 64; x++)
        sum += xyMapPanels(x, y);
  Serial.printf("MapPanels\t%luus \tsum: %lu\r\n", micros() - us, sum);

  us = micros(), sum = 0;
  for (uint32_t i = 0; i < iterations; i++)
    for (uint16_t y = 0; y < 64; y++)
      for (uint16_t x = 0; x < 64; x++)
        sum += xyMapPanels_wh(x, y);
  Serial.printf("MapPanels_wh\t%luus \tsum: %lu\r\n", micros() - us, sum);

}
#endif