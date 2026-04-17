#pragma once

#include "ControlContext.h"

namespace GlycolMode {

struct Context : ControlContext {
    Context(
        ControlContext& controlContext,
        GlycolLearnedParams& learnedParams,
        GlycolConfig& glycolConfig,
        GlycolRuntimeState& runtimeState
    )
        : ControlContext(controlContext),
          learned(learnedParams),
          config(glycolConfig),
          runtime(runtimeState) {}

    GlycolLearnedParams& learned;
    GlycolConfig& config;
    GlycolRuntimeState& runtime;
};

void updatePID(Context& ctx, unsigned char& integralUpdateCounter);
void updateState(Context& ctx);

} // namespace GlycolMode
