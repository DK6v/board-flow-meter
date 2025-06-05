#pragma once

#include <list>
#include <array>
#include <string>
#include <sstream>

#include <Arduino.h>
#include <WiFiManager.h>

#include "TimerDispatcher.h"
#include "Reporter.h"
#include "NonVolitileCounter.h"

#include "PinBase.h"

namespace app {

class PinCounter : public PinBase, public TimerListener {
public:
    using VoidCallbackPtr = void (*)(void);
    
    PinCounter(uint8_t pin,
               NonVolitileCounter& counter,
               Reporter& reporter,
               const char* name,
               const uint32_t multiplier);

    ~PinCounter() = default;

    void attach(VoidCallbackPtr callback) const;
    void process();

    uint32_t total() const;

    bool empty() const;
    void reset();

    operator uint32_t() const;
    PinCounter& operator++();
    PinCounter& operator--();

    void onInterrupt();

    void sendMetric();

    // Implement TimerListiner
    void onTimer();

    long getValue();
    void setValue(long value);

private:
    std::string mName;
    Reporter& mReporter;

    uint64_t mBucket;
    NonVolitileCounter& mTotal;
    uint32_t mMultiplier;

    msec mLastState;
    msec mLastCheckMs;
};

} // namespace fm