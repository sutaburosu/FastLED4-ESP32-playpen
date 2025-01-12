#pragma once
#include "LD2450.h"


void radar(CRGB *leds, XYMap &xyMap, LD2450 &ld2450)
{
  const int gotTargets = ld2450.read();
  if (gotTargets <= 0)
    return;

  uint8_t printCnt = 0;
  static uint16_t pos[3]{0, 0, 0};
  static uint32_t lastMillis[3]{0, 0, 0};
  for (int i = 0; i < ld2450.getSensorSupportedTargetCount(); i++)
  {
    const LD2450::RadarTarget target = ld2450.getTarget(i);
    // if (0 == i)
    //   target.valid = true, target.speed = sinf(millis() / 1000.f) * 100;

    if (target.valid && abs(target.speed) > 0)
    {
      printCnt++;
      // Serial.printf("id:%u xy:%5d,%5d res:%4u spd:%4d dst:%5u (%d %d, %d %d)  ",
      //               target.id, target.x, target.y, target.resolution, target.speed, target.distance,
      //               target.xmin, target.xmax, target.ymin, target.ymax);
      if (lastMillis[i] - millis() >= 50)
      {
        if (target.speed > 0)
          pos[i] += target.speed;
        else
          pos[i] -= -target.speed;
        lastMillis[i] = millis();
      }
    }
    // Reset if target is not moving
    if (lastMillis[i] && (millis() - lastMillis[i] > 5000))
    {
      lastMillis[i] = 0;
    }
    if (lastMillis[i])
    {
      // plot a scroller indicating the speed
      uint16_t ipos = pos[i];
      for (int x = 0; x < MATRIX_WIDTH; x++)
      {
        leds[xyMap.mapToIndex(x, i)] = CHSV(millis() / 32 + i * 86, 255, ((ipos >> 10) % 8) * 32);
        ipos += 256 * 4; // * (target.speed >= 0 ? 1 : -1);
      }
    }
  }
  if (printCnt > 0)
  {
    // Serial.println();
  }
}
