#pragma once

#include <Arduino.h>

#include "PinBase.h"

namespace app {

// -------------------------------------------------------

class PinLed : public PinBase {
public:
    PinLed(uint8_t pin, bool inverted = false);
    ~PinLed() = default;

    void setDimm(uint8_t dimm);

    void on() const;
    void off() const;
    void blink(unsigned long intervalMs = 100);
    void shortBlink();
private:
    uint8_t mDimm;
    uint64_t mLastMs;
    bool mInverted;
};

} // namespace fm