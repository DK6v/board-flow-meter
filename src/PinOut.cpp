#include <Arduino.h>

#include "PinOut.h"

namespace app {

// -------------------------------------------------------

PinOut::PinOut(uint8_t pin, bool inverted)
    : PinBase(pin),
      mLastMs(0), mInverted(inverted) {

    pinMode(pin, OUTPUT);
    digitalWrite(pin, ((mInverted) ? HIGH : LOW));
}

void PinOut::on() const {
    digitalWrite(mPin, ((mInverted) ? LOW : HIGH));
}

void PinOut::off() const {
    digitalWrite(mPin, ((mInverted) ? HIGH : LOW));
}


} // namespace fm