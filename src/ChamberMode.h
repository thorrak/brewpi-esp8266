#pragma once

#include "ControlContext.h"

namespace ChamberMode {

struct Context : ControlContext {
    Context(ControlContext& controlContext, bool& posPeakDetect, bool& negPeakDetect)
        : ControlContext(controlContext),
          doPosPeakDetect(posPeakDetect),
          doNegPeakDetect(negPeakDetect) {}

    bool& doPosPeakDetect;
    bool& doNegPeakDetect;
};

void updatePID(Context& ctx, unsigned char& integralUpdateCounter);
void updateState(Context& ctx, bool stayIdle);
void detectPeaks(Context& ctx);

} // namespace ChamberMode
