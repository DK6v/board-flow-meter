#include <Arduino.h>

#include "PinLed.h"

namespace app {

// -------------------------------------------------------

PinLed::PinLed(uint8_t pin, bool inverted)
    : PinBase(pin),
      mDimm(0), mLastMs(0), mInverted(inverted) {

    pinMode(pin, OUTPUT);
    digitalWrite(pin, ((mInverted) ? HIGH : LOW));
}

void PinLed::on() const {
    if (mDimm == 0) {
        digitalWrite(mPin, ((mInverted) ? LOW : HIGH));
    } else {
        analogWrite(mPin, mDimm);
    }
}

void PinLed::off() const {
    if (mDimm == 0) {
        digitalWrite(mPin, ((mInverted) ? HIGH : LOW));
    } else {
        analogWrite(mPin, 0);
    }
}

void PinLed::setDimm(uint8_t dimm) {
    mDimm = dimm;
}

void PinLed::blink(unsigned long intervalMs) {

    // Wait if several blinks in a row
    uint64_t waitMs = millis() - mLastMs;
    if (waitMs < intervalMs) {
        delay(intervalMs - waitMs);
    }

    this->on();
    delay(intervalMs);
    this->off();
    mLastMs = millis();
}

void PinLed::shortBlink() {
    this->blink(5);
}

} // namespace fm