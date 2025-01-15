#pragma once
#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>
#include <esp_wifi.h>

#include <FastLED.h>
#include <fl/json.h>
#include <fl/slice.h>
#include <fl/ui.h>
#include <fx/fx_engine.h>
#include <fl/xymap.h>
#include <fx/2d/noisepalette.h>
#include <fx/2d/animartrix.hpp>

#include "LD2450.h"
#include "XY.hpp"
#include "fxSui.hpp"
#include "preferences.hpp"
#include "radar.hpp"
#include "telemetry.hpp"
#include "web_pages.hpp"
#include "wifi.hpp"

// TODO Is there still no way to directly embed a binary file in C++23‽ C23 has it.
// 
// extern "C" {
//   const uint8_t image_data[] = {
//     #embed "image.png"
//   };
// }

