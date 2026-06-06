#ifndef APPLICATION_RUNTIME_STATE_HPP
#define APPLICATION_RUNTIME_STATE_HPP

#include "../app_types.h"

namespace application {

struct RuntimeState {
    TempSample currentSample;
    TempSample pendingLogSample;
    bool hasCurrent;
    bool logPending;

    void reset()
    {
        hasCurrent = false;
        logPending = false;
    }

    const TempSample* current() const
    {
        return &currentSample;
    }

    uint8_t hasCurrentU8() const
    {
        return hasCurrent ? 1u : 0u;
    }

    void markSampleCollected()
    {
        hasCurrent = true;
        pendingLogSample = currentSample;
        logPending = true;
    }

    void clearLogPending()
    {
        logPending = false;
    }
};

} // namespace application

#endif
