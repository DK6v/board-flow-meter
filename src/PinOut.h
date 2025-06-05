#pragma once

#include <Arduino.h>

#include "PinBase.h"

namespace app {

class PinOut : public PinBase {
public:
    PinOut(uint8_t pin, bool inverted = false);
    ~PinOut() = default;

    void on() const;
    void off() const;
private:
    uint64_t mLastMs;
    bool mInverted;
};

} // namespace fm