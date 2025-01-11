#pragma once
#include "crgb.h"
#include "fx/fx2d.h"
#include "fl/namespace.h"
#include "fl/ptr.h"
#include "fl/scoped_ptr.h"
#include "fl/dbg.h"
#include "fl/xymap.h"

#define FX_WATER_INTERNAL

namespace fl {

FASTLED_SMART_PTR(FxWater);


// A FastLED FX engine effect which draws a water ripple effect
class FxWater : public Fx2d {
  public:
    FxWater(XYMap xyMap) : Fx2d(xyMap) {
        mXyMap.convertToLookUpTable();

        }
};

}  // namespace fl