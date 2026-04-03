#pragma once

#include <list>
#include <array>
#include <string>
#include <sstream>

#include <Arduino.h>
#include <WiFiManager.h>

#include <reporter.h>
#include <config.h>

#include "TimerDispatcher.h"

#include "PinBase.h"

namespace app {

class PinCounter : public PinBase, public TimerListener {
public:
    using VoidCallbackPtr = void (*)(void);

    PinCounter(uint8_t pin,
               reporter::Reporter &reporter,
               const char *name,
               const uint32_t multiplier);

    ~PinCounter() = default;

    void attach(VoidCallbackPtr callback) const;
    bool process();

    PinCounter& operator++();

    void sendMetric();

    // Implement TimerListiner
    void onTimer();

    long getValue();
    void setValue(unsigned long value);

private:
    std::string mName;
    reporter::Reporter& mReporter;

    uint64_t mBucket;
    uint32_t mCounterTotal;
    uint32_t mMultiplier;

    msec mLastState;
    msec mLastCheckMs;
};

} // namespace fm