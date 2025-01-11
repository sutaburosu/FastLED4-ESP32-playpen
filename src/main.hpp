#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include <stdio.h>
#include <string>
#include <esp_sntp.h>
#include <time.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>
#include <lwip/sockets.h>
#include <lwip/netdb.h>

#include <FastLED.h>
#include <fl/json.h>
#include <fl/slice.h>
#include <fl/ui.h>
#include <fx/fx_engine.h>
#include <fl/xymap.h>
#include <fx/2d/noisepalette.h>
#include <fx/2d/animartrix.hpp>

void radar();

#include "telemetry.hpp"



// TODO Is there still no way to directly embed a binary file in C++23‽ C23 has it.
// 
// extern "C" {
//   const uint8_t image_data[] = {
//     #embed "image.png"
//   };
// }

// Return a String representing the current time
String timeString()
{
  time_t now;
  struct tm timeinfo;
  char timeString[32];
  time(&now);
  localtime_r(&now, &timeinfo);
  strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S:%Z", &timeinfo);
  return String(timeString);
}

// Send some system statistics at most once per second
void low_freq_stats()
{
  static uint32_t last_ms_low_freq = 0;
  if (millis() - last_ms_low_freq < 1000)
    return;
  last_ms_low_freq = millis();

  telemetry.update("Time", timeString());
  telemetry.update("HeapMaxAlloc", String(ESP.getMaxAllocHeap()), "bytes", "np");
  telemetry.update("PSRAMMaxAlloc", String(ESP.getMaxAllocPsram()), "bytes", "np");
  telemetry.update("FreeHeap", String(ESP.getFreeHeap()), "bytes", "np");
  telemetry.update("MinFreeHeap", String(ESP.getMinFreeHeap()), "bytes", "");
  telemetry.update("FreePSRAM", String(ESP.getFreePsram()), "bytes", "np");
  telemetry.update("MinFreePSRAM", String(ESP.getMinFreePsram()), "bytes", "");
  telemetry.update("Uptime", String(millis() / 3600000.f), "hours", "np");
}
